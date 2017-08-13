#include <poi/ast.h>
#include <poi/error.h>
#include <poi/util.h>
#include <poi/value.h>

///////////////////////////////////////////////////////////////////////////////
// Error handling                                                            //
///////////////////////////////////////////////////////////////////////////////

namespace Poi {
  // This is used to signal errors in pattern matching.
  class MatchError : public Error {
  public:
    explicit MatchError(
      const std::string &message // No trailing line break
    ) : Error(message) {
    };

    explicit MatchError(
      const std::string &message, // No trailing line break
      const std::string &source_name,
      const std::string &source
    ) : Error (message, source_name, source) {
    };

    explicit MatchError(
      const std::string &message, // No trailing line break
      const std::string &source_name,
      const std::string &source,
      size_t start_pos, // Inclusive
      size_t end_pos // Exclusive
    ) : Error (message, source_name, source, start_pos, end_pos) {
    };
  };
}

///////////////////////////////////////////////////////////////////////////////
// Helpers                                                                   //
///////////////////////////////////////////////////////////////////////////////

void pattern_match(
  std::unordered_map<size_t, std::shared_ptr<Poi::Value>> &environment,
  Poi::StringPool &pool,
  std::shared_ptr<Poi::Pattern> pattern,
  std::shared_ptr<Poi::Value> value
) {
  auto variable_pattern = std::dynamic_pointer_cast<Poi::VariablePattern>(
    pattern
  );
  if (variable_pattern) {
    auto iter = environment.find(variable_pattern->variable);
    if (iter != environment.end()) {
      auto proxy_value = std::dynamic_pointer_cast<Poi::ProxyValue>(
        iter->second
      );
      if (proxy_value) {
        proxy_value->value = value;
      }
      environment.erase(iter);
    }
    environment.insert({ variable_pattern->variable, value });
    return;
  }

  auto constructor_pattern = std::dynamic_pointer_cast<
    Poi::ConstructorPattern
  >(pattern);
  if (constructor_pattern) {
    auto value_data = std::dynamic_pointer_cast<Poi::DataValue>(value);
    if (value_data) {
      if (
        value_data->type->constructor_params->find(
          constructor_pattern->constructor
        ) == value_data->type->constructor_params->end()
      ) {
        throw Poi::Error(
          "'" + pool.find(constructor_pattern->constructor) +
            "' is not a constructor of " + value_data->type->show(pool) + ".",
          pool.find(pattern->source_name),
          pool.find(pattern->source),
          pattern->start_pos,
          pattern->end_pos
        );
      }
      if (constructor_pattern->constructor == value_data->constructor) {
        if (
          constructor_pattern->parameters->size() ==
          value_data->type->constructor_params->at(
            value_data->constructor
          ).size()
        ) {
          size_t member_index = 0;
          for (auto &parameter : *(constructor_pattern->parameters)) {
            pattern_match(
              environment,
              pool,
              parameter,
              value_data->captures->at(
                value_data->type->constructor_params->at(
                  value_data->constructor
                ).at(member_index)
              )
            );
            ++member_index;
          }
          return;
        }
      }
    }
    throw Poi::MatchError(
      "Unable to match " + value->show(pool) + " to this pattern.",
      pool.find(pattern->source_name),
      pool.find(pattern->source),
      pattern->start_pos,
      pattern->end_pos
    );
  }
}

///////////////////////////////////////////////////////////////////////////////
// Node                                                                      //
///////////////////////////////////////////////////////////////////////////////

Poi::Node::Node(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos
) :
  source_name(source_name),
  source(source),
  start_pos(start_pos),
  end_pos(end_pos) {
}

Poi::Node::~Node() {
}

///////////////////////////////////////////////////////////////////////////////
// Pattern                                                                   //
///////////////////////////////////////////////////////////////////////////////

Poi::Pattern::Pattern(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos
) : Node(
    source_name,
    source,
    start_pos,
    end_pos
  ) {
}

Poi::Pattern::~Pattern() {
}

///////////////////////////////////////////////////////////////////////////////
// VariablePattern                                                           //
///////////////////////////////////////////////////////////////////////////////

Poi::VariablePattern::VariablePattern(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  size_t variable
) : Pattern(
    source_name,
    source,
    start_pos,
    end_pos
  ), variable(variable) {
}

std::string Poi::VariablePattern::show(const Poi::StringPool &pool) const {
  return pool.find(variable);
}

///////////////////////////////////////////////////////////////////////////////
// ConstructorPattern                                                        //
///////////////////////////////////////////////////////////////////////////////

Poi::ConstructorPattern::ConstructorPattern(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  size_t constructor,
  std::shared_ptr<std::vector<std::shared_ptr<Poi::Pattern>>> parameters
) : Pattern(
    source_name,
    source,
    start_pos,
    end_pos
  ),
  constructor(constructor),
  parameters(parameters) {
}

std::string Poi::ConstructorPattern::show(const Poi::StringPool &pool) const {
  std::string result = "{" + pool.find(constructor);
  for (
    auto iter = parameters->begin();
    iter != parameters->end();
    ++iter
  ) {
    result += " " + (*iter)->show(pool);
  }
  result += "}";
  return result;
}

///////////////////////////////////////////////////////////////////////////////
// Term                                                                      //
///////////////////////////////////////////////////////////////////////////////

Poi::Term::Term(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<std::unordered_set<size_t>> free_variables
) : Node(
    source_name,
    source,
    start_pos,
    end_pos
  ), free_variables(free_variables) {
}

Poi::Term::~Term() {
}

///////////////////////////////////////////////////////////////////////////////
// Variable                                                                  //
///////////////////////////////////////////////////////////////////////////////

Poi::Variable::Variable(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<std::unordered_set<size_t>> free_variables,
  size_t variable
) : Term(
    source_name,
    source,
    start_pos,
    end_pos,
    free_variables
  ), variable(variable) {
}

std::string Poi::Variable::show(const Poi::StringPool &pool) const {
  return pool.find(variable);
}

std::shared_ptr<Poi::Value> Poi::Variable::eval(
  std::shared_ptr<Poi::Term> term,
  std::unordered_map<size_t, std::shared_ptr<Poi::Value>> &environment,
  Poi::StringPool &pool
) const {
  auto value = environment.at(variable);
  while (true) {
    auto proxy_value = std::dynamic_pointer_cast<Poi::ProxyValue>(value);
    if (proxy_value) {
      value = proxy_value->value;
      if (!value) {
        throw Poi::Error(
          "Recursive references must occur in function bodies.",
          pool.find(source_name),
          pool.find(source),
          start_pos,
          end_pos
        );
      }
    } else {
      break;
    }
  }
  return value;
}

///////////////////////////////////////////////////////////////////////////////
// Function                                                                  //
///////////////////////////////////////////////////////////////////////////////

Poi::Function::Function(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<std::unordered_set<size_t>> free_variables,
  std::shared_ptr<Poi::Pattern> pattern,
  std::shared_ptr<Poi::Term> body
) : Term(
    source_name,
    source,
    start_pos,
    end_pos,
    free_variables
  ), pattern(pattern), body(body) {
}

std::string Poi::Function::show(const Poi::StringPool &pool) const {
  return "(" + pattern->show(pool) + " -> " + body->show(pool) + ")";
}

std::shared_ptr<Poi::Value> Poi::Function::eval(
  std::shared_ptr<Poi::Term> term,
  std::unordered_map<size_t, std::shared_ptr<Poi::Value>> &environment,
  Poi::StringPool &pool
) const {
  auto captures = std::make_shared<
    std::unordered_map<size_t, std::shared_ptr<Poi::Value>>
  >();
  for (auto iter : *free_variables) {
    captures->insert({ iter, environment.at(iter) });
  }
  return std::make_shared<Poi::FunctionValue>(
    std::dynamic_pointer_cast<Poi::Function>(term),
    captures
  );
}

///////////////////////////////////////////////////////////////////////////////
// Application                                                               //
///////////////////////////////////////////////////////////////////////////////

Poi::Application::Application(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<std::unordered_set<size_t>> free_variables,
  std::shared_ptr<Poi::Term> function,
  std::shared_ptr<Poi::Term> operand
) : Term(
    source_name,
    source,
    start_pos,
    end_pos,
    free_variables
), function(function), operand(operand) {
}

std::string Poi::Application::show(const Poi::StringPool &pool) const {
  return "(" + function->show(pool) + " " + operand->show(pool) + ")";
}

std::shared_ptr<Poi::Value> Poi::Application::eval(
  std::shared_ptr<Poi::Term> term,
  std::unordered_map<size_t, std::shared_ptr<Poi::Value>> &environment,
  Poi::StringPool &pool
) const {
  auto function_value = function->eval(function, environment, pool);
  auto operand_value = operand->eval(operand, environment, pool);
  auto function_value_fun = std::dynamic_pointer_cast<Poi::FunctionValue>(
    function_value
  );
  if (!function_value_fun) {
    throw Poi::Error(
      function_value->show(pool) + " is not a function.",
      pool.find(source_name),
      pool.find(source),
      start_pos,
      end_pos
    );
  }
  std::unordered_map<size_t, std::shared_ptr<Poi::Value>> new_environment;
  for (auto iter : *(function_value_fun->captures)) {
    new_environment.insert(iter);
  }
  try {
    pattern_match(
      new_environment,
      pool,
      function_value_fun->function->pattern,
      operand_value
    );
  } catch (Poi::MatchError &e) {
    throw Poi::Error(e.what());
  }
  return function_value_fun->function->body->eval(
    function_value_fun->function->body,
    new_environment,
    pool
  );
}

///////////////////////////////////////////////////////////////////////////////
// Binding                                                                   //
///////////////////////////////////////////////////////////////////////////////

Poi::Binding::Binding(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<std::unordered_set<size_t>> free_variables,
  std::shared_ptr<Poi::Pattern> pattern,
  std::shared_ptr<Poi::Term> definition,
  std::shared_ptr<Poi::Term> body
) : Term(
    source_name,
    source,
    start_pos,
    end_pos,
    free_variables
), pattern(pattern), definition(definition), body(body) {
}

std::string Poi::Binding::show(const Poi::StringPool &pool) const {
  return
    "(" +
    pattern->show(pool) +
    " = " +
    definition->show(pool) +
    ", " +
    body->show(pool) +
    ")";
}

std::shared_ptr<Poi::Value> Poi::Binding::eval(
  std::shared_ptr<Poi::Term> term,
  std::unordered_map<size_t, std::shared_ptr<Poi::Value>> &environment,
  Poi::StringPool &pool
) const {
  std::unordered_set<size_t> variables;
  Poi::variables_from_pattern(variables, pattern, pool);
  auto new_environment = environment;
  for (auto &variable : variables) {
    new_environment.insert({
      variable,
      std::static_pointer_cast<Poi::Value>(
        std::make_shared<Poi::ProxyValue>(nullptr)
      )
    });
  }
  auto definition_value = definition->eval(definition, new_environment, pool);
  try {
    pattern_match(
      new_environment,
      pool,
      pattern,
      definition_value
    );
  } catch (Poi::MatchError &e) {
    throw Poi::Error(e.what());
  }
  return body->eval(body, new_environment, pool);
}

///////////////////////////////////////////////////////////////////////////////
// DataType                                                                  //
///////////////////////////////////////////////////////////////////////////////

Poi::DataType::DataType(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<std::unordered_set<size_t>> free_variables,
  std::shared_ptr<std::vector<size_t>> constructor_names,
  std::shared_ptr<
    std::unordered_map<size_t, std::vector<size_t>>
  > constructor_params
) : Term(
    source_name,
    source,
    start_pos,
    end_pos,
    free_variables
  ),
  constructor_names(constructor_names),
  constructor_params(constructor_params) {
}

std::string Poi::DataType::show(const Poi::StringPool &pool) const {
  std::string result = "data {";
  for (
    auto iter = constructor_names->begin();
    iter != constructor_names->end();
    ++iter
  ) {
    result += pool.find(*iter);
    for (auto param : constructor_params->at(*iter)) {
      result += " " + pool.find(param);
    }
    if (std::next(iter) != constructor_names->end()) {
      result += ", ";
    }
  }
  result += "}";
  return result;
}

std::shared_ptr<Poi::Value> Poi::DataType::eval(
  std::shared_ptr<Poi::Term> term,
  std::unordered_map<size_t, std::shared_ptr<Poi::Value>> &environment,
  Poi::StringPool &pool
) const {
  return std::make_shared<Poi::DataTypeValue>(
    std::dynamic_pointer_cast<Poi::DataType>(term)
  );
}

///////////////////////////////////////////////////////////////////////////////
// Member                                                                    //
///////////////////////////////////////////////////////////////////////////////

Poi::Member::Member(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<std::unordered_set<size_t>> free_variables,
  std::shared_ptr<Poi::Term> object,
  size_t field
) : Term(
    source_name,
    source,
    start_pos,
    end_pos,
    free_variables
), object(object), field(field) {
}

std::string Poi::Member::show(const Poi::StringPool &pool) const {
  return "(" + object->show(pool) + "." + pool.find(field) + ")";
}

std::shared_ptr<Poi::Value> Poi::Member::eval(
  std::shared_ptr<Poi::Term> term,
  std::unordered_map<size_t, std::shared_ptr<Poi::Value>> &environment,
  Poi::StringPool &pool
) const {
  auto object_value = object->eval(object, environment, pool);
  auto data_type_value = std::dynamic_pointer_cast<Poi::DataTypeValue>(
    object_value
  );
  if (data_type_value) {
    // Make sure the constructor exists.
    auto constructor_params = data_type_value->data_type->constructor_params;
    auto constructor = constructor_params->find(field);
    if (constructor == constructor_params->end()) {
      throw Poi::Error(
        "'" + pool.find(field) + "' is not a constructor of " +
          data_type_value->show(pool),
        pool.find(source_name),
        pool.find(source),
        start_pos,
        end_pos
      );
    }

    if (constructor_params->at(field).empty()) {
      // The constructor has no parameters. Instantiate immediately.
      return std::make_shared<Poi::DataValue>(
        data_type_value->data_type,
        field,
        std::make_shared<
          std::unordered_map<size_t, std::shared_ptr<Poi::Value>>
        >()
      );
    } else {
      // The constructor has some parameters. Return a function.
      auto free_variables = std::make_shared<std::unordered_set<size_t>>();
      free_variables->insert(
        constructor->second.begin(),
        constructor->second.end()
      );
      auto data = std::make_shared<Poi::Data>(
        source_name,
        source,
        start_pos,
        end_pos,
        free_variables,
        data_type_value->data_type,
        field
      );

      auto function = std::static_pointer_cast<Poi::Term>(data);
      for (
        auto iter = constructor->second.rbegin();
        iter != constructor->second.rend();
        ++iter
      ) {
        free_variables = std::make_shared<std::unordered_set<size_t>>();
        free_variables->insert(
          function->free_variables->begin(),
          function->free_variables->end()
        );
        free_variables->erase(*iter);
        auto variable_pattern = std::make_shared<Poi::VariablePattern>(
          function->source_name,
          function->source,
          function->start_pos,
          function->end_pos,
          *iter
        );
        function = std::make_shared<Poi::Function>(
          function->source_name,
          function->source,
          function->start_pos,
          function->end_pos,
          free_variables,
          variable_pattern,
          function
        );
      }

      auto captures = std::make_shared<
        std::unordered_map<size_t, std::shared_ptr<Poi::Value>>
      >();
      return std::make_shared<Poi::FunctionValue>(
        std::dynamic_pointer_cast<Poi::Function>(function),
        captures
      );
    }
  } else {
    auto data_value = std::dynamic_pointer_cast<Poi::DataValue>(
      object_value
    );
    if (data_value) {
      auto captures = data_value->captures;
      auto capture = captures->find(field);
      if (capture == captures->end()) {
        throw Poi::Error(
          object_value->show(pool) +
            " has no member '" + pool.find(field) + "'",
          pool.find(source_name),
          pool.find(source),
          start_pos,
          end_pos
        );
      } else {
        return capture->second;
      }
    } else {
      throw Poi::Error(
        object_value->show(pool) + " has no member '" + pool.find(field) + "'",
        pool.find(source_name),
        pool.find(source),
        start_pos,
        end_pos
      );
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// Data                                                                      //
///////////////////////////////////////////////////////////////////////////////

Poi::Data::Data(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<std::unordered_set<size_t>> free_variables,
  std::shared_ptr<Poi::DataType> type, size_t constructor
) : Term(
    source_name,
    source,
    start_pos,
    end_pos,
    free_variables
), type(type), constructor(constructor) {
}

std::string Poi::Data::show(const Poi::StringPool &pool) const {
  return "<" + type->show(pool) + "." + pool.find(constructor) + ">";
}

std::shared_ptr<Poi::Value> Poi::Data::eval(
  std::shared_ptr<Poi::Term> term,
  std::unordered_map<size_t, std::shared_ptr<Poi::Value>> &environment,
  Poi::StringPool &pool
) const {
  auto captures = std::make_shared<
    std::unordered_map<size_t, std::shared_ptr<Poi::Value>>
  >();
  for (auto iter : *free_variables) {
    captures->insert({ iter, environment.at(iter) });
  }
  return std::make_shared<Poi::DataValue>(type, constructor, captures);
}

///////////////////////////////////////////////////////////////////////////////
// Match                                                                     //
///////////////////////////////////////////////////////////////////////////////

Poi::Match::Match(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<std::unordered_set<size_t>> free_variables,
  std::shared_ptr<Poi::Term> discriminee,
  std::shared_ptr<
    std::vector<std::shared_ptr<Poi::Function>>
  > cases
) : Term(
    source_name,
    source,
    start_pos,
    end_pos,
    free_variables
), discriminee(discriminee), cases(cases) {
}

std::string Poi::Match::show(const Poi::StringPool &pool) const {
  std::string result = "match " + discriminee->show(pool) + " {";
  bool first = true;
  for (auto &c : *cases) {
    if (!first) {
      result += ", ";
    }
    result += c->show(pool);
    first = false;
  }
  result += "}";
  return result;
}

std::shared_ptr<Poi::Value> Poi::Match::eval(
  std::shared_ptr<Poi::Term> term,
  std::unordered_map<size_t, std::shared_ptr<Poi::Value>> &environment,
  Poi::StringPool &pool
) const {
  auto discriminee_value = discriminee->eval(discriminee, environment, pool);

  std::shared_ptr<Poi::Value> result;
  for (auto &c : *cases) {
    auto new_environment = environment;
    try {
      pattern_match(
        new_environment,
        pool,
        c->pattern,
        discriminee_value
      );
    } catch (Poi::MatchError &e) {
      continue;
    }
    if (!result) {
      result = c->body->eval(c->body, new_environment, pool);
    }
  }
  if (result) {
    return result;
  }
  throw Poi::Error(
    discriminee_value->show(pool) + " didn't match any of the cases here.",
    pool.find(source_name),
    pool.find(source),
    start_pos,
    end_pos
  );
}
