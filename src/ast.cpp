#include <algorithm>
#include <poi/ast.h>
#include <poi/bytecode.h>
#include <poi/error.h>
#include <poi/ir.h>
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

std::size_t Poi::Variable::emit_ir(
  std::shared_ptr<const Poi::Term> term,
  BasicBlock &current_block,
  std::size_t destination,
  bool tail_position,
  const std::unordered_map<std::size_t, VariableInfo> &environment
) const {
  auto variable_iter = environment.find(variable);
  if (variable_iter->second.is_fixpoint) {
    current_block.get_instructions()->push_back(
      std::make_shared<IrDerefFixpoint>(
        destination,
        variable_iter->second.stack_location,
        std::static_pointer_cast<const Node>(term)
      )
    );
  } else {
    current_block.get_instructions()->push_back(
      std::make_shared<IrCopy>(
        destination,
        variable_iter->second.stack_location,
        std::static_pointer_cast<const Node>(term)
      )
    );
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

std::size_t Poi::Function::emit_ir(
  std::shared_ptr<const Poi::Term> term,
  BasicBlock &current_block,
  std::size_t destination,
  bool tail_position,
  const std::unordered_map<std::size_t, VariableInfo> &environment
) const {
  std::unordered_map<std::size_t, VariableInfo> body_environment;
  auto variable = std::dynamic_pointer_cast<
    const VariablePattern
  >(pattern)->variable;
  body_environment.insert({ variable, VariableInfo(0, false) });

  auto captures = std::make_shared<std::vector<std::uint16_t>>();
  std::size_t index = 1;
  for (auto iter : *free_variables) {
    auto capture_info = environment.at(iter);
    body_environment.insert({
      iter,
      VariableInfo(index, capture_info.is_fixpoint)
    });
    captures->push_back(capture_info.stack_location);
    index++;
  }

  auto body_block = std::make_shared<BasicBlock>();
  body->emit_ir(
    body,
    *body_block,
    index,
    true,
    body_environment
  );

  auto body_instructions = body_block->get_instructions();
  if (!body_instructions->back()->terminates_block()) {
    auto return_instruction = std::make_shared<IrReturn>(
      index,
      std::static_pointer_cast<const Node>(term)
    );
    body_instructions->push_back(return_instruction);
  }

  auto function = std::make_shared<IrCreateFunction>(
    destination,
    body_block,
    captures,
    std::static_pointer_cast<const Node>(term)
  );

  current_block.get_instructions()->push_back(function);
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

std::size_t Poi::Application::emit_ir(
  std::shared_ptr<const Poi::Term> term,
  BasicBlock &current_block,
  std::size_t destination,
  bool tail_position,
  const std::unordered_map<std::size_t, VariableInfo> &environment
) const {
  auto function_footprint = function->emit_ir(
    function,
    current_block,
    destination + 1,
    false,
    environment
  );

  auto operand_footprint = operand->emit_ir(
    operand,
    current_block,
    destination + 1 + function_footprint,
    false,
    environment
  );

  if (tail_position) {
    current_block.get_instructions()->push_back(
      std::make_shared<IrCallTail>(
        destination + 1,
        destination + 1 + function_footprint,
        std::static_pointer_cast<const Node>(term)
      )
    );
  } else {
    current_block.get_instructions()->push_back(
      std::make_shared<IrCallNonTail>(
        destination,
        destination + 1,
        destination + 1 + function_footprint,
        std::static_pointer_cast<const Node>(term)
      )
    );
  }

  return destination + 1 + function_footprint + operand_footprint;
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

std::size_t Poi::Binding::emit_ir(
  std::shared_ptr<const Poi::Term> term,
  BasicBlock &current_block,
  std::size_t destination,
  bool tail_position,
  const std::unordered_map<std::size_t, VariableInfo> &environment
) const {
  current_block.get_instructions()->push_back(
    std::make_shared<IrBeginFixpoint>(
      destination + 1,
      std::static_pointer_cast<const Node>(term)
    )
  );

  auto new_environment = environment;
  auto variable = std::dynamic_pointer_cast<
    const VariablePattern
  >(pattern)->variable;
  auto variable_iter = new_environment.find(variable);
  if (variable_iter != new_environment.end()) {
    new_environment.erase(variable_iter);
  }
  new_environment.insert({ variable, VariableInfo(destination + 1, true) });

  auto definition_footprint = definition->emit_ir(
    definition,
    current_block,
    destination + 2,
    false,
    new_environment
  );

  current_block.get_instructions()->push_back(
    std::make_shared<IrEndFixpoint>(
      destination + 1,
      destination + 2,
      std::static_pointer_cast<const Node>(term)
    )
  );

  auto body_footprint = body->emit_ir(
    body,
    current_block,
    destination + 2 + definition_footprint,
    tail_position,
    new_environment
  );

  if (!current_block.get_instructions()->back()->terminates_block()) {
    current_block.get_instructions()->push_back(
      std::make_shared<IrCopy>(
        destination,
        destination + 2 + definition_footprint,
        std::static_pointer_cast<const Node>(term)
      )
    );
  }

  return destination + 2 + definition_footprint + body_footprint;
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

std::size_t Poi::DataType::emit_ir(
  std::shared_ptr<const Poi::Term> term,
  BasicBlock &current_block,
  std::size_t destination,
  bool tail_position,
  const std::unordered_map<std::size_t, VariableInfo> &environment
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

std::size_t Poi::Data::emit_ir(
  std::shared_ptr<const Poi::Term> term,
  BasicBlock &current_block,
  std::size_t destination,
  bool tail_position,
  const std::unordered_map<std::size_t, VariableInfo> &environment
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

std::size_t Poi::Member::emit_ir(
  std::shared_ptr<const Poi::Term> term,
  BasicBlock &current_block,
  std::size_t destination,
  bool tail_position,
  const std::unordered_map<std::size_t, VariableInfo> &environment
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

std::size_t Poi::Match::emit_ir(
  std::shared_ptr<const Poi::Term> term,
  BasicBlock &current_block,
  std::size_t destination,
  bool tail_position,
  const std::unordered_map<std::size_t, VariableInfo> &environment
) const {
  return 0;
}
