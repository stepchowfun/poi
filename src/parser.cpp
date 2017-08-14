#include <functional>
#include <poi/error.h>
#include <poi/parser.h>
#include <tuple>
#include <type_traits>
#include <unordered_map>

/*
  Poi uses a packrat parser, i.e., a recursive descent parser with memoization.
  This guarantees linear-time parsing. In the following grammar, nonterminals
  are written in UpperCamelCase and terminals (tokens) are written in
  MACRO_CASE.

    Term =
      Variable |
      Function |
      Application |
      Binding |
      DataType |
      Member |
      Match |
      Group
    Variable = IDENTIFIER
    Function = Pattern ARROW Term
    Application =
      (Variable | Application | DataType | Member | Match | Group)
      (Variable | DataType | Member | Match | Group)
    Binding = Pattern EQUALS Term SEPARATOR Term
    DataType = LEFT_CURLY DataConstructorList RIGHT_CURLY
    DataConstructorList = | DataConstructor DataConstructorTail
    DataConstructorTail = | SEPARATOR DataConstructor DataConstructorTail
    DataConstructor = IDENTIFIER DataConstructorParams
    DataConstructorParams = | IDENTIFIER DataConstructorParams
    Member = (Variable | DataType | Member | Match | Group) DOT IDENTIFIER
    Group = LEFT_PAREN Term RIGHT_PAREN
    Pattern = IDENTIFIER | LEFT_CURLY IDENTIFIER PatternList RIGHT_CURLY
    PatternList = | Pattern PatternList
    Match = MATCH Term LEFT_CURLY CaseList RIGHT_CURLY
    CaseList = Function CaseListTail
    CaseListTail = | SEPARATOR Function CaseListTail

  There are two problems with the grammar above: the Application and Member
  rules are left-recursive, and packrat parsers can't handle left-recursion:

    Application =
      (Variable | Application | DataType | Member | Match | Group)
      (Variable | DataType | Member | Match | Group)
    Member = (Variable | DataType | Member | Group) DOT IDENTIFIER

  To fix Application, we rewrite the rule to use right-recursion instead:

    Application =
      (Variable | DataType | Member | Match | Group)
      (Variable | Application | DataType | Member | Match | Group)

  This makes Application have right-associativity, which is not what we want.
  In the parsing rule for Application, we use a special trick to flip the
  associativity from right to left. Instead of building up the tree from the
  results of right-recursive calls, we pass the left term (`application_prior`)
  to the right-recursive call and let it assemble the tree with left-
  associativity.

  To fix Member, we rewrite the rule to eliminate the left recursion:

    Member = (Variable | DataType | Match | Group) DOT IDENTIFIER MemberSuffix
    MemberSuffix = | DOT IDENTIFIER

*/

///////////////////////////////////////////////////////////////////////////////
// Error handling                                                            //
///////////////////////////////////////////////////////////////////////////////

namespace Poi {
  // If we encounter an error while parsing, we may backtrack and try a
  // different branch of execution. This may lead to more errors. If all the
  // branches lead to an error, we need to choose one of the errors to show
  // to the user. To pick the most relevant one, we assign each error a
  // confidence level.
  enum class ErrorConfidence {
    LOW, // The error provides no useful information other than the fact that
         // the parse failed. Hopefully another branch will succeed or fail
         // with a more useful error.
    MED, // The error provides some useful information about the failure, but
         // we aren't 100% confident that this is the right error to show the
         // user. Another branch may succeed or fail with a more useful error.
    HIGH // This is definitely the error we will show to the user. We are so
         // confident in this error that it actually takes precedence over a
         // successful branch (because we would have received a failure later
         // on anyway).
  };

  // This is just like Poi::Error, except it also includes a confidence level.
  class ParseError : public Error {
  public:
    const Poi::ErrorConfidence confidence;

    explicit ParseError(
      const std::string &message, // No trailing line break
      Poi::ErrorConfidence confidence
    ) : Error(message), confidence(confidence) {
    };

    explicit ParseError(
      const std::string &message, // No trailing line break
      const std::string &source_name,
      const std::string &source,
      Poi::ErrorConfidence confidence
    ) :
      Error (message, source_name, source),
      confidence(confidence) {
    };

    explicit ParseError(
      const std::string &message, // No trailing line break
      const std::string &source_name,
      const std::string &source,
      size_t start_pos, // Inclusive
      size_t end_pos, // Exclusive
      Poi::ErrorConfidence confidence
    ) :
      Error (message, source_name, source, start_pos, end_pos),
      confidence(confidence) {
    };
  };

  // A successful parse results in an AST node and an iterator pointing to the
  // next token to parse. A failed parse results in a ParseError.
  template <typename T> class ParseResult {
  public:
    // Exactly one of these two should be a null pointer.
    const std::shared_ptr<const T> node;
    const std::shared_ptr<const ParseError> error;

    // If `node` is present, this should point to the first token after `node`.
    // Otherwise, its value is unspecified.
    const std::vector<Poi::Token>::const_iterator next;

    // Success constructor
    explicit ParseResult(
      std::shared_ptr<const T> node,
      std::vector<Poi::Token>::const_iterator next
    ) : node(node), next(next) {
    }

    // Failure constructor
    explicit ParseResult(
      std::shared_ptr<const ParseError> error
    ) : error(error) {
    }

    // Cast a ParseResult<T> to a ParseResult<U>, where T <: U
    template <typename U> ParseResult<U> upcast() const {
      if (node) {
        return ParseResult<U>(std::static_pointer_cast<const U>(node), next);
      } else {
        return ParseResult<U>(error);
      }
    }

    // Cast a ParseResult<T> to a ParseResult<U>, where U <: T
    template <typename U> ParseResult<U> downcast() const {
      if (node) {
        return ParseResult<U>(std::dynamic_pointer_cast<const U>(node), next);
      } else {
        return ParseResult<U>(error);
      }
    }

    // Choose between this and another ParseResult. If one is a success and the
    // other is an error, choose the success. If both are successes, choose
    // the biggest node. If both are errors, choose based on the confidence.
    ParseResult<T> choose(const ParseResult<T> &other) const {
      if (error && error->confidence == Poi::ErrorConfidence::HIGH) {
        return *this;
      }
      if (
        other.error &&
        other.error->confidence == Poi::ErrorConfidence::HIGH
      ) {
        return other;
      }
      if (node) {
        if (other.node) {
          if (other.node->end_pos > node->end_pos) {
            return other;
          } else {
            return *this;
          }
        } else {
          return *this;
        }
      } else {
        if (other.node) {
          return other;
        } else {
          if (other.error->confidence > error->confidence) {
            return other;
          } else {
            return *this;
          }
        }
      }
    }
  };
}

///////////////////////////////////////////////////////////////////////////////
// Memoization                                                               //
///////////////////////////////////////////////////////////////////////////////

// This enum represents the set of functions which memoize their result.
enum class MemoType {
  PATTERN,
  VARIABLE_PATTERN,
  CONSTRUCTOR_PATTERN,
  TERM,
  VARIABLE,
  FUNCTION,
  APPLICATION,
  BINDING,
  DATA_TYPE,
  MEMBER,
  MATCH,
  GROUP
};

// The type of keys used in the memoization table
using MemoKey = std::tuple<
  MemoType, // Represents the function doing the memoizing
  std::vector<Poi::Token>::const_iterator, // The start token
  std::shared_ptr<const Poi::Term> // The `application_prior` term
>;

// The type of the memoization table
using MemoMap = std::unordered_map<
  MemoKey,
  Poi::ParseResult<Poi::Node>,
  std::function<size_t(const MemoKey &key)>
>;

// A helper function for constructing a memoization key
MemoKey memo_key(
  MemoType type,
  std::vector<Poi::Token>::const_iterator start,
  std::shared_ptr<const Poi::Term> application_prior = nullptr
) {
  return std::make_tuple(type, start, application_prior);
}

// Memoize and return a ParseResult representing a success
template <typename T> Poi::ParseResult<T> memo_success(
  MemoMap &memo,
  const MemoKey &key,
  std::shared_ptr<const T> node,
  std::vector<Poi::Token>::const_iterator next
) {
  auto parse_result = Poi::ParseResult<T>(node, next);
  memo.insert({key, parse_result.template upcast<Poi::Node>()});
  return parse_result;
}

// Memoize and return a ParseResult representing a failure
template <typename T> Poi::ParseResult<T> memo_error(
  MemoMap &memo,
  const MemoKey &key,
  std::shared_ptr<const Poi::ParseError> error
) {
  auto parse_result = Poi::ParseResult<T>(error);
  memo.insert({key, parse_result.template upcast<Poi::Node>()});
  return parse_result;
}

///////////////////////////////////////////////////////////////////////////////
// Parsing                                                                   //
///////////////////////////////////////////////////////////////////////////////

// Forward declarations necessary for mutual recursion

Poi::ParseResult<Poi::Pattern> parse_pattern(
  MemoMap &memo,
  const Poi::StringPool &pool,
  const Poi::TokenStream &token_stream,
  const std::unordered_set<size_t> &environment,
  std::vector<Poi::Token>::const_iterator iter
);

Poi::ParseResult<Poi::VariablePattern> parse_variable_pattern(
  MemoMap &memo,
  const Poi::StringPool &pool,
  const Poi::TokenStream &token_stream,
  const std::unordered_set<size_t> &environment,
  std::vector<Poi::Token>::const_iterator iter
);

Poi::ParseResult<Poi::ConstructorPattern> parse_constructor_pattern(
  MemoMap &memo,
  const Poi::StringPool &pool,
  const Poi::TokenStream &token_stream,
  const std::unordered_set<size_t> &environment,
  std::vector<Poi::Token>::const_iterator iter
);

Poi::ParseResult<Poi::Term> parse_term(
  MemoMap &memo,
  const Poi::StringPool &pool,
  const Poi::TokenStream &token_stream,
  const std::unordered_set<size_t> &environment,
  std::vector<Poi::Token>::const_iterator iter
);

Poi::ParseResult<Poi::Variable> parse_variable(
  MemoMap &memo,
  const Poi::StringPool &pool,
  const Poi::TokenStream &token_stream,
  const std::unordered_set<size_t> &environment,
  std::vector<Poi::Token>::const_iterator iter
);

Poi::ParseResult<Poi::Function> parse_function(
  MemoMap &memo,
  const Poi::StringPool &pool,
  const Poi::TokenStream &token_stream,
  const std::unordered_set<size_t> &environment,
  std::vector<Poi::Token>::const_iterator iter
);

Poi::ParseResult<Poi::Application> parse_application(
  MemoMap &memo,
  const Poi::StringPool &pool,
  const Poi::TokenStream &token_stream,
  const std::unordered_set<size_t> &environment,
  std::vector<Poi::Token>::const_iterator iter,
  std::shared_ptr<const Poi::Term> application_prior
);

Poi::ParseResult<Poi::Binding> parse_binding(
  MemoMap &memo,
  const Poi::StringPool &pool,
  const Poi::TokenStream &token_stream,
  const std::unordered_set<size_t> &environment,
  std::vector<Poi::Token>::const_iterator iter
);

Poi::ParseResult<Poi::DataType> parse_data_type(
  MemoMap &memo,
  const Poi::StringPool &pool,
  const Poi::TokenStream &token_stream,
  const std::unordered_set<size_t> &environment,
  std::vector<Poi::Token>::const_iterator iter
);

Poi::ParseResult<Poi::Member> parse_member(
  MemoMap &memo,
  const Poi::StringPool &pool,
  const Poi::TokenStream &token_stream,
  const std::unordered_set<size_t> &environment,
  std::vector<Poi::Token>::const_iterator iter
);

Poi::ParseResult<Poi::Match> parse_match(
  MemoMap &memo,
  const Poi::StringPool &pool,
  const Poi::TokenStream &token_stream,
  const std::unordered_set<size_t> &environment,
  std::vector<Poi::Token>::const_iterator iter
);

Poi::ParseResult<Poi::Term> parse_group(
  MemoMap &memo,
  const Poi::StringPool &pool,
  const Poi::TokenStream &token_stream,
  const std::unordered_set<size_t> &environment,
  std::vector<Poi::Token>::const_iterator iter
);

// The definitions of the parsing functions

Poi::ParseResult<Poi::Pattern> parse_pattern(
  MemoMap &memo,
  const Poi::StringPool &pool,
  const Poi::TokenStream &token_stream,
  const std::unordered_set<size_t> &environment,
  std::vector<Poi::Token>::const_iterator iter
) {
  // Check if we can reuse a memoized result.
  auto key = memo_key(MemoType::PATTERN, iter);
  auto memo_result = memo.find(key);
  if (memo_result != memo.end()) {
    return memo_result->second.downcast<Poi::Pattern>();
  }

  // Make sure we have some tokens to parse.
  if (iter == token_stream.tokens->end()) {
    return memo_error<Poi::Pattern>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        "No pattern to parse.",
        pool.find(token_stream.source_name),
        pool.find(token_stream.source),
        Poi::ErrorConfidence::LOW
      )
    );
  }

  // A pattern is one of the following constructs.
  auto pattern = Poi::ParseResult<Poi::Pattern>(
    std::make_shared<Poi::ParseError>(
      "Unexpected token.",
      pool.find(iter->source_name),
      pool.find(iter->source),
      iter->start_pos,
      iter->end_pos,
      Poi::ErrorConfidence::LOW
    )
  ).choose(
    parse_variable_pattern(
      memo, pool, token_stream, environment, iter
    ).upcast<Poi::Pattern>()
  ).choose(
    parse_constructor_pattern(
      memo, pool, token_stream, environment, iter
    ).upcast<Poi::Pattern>()
  );
  if (pattern.error) {
    return memo_error<Poi::Pattern>(memo, key, pattern.error);
  }

  // Memoize and return the result.
  return memo_success<Poi::Pattern>(memo, key, pattern.node, pattern.next);
}

Poi::ParseResult<Poi::VariablePattern> parse_variable_pattern(
  MemoMap &memo,
  const Poi::StringPool &pool,
  const Poi::TokenStream &token_stream,
  const std::unordered_set<size_t> &environment,
  std::vector<Poi::Token>::const_iterator iter
) {
  // Check if we can reuse a memoized result.
  auto key = memo_key(MemoType::VARIABLE_PATTERN, iter);
  auto memo_result = memo.find(key);
  if (memo_result != memo.end()) {
    return memo_result->second.downcast<Poi::VariablePattern>();
  }

  // Make sure we have some tokens to parse.
  if (iter == token_stream.tokens->end()) {
    return memo_error<Poi::VariablePattern>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        "No variable pattern to parse.",
        pool.find(token_stream.source_name),
        pool.find(token_stream.source),
        Poi::ErrorConfidence::LOW
      )
    );
  }

  // Check whether the current token is an identifier.
  if (iter->type != Poi::TokenType::IDENTIFIER) {
    return memo_error<Poi::VariablePattern>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        "A variable pattern must be an identifier.",
        pool.find(iter->source_name),
        pool.find(iter->source),
        iter->start_pos,
        iter->end_pos,
        Poi::ErrorConfidence::LOW
      )
    );
  }

  // Construct the VariablePattern.
  auto variables = std::make_shared<const std::unordered_set<size_t>>();
  std::const_pointer_cast<
    std::unordered_set<size_t>
  >(variables)->insert(iter->literal);
  auto variable_pattern = std::make_shared<Poi::VariablePattern>(
    iter->source_name,
    iter->source,
    iter->start_pos,
    iter->end_pos,
    iter->literal,
    variables
  );
  ++iter;

  // Memoize and return the result.
  return memo_success<Poi::VariablePattern>(memo, key, variable_pattern, iter);
}

Poi::ParseResult<Poi::ConstructorPattern> parse_constructor_pattern(
  MemoMap &memo,
  const Poi::StringPool &pool,
  const Poi::TokenStream &token_stream,
  const std::unordered_set<size_t> &environment,
  std::vector<Poi::Token>::const_iterator iter
) {
  // Check if we can reuse a memoized result.
  auto key = memo_key(MemoType::CONSTRUCTOR_PATTERN, iter);
  auto memo_result = memo.find(key);
  if (memo_result != memo.end()) {
    return memo_result->second.downcast<Poi::ConstructorPattern>();
  }

  // Mark the beginning of the ConstructorPattern.
  auto start = iter;

  // Make sure we have some tokens to parse.
  if (iter == token_stream.tokens->end()) {
    return memo_error<Poi::ConstructorPattern>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        "No constructor pattern to parse.",
        pool.find(token_stream.source_name),
        pool.find(token_stream.source),
        Poi::ErrorConfidence::LOW
      )
    );
  }

  // Parse the LEFT_CURLY.
  if (iter->type != Poi::TokenType::LEFT_CURLY) {
    return memo_error<Poi::ConstructorPattern>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        "Expected '{' to introduce this constructor pattern.",
        pool.find(start->source_name),
        pool.find(start->source),
        start->start_pos,
        iter->end_pos,
        Poi::ErrorConfidence::LOW
      )
    );
  }
  ++iter;

  // Parse the IDENTIFIER.
  if (iter->type != Poi::TokenType::IDENTIFIER) {
    return memo_error<Poi::ConstructorPattern>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        "A constructor pattern must begin with the name of a constructor.",
        pool.find(iter->source_name),
        pool.find(iter->source),
        iter->start_pos,
        iter->end_pos,
        Poi::ErrorConfidence::MED
      )
    );
  }
  auto constructor_name = iter->literal;
  ++iter;

  // Parse the parameters. Note that the lexical analyzer guarantees
  // curly braces are matched.
  auto parameters = std::make_shared<
    std::vector<std::shared_ptr<const Poi::Pattern>>
  >();
  auto variables = std::make_shared<const std::unordered_set<size_t>>();
  while (iter->type != Poi::TokenType::RIGHT_CURLY) {
    auto parameter = Poi::ParseResult<Poi::Pattern>(
      std::make_shared<Poi::ParseError>(
        "Unexpected token.",
        pool.find(iter->source_name),
        pool.find(iter->source),
        iter->start_pos,
        iter->end_pos,
        Poi::ErrorConfidence::LOW
      )
    ).choose(
      parse_pattern(
        memo,
        pool,
        token_stream,
        environment,
        iter
      )
    );
    if (parameter.error) {
      return memo_error<Poi::ConstructorPattern>(
        memo,
        key,
        std::make_shared<Poi::ParseError>(
          parameter.error->what(),
          Poi::ErrorConfidence::MED
        )
      );
    }
    iter = parameter.next;
    parameters->push_back(parameter.node);
    for (auto &variable : *(parameter.node->variables)) {
      if (variables->find(variable) != variables->end()) {
        return memo_error<Poi::ConstructorPattern>(
          memo,
          key,
          std::make_shared<Poi::ParseError>(
            "Duplicate variable '" + pool.find(variable) + "' in pattern.",
            pool.find(parameter.node->source_name),
            pool.find(parameter.node->source),
            parameter.node->start_pos,
            parameter.node->end_pos,
            Poi::ErrorConfidence::MED
          )
        );
      }
      std::const_pointer_cast<
        std::unordered_set<size_t>
      >(variables)->insert(variable);
    }
  }

  // Skip the closing RIGHT_CURLY.
  ++iter;

  // Construct the ConstructorPattern.
  auto constructor_pattern = std::make_shared<Poi::ConstructorPattern>(
    start->source_name,
    start->source,
    start->start_pos,
    (iter - 1)->end_pos,
    constructor_name,
    parameters,
    variables
  );

  // Memoize and return the result.
  return memo_success<Poi::ConstructorPattern>(
    memo,
    key,
    constructor_pattern,
    iter
  );
}

Poi::ParseResult<Poi::Term> parse_term(
  MemoMap &memo,
  const Poi::StringPool &pool,
  const Poi::TokenStream &token_stream,
  const std::unordered_set<size_t> &environment,
  std::vector<Poi::Token>::const_iterator iter
) {
  // Check if we can reuse a memoized result.
  auto key = memo_key(MemoType::TERM, iter);
  auto memo_result = memo.find(key);
  if (memo_result != memo.end()) {
    return memo_result->second.downcast<Poi::Term>();
  }

  // Make sure we have some tokens to parse.
  if (iter == token_stream.tokens->end()) {
    return memo_error<Poi::Term>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        "No term to parse.",
        pool.find(token_stream.source_name),
        pool.find(token_stream.source),
        Poi::ErrorConfidence::LOW
      )
    );
  }

  // A term is one of the following constructs.
  auto term = Poi::ParseResult<Poi::Term>(
    std::make_shared<Poi::ParseError>(
      "Unexpected token.",
      pool.find(iter->source_name),
      pool.find(iter->source),
      iter->start_pos,
      iter->end_pos,
      Poi::ErrorConfidence::LOW
    )
  ).choose(
    parse_variable(
      memo, pool, token_stream, environment, iter
    ).upcast<Poi::Term>()
  ).choose(
    parse_function(
      memo, pool, token_stream, environment, iter
    ).upcast<Poi::Term>()
  ).choose(
    parse_application(
      memo, pool, token_stream, environment, iter, nullptr
    ).upcast<Poi::Term>()
  ).choose(
    parse_binding(
      memo, pool, token_stream, environment, iter
    ).upcast<Poi::Term>()
  ).choose(
    parse_data_type(
      memo, pool, token_stream, environment, iter
    ).upcast<Poi::Term>()
  ).choose(
    parse_member(
      memo, pool, token_stream, environment, iter
    ).upcast<Poi::Term>()
  ).choose(
    parse_match(
      memo, pool, token_stream, environment, iter
    ).upcast<Poi::Term>()
  ).choose(
    parse_group(
      memo, pool, token_stream, environment, iter
    ).upcast<Poi::Term>()
  );
  if (term.error) {
    return memo_error<Poi::Term>(memo, key, term.error);
  }

  // Memoize and return the result.
  return memo_success<Poi::Term>(memo, key, term.node, term.next);
}

Poi::ParseResult<Poi::Variable> parse_variable(
  MemoMap &memo,
  const Poi::StringPool &pool,
  const Poi::TokenStream &token_stream,
  const std::unordered_set<size_t> &environment,
  std::vector<Poi::Token>::const_iterator iter
) {
  // Check if we can reuse a memoized result.
  auto key = memo_key(MemoType::VARIABLE, iter);
  auto memo_result = memo.find(key);
  if (memo_result != memo.end()) {
    return memo_result->second.downcast<Poi::Variable>();
  }

  // Make sure we have some tokens to parse.
  if (iter == token_stream.tokens->end()) {
    return memo_error<Poi::Variable>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        "No variable to parse.",
        pool.find(token_stream.source_name),
        pool.find(token_stream.source),
        Poi::ErrorConfidence::LOW
      )
    );
  }

  // Parse the IDENTIFIER.
  if (iter->type != Poi::TokenType::IDENTIFIER) {
    return memo_error<Poi::Variable>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        "A variable must be an identifier.",
        pool.find(iter->source_name),
        pool.find(iter->source),
        iter->start_pos,
        iter->end_pos,
        Poi::ErrorConfidence::LOW
      )
    );
  }
  auto variable_name = iter->literal;

  // Look up the variable in the environment.
  if (environment.find(variable_name) == environment.end()) {
    Poi::ErrorConfidence confidence = Poi::ErrorConfidence::LOW;
    if (
      iter + 1 == token_stream.tokens->end() || (
        (iter + 1)->type != Poi::TokenType::ARROW &&
        (iter + 1)->type != Poi::TokenType::EQUALS
      )
    ) {
      confidence = Poi::ErrorConfidence::HIGH;
    }
    return memo_error<Poi::Variable>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        "Undefined variable '" + pool.find(variable_name) + "'.",
        pool.find(iter->source_name),
        pool.find(iter->source),
        iter->start_pos,
        iter->end_pos,
        confidence
      )
    );
  }
  ++iter;

  // Construct the Variable.
  auto free_variables = std::make_shared<std::unordered_set<size_t>>();
  free_variables->insert(variable_name);
  auto variable = std::make_shared<Poi::Variable>(
    (iter - 1)->source_name,
    (iter - 1)->source,
    (iter - 1)->start_pos,
    (iter - 1)->end_pos,
    free_variables,
    variable_name
  );

  // Memoize and return the result.
  return memo_success<Poi::Variable>(memo, key, variable, iter);
}

Poi::ParseResult<Poi::Function> parse_function(
  MemoMap &memo,
  const Poi::StringPool &pool,
  const Poi::TokenStream &token_stream,
  const std::unordered_set<size_t> &environment,
  std::vector<Poi::Token>::const_iterator iter
) {
  // Check if we can reuse a memoized result.
  auto key = memo_key(MemoType::FUNCTION, iter);
  auto memo_result = memo.find(key);
  if (memo_result != memo.end()) {
    return memo_result->second.downcast<Poi::Function>();
  }

  // Mark the beginning of the Function.
  auto start = iter;

  // Make sure we have some tokens to parse.
  if (iter == token_stream.tokens->end()) {
    return memo_error<Poi::Function>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        "No function to parse.",
        pool.find(token_stream.source_name),
        pool.find(token_stream.source),
        Poi::ErrorConfidence::LOW
      )
    );
  }

  // Parse the Pattern.
  auto pattern = Poi::ParseResult<Poi::Pattern>(
    std::make_shared<Poi::ParseError>(
      "No pattern found for this binding.",
      pool.find(start->source_name),
      pool.find(start->source),
      start->start_pos,
      iter->end_pos,
      Poi::ErrorConfidence::LOW
    )
  ).choose(parse_pattern(memo, pool, token_stream, environment, iter));
  if (pattern.error) {
    return memo_error<Poi::Function>(memo, key, pattern.error);
  }
  iter = pattern.next;

  // Parse the ARROW.
  if (
    iter == token_stream.tokens->end() ||
    iter->type != Poi::TokenType::ARROW
  ) {
    return memo_error<Poi::Function>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        "Expected '->' in this function.",
        pool.find(start->source_name),
        pool.find(start->source),
        start->start_pos,
        (iter - 1)->end_pos,
        Poi::ErrorConfidence::LOW
      )
    );
  }
  ++iter;

  // Add the pattern variables to the environment.
  auto new_environment = environment;
  new_environment.insert(
    pattern.node->variables->begin(),
    pattern.node->variables->end()
  );

  // Parse the body.
  auto body = Poi::ParseResult<Poi::Term>(
    std::make_shared<Poi::ParseError>(
      "No body found for this function.",
      pool.find(start->source_name),
      pool.find(start->source),
      start->start_pos,
      (iter - 1)->end_pos,
      Poi::ErrorConfidence::LOW
    )
  ).choose(parse_term(memo, pool, token_stream, new_environment, iter));
  if (body.error) {
    return memo_error<Poi::Function>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        body.error->what(),
        Poi::ErrorConfidence::HIGH
      )
    );
  }
  iter = body.next;

  // Construct the Function.
  auto free_variables = std::make_shared<std::unordered_set<size_t>>();
  free_variables->insert(
    body.node->free_variables->begin(),
    body.node->free_variables->end()
  );
  for (auto &variable : *(pattern.node->variables)) {
    free_variables->erase(variable);
  }
  auto function = std::make_shared<Poi::Function>(
    start->source_name,
    start->source,
    start->start_pos,
    (iter - 1)->end_pos,
    free_variables,
    pattern.node,
    body.node
  );

  // Memoize and return the result.
  return memo_success<Poi::Function>(memo, key, function, iter);
}

Poi::ParseResult<Poi::Application> parse_application(
  MemoMap &memo,
  const Poi::StringPool &pool,
  const Poi::TokenStream &token_stream,
  const std::unordered_set<size_t> &environment,
  std::vector<Poi::Token>::const_iterator iter,
  std::shared_ptr<const Poi::Term> application_prior
) {
  // Check if we can reuse a memoized result.
  auto key = memo_key(MemoType::APPLICATION, iter, application_prior);
  auto memo_result = memo.find(key);
  if (memo_result != memo.end()) {
    return memo_result->second.downcast<Poi::Application>();
  }

  // Make sure we have some tokens to parse.
  if (iter == token_stream.tokens->end()) {
    return memo_error<Poi::Application>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        "No left subterm to parse.",
        pool.find(token_stream.source_name),
        pool.find(token_stream.source),
        Poi::ErrorConfidence::LOW
      )
    );
  }

  // Parse the left term.
  auto left = Poi::ParseResult<Poi::Term>(
    std::make_shared<Poi::ParseError>(
      "Unexpected token.",
      pool.find(iter->source_name),
      pool.find(iter->source),
      iter->start_pos,
      iter->end_pos,
      Poi::ErrorConfidence::LOW
    )
  ).choose(
    parse_variable(
      memo, pool, token_stream, environment, iter
    ).upcast<Poi::Term>()
  ).choose(
    parse_data_type(
      memo, pool, token_stream, environment, iter
    ).upcast<Poi::Term>()
  ).choose(
    parse_member(
      memo, pool, token_stream, environment, iter
    ).upcast<Poi::Term>()
  ).choose(
    parse_match(
      memo, pool, token_stream, environment, iter
    ).upcast<Poi::Term>()
  ).choose(
    parse_group(
      memo, pool, token_stream, environment, iter
    )
  );
  if (left.error) {
    return memo_error<Poi::Application>(memo, key, left.error);
  }
  iter = left.next;

  // Make sure we have some tokens to parse.
  if (iter == token_stream.tokens->end()) {
    return memo_error<Poi::Application>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        "No right subterm to parse.",
        pool.find(token_stream.source_name),
        pool.find(token_stream.source),
        Poi::ErrorConfidence::LOW
      )
    );
  }

  // Parse the right term.
  auto right = std::make_shared<Poi::ParseResult<Poi::Term>>(
    Poi::ParseResult<Poi::Term>(
      std::make_shared<Poi::ParseError>(
        "Unexpected token.",
        pool.find(iter->source_name),
        pool.find(iter->source),
        iter->start_pos,
        iter->end_pos,
        Poi::ErrorConfidence::LOW
      )
    ).choose(
      parse_variable(
        memo, pool, token_stream, environment, iter
      ).upcast<Poi::Term>()
    ).choose(
      parse_data_type(
        memo, pool, token_stream, environment, iter
      ).upcast<Poi::Term>()
    ).choose(
      parse_member(
        memo, pool, token_stream, environment, iter
      ).upcast<Poi::Term>()
    ).choose(
      parse_match(
        memo, pool, token_stream, environment, iter
      ).upcast<Poi::Term>()
    ).choose(
      parse_group(
        memo, pool, token_stream, environment, iter
      )
    )
  );
  if (application_prior) {
    auto prior_of_left_free_variables = std::make_shared<
      std::unordered_set<size_t>
    >();
    prior_of_left_free_variables->insert(
      application_prior->free_variables->begin(),
      application_prior->free_variables->end()
    );
    prior_of_left_free_variables->insert(
      left.node->free_variables->begin(),
      left.node->free_variables->end()
    );
    auto prior_of_left = std::make_shared<Poi::Application>(
      application_prior->source_name,
      application_prior->source,
      application_prior->start_pos,
      left.node->end_pos,
      prior_of_left_free_variables,
      application_prior,
      left.node
    );
    right = std::make_shared<Poi::ParseResult<Poi::Term>>(
      right->choose(
        parse_application(
          memo,
          pool,
          token_stream,
          environment,
          iter,
          prior_of_left
        ).upcast<Poi::Term>()
      )
    );
  } else {
    right = std::make_shared<Poi::ParseResult<Poi::Term>>(
      right->choose(
        parse_application(
          memo,
          pool,
          token_stream,
          environment,
          iter,
          left.node
        ).upcast<Poi::Term>()
      )
    );
  }
  if (right->error) {
    return memo_error<Poi::Application>(memo, key, right->error);
  }
  bool right_includes_left = right->node->start_pos < iter->start_pos;
  iter = right->next;

  // Construct the Application. Special care is taken to construct the tree
  // with left-associativity, even though we are parsing with right-recursion.
  std::shared_ptr<const Poi::Application> application;
  if (right_includes_left) {
    application = std::dynamic_pointer_cast<
      const Poi::Application
    >(right->node);
  } else {
    if (application_prior) {
      auto prior_of_left_free_variables = std::make_shared<
        std::unordered_set<size_t>
      >();
      prior_of_left_free_variables->insert(
        application_prior->free_variables->begin(),
        application_prior->free_variables->end()
      );
      prior_of_left_free_variables->insert(
        left.node->free_variables->begin(),
        left.node->free_variables->end()
      );
      auto prior_of_left = std::make_shared<Poi::Application>(
        application_prior->source_name,
        application_prior->source,
        application_prior->start_pos,
        left.node->end_pos,
        prior_of_left_free_variables,
        application_prior,
        left.node
      );

      auto free_variables = std::make_shared<
        std::unordered_set<size_t>
      >();
      free_variables->insert(
        prior_of_left->free_variables->begin(),
        prior_of_left->free_variables->end()
      );
      free_variables->insert(
        right->node->free_variables->begin(),
        right->node->free_variables->end()
      );

      application = std::make_shared<Poi::Application>(
        prior_of_left->source_name,
        prior_of_left->source,
        prior_of_left->start_pos,
        right->node->end_pos,
        free_variables,
        prior_of_left,
        right->node
      );
    } else {
      auto free_variables = std::make_shared<std::unordered_set<size_t>>();
      free_variables->insert(
        left.node->free_variables->begin(),
        left.node->free_variables->end()
      );
      free_variables->insert(
        right->node->free_variables->begin(),
        right->node->free_variables->end()
      );
      application = std::make_shared<Poi::Application>(
        left.node->source_name,
        left.node->source,
        left.node->start_pos,
        right->node->end_pos,
        free_variables,
        left.node,
        right->node
      );
    }
  }

  // Memoize and return the result.
  return memo_success<Poi::Application>(memo, key, application, iter);
}

Poi::ParseResult<Poi::Binding> parse_binding(
  MemoMap &memo,
  const Poi::StringPool &pool,
  const Poi::TokenStream &token_stream,
  const std::unordered_set<size_t> &environment,
  std::vector<Poi::Token>::const_iterator iter
) {
  // Check if we can reuse a memoized result.
  auto key = memo_key(MemoType::BINDING, iter);
  auto memo_result = memo.find(key);
  if (memo_result != memo.end()) {
    return memo_result->second.downcast<Poi::Binding>();
  }

  // Mark the beginning of the Binding.
  auto start = iter;

  // Make sure we have some tokens to parse.
  if (iter == token_stream.tokens->end()) {
    return memo_error<Poi::Binding>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        "No binding to parse.",
        pool.find(token_stream.source_name),
        pool.find(token_stream.source),
        Poi::ErrorConfidence::LOW
      )
    );
  }

  // Parse the Pattern.
  auto pattern = Poi::ParseResult<Poi::Pattern>(
    std::make_shared<Poi::ParseError>(
      "No pattern found for this binding.",
      pool.find(start->source_name),
      pool.find(start->source),
      start->start_pos,
      iter->end_pos,
      Poi::ErrorConfidence::LOW
    )
  ).choose(parse_pattern(memo, pool, token_stream, environment, iter));
  if (pattern.error) {
    return memo_error<Poi::Binding>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        pattern.error->what(),
        Poi::ErrorConfidence::LOW
      )
    );
  }
  iter = pattern.next;

  // Parse the EQUALS.
  if (
    iter == token_stream.tokens->end() ||
    iter->type != Poi::TokenType::EQUALS
  ) {
    return memo_error<Poi::Binding>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        "Expected '=' in this binding.",
        pool.find(start->source_name),
        pool.find(start->source),
        start->start_pos,
        (iter - 1)->end_pos,
        Poi::ErrorConfidence::LOW
      )
    );
  }
  ++iter;

  // Add the pattern variables to the environment.
  auto new_environment = environment;
  new_environment.insert(
    pattern.node->variables->begin(),
    pattern.node->variables->end()
  );

  // Parse the definition.
  auto definition = Poi::ParseResult<Poi::Term>(
    std::make_shared<Poi::ParseError>(
      "No definition found for this binding.",
      pool.find(start->source_name),
      pool.find(start->source),
      start->start_pos,
      (iter - 1)->end_pos,
      Poi::ErrorConfidence::LOW
    )
  ).choose(parse_term(memo, pool, token_stream, new_environment, iter));
  if (definition.error) {
    return memo_error<Poi::Binding>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        definition.error->what(),
        Poi::ErrorConfidence::HIGH
      )
    );
  }
  iter = definition.next;

  // Parse the SEPARATOR.
  if (
    iter == token_stream.tokens->end() ||
    iter->type != Poi::TokenType::SEPARATOR
  ) {
    return memo_error<Poi::Binding>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        "Expected a body for this binding.",
        pool.find(start->source_name),
        pool.find(start->source),
        start->start_pos,
        (iter - 1)->end_pos,
        Poi::ErrorConfidence::HIGH
      )
    );
  }
  ++iter;

  // Parse the body.
  auto body = Poi::ParseResult<Poi::Term>(
    std::make_shared<Poi::ParseError>(
      "No body found for this binding.",
      pool.find(start->source_name),
      pool.find(start->source),
      start->start_pos,
      (iter - 1)->end_pos,
      Poi::ErrorConfidence::LOW
    )
  ).choose(parse_term(memo, pool, token_stream, new_environment, iter));
  if (body.error) {
    return memo_error<Poi::Binding>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        body.error->what(),
        Poi::ErrorConfidence::HIGH
      )
    );
  }
  iter = body.next;

  // Construct the Binding.
  auto free_variables = std::make_shared<std::unordered_set<size_t>>();
  free_variables->insert(
    definition.node->free_variables->begin(),
    definition.node->free_variables->end()
  );
  free_variables->insert(
    body.node->free_variables->begin(),
    body.node->free_variables->end()
  );
  for (auto &variable : *(pattern.node->variables)) {
    free_variables->erase(variable);
  }
  auto binding = std::make_shared<Poi::Binding>(
    start->source_name,
    start->source,
    start->start_pos,
    (iter - 1)->end_pos,
    free_variables,
    pattern.node,
    definition.node,
    body.node
  );

  // Memoize and return the result.
  return memo_success<Poi::Binding>(memo, key, binding, iter);
}

Poi::ParseResult<Poi::DataType> parse_data_type(
  MemoMap &memo,
  const Poi::StringPool &pool,
  const Poi::TokenStream &token_stream,
  const std::unordered_set<size_t> &environment,
  std::vector<Poi::Token>::const_iterator iter
) {
  // Check if we can reuse a memoized result.
  auto key = memo_key(MemoType::DATA_TYPE, iter);
  auto memo_result = memo.find(key);
  if (memo_result != memo.end()) {
    return memo_result->second.downcast<Poi::DataType>();
  }

  // Mark the beginning of the DataType.
  auto start = iter;

  // Make sure we have some tokens to parse.
  if (iter == token_stream.tokens->end()) {
    return memo_error<Poi::DataType>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        "No data type to parse.",
        pool.find(token_stream.source_name),
        pool.find(token_stream.source),
        Poi::ErrorConfidence::LOW
      )
    );
  }

  // Parse the LEFT_CURLY.
  if (iter->type != Poi::TokenType::LEFT_CURLY) {
    return memo_error<Poi::DataType>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        "Expected '{' to introduce a data type.",
        pool.find(start->source_name),
        pool.find(start->source),
        start->start_pos,
        iter->end_pos,
        Poi::ErrorConfidence::LOW
      )
    );
  }
  ++iter;

  // Parse the constructors. Note that the lexical analyzer guarantees
  // parentheses are matched.
  auto constructor_names = std::make_shared<std::vector<size_t>>();
  auto constructor_params = std::make_shared<
    std::unordered_map<size_t, std::vector<size_t>>
  >();
  auto constructors = std::make_shared<
    std::unordered_map<size_t, std::shared_ptr<const Poi::Term>>
  >();
  std::vector<std::shared_ptr<Poi::Data>> data_terms;
  bool first = true;
  while (iter->type != Poi::TokenType::RIGHT_CURLY) {
    // Parse the SEPARATOR if applicable.
    if (first) {
      first = false;
    } else {
      ++iter;
    }

    // Mark the start of the constructor for nice error reporting.
    auto constructor_start = iter;

    // Parse the name of the constructor.
    if (iter->type != Poi::TokenType::IDENTIFIER) {
      return memo_error<Poi::DataType>(
        memo,
        key,
        std::make_shared<Poi::ParseError>(
          "Invalid data constructor.",
          pool.find(constructor_start->source_name),
          pool.find(constructor_start->source),
          constructor_start->start_pos,
          iter->end_pos,
          Poi::ErrorConfidence::MED
        )
      );
    }
    auto name = iter->literal;
    ++iter;

    // Parse the parameters.
    std::vector<size_t> params;
    std::unordered_set<size_t> params_set;
    while (
      iter->type != Poi::TokenType::SEPARATOR &&
      iter->type != Poi::TokenType::RIGHT_CURLY
    ) {
      if (iter->type != Poi::TokenType::IDENTIFIER) {
        return memo_error<Poi::DataType>(
          memo,
          key,
          std::make_shared<Poi::ParseError>(
            "Invalid data constructor.",
            pool.find(constructor_start->source_name),
            pool.find(constructor_start->source),
            constructor_start->start_pos,
            iter->end_pos,
            Poi::ErrorConfidence::MED
          )
        );
      }
      auto parameter = iter->literal;

      // Check whether a parameter of this name already exists.
      if (params_set.find(parameter) != params_set.end()) {
        return memo_error<Poi::DataType>(
          memo,
          key,
          std::make_shared<Poi::ParseError>(
            "Duplicate parameter '" +
               pool.find(parameter) +
               "' in data constructor '" +
               pool.find(name) +
               "'.",
            pool.find(iter->source_name),
            pool.find(iter->source),
            iter->start_pos,
            iter->end_pos,
            Poi::ErrorConfidence::MED
          )
        );
      }

      params.push_back(parameter);
      params_set.insert(parameter);
      ++iter;
    }

    // Check whether a constructor of this name already exists.
    auto existing_constructor = constructor_params->find(name);
    if (existing_constructor != constructor_params->end()) {
      return memo_error<Poi::DataType>(
        memo,
        key,
        std::make_shared<Poi::ParseError>(
          "Duplicate constructor '" +
           pool.find(name) +
           "' in data type.",
          pool.find(constructor_start->source_name),
          pool.find(constructor_start->source),
          constructor_start->start_pos,
          (iter - 1)->end_pos,
          Poi::ErrorConfidence::MED
        )
      );
    }

    // Register the constructor name and parameters.
    constructor_names->push_back(name);
    constructor_params->insert({ name, params });

    // Construct the constructor.
    auto free_variables = std::make_shared<std::unordered_set<size_t>>();
    free_variables->insert(
      params.begin(),
      params.end()
    );
    auto data_term = std::make_shared<Poi::Data>(
      constructor_start->source_name,
      constructor_start->source,
      constructor_start->start_pos,
      (iter - 1)->end_pos,
      free_variables,
      std::weak_ptr<Poi::DataType>(),
      name
    );
    data_terms.push_back(data_term);
    auto constructor = std::static_pointer_cast<Poi::Term>(data_term);
    for (auto iter = params.rbegin(); iter != params.rend(); ++iter) {
      free_variables = std::make_shared<std::unordered_set<size_t>>();
      free_variables->insert(
        constructor->free_variables->begin(),
        constructor->free_variables->end()
      );
      free_variables->erase(*iter);
      auto variables = std::make_shared<const std::unordered_set<size_t>>();
      std::const_pointer_cast<
        std::unordered_set<size_t>
      >(variables)->insert(*iter);
      auto variable_pattern = std::make_shared<Poi::VariablePattern>(
        constructor->source_name,
        constructor->source,
        constructor->start_pos,
        constructor->end_pos,
        *iter,
        variables
      );
      constructor = std::static_pointer_cast<Poi::Term>(
        std::make_shared<Poi::Function>(
          constructor->source_name,
          constructor->source,
          constructor->start_pos,
          constructor->end_pos,
          free_variables,
          variable_pattern,
          constructor
        )
      );
    }
    constructors->insert({ name, constructor });
  }

  // Skip the closing RIGHT_CURLY.
  ++iter;

  // Construct the DataType.
  auto data_type = std::make_shared<Poi::DataType>(
    start->source_name,
    start->source,
    start->start_pos,
    (iter - 1)->end_pos,
    std::make_shared<std::unordered_set<size_t>>(),
    constructor_names,
    constructor_params,
    constructors
  );

  // "Tie the knot" by giving the Data terms a reference to the DataType.
  for (auto &data_term : data_terms) {
    const_cast<std::weak_ptr<const Poi::DataType> &>(
      data_term->data_type
    ) = std::weak_ptr<const Poi::DataType>(data_type);
  }

  // Memoize and return the result.
  return memo_success<Poi::DataType>(memo, key, data_type, iter);
}

Poi::ParseResult<Poi::Member> parse_member(
  MemoMap &memo,
  const Poi::StringPool &pool,
  const Poi::TokenStream &token_stream,
  const std::unordered_set<size_t> &environment,
  std::vector<Poi::Token>::const_iterator iter
) {
  // Check if we can reuse a memoized result.
  auto key = memo_key(MemoType::MEMBER, iter);
  auto memo_result = memo.find(key);
  if (memo_result != memo.end()) {
    return memo_result->second.downcast<Poi::Member>();
  }

  // Mark the beginning of the Member.
  auto start = iter;

  // Make sure we have some tokens to parse.
  if (iter == token_stream.tokens->end()) {
    return memo_error<Poi::Member>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        "No member to parse.",
        pool.find(token_stream.source_name),
        pool.find(token_stream.source),
        Poi::ErrorConfidence::LOW
      )
    );
  }

  // Parse the object.
  auto object = Poi::ParseResult<Poi::Term>(
    std::make_shared<Poi::ParseError>(
      "Unexpected token.",
      pool.find(iter->source_name),
      pool.find(iter->source),
      iter->start_pos,
      iter->end_pos,
      Poi::ErrorConfidence::LOW
    )
  ).choose(
    parse_variable(
      memo, pool, token_stream, environment, iter
    ).upcast<Poi::Term>()
  ).choose(
    parse_data_type(
      memo, pool, token_stream, environment, iter
    ).upcast<Poi::Term>()
  ).choose(
    parse_group(
      memo, pool, token_stream, environment, iter
    ).upcast<Poi::Term>()
  );
  if (object.error) {
    return memo_error<Poi::Member>(memo, key, object.error);
  }
  iter = object.next;

  // Parse the DOT.
  if (
    iter == token_stream.tokens->end() ||
    iter->type != Poi::TokenType::DOT
  ) {
    return memo_error<Poi::Member>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        "Expected '.' for this member access.",
        pool.find(start->source_name),
        pool.find(start->source),
        start->start_pos,
        (iter - 1)->end_pos,
        Poi::ErrorConfidence::LOW
      )
    );
  }
  ++iter;

  // Parse the IDENTIFIER.
  if (
    iter == token_stream.tokens->end() ||
    iter->type != Poi::TokenType::IDENTIFIER
  ) {
    return memo_error<Poi::Member>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        "Invalid member access.",
        pool.find(start->source_name),
        pool.find(start->source),
        start->start_pos,
        (iter - 1)->end_pos,
        Poi::ErrorConfidence::HIGH
      )
    );
  }
  auto field_name = iter->literal;
  ++iter;

  // Construct the Member.
  auto free_variables = std::make_shared<std::unordered_set<size_t>>();
  free_variables->insert(
    object.node->free_variables->begin(),
    object.node->free_variables->end()
  );
  auto member = std::make_shared<Poi::Member>(
    start->source_name,
    start->source,
    start->start_pos,
    (iter - 1)->end_pos,
    free_variables,
    object.node,
    field_name
  );

  // Check for additional member accesses.
  while (
    iter != token_stream.tokens->end() &&
    iter->type == Poi::TokenType::DOT
  ) {
    // Skip the DOT.
    ++iter;

    // Parse the IDENTIFIER.
    if (
      iter == token_stream.tokens->end() ||
      iter->type != Poi::TokenType::IDENTIFIER
    ) {
      return memo_error<Poi::Member>(
        memo,
        key,
        std::make_shared<Poi::ParseError>(
          "Invalid member access.",
          pool.find(start->source_name),
          pool.find(start->source),
          start->start_pos,
          (iter - 1)->end_pos,
          Poi::ErrorConfidence::HIGH
        )
      );
    }
    field_name = iter->literal;
    ++iter;

    // Construct the new Member.
    free_variables = std::make_shared<std::unordered_set<size_t>>();
    free_variables->insert(
      member->free_variables->begin(),
      member->free_variables->end()
    );
    member = std::make_shared<Poi::Member>(
      start->source_name,
      start->source,
      start->start_pos,
      (iter - 1)->end_pos,
      free_variables,
      member,
      field_name
    );
  }

  // Memoize and return the result.
  return memo_success<Poi::Member>(memo, key, member, iter);
}

Poi::ParseResult<Poi::Match> parse_match(
  MemoMap &memo,
  const Poi::StringPool &pool,
  const Poi::TokenStream &token_stream,
  const std::unordered_set<size_t> &environment,
  std::vector<Poi::Token>::const_iterator iter
) {
  // Check if we can reuse a memoized result.
  auto key = memo_key(MemoType::MATCH, iter);
  auto memo_result = memo.find(key);
  if (memo_result != memo.end()) {
    return memo_result->second.downcast<Poi::Match>();
  }

  // Mark the beginning of the Match.
  auto start = iter;

  // Make sure we have some tokens to parse.
  if (iter == token_stream.tokens->end()) {
    return memo_error<Poi::Match>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        "No match expression to parse.",
        pool.find(token_stream.source_name),
        pool.find(token_stream.source),
        Poi::ErrorConfidence::LOW
      )
    );
  }

  // Parse the MATCH.
  if (iter->type != Poi::TokenType::MATCH) {
    return memo_error<Poi::Match>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        "Expected 'match' to start this match expression.",
        pool.find(start->source_name),
        pool.find(start->source),
        start->start_pos,
        iter->end_pos,
        Poi::ErrorConfidence::LOW
      )
    );
  }
  ++iter;

  // Parse the discriminee.
  auto discriminee = parse_term(
    memo, pool, token_stream, environment, iter
  ).choose(
    Poi::ParseResult<Poi::Term>(
      std::make_shared<Poi::ParseError>(
        "No discriminee found for this match expression.",
        pool.find(start->source_name),
        pool.find(start->source),
        start->start_pos,
        (iter - 1)->end_pos,
        Poi::ErrorConfidence::LOW
      )
    )
  );
  if (discriminee.error) {
    return memo_error<Poi::Match>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        discriminee.error->what(),
        Poi::ErrorConfidence::HIGH
      )
    );
  }
  auto free_variables = std::make_shared<std::unordered_set<size_t>>();
  free_variables->insert(
    discriminee.node->free_variables->begin(),
    discriminee.node->free_variables->end()
  );
  iter = discriminee.next;

  // Parse the LEFT_CURLY.
  if (
    iter == token_stream.tokens->end() ||
    iter->type != Poi::TokenType::LEFT_CURLY
  ) {
    return memo_error<Poi::Match>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        "Expected '{' to begin the cases for this match expression.",
        pool.find(start->source_name),
        pool.find(start->source),
        start->start_pos,
        (iter - 1)->end_pos,
        Poi::ErrorConfidence::HIGH
      )
    );
  }
  ++iter;

  // Parse the cases. Note that the lexical analyzer guarantees
  // curly braces are matched.
  auto cases = std::make_shared<
    std::vector<std::shared_ptr<const Poi::Function>>
  >();
  bool first = true;
  while (iter->type != Poi::TokenType::RIGHT_CURLY) {
    // Parse the SEPARATOR if applicable.
    if (first) {
      first = false;
    } else {
      if (iter->type != Poi::TokenType::SEPARATOR) {
        return memo_error<Poi::Match>(
          memo,
          key,
          std::make_shared<Poi::ParseError>(
            "Invalid case in this match expression.",
            pool.find(iter->source_name),
            pool.find(iter->source),
            iter->start_pos,
            iter->end_pos,
            Poi::ErrorConfidence::HIGH
          )
        );
      }
      ++iter;
    }

    // Parse the case.
    auto c = Poi::ParseResult<Poi::Function>(
      std::make_shared<Poi::ParseError>(
        "Invalid case in this match expression.",
        pool.find(iter->source_name),
        pool.find(iter->source),
        iter->start_pos,
        iter->end_pos,
        Poi::ErrorConfidence::LOW
      )
    ).choose(parse_function(memo, pool, token_stream, environment, iter));
    if (c.error) {
      return memo_error<Poi::Match>(
        memo,
        key,
        std::make_shared<Poi::ParseError>(
          c.error->what(),
          Poi::ErrorConfidence::HIGH
        )
      );
    }
    free_variables->insert(
      c.node->free_variables->begin(),
      c.node->free_variables->end()
    );
    cases->push_back(c.node);
    iter = c.next;
  }

  // Make sure we have at least one case.
  if (cases->empty()) {
    return memo_error<Poi::Match>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        "A match expression must have at least one case.",
        pool.find(start->source_name),
        pool.find(start->source),
        start->start_pos,
        iter->end_pos,
        Poi::ErrorConfidence::HIGH
      )
    );
  }

  // Skip the closing RIGHT_CURLY.
  ++iter;

  // Construct the Match.
  auto match = std::make_shared<Poi::Match>(
    start->source_name,
    start->source,
    start->start_pos,
    (iter - 1)->end_pos,
    free_variables,
    discriminee.node,
    cases
  );

  // Memoize and return the result.
  return memo_success<Poi::Match>(memo, key, match, iter);
}

Poi::ParseResult<Poi::Term> parse_group(
  MemoMap &memo,
  const Poi::StringPool &pool,
  const Poi::TokenStream &token_stream,
  const std::unordered_set<size_t> &environment,
  std::vector<Poi::Token>::const_iterator iter
) {
  // Check if we can reuse a memoized result.
  auto key = memo_key(MemoType::GROUP, iter);
  auto memo_result = memo.find(key);
  if (memo_result != memo.end()) {
    return memo_result->second.downcast<Poi::Term>();
  }

  // Mark the beginning of the Group.
  auto start = iter;

  // Make sure we have some tokens to parse.
  if (iter == token_stream.tokens->end()) {
    return memo_error<Poi::Term>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        "No group to parse.",
        pool.find(token_stream.source_name),
        pool.find(token_stream.source),
        Poi::ErrorConfidence::LOW
      )
    );
  }

  // Parse the LEFT_PAREN.
  if (iter->type != Poi::TokenType::LEFT_PAREN) {
    return memo_error<Poi::Term>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        "Expected '(' to start this group.",
        pool.find(start->source_name),
        pool.find(start->source),
        start->start_pos,
        iter->end_pos,
        Poi::ErrorConfidence::LOW
      )
    );
  }
  ++iter;

  // Parse the body.
  auto body = parse_term(
    memo, pool, token_stream, environment, iter
  ).choose(
    Poi::ParseResult<Poi::Term>(
      std::make_shared<Poi::ParseError>(
        "No body found for this group.",
        pool.find(start->source_name),
        pool.find(start->source),
        start->start_pos,
        (iter - 1)->end_pos,
        Poi::ErrorConfidence::LOW
      )
    )
  );
  if (body.error) {
    return memo_error<Poi::Term>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        body.error->what(),
        Poi::ErrorConfidence::HIGH
      )
    );
  }
  iter = body.next;

  // Parse the RIGHT_PAREN.
  if (
    iter == token_stream.tokens->end() ||
    iter->type != Poi::TokenType::RIGHT_PAREN
  ) {
    return memo_error<Poi::Term>(
      memo,
      key,
      std::make_shared<Poi::ParseError>(
        "Expected ')' to close this group.",
        pool.find(start->source_name),
        pool.find(start->source),
        start->start_pos,
        (iter - 1)->end_pos,
        Poi::ErrorConfidence::HIGH
      )
    );
  }
  ++iter;

  // Memoize and return the result.
  return memo_success<Poi::Term>(memo, key, body.node, iter);
}

///////////////////////////////////////////////////////////////////////////////
// Public interface                                                          //
///////////////////////////////////////////////////////////////////////////////

std::shared_ptr<const Poi::Term> Poi::parse(
  const Poi::TokenStream &token_stream,
  const Poi::StringPool &pool
) {
  // The environment records which variables are currently in scope.
  std::unordered_set<size_t> environment;

  // Memoize the results of recursive descent calls.
  // This is the "packrat parser" technique.
  auto tokens_end = token_stream.tokens->end();
  MemoMap memo(1000, [tokens_end](const MemoKey &key) {
    // Unpack the tuple.
    auto memo_type = std::get<0>(key);
    auto start = std::get<1>(key);
    auto application_prior = std::get<2>(key);

    // Get the hash of each component.
    size_t memo_type_hash =
      static_cast<typename std::underlying_type<MemoType>::type>(memo_type);
    size_t start_hash = 0;
    if (start != tokens_end) {
      start_hash = reinterpret_cast<size_t>(&(*start));
    }
    size_t application_prior_hash = 0;
    if (application_prior) {
      application_prior_hash = reinterpret_cast<size_t>(&(*application_prior));
    }

    // To combine the hashes, we use the hash_combine trick from Boost.
    size_t combined_hash = memo_type_hash;
    combined_hash ^= 0x9e3779b9 +
      (combined_hash << 6) + (combined_hash >> 2) + start_hash;
    combined_hash ^= 0x9e3779b9 +
      (combined_hash << 6) + (combined_hash >> 2) + application_prior_hash;
    return combined_hash;
  });

  // Make sure we have some tokens to parse.
  if (token_stream.tokens->empty()) {
    throw Poi::Error(
      "Nothing to parse.",
      pool.find(token_stream.source_name),
      pool.find(token_stream.source)
    );
  }

  // Parse the term.
  Poi::ParseResult<Poi::Term> term = parse_term(
    memo,
    pool,
    token_stream,
    environment,
    token_stream.tokens->begin()
  );

  // Check if there was an error.
  if (term.error) {
    throw Poi::Error(term.error->what());
  }

  // Make sure we parsed the whole file.
  if (term.next != token_stream.tokens->end()) {
    throw Poi::Error(
      "The end of the file was expected here.",
      pool.find(term.next->source_name),
      pool.find(term.next->source),
      term.next->start_pos,
      term.next->end_pos
    );
  }

  // Return the term.
  return term.node;
}
