#include <algorithm>
#include <cassert>
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
  Poi::Register destination,
  const std::shared_ptr<const Node> node
) : Poi::IrInstruction(node), destination(destination) {
}

Poi::Register Poi::IrBeginFixpoint::max_register() const {
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

std::string Poi::IrBeginFixpoint::show(const StringPool &pool) const {
  return std::string("BEGIN_FIXPOINT") +
    " destination=" + std::to_string(destination);
}

///////////////////////////////////////////////////////////////////////////////
// IrCall                                                                    //
///////////////////////////////////////////////////////////////////////////////

Poi::IrCall::IrCall(
  Poi::Register destination,
  Poi::Register function,
  Poi::Register argument,
  const std::shared_ptr<const Node> node
) :
  Poi::IrInstruction(node),
  destination(destination),
  function(function),
  argument(argument) {
}

Poi::Register Poi::IrCall::max_register() const {
  return std::max(destination, std::max(function, argument));
}

void Poi::IrCall::emit_bytecode(
  Poi::BytecodeBlock &archive,
  Poi::BytecodeBlock &current
) const {
  Bytecode bc;
  bc.type = BytecodeType::CALL;
  bc.call_non_tail_args.destination = destination;
  bc.call_non_tail_args.function = function;
  bc.call_non_tail_args.argument = argument;
  current.push(bc, node);
}

std::string Poi::IrCall::show(const StringPool &pool) const {
  return std::string("CALL") +
    " destination=" + std::to_string(destination) +
    " function=" + std::to_string(function) +
    " argument=" + std::to_string(argument);
}

///////////////////////////////////////////////////////////////////////////////
// IrCreateFunction                                                          //
///////////////////////////////////////////////////////////////////////////////

Poi::IrCreateFunction::IrCreateFunction(
  Poi::Register destination,
  std::shared_ptr<const IrBlock> body,
  std::shared_ptr<std::vector<Poi::Register>> captures,
  const std::shared_ptr<const Node> node
) :
  Poi::IrInstruction(node),
  destination(destination),
  body(body),
  captures(captures) {
}

Poi::Register Poi::IrCreateFunction::max_register() const {
  Register highest_register = destination;
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

std::string Poi::IrCreateFunction::show(const StringPool &pool) const {
  auto result = std::string("CREATE_FUNCTION") +
    " destination=" + std::to_string(destination) +
    " body=[\n" + body->show(pool) + "]" +
    " captures=[";
    for (Register i = 0; i < captures->size(); i++) {
      if (i != 0) {
        result += ", ";
      }
      result += std::to_string(captures->at(i));
    }
    result += "]";
    return result;
}

///////////////////////////////////////////////////////////////////////////////
// IrData                                                                    //
///////////////////////////////////////////////////////////////////////////////

Poi::IrData::IrData(
  Poi::Register destination,
  std::size_t constructor,
  std::shared_ptr<std::vector<Poi::Register>> captures,
  std::shared_ptr<std::vector<std::size_t>> parameters,
  const std::shared_ptr<const Node> node
) :
  Poi::IrInstruction(node),
  destination(destination),
  constructor(constructor),
  captures(captures),
  parameters(parameters) {
}

Poi::Register Poi::IrData::max_register() const {
  Register highest_register = destination;
  for (auto &capture : *captures) {
    highest_register = std::max(highest_register, capture);
  }
  return highest_register;
}

void Poi::IrData::emit_bytecode(
  Poi::BytecodeBlock &archive,
  Poi::BytecodeBlock &current
) const {
  assert(false);
}

std::string Poi::IrData::show(const StringPool &pool) const {
  auto result = std::string("DATA") +
    " name=" + pool.find(constructor) +
    " destination=" + std::to_string(destination) +
    " parameters=[";
    for (Register i = 0; i < captures->size(); i++) {
      if (i != 0) {
        result += ", ";
      }
      result += pool.find(parameters->at(i)) + "=" +
        std::to_string(captures->at(i));
    }
    result += "]";
    return result;
}

///////////////////////////////////////////////////////////////////////////////
// IrDataType                                                                //
///////////////////////////////////////////////////////////////////////////////

Poi::IrDataType::IrDataType(
  Poi::Register destination,
  std::shared_ptr<std::vector<Poi::Register>> constructors,
  const std::shared_ptr<const Node> node
) :
  Poi::IrInstruction(node),
  destination(destination),
  constructors(constructors) {
}

Poi::Register Poi::IrDataType::max_register() const {
  return destination;
}

void Poi::IrDataType::emit_bytecode(
  Poi::BytecodeBlock &archive,
  Poi::BytecodeBlock &current
) const {
  assert(false);
}

std::string Poi::IrDataType::show(const StringPool &pool) const {
  auto result = std::string("DATA_TYPE") +
    " destination=" + std::to_string(destination) +
    " constructors=[";
    for (Register i = 0; i < constructors->size(); i++) {
      if (i != 0) {
        result += ", ";
      }
      result += std::to_string(constructors->at(i));
    }
    result += "]";
    return result;
}

///////////////////////////////////////////////////////////////////////////////
// IrDerefFixpoint                                                           //
///////////////////////////////////////////////////////////////////////////////

Poi::IrDerefFixpoint::IrDerefFixpoint(
  Poi::Register destination,
  Poi::Register fixpoint,
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

Poi::Register Poi::IrDerefFixpoint::max_register() const {
  return std::max(destination, fixpoint);
}

std::string Poi::IrDerefFixpoint::show(const StringPool &pool) const {
  return std::string("DEREF_FIXPOINT") +
    " destination=" + std::to_string(destination) +
    " fixpoint=" + std::to_string(fixpoint);
}

///////////////////////////////////////////////////////////////////////////////
// IrEndFixpoint                                                             //
///////////////////////////////////////////////////////////////////////////////

Poi::IrEndFixpoint::IrEndFixpoint(
  Poi::Register fixpoint,
  Poi::Register target,
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

Poi::Register Poi::IrEndFixpoint::max_register() const {
  return std::max(fixpoint, target);
}

std::string Poi::IrEndFixpoint::show(const StringPool &pool) const {
  return std::string("END_FIXPOINT") +
    " fixpoint=" + std::to_string(fixpoint) +
    " target=" + std::to_string(target);
}

///////////////////////////////////////////////////////////////////////////////
// IrExit                                                                    //
///////////////////////////////////////////////////////////////////////////////

Poi::IrExit::IrExit(
  Poi::Register value,
  const std::shared_ptr<const Node> node
) :
  Poi::IrInstruction(node),
  value(value) {
}

Poi::Register Poi::IrExit::max_register() const {
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

std::string Poi::IrExit::show(const StringPool &pool) const {
  return std::string("EXIT") +
    " value=" + std::to_string(value);
}

///////////////////////////////////////////////////////////////////////////////
// IrMove                                                                    //
///////////////////////////////////////////////////////////////////////////////

Poi::IrMove::IrMove(
  Poi::Register destination,
  Poi::Register source,
  const std::shared_ptr<const Node> node
) :
  Poi::IrInstruction(node),
  destination(destination),
  source(source) {
}

Poi::Register Poi::IrMove::max_register() const {
  return std::max(destination, source);
}

void Poi::IrMove::emit_bytecode(
  Poi::BytecodeBlock &archive,
  Poi::BytecodeBlock &current
) const {
  assert(false);
}

std::string Poi::IrMove::show(const StringPool &pool) const {
  return std::string("MOVE") +
    " destination=" + std::to_string(destination) +
    " source=" + std::to_string(source);
}

///////////////////////////////////////////////////////////////////////////////
// IrMember                                                                  //
///////////////////////////////////////////////////////////////////////////////

Poi::IrMember::IrMember(
  Poi::Register destination,
  Poi::Register object,
  std::size_t field,
  const std::shared_ptr<const Node> node
) :
  Poi::IrInstruction(node),
  destination(destination),
  object(object),
  field(field) {
}

Poi::Register Poi::IrMember::max_register() const {
  return std::max(destination, object);
}

void Poi::IrMember::emit_bytecode(
  Poi::BytecodeBlock &archive,
  Poi::BytecodeBlock &current
) const {
  assert(false);
}

std::string Poi::IrMember::show(const StringPool &pool) const {
  return std::string("MEMBER") +
    " destination=" + std::to_string(destination) +
    " object=" + std::to_string(object) +
    " field=" + pool.find(field);
}

///////////////////////////////////////////////////////////////////////////////
// IrReturn                                                                  //
///////////////////////////////////////////////////////////////////////////////

Poi::IrReturn::IrReturn(
  Poi::Register value,
  const std::shared_ptr<const Node> node
) :
  Poi::IrInstruction(node),
  value(value) {
}

Poi::Register Poi::IrReturn::max_register() const {
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

std::string Poi::IrReturn::show(const StringPool &pool) const {
  return std::string("RETURN") +
    " value=" + std::to_string(value);
}

///////////////////////////////////////////////////////////////////////////////
// IrTailCall                                                                //
///////////////////////////////////////////////////////////////////////////////

Poi::IrTailCall::IrTailCall(
  Poi::Register function,
  Poi::Register argument,
  const std::shared_ptr<const Node> node
) : Poi::IrInstruction(node), function(function), argument(argument) {
}

Poi::Register Poi::IrTailCall::max_register() const {
  return std::max(function, argument);
}

void Poi::IrTailCall::emit_bytecode(
  Poi::BytecodeBlock &archive,
  Poi::BytecodeBlock &current
) const {
  Bytecode bc;
  bc.type = BytecodeType::TAIL_CALL;
  bc.call_tail_args.function = function;
  bc.call_tail_args.argument = argument;
  current.push(bc, node);
}

std::string Poi::IrTailCall::show(const StringPool &pool) const {
  return std::string("TAIL_CALL") +
    " function=" + std::to_string(function) +
    " argument=" + std::to_string(argument);
}

///////////////////////////////////////////////////////////////////////////////
// IrBlock                                                                   //
///////////////////////////////////////////////////////////////////////////////

Poi::IrBlock::IrBlock() {
  instructions = std::make_shared<
    std::vector<std::shared_ptr<const IrInstruction>>
  >();
}

Poi::Register Poi::IrBlock::frame_size() const {
  Register num_registers = 0;
  for (auto &instruction : *instructions) {
    num_registers = std::max(
      num_registers,
      static_cast<Register>(instruction->max_register() + 1)
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

std::string Poi::IrBlock::show(const StringPool &pool) const {
  std::string result;
  for (auto &instruction : *instructions) {
    result += instruction->show(pool) + "\n";
  }
  return result;
}
