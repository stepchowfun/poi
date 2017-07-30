#include <poi/ast.h>
#include <poi/error.h>

///////////////////////////////////////////////////////////////////////////////
// Term                                                                      //
///////////////////////////////////////////////////////////////////////////////

poi::Term::~Term() {
}

///////////////////////////////////////////////////////////////////////////////
// Variable                                                                  //
///////////////////////////////////////////////////////////////////////////////

poi::Variable::Variable(
  std::shared_ptr<std::string> name,
  size_t variable_id
) : name(name), variable_id(variable_id) {
}

std::unique_ptr<poi::Term> poi::Variable::clone() {
  return std::unique_ptr<Term>(new Variable(name, variable_id));
}

std::string poi::Variable::show() {
  return *name;
}

std::shared_ptr<poi::Value> poi::Variable::eval(
  std::shared_ptr<poi::Term> term,
  std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment
) {
  return environment[variable_id];
}

///////////////////////////////////////////////////////////////////////////////
// Abstraction                                                               //
///////////////////////////////////////////////////////////////////////////////

poi::Abstraction::Abstraction(
  std::shared_ptr<std::string> variable,
  size_t variable_id,
  std::shared_ptr<poi::Term> body
) : variable(variable), variable_id(variable_id), body(body) {
}

std::unique_ptr<poi::Term> poi::Abstraction::clone() {
  return std::unique_ptr<Term>(new Abstraction(
    variable,
    variable_id,
    std::shared_ptr<Term>(
      body ?
        static_cast<Term*>(body->clone().release()) :
        nullptr
    )
  ));
}

std::string poi::Abstraction::show() {
  return
    "(" +
    *variable +
    " -> " +
    (body ? body->show() : "?") +
    ")";
}

std::shared_ptr<poi::Value> poi::Abstraction::eval(
  std::shared_ptr<poi::Term> term,
  std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment
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

std::unique_ptr<poi::Term> poi::Application::clone() {
  return std::unique_ptr<Term>(new Application(
    std::shared_ptr<Term>(
      abstraction ?
        static_cast<Term*>(abstraction->clone().release()) :
        nullptr
    ), std::shared_ptr<Term>(
      operand ?
        static_cast<Term*>(operand->clone().release()) :
        nullptr
    )
  ));
}

std::string poi::Application::show() {
  return
    "(" +
    (abstraction ? abstraction->show() : "?") + " " +
    (operand ? operand->show() : "?") +
    ")";
}

std::shared_ptr<poi::Value> poi::Application::eval(
  std::shared_ptr<poi::Term> term,
  std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment
) {
  auto abstraction_value = abstraction->eval(abstraction, environment);
  auto operand_value = operand->eval(operand, environment);
  auto abstraction_value_fun = std::dynamic_pointer_cast<poi::FunctionValue>(
    abstraction_value
  );
  if (!abstraction_value_fun) {
    throw poi::Error(
      abstraction_value->show() + "is not a function.",
      *source, *source_name,
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
    abstraction_value_fun->abstraction->variable_id,
    operand_value
  });
  return abstraction_value_fun->abstraction->body->eval(
    abstraction_value_fun->abstraction->body,
    new_environment
  );
}

///////////////////////////////////////////////////////////////////////////////
// Let                                                                       //
///////////////////////////////////////////////////////////////////////////////

poi::Let::Let(
  std::shared_ptr<std::string> variable,
  size_t variable_id,
  std::shared_ptr<poi::Term> definition,
  std::shared_ptr<poi::Term> body
) :
  variable(variable),
  variable_id(variable_id),
  definition(definition),
  body(body) {
}

std::unique_ptr<poi::Term> poi::Let::clone() {
  return std::unique_ptr<Term>(new Let(
    variable,
    variable_id,
    std::shared_ptr<Term>(
      definition ?
        static_cast<Term*>(definition->clone().release()) :
        nullptr
    ),
    std::shared_ptr<Term>(
      body ?
        static_cast<Term*>(body->clone().release()) :
        nullptr
    )
  ));
}

std::string poi::Let::show() {
  return
    "(" +
    *variable +
    " = " +
    (definition ? definition->show() : "?") +
    ", " +
    (body ? body->show() : "?") +
    ")";
}

std::shared_ptr<poi::Value> poi::Let::eval(
  std::shared_ptr<poi::Term> term,
  std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment
) {
  auto definition_value = definition->eval(definition, environment);
  auto new_environment = environment;
  new_environment.insert({
    variable_id,
    definition_value
  });
  return body->eval(
    body,
    new_environment
  );
}

///////////////////////////////////////////////////////////////////////////////
// Group                                                                     //
///////////////////////////////////////////////////////////////////////////////

poi::Group::Group(
  std::shared_ptr<poi::Term> body
) : body(body) {
}

std::unique_ptr<poi::Term> poi::Group::clone() {
  return std::unique_ptr<Term>(new Group(
    body ? std::shared_ptr<Term>(body->clone().release()) : nullptr
  ));
}

std::string poi::Group::show() {
  return "(" + (body ? body->show() : "<null>") + ")";
}

std::shared_ptr<poi::Value> poi::Group::eval(
  std::shared_ptr<poi::Term> term,
  std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment
) {
  return body->eval(body, environment);
}
