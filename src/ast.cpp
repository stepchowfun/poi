#include <algorithm>
#include <poi/ast.h>
#include <poi/error.h>
#include <poi/instruction.h>
#include <poi/value.h>

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
  size_t end_pos,
  std::shared_ptr<const std::unordered_set<size_t>> variables
) : Node(
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
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  size_t variable,
  std::shared_ptr<const std::unordered_set<size_t>> variables
) : Pattern(
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
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  size_t constructor,
  std::shared_ptr<
    const std::vector<std::shared_ptr<const Poi::Pattern>>
  > parameters,
  std::shared_ptr<const std::unordered_set<size_t>> variables
) : Pattern(
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
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<const std::unordered_set<size_t>> free_variables
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
  std::shared_ptr<const std::unordered_set<size_t>> free_variables,
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

size_t Poi::Variable::emit_instructions(
  std::vector<Poi::Instruction> &program,
  std::vector<Poi::Instruction> &expression,
  const std::unordered_map<size_t, size_t> &environment,
  size_t destination,
  bool tail_position
) const {
  Instruction copy;
  copy.node = static_cast<const Poi::Node *>(this);
  copy.type = Poi::InstructionType::COPY;
  copy.copy_args.destination = destination;
  copy.copy_args.source = environment.find(variable)->second;
  expression.push_back(copy);
  return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Function                                                                  //
///////////////////////////////////////////////////////////////////////////////

Poi::Function::Function(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<const std::unordered_set<size_t>> free_variables,
  std::shared_ptr<const Poi::Pattern> pattern,
  std::shared_ptr<const Poi::Term> body
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

size_t Poi::Function::emit_instructions(
  std::vector<Poi::Instruction> &program,
  std::vector<Poi::Instruction> &expression,
  const std::unordered_map<size_t, size_t> &environment,
  size_t destination,
  bool tail_position
) const {
  Instruction function;
  function.node = static_cast<const Poi::Node *>(this);
  function.type = Poi::InstructionType::CREATE_FUNCTION;
  function.create_function_args.destination = destination;
  function.create_function_args.body = program.size();

  std::unordered_map<size_t, size_t> body_environment;
  auto variable = std::dynamic_pointer_cast<
    const VariablePattern
  >(pattern)->variable;

  function.create_function_args.captures = new size_t[free_variables->size()];
  body_environment.insert({ variable, 2 });
  size_t index = 3;
  for (auto iter : *free_variables) {
    body_environment.insert({ iter, index });
    function.create_function_args.captures[index - 3] = environment.at(iter);
    index++;
  }

  function.create_function_args.num_captures = free_variables->size();
  function.create_function_args.frame_size = index + body->emit_instructions(
    program,
    program,
    body_environment,
    index,
    true
  );

  if (program.back().type != Poi::InstructionType::CALL_TAIL) {
    Instruction return_instruction;
    return_instruction.node = static_cast<const Poi::Node *>(this);
    return_instruction.type = Poi::InstructionType::RETURN;
    return_instruction.return_args.value = index;
    program.push_back(return_instruction);
  }

  expression.push_back(function);
  return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Application                                                               //
///////////////////////////////////////////////////////////////////////////////

Poi::Application::Application(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<const std::unordered_set<size_t>> free_variables,
  std::shared_ptr<const Poi::Term> function,
  std::shared_ptr<const Poi::Term> operand
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

size_t Poi::Application::emit_instructions(
  std::vector<Poi::Instruction> &program,
  std::vector<Poi::Instruction> &expression,
  const std::unordered_map<size_t, size_t> &environment,
  size_t destination,
  bool tail_position
) const {
  auto function_footprint = function->emit_instructions(
    program,
    expression,
    environment,
    destination,
    false
  );
  auto operand_footprint = operand->emit_instructions(
    program,
    expression,
    environment,
    destination + function_footprint,
    false
  );

  Instruction call;
  call.node = this;
  if (tail_position) {
    call.type = Poi::InstructionType::CALL_TAIL;
    call.call_tail_args.destination = destination;
    call.call_tail_args.function = destination;
    call.call_tail_args.argument = destination + function_footprint;
  } else {
    call.type = Poi::InstructionType::CALL_NON_TAIL;
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
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<const std::unordered_set<size_t>> free_variables,
  std::shared_ptr<const Poi::Pattern> pattern,
  std::shared_ptr<const Poi::Term> definition,
  std::shared_ptr<const Poi::Term> body
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

size_t Poi::Binding::emit_instructions(
  std::vector<Poi::Instruction> &program,
  std::vector<Poi::Instruction> &expression,
  const std::unordered_map<size_t, size_t> &environment,
  size_t destination,
  bool tail_position
) const {
  Instruction begin_fixpoint;
  begin_fixpoint.node = static_cast<const Poi::Node *>(this);
  begin_fixpoint.type = Poi::InstructionType::BEGIN_FIXPOINT;
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
  new_environment.insert({ variable, destination });

  auto definition_footprint = definition->emit_instructions(
    program,
    expression,
    new_environment,
    destination + 1,
    true
  );

  Instruction end_fixpoint;
  end_fixpoint.node = static_cast<const Poi::Node *>(this);
  end_fixpoint.type = Poi::InstructionType::END_FIXPOINT;
  end_fixpoint.end_fixpoint_args.fixpoint = destination;
  end_fixpoint.end_fixpoint_args.target = destination + 1;
  expression.push_back(end_fixpoint);

  auto body_footprint = body->emit_instructions(
    program,
    expression,
    new_environment,
    destination + 1,
    true
  );

  Instruction copy;
  copy.node = static_cast<const Poi::Node *>(this);
  copy.type = Poi::InstructionType::COPY;
  copy.copy_args.destination = destination;
  copy.copy_args.source = destination + 1;
  expression.push_back(copy);

  return std::max<size_t>(definition_footprint, body_footprint) + 1;
}

///////////////////////////////////////////////////////////////////////////////
// DataType                                                                  //
///////////////////////////////////////////////////////////////////////////////

Poi::DataType::DataType(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<const std::unordered_set<size_t>> free_variables,
  std::shared_ptr<const std::vector<size_t>> constructor_names,
  std::shared_ptr<
    const std::unordered_map<size_t, std::vector<size_t>>
  > constructor_params,
  std::shared_ptr<
    const std::unordered_map<size_t, std::shared_ptr<const Poi::Term>>
  > constructors
) : Term(
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

size_t Poi::DataType::emit_instructions(
  std::vector<Poi::Instruction> &program,
  std::vector<Poi::Instruction> &expression,
  const std::unordered_map<size_t, size_t> &environment,
  size_t destination,
  bool tail_position
) const {
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Data                                                                      //
///////////////////////////////////////////////////////////////////////////////

Poi::Data::Data(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<const std::unordered_set<size_t>> free_variables,
  std::weak_ptr<const Poi::DataType> data_type,
  size_t constructor
) : Term(
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

size_t Poi::Data::emit_instructions(
  std::vector<Poi::Instruction> &program,
  std::vector<Poi::Instruction> &expression,
  const std::unordered_map<size_t, size_t> &environment,
  size_t destination,
  bool tail_position
) const {
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Member                                                                    //
///////////////////////////////////////////////////////////////////////////////

Poi::Member::Member(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<const std::unordered_set<size_t>> free_variables,
  std::shared_ptr<const Poi::Term> object,
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

size_t Poi::Member::emit_instructions(
  std::vector<Poi::Instruction> &program,
  std::vector<Poi::Instruction> &expression,
  const std::unordered_map<size_t, size_t> &environment,
  size_t destination,
  bool tail_position
) const {
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Match                                                                     //
///////////////////////////////////////////////////////////////////////////////

Poi::Match::Match(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<const std::unordered_set<size_t>> free_variables,
  std::shared_ptr<const Poi::Term> discriminee,
  std::shared_ptr<
    const std::vector<std::shared_ptr<const Poi::Function>>
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

size_t Poi::Match::emit_instructions(
  std::vector<Poi::Instruction> &program,
  std::vector<Poi::Instruction> &expression,
  const std::unordered_map<size_t, size_t> &environment,
  size_t destination,
  bool tail_position
) const {
  return 0;
}
