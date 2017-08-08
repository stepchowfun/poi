#include <poi/ast.h>
#include <poi/error.h>
#include <poi/value.h>

///////////////////////////////////////////////////////////////////////////////
// Node                                                                      //
///////////////////////////////////////////////////////////////////////////////

Poi::Node::Node(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<std::unordered_set<size_t>> free_variables
) :
  source_name(source_name),
  source(source),
  start_pos(start_pos),
  end_pos(end_pos),
  free_variables(free_variables) {
}

Poi::Node::~Node() {
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
    end_pos,
    free_variables
  ) {
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
  return environment.at(variable);
}

///////////////////////////////////////////////////////////////////////////////
// Abstraction                                                               //
///////////////////////////////////////////////////////////////////////////////

Poi::Abstraction::Abstraction(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<std::unordered_set<size_t>> free_variables,
  size_t variable,
  std::shared_ptr<Poi::Term> body
) : Term(
    source_name,
    source,
    start_pos,
    end_pos,
    free_variables
  ), variable(variable), body(body) {
}

std::string Poi::Abstraction::show(const Poi::StringPool &pool) const {
  return "(" + pool.find(variable) + " -> " + body->show(pool) + ")";
}

std::shared_ptr<Poi::Value> Poi::Abstraction::eval(
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
    std::dynamic_pointer_cast<Poi::Abstraction>(term),
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
  std::shared_ptr<Poi::Term> abstraction,
  std::shared_ptr<Poi::Term> operand
) : Term(
    source_name,
    source,
    start_pos,
    end_pos,
    free_variables
), abstraction(abstraction), operand(operand) {
}

std::string Poi::Application::show(const Poi::StringPool &pool) const {
  return "(" + abstraction->show(pool) + " " + operand->show(pool) + ")";
}

std::shared_ptr<Poi::Value> Poi::Application::eval(
  std::shared_ptr<Poi::Term> term,
  std::unordered_map<size_t, std::shared_ptr<Poi::Value>> &environment,
  Poi::StringPool &pool
) const {
  auto abstraction_value = abstraction->eval(abstraction, environment, pool);
  auto operand_value = operand->eval(operand, environment, pool);
  auto abstraction_value_fun = std::dynamic_pointer_cast<Poi::FunctionValue>(
    abstraction_value
  );
  if (!abstraction_value_fun) {
    throw Poi::Error(
      abstraction_value->show(pool) + " is not a function.",
      pool.find(source_name), pool.find(source),
      start_pos, end_pos
    );
  }
  std::unordered_map<
    size_t,
    std::shared_ptr<Poi::Value>
  > new_environment;
  for (auto iter : *(abstraction_value_fun->captures)) {
    new_environment.insert(iter);
  }
  new_environment.insert({
    abstraction_value_fun->abstraction->variable,
    operand_value
  });
  return abstraction_value_fun->abstraction->body->eval(
    abstraction_value_fun->abstraction->body,
    new_environment,
    pool
  );
}

///////////////////////////////////////////////////////////////////////////////
// Let                                                                       //
///////////////////////////////////////////////////////////////////////////////

Poi::Let::Let(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<std::unordered_set<size_t>> free_variables,
  size_t variable,
  std::shared_ptr<Poi::Term> definition,
  std::shared_ptr<Poi::Term> body
) : Term(
    source_name,
    source,
    start_pos,
    end_pos,
    free_variables
), variable(variable), definition(definition), body(body) {
}

std::string Poi::Let::show(const Poi::StringPool &pool) const {
  return
    "(" +
    pool.find(variable) +
    " = " +
    definition->show(pool) +
    ", " +
    body->show(pool) +
    ")";
}

std::shared_ptr<Poi::Value> Poi::Let::eval(
  std::shared_ptr<Poi::Term> term,
  std::unordered_map<size_t, std::shared_ptr<Poi::Value>> &environment,
  Poi::StringPool &pool
) const {
  auto definition_value = definition->eval(definition, environment, pool);
  auto new_environment = environment;
  new_environment.insert({
    variable,
    definition_value
  });
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
  std::string result = "data (";
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
  result += ")";
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
    auto constructor_names = data_type_value->data_type->constructor_names;
    auto constructor = constructor_params->find(field);
    if (constructor == constructor_params->end()) {
      throw Poi::Error(
        "'" + pool.find(field) + "' is not a constructor of " +
          data_type_value->show(pool),
        pool.find(source_name), pool.find(source),
        start_pos, end_pos
      );
    }

    if (constructor_params->at(field).empty()) {
      // The constructor has no parameters. Instantiate immediately.
      return std::make_shared<Poi::DataValue>(
        data_type_value,
        field,
        std::make_shared<
          std::unordered_map<size_t, std::shared_ptr<Poi::Value>>
        >()
      );
    } else {
      // The constructor has some parameters. Return an abstraction.
      auto free_variables = std::make_shared<std::unordered_set<size_t>>();
      free_variables->insert(
        constructor->second.begin(),
        constructor->second.end()
      );
      auto data = std::make_shared<Poi::Data>(
        term->source_name,
        term->source,
        term->start_pos,
        term->end_pos,
        free_variables,
        data_type_value,
        field
      );

      auto abstraction = std::static_pointer_cast<Poi::Term>(data);
      for (
        auto iter = constructor->second.rbegin();
        iter != constructor->second.rend();
        ++iter
      ) {
        free_variables = std::make_shared<std::unordered_set<size_t>>();
        free_variables->insert(
          abstraction->free_variables->begin(),
          abstraction->free_variables->end()
        );
        free_variables->erase(*iter);
        abstraction = std::make_shared<Poi::Abstraction>(
          abstraction->source_name,
          abstraction->source,
          abstraction->start_pos,
          abstraction->end_pos,
          free_variables,
          *iter,
          abstraction
        );
      }

      auto captures = std::make_shared<
        std::unordered_map<size_t, std::shared_ptr<Poi::Value>>
      >();
      return std::make_shared<Poi::FunctionValue>(
        std::dynamic_pointer_cast<Poi::Abstraction>(abstraction),
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
          pool.find(source_name), pool.find(source),
          start_pos, end_pos
        );
      } else {
        return capture->second;
      }
    } else {
      throw Poi::Error(
        object_value->show(pool) + " has no member '" + pool.find(field) + "'",
        pool.find(source_name), pool.find(source),
        start_pos, end_pos
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
  std::shared_ptr<Poi::DataTypeValue> type, size_t constructor
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
