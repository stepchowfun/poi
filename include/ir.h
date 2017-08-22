/*
  This header declares the intermediate representation.
*/

#ifndef POI_IR_H
#define POI_IR_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <poi/ast.h>
#include <poi/bytecode.h>
#include <vector>

namespace Poi {
  class IrInstruction;

  class BasicBlock {
  public:
    explicit BasicBlock();
    std::shared_ptr<
      std::vector<std::shared_ptr<const IrInstruction>>
    > get_instructions();
    std::uint16_t frame_size() const;
    void emit_bytecode(
      std::vector<Bytecode> &archive,
      std::vector<Bytecode> &current
    ) const;
    std::string show() const;

  private:
    std::shared_ptr<
      std::vector<std::shared_ptr<const IrInstruction>>
    > instructions;
  };

  class IrInstruction {
  public:
    const std::shared_ptr<const Node> node;

    explicit IrInstruction(const std::shared_ptr<const Node> node);
    virtual ~IrInstruction() = 0;
    virtual bool terminates_block() const = 0;
    virtual std::uint16_t max_register() const = 0;
    virtual void emit_bytecode(
      std::vector<Bytecode> &archive,
      std::vector<Bytecode> &current
    ) const = 0; // Returns the number of registers used so far
    virtual std::string show() const = 0;
  };

  class IrBeginFixpoint : public IrInstruction {
  public:
    const std::uint16_t destination;

    explicit IrBeginFixpoint(
      std::uint16_t destination,
      const std::shared_ptr<const Node> node
    );
    bool terminates_block() const override;
    std::uint16_t max_register() const override;
    void emit_bytecode(
      std::vector<Bytecode> &archive,
      std::vector<Bytecode> &current
    ) const override;
    std::string show() const override;
  };

  class IrCallNonTail : public IrInstruction {
  public:
    const std::uint16_t destination;
    const std::uint16_t function;
    const std::uint16_t argument;

    explicit IrCallNonTail(
      std::uint16_t destination,
      std::uint16_t function,
      std::uint16_t argument,
      const std::shared_ptr<const Node> node
    );
    bool terminates_block() const override;
    std::uint16_t max_register() const override;
    void emit_bytecode(
      std::vector<Bytecode> &archive,
      std::vector<Bytecode> &current
    ) const override;
    std::string show() const override;
  };

  class IrCallTail : public IrInstruction {
  public:
    const std::uint16_t function;
    const std::uint16_t argument;

    explicit IrCallTail(
      std::uint16_t function,
      std::uint16_t argument,
      const std::shared_ptr<const Node> node
    );
    bool terminates_block() const override;
    std::uint16_t max_register() const override;
    void emit_bytecode(
      std::vector<Bytecode> &archive,
      std::vector<Bytecode> &current
    ) const override;
    std::string show() const override;
  };

  class IrCopy : public IrInstruction {
  public:
    const std::uint16_t destination;
    const std::uint16_t source;

    explicit IrCopy(
      std::uint16_t destination,
      std::uint16_t source,
      const std::shared_ptr<const Node> node
    );
    bool terminates_block() const override;
    std::uint16_t max_register() const override;
    void emit_bytecode(
      std::vector<Bytecode> &archive,
      std::vector<Bytecode> &current
    ) const override;
    std::string show() const override;
  };

  class IrCreateFunction : public IrInstruction {
  public:
    const std::uint16_t destination;
    const std::shared_ptr<const BasicBlock> body;
    const std::shared_ptr<std::vector<std::uint16_t>> captures;

    explicit IrCreateFunction(
      std::uint16_t destination,
      std::shared_ptr<const BasicBlock> body,
      std::shared_ptr<std::vector<std::uint16_t>> captures,
      const std::shared_ptr<const Node> node
    );
    bool terminates_block() const override;
    std::uint16_t max_register() const override;
    void emit_bytecode(
      std::vector<Bytecode> &archive,
      std::vector<Bytecode> &current
    ) const override;
    std::string show() const override;
  };

  class IrDerefFixpoint : public IrInstruction {
  public:
    const std::uint16_t destination;
    const std::uint16_t fixpoint;

    explicit IrDerefFixpoint(
      std::uint16_t destination,
      std::uint16_t fixpoint,
      const std::shared_ptr<const Node> node
    );
    bool terminates_block() const override;
    std::uint16_t max_register() const override;
    void emit_bytecode(
      std::vector<Bytecode> &archive,
      std::vector<Bytecode> &current
    ) const override;
    std::string show() const override;
  };

  class IrEndFixpoint : public IrInstruction {
  public:
    const std::uint16_t fixpoint;
    const std::uint16_t target;

    explicit IrEndFixpoint(
      std::uint16_t fixpoint,
      std::uint16_t target,
      const std::shared_ptr<const Node> node
    );
    bool terminates_block() const override;
    std::uint16_t max_register() const override;
    void emit_bytecode(
      std::vector<Bytecode> &archive,
      std::vector<Bytecode> &current
    ) const override;
    std::string show() const override;
  };

  class IrExit : public IrInstruction {
  public:
    const std::uint16_t value;

    explicit IrExit(
      std::uint16_t value,
      const std::shared_ptr<const Node> node
    );
    bool terminates_block() const override;
    std::uint16_t max_register() const override;
    void emit_bytecode(
      std::vector<Bytecode> &archive,
      std::vector<Bytecode> &current
    ) const override;
    std::string show() const override;
  };

  class IrReturn : public IrInstruction {
  public:
    const std::uint16_t value;

    explicit IrReturn(
      std::uint16_t value,
      const std::shared_ptr<const Node> node
    );
    bool terminates_block() const override;
    std::uint16_t max_register() const override;
    void emit_bytecode(
      std::vector<Bytecode> &archive,
      std::vector<Bytecode> &current
    ) const override;
    std::string show() const override;
  };
}

#endif
