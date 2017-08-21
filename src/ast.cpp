#include <algorithm>
#include <poi/ast.h>
#include <poi/bytecode.h>
#include <poi/error.h>
#include <poi/value.h>

///////////////////////////////////////////////////////////////////////////////
// VariableInfo                                                              //
///////////////////////////////////////////////////////////////////////////////

Poi::VariableInfo::VariableInfo(
  std::size_t stack_location,
  bool is_fixpoint
) : stack_location(stack_location), is_fixpoint(is_fixpoint) {
}

///////////////////////////////////////////////////////////////////////////////
// Node                                                                      //
///////////////////////////////////////////////////////////////////////////////

Poi::Node::Node(
  std::size_t source_name,
  std::size_t source,
  std::size_t start_pos,
  std::size_t end_pos
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
  std::size_t source_name,
  std::size_t source,
  std::size_t start_pos,
  std::size_t end_pos,
  std::shared_ptr<const std::unordered_set<std::size_t>> variables
) : Poi::Node(
    source_name,
    source,
    start_pos,
    end_pos
  ), variables(variables) {
}

Poi::Pattern::~Pattern() {
}

///////////////////////////////////////////////////////////////////////////////
// VariablePattern                                                           //
///////////////////////////////////////////////////////////////////////////////

Poi::VariablePattern::VariablePattern(
  std::size_t source_name,
  std::size_t source,
  std::size_t start_pos,
  std::size_t end_pos,
  std::size_t variable,
  std::shared_ptr<const std::unordered_set<std::size_t>> variables
) : Poi::Pattern(
    source_name,
    source,
    start_pos,
    end_pos,
    variables
  ), variable(variable) {
}

std::string Poi::VariablePattern::show(const Poi::StringPool &pool) const {
  return pool.find(variable);
}

///////////////////////////////////////////////////////////////////////////////
// ConstructorPattern                                                        //
///////////////////////////////////////////////////////////////////////////////

Poi::ConstructorPattern::ConstructorPattern(
  std::size_t source_name,
  std::size_t source,
  std::size_t start_pos,
  std::size_t end_pos,
  std::size_t constructor,
  std::shared_ptr<
    const std::vector<std::shared_ptr<const Poi::Pattern>>
  > parameters,
  std::shared_ptr<const std::unordered_set<std::size_t>> variables
) : Poi::Pattern(
    source_name,
    source,
    start_pos,
    end_pos,
    variables
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
  std::size_t source_name,
  std::size_t source,
  std::size_t start_pos,
  std::size_t end_pos,
  std::shared_ptr<const std::unordered_set<std::size_t>> free_variables
) : Poi::Node(
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
  std::size_t source_name,
  std::size_t source,
  std::size_t start_pos,
  std::size_t end_pos,
  std::shared_ptr<const std::unordered_set<std::size_t>> free_variables,
  std::size_t variable
) : Poi::Term(
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

std::size_t Poi::Variable::emit_bytecode(
  std::vector<Poi::Bytecode> &program,
  std::vector<Poi::Bytecode> &expression,
  const std::unordered_map<std::size_t, Poi::VariableInfo> &environment,
  std::size_t destination,
  bool tail_position
) const {
  auto variable_iter = environment.find(variable);
  if (variable_iter->second.is_fixpoint) {
    Bytecode deref_fixpoint;
    deref_fixpoint.type = BytecodeType::DEREF_FIXPOINT;
    deref_fixpoint.deref_fixpoint_args.destination = destination;
    deref_fixpoint.deref_fixpoint_args.source =
      variable_iter->second.stack_location;
    expression.push_back(deref_fixpoint);
  } else {
    Bytecode copy;
    copy.type = BytecodeType::COPY;
    copy.copy_args.destination = destination;
    copy.copy_args.source = variable_iter->second.stack_location;
    expression.push_back(copy);
  }
  return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Function                                                                  //
///////////////////////////////////////////////////////////////////////////////

Poi::Function::Function(
  std::size_t source_name,
  std::size_t source,
  std::size_t start_pos,
  std::size_t end_pos,
  std::shared_ptr<const std::unordered_set<std::size_t>> free_variables,
  std::shared_ptr<const Poi::Pattern> pattern,
  std::shared_ptr<const Poi::Term> body
) : Poi::Term(
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

std::size_t Poi::Function::emit_bytecode(
  std::vector<Poi::Bytecode> &program,
  std::vector<Poi::Bytecode> &expression,
  const std::unordered_map<std::size_t, Poi::VariableInfo> &environment,
  std::size_t destination,
  bool tail_position
) const {
  Bytecode function;
  function.type = BytecodeType::CREATE_FUNCTION;
  function.create_function_args.destination = destination;
  function.create_function_args.body = program.size();

  std::unordered_map<std::size_t, VariableInfo> body_environment;
  auto variable = std::dynamic_pointer_cast<
    const VariablePattern
  >(pattern)->variable;

  function.create_function_args.captures = new std::size_t[
    free_variables->size()
  ];
  body_environment.insert({ variable, VariableInfo(0, false) });
  std::size_t index = 1;
  for (auto iter : *free_variables) {
    auto capture_info = environment.at(iter);
    body_environment.insert({
      iter,
      VariableInfo(index, capture_info.is_fixpoint)
    });
    function.create_function_args.captures[index - 1] =
      capture_info.stack_location;
    index++;
  }

  function.create_function_args.num_captures = free_variables->size();
  function.create_function_args.frame_size = index + body->emit_bytecode(
    program,
    program,
    body_environment,
    index,
    true
  );

  if (program.back().type != BytecodeType::CALL_TAIL) {
    Bytecode return_bytecode;
    return_bytecode.type = BytecodeType::RETURN;
    return_bytecode.return_args.value = index;
    program.push_back(return_bytecode);
  }

  expression.push_back(function);
  return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Application                                                               //
///////////////////////////////////////////////////////////////////////////////

Poi::Application::Application(
  std::size_t source_name,
  std::size_t source,
  std::size_t start_pos,
  std::size_t end_pos,
  std::shared_ptr<const std::unordered_set<std::size_t>> free_variables,
  std::shared_ptr<const Poi::Term> function,
  std::shared_ptr<const Poi::Term> operand
) : Poi::Term(
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

std::size_t Poi::Application::emit_bytecode(
  std::vector<Poi::Bytecode> &program,
  std::vector<Poi::Bytecode> &expression,
  const std::unordered_map<std::size_t, Poi::VariableInfo> &environment,
  std::size_t destination,
  bool tail_position
) const {
  auto function_footprint = function->emit_bytecode(
    program,
    expression,
    environment,
    destination,
    false
  );
  auto operand_footprint = operand->emit_bytecode(
    program,
    expression,
    environment,
    destination + function_footprint,
    false
  );

  Bytecode call;
  if (tail_position) {
    call.type = BytecodeType::CALL_TAIL;
    call.call_tail_args.function = destination;
    call.call_tail_args.argument = destination + function_footprint;
  } else {
    call.type = BytecodeType::CALL_NON_TAIL;
    call.call_non_tail_args.destination = destination;
    call.call_non_tail_args.function = destination;
    call.call_non_tail_args.argument = destination + function_footprint;
  }

  expression.push_back(call);
  return function_footprint + operand_footprint;
}

///////////////////////////////////////////////////////////////////////////////
// Binding                                                                   //
///////////////////////////////////////////////////////////////////////////////

Poi::Binding::Binding(
  std::size_t source_name,
  std::size_t source,
  std::size_t start_pos,
  std::size_t end_pos,
  std::shared_ptr<const std::unordered_set<std::size_t>> free_variables,
  std::shared_ptr<const Poi::Pattern> pattern,
  std::shared_ptr<const Poi::Term> definition,
  std::shared_ptr<const Poi::Term> body
) : Poi::Term(
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

std::size_t Poi::Binding::emit_bytecode(
  std::vector<Poi::Bytecode> &program,
  std::vector<Poi::Bytecode> &expression,
  const std::unordered_map<std::size_t, Poi::VariableInfo> &environment,
  std::size_t destination,
  bool tail_position
) const {
  Bytecode begin_fixpoint;
  begin_fixpoint.type = BytecodeType::BEGIN_FIXPOINT;
  begin_fixpoint.begin_fixpoint_args.destination = destination;
  expression.push_back(begin_fixpoint);

  auto new_environment = environment;
  auto variable = std::dynamic_pointer_cast<
    const VariablePattern
  >(pattern)->variable;
  auto variable_iter = new_environment.find(variable);
  if (variable_iter != new_environment.end()) {
    new_environment.erase(variable_iter);
  }
  new_environment.insert({ variable, VariableInfo(destination, true) });

  auto definition_footprint = definition->emit_bytecode(
    program,
    expression,
    new_environment,
    destination + 1,
    false
  );

  Bytecode end_fixpoint;
  end_fixpoint.type = BytecodeType::END_FIXPOINT;
  end_fixpoint.end_fixpoint_args.fixpoint = destination;
  end_fixpoint.end_fixpoint_args.target = destination + 1;
  expression.push_back(end_fixpoint);

  auto body_footprint = body->emit_bytecode(
    program,
    expression,
    new_environment,
    destination + 1,
    tail_position
  );

  if (expression.back().type != BytecodeType::CALL_TAIL) {
    Bytecode copy;
    copy.type = BytecodeType::COPY;
    copy.copy_args.destination = destination;
    copy.copy_args.source = destination + 1;
    expression.push_back(copy);
  }

  return std::max<std::size_t>(definition_footprint, body_footprint) + 1;
}

///////////////////////////////////////////////////////////////////////////////
// DataType                                                                  //
///////////////////////////////////////////////////////////////////////////////

Poi::DataType::DataType(
  std::size_t source_name,
  std::size_t source,
  std::size_t start_pos,
  std::size_t end_pos,
  std::shared_ptr<const std::unordered_set<std::size_t>> free_variables,
  std::shared_ptr<const std::vector<std::size_t>> constructor_names,
  std::shared_ptr<
    const std::unordered_map<std::size_t, std::vector<std::size_t>>
  > constructor_params,
  std::shared_ptr<
    const std::unordered_map<std::size_t, std::shared_ptr<const Poi::Term>>
  > constructors
) : Poi::Term(
    source_name,
    source,
    start_pos,
    end_pos,
    free_variables
  ),
  constructor_names(constructor_names),
  constructor_params(constructor_params),
  constructors(constructors) {
}

std::string Poi::DataType::show(const Poi::StringPool &pool) const {
  std::string result = "{";
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

std::size_t Poi::DataType::emit_bytecode(
  std::vector<Poi::Bytecode> &program,
  std::vector<Poi::Bytecode> &expression,
  const std::unordered_map<std::size_t, Poi::VariableInfo> &environment,
  std::size_t destination,
  bool tail_position
) const {
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Data                                                                      //
///////////////////////////////////////////////////////////////////////////////

Poi::Data::Data(
  std::size_t source_name,
  std::size_t source,
  std::size_t start_pos,
  std::size_t end_pos,
  std::shared_ptr<const std::unordered_set<std::size_t>> free_variables,
  std::weak_ptr<const Poi::DataType> data_type,
  std::size_t constructor
) : Poi::Term(
    source_name,
    source,
    start_pos,
    end_pos,
    free_variables
), data_type(data_type), constructor(constructor) {
}

std::string Poi::Data::show(const Poi::StringPool &pool) const {
  return
    "<" +
    data_type.lock()->show(pool) +
    "." +
    pool.find(constructor) +
    ">";
}

std::size_t Poi::Data::emit_bytecode(
  std::vector<Poi::Bytecode> &program,
  std::vector<Poi::Bytecode> &expression,
  const std::unordered_map<std::size_t, Poi::VariableInfo> &environment,
  std::size_t destination,
  bool tail_position
) const {
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Member                                                                    //
///////////////////////////////////////////////////////////////////////////////

Poi::Member::Member(
  std::size_t source_name,
  std::size_t source,
  std::size_t start_pos,
  std::size_t end_pos,
  std::shared_ptr<const std::unordered_set<std::size_t>> free_variables,
  std::shared_ptr<const Poi::Term> object,
  std::size_t field
) : Poi::Term(
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

std::size_t Poi::Member::emit_bytecode(
  std::vector<Poi::Bytecode> &program,
  std::vector<Poi::Bytecode> &expression,
  const std::unordered_map<std::size_t, Poi::VariableInfo> &environment,
  std::size_t destination,
  bool tail_position
) const {
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Match                                                                     //
///////////////////////////////////////////////////////////////////////////////

Poi::Match::Match(
  std::size_t source_name,
  std::size_t source,
  std::size_t start_pos,
  std::size_t end_pos,
  std::shared_ptr<const std::unordered_set<std::size_t>> free_variables,
  std::shared_ptr<const Poi::Term> discriminee,
  std::shared_ptr<
    const std::vector<std::shared_ptr<const Poi::Function>>
  > cases
) : Poi::Term(
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

std::size_t Poi::Match::emit_bytecode(
  std::vector<Poi::Bytecode> &program,
  std::vector<Poi::Bytecode> &expression,
  const std::unordered_map<std::size_t, Poi::VariableInfo> &environment,
  std::size_t destination,
  bool tail_position
) const {
  return 0;
}
