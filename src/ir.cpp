#include <algorithm>
#include <poi/ir.h>

///////////////////////////////////////////////////////////////////////////////
// IrInstruction                                                             //
///////////////////////////////////////////////////////////////////////////////

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
  std::uint16_t destination,
  const std::shared_ptr<const Node> node
) : Poi::IrInstruction(node), destination(destination) {
}

bool Poi::IrBeginFixpoint::terminates_block() const {
  return false;
}

std::uint16_t Poi::IrBeginFixpoint::max_register() const {
  return destination;
}

void Poi::IrBeginFixpoint::emit_bytecode(
  Poi::BytecodeBlock &archive,
  Poi::BytecodeBlock &current
) const {
  Bytecode bc;
  bc.type = BytecodeType::BEGIN_FIXPOINT;
  bc.begin_fixpoint_args.destination = destination;
  current.push(bc, node);
}

std::string Poi::IrBeginFixpoint::show() const {
  return std::string("BEGIN_FIXPOINT") +
    " destination=" + std::to_string(destination);
}

///////////////////////////////////////////////////////////////////////////////
// IrCallNonTail                                                             //
///////////////////////////////////////////////////////////////////////////////

Poi::IrCallNonTail::IrCallNonTail(
  std::uint16_t destination,
  std::uint16_t function,
  std::uint16_t argument,
  const std::shared_ptr<const Node> node
) :
  Poi::IrInstruction(node),
  destination(destination),
  function(function),
  argument(argument) {
}

bool Poi::IrCallNonTail::terminates_block() const {
  return false;
}

std::uint16_t Poi::IrCallNonTail::max_register() const {
  return std::max(destination, std::max(function, argument));
}

void Poi::IrCallNonTail::emit_bytecode(
  Poi::BytecodeBlock &archive,
  Poi::BytecodeBlock &current
) const {
  Bytecode bc;
  bc.type = BytecodeType::CALL_NON_TAIL;
  bc.call_non_tail_args.destination = destination;
  bc.call_non_tail_args.function = function;
  bc.call_non_tail_args.argument = argument;
  current.push(bc, node);
}

std::string Poi::IrCallNonTail::show() const {
  return std::string("CALL_NON_TAIL") +
    " destination=" + std::to_string(destination) +
    " function=" + std::to_string(function) +
    " argument=" + std::to_string(argument);
}

///////////////////////////////////////////////////////////////////////////////
// IrCallTail                                                                //
///////////////////////////////////////////////////////////////////////////////

Poi::IrCallTail::IrCallTail(
  std::uint16_t function,
  std::uint16_t argument,
  const std::shared_ptr<const Node> node
) : Poi::IrInstruction(node), function(function), argument(argument) {
}

bool Poi::IrCallTail::terminates_block() const {
  return true;
}

std::uint16_t Poi::IrCallTail::max_register() const {
  return std::max(function, argument);
}

void Poi::IrCallTail::emit_bytecode(
  Poi::BytecodeBlock &archive,
  Poi::BytecodeBlock &current
) const {
  Bytecode bc;
  bc.type = BytecodeType::CALL_TAIL;
  bc.call_tail_args.function = function;
  bc.call_tail_args.argument = argument;
  current.push(bc, node);
}

std::string Poi::IrCallTail::show() const {
  return std::string("CALL_TAIL") +
    " function=" + std::to_string(function) +
    " argument=" + std::to_string(argument);
}

///////////////////////////////////////////////////////////////////////////////
// IrCopy                                                                    //
///////////////////////////////////////////////////////////////////////////////

Poi::IrCopy::IrCopy(
  std::uint16_t destination,
  std::uint16_t source,
  const std::shared_ptr<const Node> node
) :
  Poi::IrInstruction(node),
  destination(destination),
  source(source) {
}

bool Poi::IrCopy::terminates_block() const {
  return false;
}

std::uint16_t Poi::IrCopy::max_register() const {
  return std::max(destination, source);
}

void Poi::IrCopy::emit_bytecode(
  Poi::BytecodeBlock &archive,
  Poi::BytecodeBlock &current
) const {
  Bytecode bc;
  bc.type = BytecodeType::COPY;
  bc.copy_args.destination = destination;
  bc.copy_args.source = source;
  current.push(bc, node);
}

std::string Poi::IrCopy::show() const {
  return std::string("COPY") +
    " destination=" + std::to_string(destination) +
    " source=" + std::to_string(source);
}

///////////////////////////////////////////////////////////////////////////////
// IrCreateFunction                                                          //
///////////////////////////////////////////////////////////////////////////////

Poi::IrCreateFunction::IrCreateFunction(
  std::uint16_t destination,
  std::shared_ptr<const IrBlock> body,
  std::shared_ptr<std::vector<std::uint16_t>> captures,
  const std::shared_ptr<const Node> node
) :
  Poi::IrInstruction(node),
  destination(destination),
  body(body),
  captures(captures) {
}

bool Poi::IrCreateFunction::terminates_block() const {
  return false;
}

std::uint16_t Poi::IrCreateFunction::max_register() const {
  std::uint16_t highest_register = destination;
  for (auto &capture : *captures) {
    highest_register = std::max(highest_register, capture);
  }
  return highest_register;
}

void Poi::IrCreateFunction::emit_bytecode(
  Poi::BytecodeBlock &archive,
  Poi::BytecodeBlock &current
) const {
  BytecodeBlock body_block;
  body->emit_bytecode(archive, body_block);
  auto body_location = archive.size();
  archive.append(body_block);

  Bytecode bc;
  bc.type = BytecodeType::CREATE_FUNCTION;
  bc.create_function_args.destination = destination;
  bc.create_function_args.frame_size = body->frame_size();
  bc.create_function_args.num_captures = captures->size();
  bc.create_function_args.captures = captures->empty() ?
    nullptr :
    &(captures->at(0));
  bc.create_function_args.body = body_location;
  current.push(bc, node);
}

std::string Poi::IrCreateFunction::show() const {
  auto result = std::string("CREATE_FUNCTION") +
    " destination=" + std::to_string(destination) +
    " body=[\n" + body->show() + "]" +
    " captures=[";
    for (std::uint16_t i = 0; i < captures->size(); i++) {
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
  std::uint16_t destination,
  std::uint16_t fixpoint,
  const std::shared_ptr<const Node> node
) :
  Poi::IrInstruction(node),
  destination(destination),
  fixpoint(fixpoint) {
}

void Poi::IrDerefFixpoint::emit_bytecode(
  Poi::BytecodeBlock &archive,
  Poi::BytecodeBlock &current
) const {
  Bytecode bc;
  bc.type = BytecodeType::DEREF_FIXPOINT;
  bc.deref_fixpoint_args.destination = destination;
  bc.deref_fixpoint_args.fixpoint = fixpoint;
  current.push(bc, node);
}

bool Poi::IrDerefFixpoint::terminates_block() const {
  return false;
}

std::uint16_t Poi::IrDerefFixpoint::max_register() const {
  return std::max(destination, fixpoint);
}

std::string Poi::IrDerefFixpoint::show() const {
  return std::string("DEREF_FIXPOINT") +
    " destination=" + std::to_string(destination) +
    " fixpoint=" + std::to_string(fixpoint);
}

///////////////////////////////////////////////////////////////////////////////
// IrEndFixpoint                                                             //
///////////////////////////////////////////////////////////////////////////////

Poi::IrEndFixpoint::IrEndFixpoint(
  std::uint16_t fixpoint,
  std::uint16_t target,
  const std::shared_ptr<const Node> node
) :
  Poi::IrInstruction(node),
  fixpoint(fixpoint),
  target(target) {
}

void Poi::IrEndFixpoint::emit_bytecode(
  Poi::BytecodeBlock &archive,
  Poi::BytecodeBlock &current
) const {
  Bytecode bc;
  bc.type = BytecodeType::END_FIXPOINT;
  bc.end_fixpoint_args.fixpoint = fixpoint;
  bc.end_fixpoint_args.target = target;
  current.push(bc, node);
}

bool Poi::IrEndFixpoint::terminates_block() const {
  return false;
}

std::uint16_t Poi::IrEndFixpoint::max_register() const {
  return std::max(fixpoint, target);
}

std::string Poi::IrEndFixpoint::show() const {
  return std::string("END_FIXPOINT") +
    " fixpoint=" + std::to_string(fixpoint) +
    " target=" + std::to_string(target);
}

///////////////////////////////////////////////////////////////////////////////
// IrExit                                                                    //
///////////////////////////////////////////////////////////////////////////////

Poi::IrExit::IrExit(
  std::uint16_t value,
  const std::shared_ptr<const Node> node
) :
  Poi::IrInstruction(node),
  value(value) {
}

bool Poi::IrExit::terminates_block() const {
  return true;
}

std::uint16_t Poi::IrExit::max_register() const {
  return value;
}

void Poi::IrExit::emit_bytecode(
  Poi::BytecodeBlock &archive,
  Poi::BytecodeBlock &current
) const {
  Bytecode bc;
  bc.type = BytecodeType::EXIT;
  bc.exit_args.value = value;
  current.push(bc, node);
}

std::string Poi::IrExit::show() const {
  return std::string("EXIT") +
    " value=" + std::to_string(value);
}

///////////////////////////////////////////////////////////////////////////////
// IrReturn                                                                  //
///////////////////////////////////////////////////////////////////////////////

Poi::IrReturn::IrReturn(
  std::uint16_t value,
  const std::shared_ptr<const Node> node
) :
  Poi::IrInstruction(node),
  value(value) {
}

bool Poi::IrReturn::terminates_block() const {
  return true;
}

std::uint16_t Poi::IrReturn::max_register() const {
  return value;
}

void Poi::IrReturn::emit_bytecode(
  Poi::BytecodeBlock &archive,
  Poi::BytecodeBlock &current
) const {
  Bytecode bc;
  bc.type = BytecodeType::RETURN;
  bc.return_args.value = value;
  current.push(bc, node);
}

std::string Poi::IrReturn::show() const {
  return std::string("RETURN") +
    " value=" + std::to_string(value);
}

///////////////////////////////////////////////////////////////////////////////
// IrBlock                                                                   //
///////////////////////////////////////////////////////////////////////////////

Poi::IrBlock::IrBlock() {
  instructions = std::make_shared<
    std::vector<std::shared_ptr<const IrInstruction>>
  >();
}

std::uint16_t Poi::IrBlock::frame_size() const {
  std::uint16_t num_registers = 0;
  for (auto &instruction : *instructions) {
    num_registers = std::max(
      num_registers,
      static_cast<std::uint16_t>(instruction->max_register() + 1)
    );
  }
  return num_registers;
}

void Poi::IrBlock::emit_bytecode(
  Poi::BytecodeBlock &archive,
  Poi::BytecodeBlock &current
) const {
  for (auto &instruction : *instructions) {
    instruction->emit_bytecode(archive, current);
  }
}

std::string Poi::IrBlock::show() const {
  std::string result;
  for (auto &instruction : *instructions) {
    result += instruction->show() + "\n";
  }
  return result;
}

std::shared_ptr<
  std::vector<std::shared_ptr<const Poi::IrInstruction>>
> Poi::IrBlock::get_instructions() {
  return instructions;
}
