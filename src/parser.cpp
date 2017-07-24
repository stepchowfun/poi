#include <functional>
#include <poi/error.h>
#include <poi/parser.h>
#include <tuple>
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

    Term = Variable | Abstraction | Application | Let | Group
    Variable = IDENTIFIER
    Abstraction = IDENTIFIER ARROW Term
    Application = Term Term
    Let = IDENTIFIER EQUALS Term SEPARATOR Term
    Group = LEFT_PAREN Term RIGHT_PAREN

  We note the following ambiguities, and the chosen resolutions:

    # Resolution: Application has higher precedence than Abstraction.
    x -> (t t)
    (x -> t) t

    # Resolution: Application is left-associative.
    (t x) t
    t (x t)

    # Resolution: Application has higher precedence than Let.
    x = t, (x t)
    (x = t, x) t

    # Resolution: The right side of an Application cannot be an Abstraction.
    (t (x -> x)) t
    t (x -> (x t))

    # Resolution: The right side of an Application cannot be a Let.
    t (x = t, (x t))
    (t (x = t, x)) t

  We can resolve the ambiguities in the grammar by expanding definitions and
  eliminating alternatives:

    Term = Variable | Abstraction | Application | Let | Group
    Variable = IDENTIFIER
    Abstraction = IDENTIFIER ARROW Term
    Application = (Variable | Application | Group) (Variable | Group)
    Let = IDENTIFIER EQUALS Term SEPARATOR Term
    Group = LEFT_PAREN Term RIGHT_PAREN

  There is still a problem with this grammar: the Application rule is left-
  recursive, and packrat parsers can't handle left-recursion:

    Application = (Variable | Application | Group) (Variable | Group)

  To fix this, we rewrite the rule to use right-recursion instead:

    Application = (Variable | Group) (Variable | Group | Application)

  This makes Application have right-associativity, which is not what we want.
  In the parsing rule for Application, we use a special trick to flip the
  associativity from right to left. Instead of building up the tree from the
  results of right-recursive calls, we pass the left term (`application_prior`)
  to the right-recursive call and let it assemble the tree with left-
  associativity.
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
    std::dynamic_pointer_cast<poi::Term>(n), \
    (next) \
  )}); \
  return n; \
} while (false)

#define MEMOIZE_AND_FAIL(memo_key, type, begin, next) do { \
  auto n = std::shared_ptr<poi::type>(); \
  (next) = (begin); \
  memo.insert({(memo_key), make_tuple( \
    std::dynamic_pointer_cast<poi::Term>(n), \
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

void span_tokens(
  poi::Term &term,
  std::vector<poi::Token>::iterator begin,
  std::vector<poi::Token>::iterator end
);

void span_terms(
  poi::Term &term,
  poi::Term &begin,
  poi::Term &end
);

std::shared_ptr<poi::Term> parse_term(
  MemoMap &memo,
  std::vector<poi::Token> &tokens,
  std::vector<poi::Token>::iterator &next,
  std::unordered_map<std::string, size_t> &environment,
  size_t environment_depth
);

std::shared_ptr<poi::Variable> parse_variable(
  MemoMap &memo,
  std::vector<poi::Token> &tokens,
  std::vector<poi::Token>::iterator &next,
  std::unordered_map<std::string, size_t> &environment,
  size_t environment_depth
);

std::shared_ptr<poi::Abstraction> parse_abstraction(
  MemoMap &memo,
  std::vector<poi::Token> &tokens,
  std::vector<poi::Token>::iterator &next,
  std::unordered_map<std::string, size_t> &environment,
  size_t environment_depth
);

std::shared_ptr<poi::Application> parse_application(
  MemoMap &memo,
  std::vector<poi::Token> &tokens,
  std::vector<poi::Token>::iterator &next,
  std::shared_ptr<poi::Term> application_prior,
  std::unordered_map<std::string, size_t> &environment,
  size_t environment_depth
);

std::shared_ptr<poi::Let> parse_let(
  MemoMap &memo,
  std::vector<poi::Token> &tokens,
  std::vector<poi::Token>::iterator &next,
  std::unordered_map<std::string, size_t> &environment,
  size_t environment_depth
);

std::shared_ptr<poi::Group> parse_group(
  MemoMap &memo,
  std::vector<poi::Token> &tokens,
  std::vector<poi::Token>::iterator &next,
  std::unordered_map<std::string, size_t> &environment,
  size_t environment_depth,
  bool top_level
);

void span_tokens(
  poi::Term &term,
  std::vector<poi::Token>::iterator begin,
  std::vector<poi::Token>::iterator end
) {
  term.source_name = begin->source_name;
  term.source = begin->source;
  term.start_pos = begin->start_pos;
  term.end_pos = (end - 1)->end_pos;
}

void span_terms(
  poi::Term &term,
  poi::Term &begin,
  poi::Term &end
) {
  term.source_name = begin.source_name;
  term.source = begin.source;
  term.start_pos = begin.start_pos;
  term.end_pos = end.end_pos;
}

std::shared_ptr<poi::Term> parse_term(
  MemoMap &memo,
  std::vector<poi::Token> &tokens,
  std::vector<poi::Token>::iterator &next,
  std::unordered_map<std::string, size_t> &environment,
  size_t environment_depth
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
    parse_variable(memo, tokens, next, environment, environment_depth)
  );
  TRY_RULE(
    begin,
    next,
    term,
    parse_abstraction(memo, tokens, next, environment, environment_depth)
  );
  TRY_RULE(
    begin,
    next,
    term,
    parse_application(
      memo,
      tokens,
      next,
      std::shared_ptr<poi::Term>(),
      environment,
      environment_depth
    )
  );
  TRY_RULE(
    begin,
    next,
    term,
    parse_let(memo, tokens, next, environment, environment_depth)
  );
  TRY_RULE(
    begin,
    next,
    term,
    parse_group(memo, tokens, next, environment, environment_depth, false)
  );

  // Memoize whatever we parsed and return it.
  MEMOIZE_AND_RETURN(memo_key, term, next);
}

std::shared_ptr<poi::Variable> parse_variable(
  MemoMap &memo,
  std::vector<poi::Token> &tokens,
  std::vector<poi::Token>::iterator &next,
  std::unordered_map<std::string, size_t> &environment,
  size_t environment_depth
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
  auto iter = environment.find(*(next->literal));
  if (iter == environment.end()) {
    if (
      next + 1 != tokens.end() && (
        (next + 1)->type == poi::TokenType::EQUALS ||
        (next + 1)->type == poi::TokenType::ARROW
      )
    ) {
      MEMOIZE_AND_FAIL(memo_key, Variable, begin, next);
    } else {
      throw poi::Error(
        "Undefined variable '" + *(next->literal) + "'.",
        *(begin->source), *(begin->source_name),
        begin->start_pos, next->end_pos
      );
    }
  }

  // Construct the Variable.
  auto variable = std::make_shared<poi::Variable>(
    next->literal,
    environment_depth - iter->second
  );
  ++next;
  span_tokens(*variable, begin, next);

  // Memoize whatever we parsed and return it.
  MEMOIZE_AND_RETURN(memo_key, variable, next);
}

std::shared_ptr<poi::Abstraction> parse_abstraction(
  MemoMap &memo,
  std::vector<poi::Token> &tokens,
  std::vector<poi::Token>::iterator &next,
  std::unordered_map<std::string, size_t> &environment,
  size_t environment_depth
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

  // Parse the ARROW token.
  if (next == tokens.end() || next->type != poi::TokenType::ARROW) {
    MEMOIZE_AND_FAIL(memo_key, Abstraction, begin, next);
  }
  ++next;

  // Add the variable to the environment.
  auto new_environment_depth = environment_depth + 1;
  auto new_environment = environment;
  new_environment.erase(*variable);
  new_environment.insert({
    *variable,
    new_environment_depth
  });

  // Parse the body.
  auto body = parse_term(
    memo,
    tokens,
    next,
    new_environment,
    new_environment_depth
  );
  if (!body) {
    throw poi::Error(
      "This function needs a body.",
      *(begin->source), *(begin->source_name),
      begin->start_pos, (next - 1)->end_pos
    );
  }

  // Construct the Abstraction.
  auto abstraction = std::make_shared<poi::Abstraction>(variable, body);
  span_tokens(*abstraction, begin, next);

  // Memoize whatever we parsed and return it.
  MEMOIZE_AND_RETURN(memo_key, abstraction, next);
}

std::shared_ptr<poi::Application> parse_application(
  MemoMap &memo,
  std::vector<poi::Token> &tokens,
  std::vector<poi::Token>::iterator &next,
  std::shared_ptr<poi::Term> application_prior,
  std::unordered_map<std::string, size_t> &environment,
  size_t environment_depth
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
    parse_variable(memo, tokens, next, environment, environment_depth)
  );
  TRY_RULE(
    left_term_begin,
    next,
    left_term,
    parse_group(memo, tokens, next, environment, environment_depth, false)
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
    parse_variable(memo, tokens, next, environment, environment_depth)
  );
  TRY_RULE(
    right_term_begin,
    next,
    right_term,
    parse_group(memo, tokens, next, environment, environment_depth, false)
  );
  std::shared_ptr<poi::Application> right_application;
  if (application_prior) {
    auto prior_of_left = std::make_shared<poi::Application>(
      application_prior,
      left_term
    );
    span_terms(*prior_of_left, *application_prior, *left_term);
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
        environment_depth
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
        environment_depth
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
      application = std::make_shared<poi::Application>(
        application = std::make_shared<poi::Application>(
          application_prior,
          left_term
        ),
        right_term
      );
      span_terms(*application, *application_prior, *right_term);
    } else {
      application = std::make_shared<poi::Application>(
        left_term,
        right_term
      );
      span_terms(*application, *left_term, *right_term);
    }
  }

  // Memoize whatever we parsed and return it.
  MEMOIZE_AND_RETURN(memo_key, application, next);
}

std::shared_ptr<poi::Let> parse_let(
  MemoMap &memo,
  std::vector<poi::Token> &tokens,
  std::vector<poi::Token>::iterator &next,
  std::unordered_map<std::string, size_t> &environment,
  size_t environment_depth
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
    environment_depth
  );
  if (!definition) {
    throw poi::Error(
      "This variable needs a definition.",
      *(begin->source), *(begin->source_name),
      begin->start_pos, (next - 1)->end_pos
    );
  }

  // Parse the SEPARATOR.
  if (next == tokens.end() || next->type != poi::TokenType::SEPARATOR) {
    throw poi::Error(
      "This let needs a body.",
      *(begin->source), *(begin->source_name),
      begin->start_pos, (next - 1)->end_pos
    );
  }
  ++next;

  // Add the variable to the environment.
  auto new_environment_depth = environment_depth + 1;
  auto new_environment = environment;
  new_environment.erase(*variable);
  new_environment.insert({
    *variable,
    new_environment_depth
  });

  // Parse the body.
  auto body = parse_term(
    memo,
    tokens,
    next,
    new_environment,
    new_environment_depth
  );
  if (!body) {
    throw poi::Error(
      "This let needs a body.",
      *(begin->source), *(begin->source_name),
      begin->start_pos, (next - 1)->end_pos
    );
  }

  // Construct the Let.
  auto let = std::make_shared<poi::Let>(
    variable,
    definition,
    body
  );
  span_tokens(*let, begin, next);

  // Memoize whatever we parsed and return it.
  MEMOIZE_AND_RETURN(memo_key, let, next);
}

std::shared_ptr<poi::Group> parse_group(
  MemoMap &memo,
  std::vector<poi::Token> &tokens,
  std::vector<poi::Token>::iterator &next,
  std::unordered_map<std::string, size_t> &environment,
  size_t environment_depth,
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
  MEMO_CHECK(memo, memo_key, Group, next);

  // Parse the LEFT_PAREN, if applicable.
  if (next == tokens.end()) {
    MEMOIZE_AND_FAIL(memo_key, Group, begin, next);
  }
  if (!top_level) {
    if (next->type != poi::TokenType::LEFT_PAREN) {
      MEMOIZE_AND_FAIL(memo_key, Group, begin, next);
    }
    ++next;
  }

  // Parse the body.
  auto body = parse_term(
    memo,
    tokens,
    next,
    environment,
    environment_depth
  );
  if (!body) {
    throw poi::Error(
      "Empty group.",
      *(begin->source), *(begin->source_name),
      begin->start_pos, (next - 1)->end_pos
    );
  }

  // Skip the RIGHT_PAREN, if applicable.
  if (!top_level) {
    if (next == tokens.end() || next->type != poi::TokenType::RIGHT_PAREN) {
      throw poi::Error(
        "This group needs to be closed with a ')'.",
        *(begin->source), *(begin->source_name),
        begin->start_pos, (next - 1)->end_pos
      );
    }
    ++next;
  }

  // Construct the Group.
  auto group = std::make_shared<poi::Group>(body);
  span_tokens(*group, begin, next);

  // Memoize whatever we parsed and return it.
  MEMOIZE_AND_RETURN(memo_key, group, next);
}

///////////////////////////////////////////////////////////////////////////////
// Public interface                                                          //
///////////////////////////////////////////////////////////////////////////////

std::shared_ptr<poi::Term> poi::parse(std::vector<poi::Token> &tokens) {
  // The environment maps variable names to AST depths for computing De Bruijn
  // indices.
  std::unordered_map<std::string, size_t> environment;
  size_t environment_depth = 0;

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
    environment_depth,
    true
  );

  // Make sure we parsed the whole file.
  if (next != tokens.end()) {
    throw poi::Error(
      "The end of the file was expected here.",
      *(next->source), *(next->source_name),
      next->start_pos, next->end_pos
    );
  }

  // All done!
  return term;
}
