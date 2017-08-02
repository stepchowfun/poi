#include <poi/ast.h>
#include <poi/error.h>
#include <poi/value.h>

///////////////////////////////////////////////////////////////////////////////
// Term                                                                      //
///////////////////////////////////////////////////////////////////////////////

poi::Term::Term(
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

poi::Term::~Term() {
}

///////////////////////////////////////////////////////////////////////////////
// Variable                                                                  //
///////////////////////////////////////////////////////////////////////////////

poi::Variable::Variable(
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

std::string poi::Variable::show(poi::StringPool &pool) {
  return pool.find(variable);
}

std::shared_ptr<poi::Value> poi::Variable::eval(
  std::shared_ptr<poi::Term> term,
  std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment,
  poi::StringPool &pool
) {
  return environment.at(variable);
}

///////////////////////////////////////////////////////////////////////////////
// Abstraction                                                               //
///////////////////////////////////////////////////////////////////////////////

poi::Abstraction::Abstraction(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<std::unordered_set<size_t>> free_variables,
  size_t variable,
  std::shared_ptr<poi::Term> body
) : Term(
    source_name,
    source,
    start_pos,
    end_pos,
    free_variables
  ), variable(variable), body(body) {
}

std::string poi::Abstraction::show(poi::StringPool &pool) {
  return "(" + pool.find(variable) + " -> " + body->show(pool) + ")";
}

std::shared_ptr<poi::Value> poi::Abstraction::eval(
  std::shared_ptr<poi::Term> term,
  std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment,
  poi::StringPool &pool
) {
  auto captures = std::make_shared<
    std::unordered_map<size_t, std::shared_ptr<poi::Value>>
  >();
  for (auto iter : *free_variables) {
    captures->insert({ iter, environment.at(iter) });
  }
  return std::make_shared<poi::FunctionValue>(
    std::dynamic_pointer_cast<poi::Abstraction>(term),
    captures
  );
}

///////////////////////////////////////////////////////////////////////////////
// Application                                                               //
///////////////////////////////////////////////////////////////////////////////

poi::Application::Application(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<std::unordered_set<size_t>> free_variables,
  std::shared_ptr<poi::Term> abstraction,
  std::shared_ptr<poi::Term> operand
) : Term(
    source_name,
    source,
    start_pos,
    end_pos,
    free_variables
), abstraction(abstraction), operand(operand) {
}

std::string poi::Application::show(poi::StringPool &pool) {
  return "(" + abstraction->show(pool) + " " + operand->show(pool) + ")";
}

std::shared_ptr<poi::Value> poi::Application::eval(
  std::shared_ptr<poi::Term> term,
  std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment,
  poi::StringPool &pool
) {
  auto abstraction_value = abstraction->eval(abstraction, environment, pool);
  auto operand_value = operand->eval(operand, environment, pool);
  auto abstraction_value_fun = std::dynamic_pointer_cast<poi::FunctionValue>(
    abstraction_value
  );
  if (!abstraction_value_fun) {
    throw poi::Error(
      abstraction_value->show(pool) + " is not a function.",
      pool.find(source), pool.find(source_name),
      start_pos, end_pos
    );
  }
  std::unordered_map<
    size_t,
    std::shared_ptr<poi::Value>
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

poi::Let::Let(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<std::unordered_set<size_t>> free_variables,
  size_t variable,
  std::shared_ptr<poi::Term> definition,
  std::shared_ptr<poi::Term> body
) : Term(
    source_name,
    source,
    start_pos,
    end_pos,
    free_variables
), variable(variable), definition(definition), body(body) {
}

std::string poi::Let::show(poi::StringPool &pool) {
  return
    "(" +
    pool.find(variable) +
    " = " +
    definition->show(pool) +
    ", " +
    body->show(pool) +
    ")";
}

std::shared_ptr<poi::Value> poi::Let::eval(
  std::shared_ptr<poi::Term> term,
  std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment,
  poi::StringPool &pool
) {
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

poi::DataType::DataType(
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

std::string poi::DataType::show(poi::StringPool &pool) {
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

std::shared_ptr<poi::Value> poi::DataType::eval(
  std::shared_ptr<poi::Term> term,
  std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment,
  poi::StringPool &pool
) {
  return std::make_shared<poi::DataTypeValue>(
    std::dynamic_pointer_cast<poi::DataType>(term)
  );
}

///////////////////////////////////////////////////////////////////////////////
// Member                                                                    //
///////////////////////////////////////////////////////////////////////////////

poi::Member::Member(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<std::unordered_set<size_t>> free_variables,
  std::shared_ptr<poi::Term> object,
  size_t field
) : Term(
    source_name,
    source,
    start_pos,
    end_pos,
    free_variables
), object(object), field(field) {
}

std::string poi::Member::show(poi::StringPool &pool) {
  return "(" + object->show(pool) + "." + pool.find(field) + ")";
}

std::shared_ptr<poi::Value> poi::Member::eval(
  std::shared_ptr<poi::Term> term,
  std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment,
  poi::StringPool &pool
) {
  auto object_value = object->eval(object, environment, pool);
  auto data_type_value = std::dynamic_pointer_cast<poi::DataTypeValue>(
    object_value
  );
  if (data_type_value) {
    // Make sure the constructor exists.
    auto constructor_params = data_type_value->data_type->constructor_params;
    auto constructor_names = data_type_value->data_type->constructor_names;
    auto constructor = constructor_params->find(field);
    if (constructor == constructor_params->end()) {
      throw poi::Error(
        "'" + pool.find(field) + "' is not a constructor of " +
          data_type_value->show(pool),
        pool.find(source), pool.find(source_name),
        start_pos, end_pos
      );
    }

    if (constructor_params->at(field).empty()) {
      // The constructor has no parameters. Instantiate immediately.
      return std::make_shared<poi::DataValue>(
        data_type_value,
        field,
        std::make_shared<
          std::unordered_map<size_t, std::shared_ptr<poi::Value>>
        >()
      );
    } else {
      // The constructor has some parameters. Return an abstraction.
      auto free_variables = std::make_shared<std::unordered_set<size_t>>();
      free_variables->insert(
        constructor->second.begin(),
        constructor->second.end()
      );
      auto data = std::make_shared<poi::Data>(
        term->source_name,
        term->source,
        term->start_pos,
        term->end_pos,
        free_variables,
        data_type_value,
        field
      );

      auto abstraction = std::static_pointer_cast<poi::Term>(data);
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
        abstraction = std::make_shared<poi::Abstraction>(
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
        std::unordered_map<size_t, std::shared_ptr<poi::Value>>
      >();
      return std::make_shared<poi::FunctionValue>(
        std::dynamic_pointer_cast<poi::Abstraction>(abstraction),
        captures
      );
    }
  } else {
    throw poi::Error(
      "eval(...) has not been implemented for poi::Member.",
      pool.find(source), pool.find(source_name),
      start_pos, end_pos
    );
  }
}

///////////////////////////////////////////////////////////////////////////////
// Data                                                                      //
///////////////////////////////////////////////////////////////////////////////

poi::Data::Data(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<std::unordered_set<size_t>> free_variables,
  std::shared_ptr<poi::DataTypeValue> type, size_t constructor
) : Term(
    source_name,
    source,
    start_pos,
    end_pos,
    free_variables
), type(type), constructor(constructor) {
}

std::string poi::Data::show(poi::StringPool &pool) {
  return "<" + type->show(pool) + "." + pool.find(constructor) + ">";
}

std::shared_ptr<poi::Value> poi::Data::eval(
  std::shared_ptr<poi::Term> term,
  std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment,
  poi::StringPool &pool
) {
  auto captures = std::make_shared<
    std::unordered_map<size_t, std::shared_ptr<poi::Value>>
  >();
  for (auto iter : *free_variables) {
    captures->insert({ iter, environment.at(iter) });
  }
  return std::make_shared<poi::DataValue>(type, constructor, captures);
}

///////////////////////////////////////////////////////////////////////////////
// Group                                                                     //
///////////////////////////////////////////////////////////////////////////////

poi::Group::Group(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<std::unordered_set<size_t>> free_variables,
  std::shared_ptr<poi::Term> body
) : Term(
    source_name,
    source,
    start_pos,
    end_pos,
    free_variables
), body(body) {
}

std::string poi::Group::show(poi::StringPool &pool) {
  return "(" + body->show(pool) + ")";
}

std::shared_ptr<poi::Value> poi::Group::eval(
  std::shared_ptr<poi::Term> term,
  std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment,
  poi::StringPool &pool
) {
  return body->eval(body, environment, pool);
}
