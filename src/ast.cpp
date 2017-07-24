#include <poi/ast.h>

poi::Term::~Term() {
}

poi::Variable::Variable(
  std::shared_ptr<std::string> name,
  size_t de_bruijn_index
) :
  name(name), de_bruijn_index(de_bruijn_index) {
}

std::unique_ptr<poi::Term> poi::Variable::clone() {
  return std::unique_ptr<Term>(new Variable(name, de_bruijn_index));
}

std::string poi::Variable::show() {
  return *name + "<" + std::to_string(de_bruijn_index) + ">";
}

poi::Abstraction::Abstraction(
  std::shared_ptr<std::string> variable,
  std::shared_ptr<poi::Term> body) :
  variable(variable),
  body(body) {
}

std::unique_ptr<poi::Term> poi::Abstraction::clone() {
  return std::unique_ptr<Term>(new Abstraction(
    variable,
    std::shared_ptr<Term>(
      body ?
        static_cast<Term*>(body->clone().release()) :
        nullptr
    )
  ));
}

std::string poi::Abstraction::show() {
  return "(" + *variable + " -> " + (body ? body->show() : "?") + ")";
}

poi::Application::Application(
  std::shared_ptr<poi::Term> abstraction,
  std::shared_ptr<poi::Term> operand) :
  abstraction(abstraction),
  operand(operand) {
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

poi::Let::Let(
  std::shared_ptr<std::string> variable,
  std::shared_ptr<poi::Term> definition,
  std::shared_ptr<poi::Term> body
) : variable(variable), definition(definition), body(body) {
}

std::unique_ptr<poi::Term> poi::Let::clone() {
  return std::unique_ptr<Term>(new Let(
    variable,
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
