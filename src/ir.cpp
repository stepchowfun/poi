#include <algorithm>
#include <poi/ir.h>

///////////////////////////////////////////////////////////////////////////////
// BasicBlock                                                                //
///////////////////////////////////////////////////////////////////////////////

Poi::BasicBlock::BasicBlock() {
  instructions = std::make_shared<
    std::vector<const std::shared_ptr<const IrInstruction>>
  >();
}

std::size_t Poi::BasicBlock::frame_size() const {
  std::size_t num_registers = 0;
  for (auto &instruction : *instructions) {
    num_registers = std::max(num_registers, instruction->max_register());
  }
  return num_registers;
}

std::shared_ptr<
  std::vector<Poi::Bytecode>
> Poi::BasicBlock::emit_bytecode() {
  auto bytecode = std::make_shared<std::vector<Poi::Bytecode>>();
  for (auto &instruction : *instructions) {
    instruction->emit_bytecode(*bytecode);
  }
  return bytecode;
}

std::string Poi::BasicBlock::show() const {
  std::string result;
  for (auto &instruction : *instructions) {
    result += instruction->show() + "\n";
  }
  return result;
}

///////////////////////////////////////////////////////////////////////////////
// IrInstruction                                                             //
///////////////////////////////////////////////////////////////////////////////

std::shared_ptr<
  std::vector<const std::shared_ptr<const Poi::IrInstruction>>
> Poi::BasicBlock::get_instructions() {
  return instructions;
}

Poi::IrInstruction::IrInstruction(
  const std::shared_ptr<const Node> node
) : node(node) {
}

Poi::IrInstruction::~IrInstruction() {
}

///////////////////////////////////////////////////////////////////////////////
// IrBeginFixpoint                                                           //
///////////////////////////////////////////////////////////////////////////////

Poi::IrBeginFixpoint::IrBeginFixpoint(
  std::size_t destination,
  const std::shared_ptr<const Node> node
) : Poi::IrInstruction(node), destination(destination) {
}

std::size_t Poi::IrBeginFixpoint::max_register() const {
  return destination;
}

void Poi::IrBeginFixpoint::emit_bytecode(
  std::vector<Poi::Bytecode> &bytecode
) const {
  Bytecode bc;
  bc.type = BytecodeType::BEGIN_FIXPOINT;
  bc.begin_fixpoint_args.destination = destination;
  bytecode.push_back(bc);
}

std::string Poi::IrBeginFixpoint::show() const {
  return std::string("BEGIN_FIXPOINT ") +
    " destination=" + std::to_string(destination);
}

///////////////////////////////////////////////////////////////////////////////
// IrCallNonTail                                                             //
///////////////////////////////////////////////////////////////////////////////

Poi::IrCallNonTail::IrCallNonTail(
  std::size_t destination,
  std::size_t function,
  std::size_t argument,
  const std::shared_ptr<const Node> node
) :
  Poi::IrInstruction(node),
  destination(destination),
  function(function),
  argument(argument) {
}

std::size_t Poi::IrCallNonTail::max_register() const {
  return std::max(destination, std::max(function, argument));
}

void Poi::IrCallNonTail::emit_bytecode(
  std::vector<Poi::Bytecode> &bytecode
) const {
  Bytecode bc;
  bc.type = BytecodeType::CALL_NON_TAIL;
  bc.call_non_tail_args.destination = destination;
  bc.call_non_tail_args.function = function;
  bc.call_non_tail_args.argument = argument;
  bytecode.push_back(bc);
}

std::string Poi::IrCallNonTail::show() const {
  return std::string("CALL_NON_TAIL ") +
    " destination=" + std::to_string(destination) +
    " function=" + std::to_string(function) +
    " argument=" + std::to_string(argument);
}

///////////////////////////////////////////////////////////////////////////////
// IrCallTail                                                                //
///////////////////////////////////////////////////////////////////////////////

Poi::IrCallTail::IrCallTail(
  std::size_t function,
  std::size_t argument,
  const std::shared_ptr<const Node> node
) : Poi::IrInstruction(node), function(function), argument(argument) {
}

std::size_t Poi::IrCallTail::max_register() const {
  return std::max(function, argument);
}

void Poi::IrCallTail::emit_bytecode(
  std::vector<Poi::Bytecode> &bytecode
) const {
  Bytecode bc;
  bc.type = BytecodeType::CALL_TAIL;
  bc.call_tail_args.function = function;
  bc.call_tail_args.argument = argument;
  bytecode.push_back(bc);
}

std::string Poi::IrCallTail::show() const {
  return std::string("CALL_TAIL ") +
    " function=" + std::to_string(function) +
    " argument=" + std::to_string(argument);
}

///////////////////////////////////////////////////////////////////////////////
// IrCopy                                                                    //
///////////////////////////////////////////////////////////////////////////////

Poi::IrCopy::IrCopy(
  std::size_t destination,
  std::size_t source,
  const std::shared_ptr<const Node> node
) :
  Poi::IrInstruction(node),
  destination(destination),
  source(source) {
}

std::size_t Poi::IrCopy::max_register() const {
  return std::max(destination, source);
}

void Poi::IrCopy::emit_bytecode(
  std::vector<Poi::Bytecode> &bytecode
) const {
  Bytecode bc;
  bc.type = BytecodeType::COPY;
  bc.copy_args.destination = destination;
  bc.copy_args.source = source;
  bytecode.push_back(bc);
}

std::string Poi::IrCopy::show() const {
  return std::string("COPY ") +
    " destination=" + std::to_string(destination) +
    " source=" + std::to_string(source);
}

///////////////////////////////////////////////////////////////////////////////
// IrCreateFunction                                                          //
///////////////////////////////////////////////////////////////////////////////

Poi::IrCreateFunction::IrCreateFunction(
  std::size_t destination,
  std::shared_ptr<const BasicBlock> body,
  std::shared_ptr<std::vector<std::size_t>> captures,
  const std::shared_ptr<const Node> node
) :
  Poi::IrInstruction(node),
  destination(destination),
  body(body),
  captures(captures) {
}

std::size_t Poi::IrCreateFunction::max_register() const {
  std::size_t highest_register = destination;
  for (auto &capture : *captures) {
    highest_register = std::max(highest_register, capture);
  }
  return highest_register;
}

void Poi::IrCreateFunction::emit_bytecode(
  std::vector<Poi::Bytecode> &bytecode
) const {
}

std::string Poi::IrCreateFunction::show() const {
  auto result = std::string("CREATE_FUNCTION ") +
    " destination=" + std::to_string(destination) +
    " body=[\n" + body->show() + "]" +
    " captures=[";
    for (std::size_t i = 0; i < captures->size(); i++) {
      if (i != 0) {
        result += ", ";
      }
      result += std::to_string(captures->at(i));
    }
    result += "]";
    return result;
}

///////////////////////////////////////////////////////////////////////////////
// IrDerefFixpoint                                                           //
///////////////////////////////////////////////////////////////////////////////

Poi::IrDerefFixpoint::IrDerefFixpoint(
  std::size_t destination,
  std::size_t fixpoint,
  const std::shared_ptr<const Node> node
) :
  Poi::IrInstruction(node),
  destination(destination),
  fixpoint(fixpoint) {
}

void Poi::IrDerefFixpoint::emit_bytecode(
  std::vector<Poi::Bytecode> &bytecode
) const {
  Bytecode bc;
  bc.type = BytecodeType::DEREF_FIXPOINT;
  bc.deref_fixpoint_args.destination = destination;
  bc.deref_fixpoint_args.fixpoint = fixpoint;
  bytecode.push_back(bc);
}

std::size_t Poi::IrDerefFixpoint::max_register() const {
  return std::max(destination, fixpoint);
}

std::string Poi::IrDerefFixpoint::show() const {
  return std::string("DEREF_FIXPOINT ") +
    " destination=" + std::to_string(destination) +
    " fixpoint=" + std::to_string(fixpoint);
}

///////////////////////////////////////////////////////////////////////////////
// IrEndFixpoint                                                             //
///////////////////////////////////////////////////////////////////////////////

Poi::IrEndFixpoint::IrEndFixpoint(
  std::size_t fixpoint,
  std::size_t target,
  const std::shared_ptr<const Node> node
) :
  Poi::IrInstruction(node),
  fixpoint(fixpoint),
  target(target) {
}

void Poi::IrEndFixpoint::emit_bytecode(
  std::vector<Poi::Bytecode> &bytecode
) const {
  Bytecode bc;
  bc.type = BytecodeType::END_FIXPOINT;
  bc.end_fixpoint_args.fixpoint = fixpoint;
  bc.end_fixpoint_args.target = target;
  bytecode.push_back(bc);
}

std::size_t Poi::IrEndFixpoint::max_register() const {
  return std::max(fixpoint, target);
}

std::string Poi::IrEndFixpoint::show() const {
  return std::string("END_FIXPOINT ") +
    " fixpoint=" + std::to_string(fixpoint) +
    " target=" + std::to_string(target);
}

///////////////////////////////////////////////////////////////////////////////
// IrExit                                                                    //
///////////////////////////////////////////////////////////////////////////////

Poi::IrExit::IrExit(
  std::size_t value,
  const std::shared_ptr<const Node> node
) :
  Poi::IrInstruction(node),
  value(value) {
}

std::size_t Poi::IrExit::max_register() const {
  return value;
}

void Poi::IrExit::emit_bytecode(
  std::vector<Poi::Bytecode> &bytecode
) const {
  Bytecode bc;
  bc.type = BytecodeType::EXIT;
  bc.exit_args.value = value;
  bytecode.push_back(bc);
}

std::string Poi::IrExit::show() const {
  return std::string("EXIT ") +
    " value=" + std::to_string(value);
}

///////////////////////////////////////////////////////////////////////////////
// IrReturn                                                                  //
///////////////////////////////////////////////////////////////////////////////

Poi::IrReturn::IrReturn(
  std::size_t value,
  const std::shared_ptr<const Node> node
) :
  Poi::IrInstruction(node),
  value(value) {
}

std::size_t Poi::IrReturn::max_register() const {
  return value;
}

void Poi::IrReturn::emit_bytecode(
  std::vector<Poi::Bytecode> &bytecode
) const {
  Bytecode bc;
  bc.type = BytecodeType::RETURN;
  bc.return_args.value = value;
  bytecode.push_back(bc);
}

std::string Poi::IrReturn::show() const {
  return std::string("RETURN ") +
    " value=" + std::to_string(value);
}