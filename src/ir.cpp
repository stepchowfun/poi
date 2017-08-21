#include <poi/ir.h>

///////////////////////////////////////////////////////////////////////////////
// BasicBlock                                                                //
///////////////////////////////////////////////////////////////////////////////

Poi::BasicBlock::BasicBlock() {
  instructions = std::make_shared<
    std::vector<const std::shared_ptr<const IrInstruction>>
  >();
}

///////////////////////////////////////////////////////////////////////////////
// IrInstruction                                                             //
///////////////////////////////////////////////////////////////////////////////

std::shared_ptr<
  std::vector<const std::shared_ptr<const Poi::IrInstruction>>
> Poi::BasicBlock::getInstructions() {
  return instructions;
}

Poi::IrInstruction::IrInstruction(
  const std::shared_ptr<const Node> node
) : node(node) {
}

///////////////////////////////////////////////////////////////////////////////
// IrBeginFixpoint                                                           //
///////////////////////////////////////////////////////////////////////////////

Poi::IrBeginFixpoint::IrBeginFixpoint(
  std::size_t destination,
  const std::shared_ptr<const Node> node
) : Poi::IrInstruction(node), destination(destination) {
}

std::string Poi::IrBeginFixpoint::show(const StringPool &pool) const {
  return node->show(pool);
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

std::string Poi::IrCallNonTail::show(const StringPool &pool) const {
  return node->show(pool);
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

std::string Poi::IrCallTail::show(const StringPool &pool) const {
  return node->show(pool);
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

std::string Poi::IrCopy::show(const StringPool &pool) const {
  return node->show(pool);
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

std::string Poi::IrCreateFunction::show(const StringPool &pool) const {
  return node->show(pool);
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

std::string Poi::IrDerefFixpoint::show(const StringPool &pool) const {
  return node->show(pool);
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

std::string Poi::IrExit::show(const StringPool &pool) const {
  return node->show(pool);
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

std::string Poi::IrReturn::show(const StringPool &pool) const {
  return node->show(pool);
}
