#include <functional>
#include <poi/error.h>
#include <poi/parser.h>
#include <tuple>
#include <type_traits>
#include <unordered_map>

/*
  Poi uses a packrat parser, i.e., a recursive descent parser with
  memoization. This guarantees linear-time parsing.

  In the following grammars, nonterminals are written in UpperCamelCase and
  terminals (tokens) are written in MACRO_CASE.

  We start with a grammar which formalizes the language constructs, but without
  concerning ourselves with precedence and associativity. The grammar below is
  ambiguous, and below we will resolve ambiguities by encoding precedence and
  associativity into the production rules.

    Term =
      Variable | Abstraction | Application | Let | DataType | Member | Group
    Variable = IDENTIFIER
    Abstraction = IDENTIFIER ARROW Term
    Application = Term Term
    Let = IDENTIFIER EQUALS Term SEPARATOR Term
    DataType = DATA LEFT_PAREN DataConstructorList RIGHT_PAREN
    DataConstructorList = | DataConstructor DataConstructorTail
    DataConstructorTail = | SEPARATOR DataConstructor DataConstructorTail
    DataConstructor = IDENTIFIER DataConstructorParams
    DataConstructorParams = | IDENTIFIER DataConstructorParams
    Member = Term DOT IDENTIFIER
    Group = LEFT_PAREN Term RIGHT_PAREN

  We note the following ambiguities, and the chosen resolutions:

    # Resolution: Application has higher precedence than Abstraction.
    x -> (t t)
    (x -> t) t

    # Resolution: Member has higher precedence than Abstraction.
    x -> (t . x)
    (x -> t) . x

    # Resolution: Application is left-associative.
    (t x) t
    t (x t)

    # Resolution: Application has higher precedence than Let.
    x = t, (x t)
    (x = t, x) t

    # Resolution: Member has higher precedence than Application.
    t (x . x)
    (t x) . x

    # Resolution: Member has higher precedence than Let.
    x = t, (x . x)
    (x = t, x) . x

    # Resolution: The right side of an Application cannot be an Abstraction.
    (t (x -> x)) t
    t (x -> (x t))

    # Resolution: The right side of an Application cannot be a Let.
    t (x = t, (x t))
    (t (x = t, x)) t

  We can resolve the ambiguities in the grammar by expanding definitions and
  eliminating alternatives:

    Term =
      Variable | Abstraction | Application | Let | DataType | Member | Group
    Variable = IDENTIFIER
    Abstraction = IDENTIFIER ARROW Term
    Application =
      (Variable | Application | DataType | Member | Group)
      (Variable | DataType | Member | Group)
    Let = IDENTIFIER EQUALS Term SEPARATOR Term
    DataType = DATA LEFT_PAREN DataConstructorList RIGHT_PAREN
    DataConstructorList = | DataConstructor DataConstructorTail
    DataConstructorTail = | SEPARATOR DataConstructor DataConstructorTail
    DataConstructor = IDENTIFIER DataConstructorParams
    DataConstructorParams = | IDENTIFIER DataConstructorParams
    Member = (Variable | DataType | Member | Group) DOT IDENTIFIER
    Group = LEFT_PAREN Term RIGHT_PAREN

  There are still two problems with this grammar: the Application and Member
  rules are left-recursive, and packrat parsers can't handle left-recursion:

    Application =
      (Variable | Application | DataType | Member | Group)
      (Variable | DataType | Member | Group)
    Member = (Variable | DataType | Member | Group) DOT IDENTIFIER

  To fix Application, we rewrite the rule to use right-recursion instead:

    Application =
      (Variable | DataType | Member | Group)
      (Variable | Application | DataType | Member | Group)

  This makes Application have right-associativity, which is not what we want.
  In the parsing rule for Application, we use a special trick to flip the
  associativity from right to left. Instead of building up the tree from the
  results of right-recursive calls, we pass the left term (`application_prior`)
  to the right-recursive call and let it assemble the tree with left-
  associativity.

  To fix Member, we rewrite the rule to eliminate the left recursion:

    Member = (Variable | DataType | Group) DOT IDENTIFIER MemberSuffix
    MemberSuffix = | DOT IDENTIFIER

*/

///////////////////////////////////////////////////////////////////////////////
// Memoization                                                               //
///////////////////////////////////////////////////////////////////////////////

enum class MemoType {
  TERM,
  VARIABLE,
  ABSTRACTION,
  APPLICATION,
  LET,
  DATA_TYPE,
  MEMBER,
  GROUP
};

using MemoKey = std::tuple<
  MemoType,
  std::vector<poi::Token>::iterator, // The start token
  bool, // Whether this term is the top-level group
  std::shared_ptr<poi::Term> // The `application_prior` term
>;

using MemoValue = std::tuple<
  std::shared_ptr<poi::Term>, // The returned term
  std::vector<poi::Token>::iterator // The token after the returned term
>;

using MemoMap = std::unordered_map<
  MemoKey,
  MemoValue,
  std::function<size_t(const MemoKey &key)>
>;

#define MEMO_KEY_GENERAL( \
  memo_type, \
  begin, \
  top_level, \
  application_prior \
) \
  make_tuple( \
    MemoType::memo_type, \
    (begin), \
    (top_level), \
    (application_prior) \
  )

#define MEMO_KEY_SIMPLE(memo_type, begin) \
  MEMO_KEY_GENERAL(memo_type, (begin), false, std::shared_ptr<poi::Term>())

#define MEMO_CHECK(memo, key, return_type, next) do { \
  auto &m = (memo); \
  auto memo_result = m.find((key)); \
  if (memo_result != m.end()) { \
    (next) = std::get<1>(memo_result->second); \
    return std::dynamic_pointer_cast<poi::return_type>( \
      std::get<0>(memo_result->second) \
    ); \
  } \
} while (false)

#define MEMOIZE_AND_RETURN(memo_key, term, next) do { \
  auto n = (term); \
  memo.insert({(memo_key), make_tuple( \
    std::static_pointer_cast<poi::Term>(n), \
    (next) \
  )}); \
  return n; \
} while (false)

#define MEMOIZE_AND_FAIL(memo_key, type, begin, next) do { \
  auto n = std::shared_ptr<poi::type>(); \
  (next) = (begin); \
  memo.insert({(memo_key), make_tuple( \
    std::static_pointer_cast<poi::Term>(n), \
    (next) \
  )}); \
  return n; \
} while (false)

///////////////////////////////////////////////////////////////////////////////
// Parsing                                                                   //
///////////////////////////////////////////////////////////////////////////////

#define TRY_RULE(begin, next, term, candidate) do { \
  auto old_next = (next); \
  (next) = (begin); \
  auto c = (candidate); \
  if (c) { \
    if (term) { \
      if (c->end_pos > term->end_pos) { \
        (term) = c; \
      } else { \
        (next) = old_next; \
      } \
    } else { \
      (term) = c; \
    } \
  } else { \
    (next) = old_next; \
  } \
} while (false)

std::shared_ptr<poi::Term> parse_term(
  MemoMap &memo,
  std::vector<poi::Token> &tokens,
  std::vector<poi::Token>::iterator &next,
  std::unordered_set<size_t> &environment,
  poi::StringPool &pool
);

std::shared_ptr<poi::Variable> parse_variable(
  MemoMap &memo,
  std::vector<poi::Token> &tokens,
  std::vector<poi::Token>::iterator &next,
  std::unordered_set<size_t> &environment,
  poi::StringPool &pool
);

std::shared_ptr<poi::Abstraction> parse_abstraction(
  MemoMap &memo,
  std::vector<poi::Token> &tokens,
  std::vector<poi::Token>::iterator &next,
  std::unordered_set<size_t> &environment,
  poi::StringPool &pool
);

std::shared_ptr<poi::Application> parse_application(
  MemoMap &memo,
  std::vector<poi::Token> &tokens,
  std::vector<poi::Token>::iterator &next,
  std::shared_ptr<poi::Term> application_prior,
  std::unordered_set<size_t> &environment,
  poi::StringPool &pool
);

std::shared_ptr<poi::Let> parse_let(
  MemoMap &memo,
  std::vector<poi::Token> &tokens,
  std::vector<poi::Token>::iterator &next,
  std::unordered_set<size_t> &environment,
  poi::StringPool &pool
);

std::shared_ptr<poi::DataType> parse_data_type(
  MemoMap &memo,
  std::vector<poi::Token> &tokens,
  std::vector<poi::Token>::iterator &next,
  std::unordered_set<size_t> &environment,
  poi::StringPool &pool
);

std::shared_ptr<poi::Member> parse_member(
  MemoMap &memo,
  std::vector<poi::Token> &tokens,
  std::vector<poi::Token>::iterator &next,
  std::unordered_set<size_t> &environment,
  poi::StringPool &pool
);

std::shared_ptr<poi::Term> parse_group(
  MemoMap &memo,
  std::vector<poi::Token> &tokens,
  std::vector<poi::Token>::iterator &next,
  std::unordered_set<size_t> &environment,
  poi::StringPool &pool,
  bool top_level
);

std::shared_ptr<poi::Term> parse_term(
  MemoMap &memo,
  std::vector<poi::Token> &tokens,
  std::vector<poi::Token>::iterator &next,
  std::unordered_set<size_t> &environment,
  poi::StringPool &pool
) {
  // Check if we can reuse a memoized result.
  auto begin = next;
  auto memo_key = MEMO_KEY_SIMPLE(TERM, begin);
  MEMO_CHECK(memo, memo_key, Term, next);

  // A term is one of the following constructs.
  std::shared_ptr<poi::Term> term;
  TRY_RULE(
    begin,
    next,
    term,
    parse_variable(memo, tokens, next, environment, pool)
  );
  TRY_RULE(
    begin,
    next,
    term,
    parse_abstraction(memo, tokens, next, environment, pool)
  );
  TRY_RULE(
    begin,
    next,
    term,
    parse_application(
      memo,
      tokens,
      next,
      nullptr,
      environment,
      pool
    )
  );
  TRY_RULE(
    begin,
    next,
    term,
    parse_let(memo, tokens, next, environment, pool)
  );
  TRY_RULE(
    begin,
    next,
    term,
    parse_data_type(memo, tokens, next, environment, pool)
  );
  TRY_RULE(
    begin,
    next,
    term,
    parse_member(memo, tokens, next, environment, pool)
  );
  TRY_RULE(
    begin,
    next,
    term,
    parse_group(memo, tokens, next, environment, pool, false)
  );

  // Memoize whatever we parsed and return it.
  MEMOIZE_AND_RETURN(memo_key, term, next);
}

std::shared_ptr<poi::Variable> parse_variable(
  MemoMap &memo,
  std::vector<poi::Token> &tokens,
  std::vector<poi::Token>::iterator &next,
  std::unordered_set<size_t> &environment,
  poi::StringPool &pool
) {
  // Check if we can reuse a memoized result.
  auto begin = next;
  auto memo_key = MEMO_KEY_SIMPLE(VARIABLE, begin);
  MEMO_CHECK(memo, memo_key, Variable, next);

  // Make sure we have an IDENTIFIER.
  if (next == tokens.end() || next->type != poi::TokenType::IDENTIFIER) {
    MEMOIZE_AND_FAIL(memo_key, Variable, begin, next);
  }

  // Look up the variable in the environment.
  if (environment.find(next->literal) == environment.end()) {
    if (
      next + 1 != tokens.end() && (
        (next + 1)->type == poi::TokenType::EQUALS ||
        (next + 1)->type == poi::TokenType::ARROW
      )
    ) {
      MEMOIZE_AND_FAIL(memo_key, Variable, begin, next);
    } else {
      throw poi::Error(
        "Undefined variable '" + pool.find(next->literal) + "'.",
        pool.find(begin->source), pool.find(begin->source_name),
        begin->start_pos, next->end_pos
      );
    }
  }

  // Construct the Variable.
  auto free_variables = std::make_shared<std::unordered_set<size_t>>();
  free_variables->insert(next->literal);
  auto variable = std::make_shared<poi::Variable>(
    begin->source_name,
    begin->source,
    begin->start_pos,
    (next - 1)->end_pos,
    free_variables,
    next->literal
  );
  ++next;

  // Memoize whatever we parsed and return it.
  MEMOIZE_AND_RETURN(memo_key, variable, next);
}

std::shared_ptr<poi::Abstraction> parse_abstraction(
  MemoMap &memo,
  std::vector<poi::Token> &tokens,
  std::vector<poi::Token>::iterator &next,
  std::unordered_set<size_t> &environment,
  poi::StringPool &pool
) {
  // Check if we can reuse a memoized result.
  auto begin = next;
  auto memo_key = MEMO_KEY_SIMPLE(ABSTRACTION, begin);
  MEMO_CHECK(memo, memo_key, Abstraction, next);

  // Parse the IDENTIFIER.
  if (next == tokens.end() || next->type != poi::TokenType::IDENTIFIER) {
    MEMOIZE_AND_FAIL(memo_key, Abstraction, begin, next);
  }
  auto variable = next->literal;
  ++next;

  // Parse the ARROW.
  if (next == tokens.end() || next->type != poi::TokenType::ARROW) {
    MEMOIZE_AND_FAIL(memo_key, Abstraction, begin, next);
  }
  ++next;

  // Add the variable to the environment.
  auto new_environment = environment;
  new_environment.insert(variable);

  // Parse the body.
  auto body = parse_term(
    memo,
    tokens,
    next,
    new_environment,
    pool
  );
  if (!body) {
    throw poi::Error(
      "This function needs a body.",
      pool.find(begin->source), pool.find(begin->source_name),
      begin->start_pos, (next - 1)->end_pos
    );
  }

  // Construct the Abstraction.
  auto free_variables = std::make_shared<std::unordered_set<size_t>>();
  free_variables->insert(
    body->free_variables->begin(),
    body->free_variables->end()
  );
  free_variables->erase(variable);
  auto abstraction = std::make_shared<poi::Abstraction>(
    begin->source_name,
    begin->source,
    begin->start_pos,
    (next - 1)->end_pos,
    free_variables,
    variable,
    body
  );

  // Memoize whatever we parsed and return it.
  MEMOIZE_AND_RETURN(memo_key, abstraction, next);
}

std::shared_ptr<poi::Application> parse_application(
  MemoMap &memo,
  std::vector<poi::Token> &tokens,
  std::vector<poi::Token>::iterator &next,
  std::shared_ptr<poi::Term> application_prior,
  std::unordered_set<size_t> &environment,
  poi::StringPool &pool
) {
  // Check if we can reuse a memoized result.
  auto begin = next;
  auto memo_key = MEMO_KEY_GENERAL(
    APPLICATION,
    begin,
    false,
    application_prior
  );
  MEMO_CHECK(memo, memo_key, Application, next);

  // Parse the left term.
  std::shared_ptr<poi::Term> left_term;
  auto left_term_begin = next;
  TRY_RULE(
    left_term_begin,
    next,
    left_term,
    parse_variable(memo, tokens, next, environment, pool)
  );
  TRY_RULE(
    left_term_begin,
    next,
    left_term,
    parse_data_type(memo, tokens, next, environment, pool)
  );
  TRY_RULE(
    left_term_begin,
    next,
    left_term,
    parse_member(memo, tokens, next, environment, pool)
  );
  TRY_RULE(
    left_term_begin,
    next,
    left_term,
    parse_group(memo, tokens, next, environment, pool, false)
  );
  if (!left_term) {
    MEMOIZE_AND_FAIL(memo_key, Application, begin, next);
  }

  // Parse the right term.
  std::shared_ptr<poi::Term> right_term;
  auto right_term_begin = next;
  TRY_RULE(
    right_term_begin,
    next,
    right_term,
    parse_variable(memo, tokens, next, environment, pool)
  );
  TRY_RULE(
    right_term_begin,
    next,
    right_term,
    parse_data_type(memo, tokens, next, environment, pool)
  );
  TRY_RULE(
    right_term_begin,
    next,
    right_term,
    parse_member(memo, tokens, next, environment, pool)
  );
  TRY_RULE(
    right_term_begin,
    next,
    right_term,
    parse_group(memo, tokens, next, environment, pool, false)
  );
  std::shared_ptr<poi::Application> right_application;
  if (application_prior) {
    auto free_variables = std::make_shared<std::unordered_set<size_t>>();
    free_variables->insert(
      application_prior->free_variables->begin(),
      application_prior->free_variables->end()
    );
    free_variables->insert(
      left_term->free_variables->begin(),
      left_term->free_variables->end()
    );
    auto prior_of_left = std::make_shared<poi::Application>(
      application_prior->source_name,
      application_prior->source,
      application_prior->start_pos,
      left_term->end_pos,
      free_variables,
      application_prior,
      left_term
    );
    TRY_RULE(
      right_term_begin,
      next,
      right_term,
      parse_application(
        memo,
        tokens,
        next,
        prior_of_left,
        environment,
        pool
      )
    );
  } else {
    TRY_RULE(
      right_term_begin,
      next,
      right_term,
      parse_application(
        memo,
        tokens,
        next,
        left_term,
        environment,
        pool
      )
    );
  }
  if (!right_term) {
    MEMOIZE_AND_FAIL(memo_key, Application, begin, next);
  }

  // Construct the Application. Special care is taken to construct the tree
  // with left-associativity, even though we are parsing with right-recursion.
  std::shared_ptr<poi::Application> application;
  right_application = std::dynamic_pointer_cast<poi::Application>(
    right_term
  );
  if (right_application) {
    application = right_application;
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
        left_term->free_variables->begin(),
        left_term->free_variables->end()
      );
      auto prior_of_left = std::make_shared<poi::Application>(
        application_prior->source_name,
        application_prior->source,
        application_prior->start_pos,
        left_term->end_pos,
        prior_of_left_free_variables,
        application_prior,
        left_term
      );

      auto free_variables = std::make_shared<
        std::unordered_set<size_t>
      >();
      free_variables->insert(
        prior_of_left->free_variables->begin(),
        prior_of_left->free_variables->end()
      );
      free_variables->insert(
        right_term->free_variables->begin(),
        right_term->free_variables->end()
      );

      application = std::make_shared<poi::Application>(
        prior_of_left->source_name,
        prior_of_left->source,
        prior_of_left->start_pos,
        right_term->end_pos,
        free_variables,
        prior_of_left,
        right_term
      );
    } else {
      auto free_variables = std::make_shared<std::unordered_set<size_t>>();
      free_variables->insert(
        left_term->free_variables->begin(),
        left_term->free_variables->end()
      );
      free_variables->insert(
        right_term->free_variables->begin(),
        right_term->free_variables->end()
      );
      application = std::make_shared<poi::Application>(
        left_term->source_name,
        left_term->source,
        left_term->start_pos,
        right_term->end_pos,
        free_variables,
        left_term,
        right_term
      );
    }
  }

  // Memoize whatever we parsed and return it.
  MEMOIZE_AND_RETURN(memo_key, application, next);
}

std::shared_ptr<poi::Let> parse_let(
  MemoMap &memo,
  std::vector<poi::Token> &tokens,
  std::vector<poi::Token>::iterator &next,
  std::unordered_set<size_t> &environment,
  poi::StringPool &pool
) {
  // Check if we can reuse a memoized result.
  auto begin = next;
  auto memo_key = MEMO_KEY_SIMPLE(LET, begin);
  MEMO_CHECK(memo, memo_key, Let, next);

  // Parse the IDENTIFIER.
  if (next == tokens.end() || next->type != poi::TokenType::IDENTIFIER) {
    MEMOIZE_AND_FAIL(memo_key, Let, begin, next);
  }
  auto variable = next->literal;
  ++next;

  // Parse the EQUALS.
  if (next == tokens.end() || next->type != poi::TokenType::EQUALS) {
    MEMOIZE_AND_FAIL(memo_key, Let, begin, next);
  }
  ++next;

  // Parse the definition.
  auto definition = parse_term(
    memo,
    tokens,
    next,
    environment,
    pool
  );
  if (!definition) {
    throw poi::Error(
      "This variable needs a definition.",
      pool.find(begin->source), pool.find(begin->source_name),
      begin->start_pos, (next - 1)->end_pos
    );
  }

  // Parse the SEPARATOR.
  if (next == tokens.end() || next->type != poi::TokenType::SEPARATOR) {
    throw poi::Error(
      "This let needs a body.",
      pool.find(begin->source), pool.find(begin->source_name),
      begin->start_pos, (next - 1)->end_pos
    );
  }
  ++next;

  // Add the variable to the environment.
  auto new_environment = environment;
  new_environment.insert(variable);

  // Parse the body.
  auto body = parse_term(
    memo,
    tokens,
    next,
    new_environment,
    pool
  );
  if (!body) {
    throw poi::Error(
      "This let needs a body.",
      pool.find(begin->source), pool.find(begin->source_name),
      begin->start_pos, (next - 1)->end_pos
    );
  }

  // Construct the Let.
  auto free_variables = std::make_shared<std::unordered_set<size_t>>();
  free_variables->insert(
    body->free_variables->begin(),
    body->free_variables->end()
  );
  free_variables->erase(variable);
  auto let = std::make_shared<poi::Let>(
    begin->source_name,
    begin->source,
    begin->start_pos,
    (next - 1)->end_pos,
    free_variables,
    variable,
    definition,
    body
  );

  // Memoize whatever we parsed and return it.
  MEMOIZE_AND_RETURN(memo_key, let, next);
}

std::shared_ptr<poi::DataType> parse_data_type(
  MemoMap &memo,
  std::vector<poi::Token> &tokens,
  std::vector<poi::Token>::iterator &next,
  std::unordered_set<size_t> &environment,
  poi::StringPool &pool
) {
  // Check if we can reuse a memoized result.
  auto begin = next;
  auto memo_key = MEMO_KEY_SIMPLE(DATA_TYPE, begin);
  MEMO_CHECK(memo, memo_key, DataType, next);

  // Parse the DATA.
  if (next == tokens.end() || next->type != poi::TokenType::DATA) {
    MEMOIZE_AND_FAIL(memo_key, DataType, begin, next);
  }
  ++next;

  // Parse the LEFT_PAREN.
  if (next == tokens.end() || next->type != poi::TokenType::LEFT_PAREN) {
    throw poi::Error(
      "Expected '(' after 'data'.",
      pool.find(begin->source), pool.find(begin->source_name),
      begin->start_pos, (next - 1)->end_pos
    );
  }
  ++next;

  // Parse the constructors. Note that the lexical analyzer guarantees
  // parentheses are matched.
  auto constructor_names = std::make_shared<std::vector<size_t>>();
  auto constructors = std::make_shared<
    std::unordered_map<size_t, std::vector<size_t>>
  >();
  bool first_constructor = true;
  while (next->type != poi::TokenType::RIGHT_PAREN) {
    // Parse the SEPARATOR if applicable.
    if (first_constructor) {
      first_constructor = false;
    } else {
      ++next;
    }

    // Mark the start of the constructor for nice error reporting.
    auto constructor_start = next;

    // Parse the name of the constructor.
    if (next->type != poi::TokenType::IDENTIFIER) {
      throw poi::Error(
        "Invalid data constructor.",
        pool.find(begin->source), pool.find(begin->source_name),
        constructor_start->start_pos, next->end_pos
      );
    }
    auto name = next->literal;
    ++next;

    // Parse the parameters.
    std::vector<size_t> params;
    while (
      next->type != poi::TokenType::SEPARATOR &&
      next->type != poi::TokenType::RIGHT_PAREN
    ) {
      if (next->type != poi::TokenType::IDENTIFIER) {
        throw poi::Error(
          "Invalid data constructor.",
          pool.find(begin->source), pool.find(begin->source_name),
          constructor_start->start_pos, next->end_pos
        );
      }
      auto parameter = next->literal;
      params.push_back(parameter);
      ++next;
    }

    // Construct the DataConstructor.
    constructor_names->push_back(name);
    constructors->insert({ name, params });
  }

  // Skip the closing RIGHT_PAREN.
  ++next;

  // Construct the DataType.
  auto data_type = std::make_shared<poi::DataType>(
    begin->source_name,
    begin->source,
    begin->start_pos,
    (next - 1)->end_pos,
    std::make_shared<std::unordered_set<size_t>>(),
    constructor_names,
    constructors
  );

  // Memoize whatever we parsed and return it.
  MEMOIZE_AND_RETURN(memo_key, data_type, next);
}

std::shared_ptr<poi::Member> parse_member(
  MemoMap &memo,
  std::vector<poi::Token> &tokens,
  std::vector<poi::Token>::iterator &next,
  std::unordered_set<size_t> &environment,
  poi::StringPool &pool
) {
  // Check if we can reuse a memoized result.
  auto begin = next;
  auto memo_key = MEMO_KEY_SIMPLE(MEMBER, begin);
  MEMO_CHECK(memo, memo_key, Member, next);

  // Parse the object.
  std::shared_ptr<poi::Term> object;
  TRY_RULE(
    begin,
    next,
    object,
    parse_variable(memo, tokens, next, environment, pool)
  );
  TRY_RULE(
    begin,
    next,
    object,
    parse_data_type(memo, tokens, next, environment, pool)
  );
  TRY_RULE(
    begin,
    next,
    object,
    parse_group(memo, tokens, next, environment, pool, false)
  );
  if (!object) {
    MEMOIZE_AND_FAIL(memo_key, Member, begin, next);
  }

  // Parse the DOT.
  if (next == tokens.end() || next->type != poi::TokenType::DOT) {
    MEMOIZE_AND_FAIL(memo_key, Member, begin, next);
  }
  ++next;

  // Parse the IDENTIFIER.
  if (next == tokens.end() || next->type != poi::TokenType::IDENTIFIER) {
    throw poi::Error(
      "Invalid member access.",
      pool.find(begin->source), pool.find(begin->source_name),
      begin->start_pos, (next - 1)->end_pos
    );
  }
  auto field = next->literal;
  ++next;

  // Construct the Member.
  auto free_variables = std::make_shared<std::unordered_set<size_t>>();
  free_variables->insert(
    object->free_variables->begin(),
    object->free_variables->end()
  );
  auto member = std::make_shared<poi::Member>(
    begin->source_name,
    begin->source,
    begin->start_pos,
    (next - 1)->end_pos,
    free_variables,
    object,
    field
  );

  // Check for additional member accesses.
  while (next != tokens.end() && next->type == poi::TokenType::DOT) {
    // Skip the DOT.
    ++next;

    // Parse the IDENTIFIER.
    if (next == tokens.end() || next->type != poi::TokenType::IDENTIFIER) {
      throw poi::Error(
        "Invalid member access.",
        pool.find(begin->source), pool.find(begin->source_name),
        begin->start_pos, (next - 1)->end_pos
      );
    }
    field = next->literal;
    ++next;

    // Construct the new Member.
    free_variables = std::make_shared<std::unordered_set<size_t>>();
    free_variables->insert(
      member->free_variables->begin(),
      member->free_variables->end()
    );
    member = std::make_shared<poi::Member>(
      begin->source_name,
      begin->source,
      begin->start_pos,
      (next - 1)->end_pos,
      free_variables,
      member,
      field
    );
  }

  // Memoize whatever we parsed and return it.
  MEMOIZE_AND_RETURN(memo_key, member, next);
}

std::shared_ptr<poi::Term> parse_group(
  MemoMap &memo,
  std::vector<poi::Token> &tokens,
  std::vector<poi::Token>::iterator &next,
  std::unordered_set<size_t> &environment,
  poi::StringPool &pool,
  bool top_level
) {
  // Check if we can reuse a memoized result.
  auto begin = next;
  auto memo_key = MEMO_KEY_GENERAL(
    GROUP,
    begin,
    top_level,
    std::shared_ptr<poi::Term>()
  );
  MEMO_CHECK(memo, memo_key, Term, next);

  // Parse the LEFT_PAREN, if applicable.
  if (next == tokens.end()) {
    MEMOIZE_AND_FAIL(memo_key, Term, begin, next);
  }
  if (!top_level) {
    if (next->type != poi::TokenType::LEFT_PAREN) {
      MEMOIZE_AND_FAIL(memo_key, Term, begin, next);
    }
    ++next;
  }

  // Parse the body.
  auto body = parse_term(
    memo,
    tokens,
    next,
    environment,
    pool
  );
  if (!body) {
    throw poi::Error(
      "Empty group.",
      pool.find(begin->source), pool.find(begin->source_name),
      begin->start_pos, (next - 1)->end_pos
    );
  }

  // Skip the RIGHT_PAREN, if applicable.
  if (!top_level) {
    if (next == tokens.end() || next->type != poi::TokenType::RIGHT_PAREN) {
      throw poi::Error(
        "This group needs to be closed with a ')'.",
        pool.find(begin->source), pool.find(begin->source_name),
        begin->start_pos, (next - 1)->end_pos
      );
    }
    ++next;
  }

  // Memoize whatever we parsed and return it.
  MEMOIZE_AND_RETURN(memo_key, body, next);
}

///////////////////////////////////////////////////////////////////////////////
// Public interface                                                          //
///////////////////////////////////////////////////////////////////////////////

std::shared_ptr<poi::Term> poi::parse(
  std::vector<poi::Token> &tokens,
  poi::StringPool &pool
) {
  // The environment records which variables are currently in scope.
  std::unordered_set<size_t> environment;

  // Memoize the results of recursive descent calls.
  // This is the "packrat parser" technique.
  MemoMap memo(1000, [&tokens](const MemoKey &key) {
    // Unpack the tuple.
    auto memo_type = std::get<0>(key);
    auto begin = std::get<1>(key);
    auto top_level = std::get<2>(key);
    auto application_prior = std::get<3>(key);

    // Get the hash of each component.
    size_t memo_type_hash =
      static_cast<typename std::underlying_type<MemoType>::type>(memo_type);
    size_t begin_hash = 0;
    if (begin != tokens.end()) {
      begin_hash = reinterpret_cast<size_t>(&(*begin));
    }
    size_t top_level_hash = top_level ? 1 : 0;
    size_t application_prior_hash = 0;
    if (application_prior) {
      application_prior_hash = reinterpret_cast<size_t>(&(*application_prior));
    }

    // To combine the hashes, we use the hash_combine trick from Boost.
    size_t combined_hash = memo_type_hash;
    combined_hash ^= 0x9e3779b9 +
      (combined_hash << 6) + (combined_hash >> 2) + begin_hash;
    combined_hash ^= 0x9e3779b9 +
      (combined_hash << 6) + (combined_hash >> 2) + top_level_hash;
    combined_hash ^= 0x9e3779b9 +
      (combined_hash << 6) + (combined_hash >> 2) + application_prior_hash;
    return combined_hash;
  });

  // Let the helper do all the work.
  std::vector<poi::Token>::iterator next = tokens.begin();
  std::shared_ptr<poi::Term> term = parse_group(
    memo,
    tokens,
    next,
    environment,
    pool,
    true
  );

  // Make sure we parsed the whole file.
  if (next != tokens.end()) {
    throw poi::Error(
      "The end of the file was expected here.",
      pool.find(next->source), pool.find(next->source_name),
      next->start_pos, next->end_pos
    );
  }

  // All done!
  return term;
}
