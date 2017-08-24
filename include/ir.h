/*
  This header declares the intermediate representation.
*/

#ifndef POI_IR_H
#define POI_IR_H

#include <memory>
#include <poi/ast.h>
#include <poi/bytecode.h>
#include <vector>

namespace Poi {
  class IrInstruction {
  public:
    const std::shared_ptr<const Node> node;

    explicit IrInstruction(const std::shared_ptr<const Node> node);
    virtual ~IrInstruction() = 0;
    virtual Register max_register() const = 0;
    virtual void emit_bytecode(
      BytecodeBlock &archive,
      BytecodeBlock &current
    ) const = 0; // Returns the number of registers used so far
    virtual std::string show(const StringPool &pool) const = 0;
  };

  class IrBeginFixpoint : public IrInstruction {
  public:
    const Register destination;

    explicit IrBeginFixpoint(
      Register destination,
      const std::shared_ptr<const Node> node
    );
    Register max_register() const override;
    void emit_bytecode(
      BytecodeBlock &archive,
      BytecodeBlock &current
    ) const override;
    std::string show(const StringPool &pool) const override;
  };

  class IrCall : public IrInstruction {
  public:
    const Register destination;
    const Register function;
    const Register argument;

    explicit IrCall(
      Register destination,
      Register function,
      Register argument,
      const std::shared_ptr<const Node> node
    );
    Register max_register() const override;
    void emit_bytecode(
      BytecodeBlock &archive,
      BytecodeBlock &current
    ) const override;
    std::string show(const StringPool &pool) const override;
  };

  class IrCreateFunction : public IrInstruction {
  public:
    const Register destination;
    const std::shared_ptr<const IrBlock> body;
    const std::shared_ptr<std::vector<Register>> captures;

    explicit IrCreateFunction(
      Register destination,
      std::shared_ptr<const IrBlock> body,
      std::shared_ptr<std::vector<Register>> captures,
      const std::shared_ptr<const Node> node
    );
    Register max_register() const override;
    void emit_bytecode(
      BytecodeBlock &archive,
      BytecodeBlock &current
    ) const override;
    std::string show(const StringPool &pool) const override;
  };

  class IrData : public IrInstruction {
    public:
    const Register destination;
    const std::size_t constructor;
    const std::shared_ptr<std::vector<Register>> captures;
    const std::shared_ptr<std::vector<std::size_t>> parameters;

    explicit IrData(
      Register destination,
      std::size_t constructor,
      std::shared_ptr<std::vector<Register>> captures,
      std::shared_ptr<std::vector<std::size_t>> parameters,
      const std::shared_ptr<const Node> node
    );
    Register max_register() const override;
    void emit_bytecode(
      BytecodeBlock &archive,
      BytecodeBlock &current
    ) const override;
    std::string show(const StringPool &pool) const override;
  };

  class IrDataType : public IrInstruction {
  public:
    const Register destination;
    const std::shared_ptr<std::vector<Register>> constructors;

    explicit IrDataType(
      Register destination,
      std::shared_ptr<std::vector<Register>> constructors,
      const std::shared_ptr<const Node> node
    );
    Register max_register() const override;
    void emit_bytecode(
      BytecodeBlock &archive,
      BytecodeBlock &current
    ) const override;
    std::string show(const StringPool &pool) const override;
  };

  class IrDerefFixpoint : public IrInstruction {
  public:
    const Register destination;
    const Register fixpoint;

    explicit IrDerefFixpoint(
      Register destination,
      Register fixpoint,
      const std::shared_ptr<const Node> node
    );
    Register max_register() const override;
    void emit_bytecode(
      BytecodeBlock &archive,
      BytecodeBlock &current
    ) const override;
    std::string show(const StringPool &pool) const override;
  };

  class IrEndFixpoint : public IrInstruction {
  public:
    const Register fixpoint;
    const Register target;

    explicit IrEndFixpoint(
      Register fixpoint,
      Register target,
      const std::shared_ptr<const Node> node
    );
    Register max_register() const override;
    void emit_bytecode(
      BytecodeBlock &archive,
      BytecodeBlock &current
    ) const override;
    std::string show(const StringPool &pool) const override;
  };

  class IrExit : public IrInstruction {
  public:
    const Register value;

    explicit IrExit(
      Register value,
      const std::shared_ptr<const Node> node
    );
    Register max_register() const override;
    void emit_bytecode(
      BytecodeBlock &archive,
      BytecodeBlock &current
    ) const override;
    std::string show(const StringPool &pool) const override;
  };

  class IrMove : public IrInstruction {
  public:
    const Register destination;
    const Register source;

    explicit IrMove(
      Register destination,
      Register source,
      const std::shared_ptr<const Node> node
    );
    Register max_register() const override;
    void emit_bytecode(
      BytecodeBlock &archive,
      BytecodeBlock &current
    ) const override;
    std::string show(const StringPool &pool) const override;
  };

  class IrMember : public IrInstruction {
  public:
    const Register destination;
    const Register object;
    const std::size_t field;

    explicit IrMember(
      Register destination,
      Register object,
      std::size_t field,
      const std::shared_ptr<const Node> node
    );
    Register max_register() const override;
    void emit_bytecode(
      BytecodeBlock &archive,
      BytecodeBlock &current
    ) const override;
    std::string show(const StringPool &pool) const override;
  };

  class IrReturn : public IrInstruction {
  public:
    const Register value;

    explicit IrReturn(
      Register value,
      const std::shared_ptr<const Node> node
    );
    Register max_register() const override;
    void emit_bytecode(
      BytecodeBlock &archive,
      BytecodeBlock &current
    ) const override;
    std::string show(const StringPool &pool) const override;
  };

  class IrTailCall : public IrInstruction {
  public:
    const Register function;
    const Register argument;

    explicit IrTailCall(
      Register function,
      Register argument,
      const std::shared_ptr<const Node> node
    );
    Register max_register() const override;
    void emit_bytecode(
      BytecodeBlock &archive,
      BytecodeBlock &current
    ) const override;
    std::string show(const StringPool &pool) const override;
  };

  class IrBlock {
  public:
    explicit IrBlock();
    Register frame_size() const;
    void emit_bytecode(
      BytecodeBlock &archive,
      BytecodeBlock &current
    ) const;
    std::string show(const StringPool &pool) const;

    std::shared_ptr<
      std::vector<std::shared_ptr<const IrInstruction>>
    > instructions;
  };
}

#endif
