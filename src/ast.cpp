#include <poi/ast.h>
#include <poi/error.h>
#include <poi/value.h>

///////////////////////////////////////////////////////////////////////////////
// Term                                                                      //
///////////////////////////////////////////////////////////////////////////////

poi::Term::~Term() {
}

///////////////////////////////////////////////////////////////////////////////
// Variable                                                                  //
///////////////////////////////////////////////////////////////////////////////

poi::Variable::Variable(size_t variable) : variable(variable) {
}

std::string poi::Variable::show(poi::StringPool &pool) {
  return pool.find(variable);
}

std::shared_ptr<poi::Value> poi::Variable::eval(
  std::shared_ptr<poi::Term> term,
  std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment,
  poi::StringPool &pool
) {
  return environment[variable];
}

///////////////////////////////////////////////////////////////////////////////
// Abstraction                                                               //
///////////////////////////////////////////////////////////////////////////////

poi::Abstraction::Abstraction(
  size_t variable,
  std::shared_ptr<poi::Term> body
) : variable(variable), body(body) {
}

std::string poi::Abstraction::show(poi::StringPool &pool) {
  return "(" + pool.find(variable) + " -> " + body->show(pool) + ")";
}

std::shared_ptr<poi::Value> poi::Abstraction::eval(
  std::shared_ptr<poi::Term> term,
  std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment,
  poi::StringPool &pool
) {
  std::unordered_map<size_t, std::shared_ptr<poi::Value>> captures;
  for (auto iter : free_variables) {
    captures.insert({ iter, environment[iter] });
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
  std::shared_ptr<poi::Term> abstraction,
  std::shared_ptr<poi::Term> operand
) : abstraction(abstraction), operand(operand) {
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
  for (auto iter : abstraction_value_fun->captures) {
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
  size_t variable,
  std::shared_ptr<poi::Term> definition,
  std::shared_ptr<poi::Term> body
) :
  variable(variable),
  definition(definition),
  body(body) {
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
// DataConstructor                                                           //
///////////////////////////////////////////////////////////////////////////////

poi::DataConstructor::DataConstructor(
  size_t name,
  std::shared_ptr<std::vector<size_t>> params
) : name(name), params(params) {
}

std::string poi::DataConstructor::show(poi::StringPool &pool) {
  auto result = pool.find(name);
  for (auto param : *params) {
    result += " " + pool.find(param);
  }
  return result;
}

///////////////////////////////////////////////////////////////////////////////
// DataType                                                                  //
///////////////////////////////////////////////////////////////////////////////

poi::DataType::DataType(
  std::shared_ptr<std::vector<poi::DataConstructor>> constructors
) : constructors(constructors) {
}

std::string poi::DataType::show(poi::StringPool &pool) {
  std::string result = "data (";
  for (
    auto iter = constructors->begin();
    iter != constructors->end();
    ++iter
  ) {
    result += iter->show(pool);
    if (iter + 1 < constructors->end()) {
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
  std::shared_ptr<poi::Term> data,
  size_t field
) : data(data), field(field) {
}

std::string poi::Member::show(poi::StringPool &pool) {
  return "(" + data->show(pool) + "." + pool.find(field) + ")";
}

std::shared_ptr<poi::Value> poi::Member::eval(
  std::shared_ptr<poi::Term> term,
  std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment,
  poi::StringPool &pool
) {
  auto data_value = data->eval(data, environment, pool);
  auto data_type_value = std::dynamic_pointer_cast<poi::DataTypeValue>(
    data_value
  );
  if (data_type_value) {
    throw poi::Error(
      "eval(...) has not been implemented for poi::Member (1).",
      pool.find(source), pool.find(source_name),
      start_pos, end_pos
    );
  } else {
    throw poi::Error(
      "eval(...) has not been implemented for poi::Member (2).",
      pool.find(source), pool.find(source_name),
      start_pos, end_pos
    );
  }
}

///////////////////////////////////////////////////////////////////////////////
// Data                                                                      //
///////////////////////////////////////////////////////////////////////////////

poi::Data::Data(std::shared_ptr<poi::DataTypeValue> type) : type(type) {
}

std::string poi::Data::show(poi::StringPool &pool) {
  return "<" + type->show(pool) + ">";
}

std::shared_ptr<poi::Value> poi::Data::eval(
  std::shared_ptr<poi::Term> term,
  std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment,
  poi::StringPool &pool
) {
  throw poi::Error(
    "eval(...) has not been implemented for poi::Data.",
    pool.find(source), pool.find(source_name),
    start_pos, end_pos
  );
}

///////////////////////////////////////////////////////////////////////////////
// Group                                                                     //
///////////////////////////////////////////////////////////////////////////////

poi::Group::Group(std::shared_ptr<poi::Term> body) : body(body) {
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
