/*
  This header declares the intermediate representation.
*/

#ifndef POI_IR_H
#define POI_IR_H

#include <cstddef>
#include <memory>
#include <poi/ast.h>
#include <vector>

namespace Poi {
  // A forward declaration for IR instructions
  class IrInstruction;

  class BasicBlock {
  public:
    explicit BasicBlock();
    std::shared_ptr<
      std::vector<const std::shared_ptr<const IrInstruction>>
    > get_instructions();
    std::size_t frame_size() const;
    std::string show() const;

  private:
    std::shared_ptr<
      std::vector<const std::shared_ptr<const IrInstruction>>
    > instructions;
  };

  class IrInstruction {
  public:
    const std::shared_ptr<const Node> node;

    explicit IrInstruction(const std::shared_ptr<const Node> node);
    virtual ~IrInstruction();
    virtual std::size_t max_register() const = 0;
    virtual std::string show() const = 0;
  };

  class IrBeginFixpoint : public IrInstruction {
  public:
    const std::size_t destination;

    explicit IrBeginFixpoint(
      std::size_t destination,
      const std::shared_ptr<const Node> node
    );
    std::size_t max_register() const override;
    std::string show() const override;
  };

  class IrCallNonTail : public IrInstruction {
  public:
    const std::size_t destination;
    const std::size_t function;
    const std::size_t argument;

    explicit IrCallNonTail(
      std::size_t destination,
      std::size_t function,
      std::size_t argument,
      const std::shared_ptr<const Node> node
    );
    std::size_t max_register() const override;
    std::string show() const override;
  };

  class IrCallTail : public IrInstruction {
  public:
    const std::size_t function;
    const std::size_t argument;

    explicit IrCallTail(
      std::size_t function,
      std::size_t argument,
      const std::shared_ptr<const Node> node
    );
    std::size_t max_register() const override;
    std::string show() const override;
  };

  class IrCopy : public IrInstruction {
  public:
    const std::size_t destination;
    const std::size_t source;

    explicit IrCopy(
      std::size_t destination,
      std::size_t source,
      const std::shared_ptr<const Node> node
    );
    std::size_t max_register() const override;
    std::string show() const override;
  };

  class IrCreateFunction : public IrInstruction {
  public:
    const std::size_t destination;
    const std::shared_ptr<const BasicBlock> body;
    const std::shared_ptr<std::vector<std::size_t>> captures;

    explicit IrCreateFunction(
      std::size_t destination,
      std::shared_ptr<const BasicBlock> body,
      std::shared_ptr<std::vector<std::size_t>> captures,
      const std::shared_ptr<const Node> node
    );
    std::size_t max_register() const override;
    std::string show() const override;
  };

  class IrDerefFixpoint : public IrInstruction {
  public:
    const std::size_t destination;
    const std::size_t fixpoint;

    explicit IrDerefFixpoint(
      std::size_t destination,
      std::size_t fixpoint,
      const std::shared_ptr<const Node> node
    );
    std::size_t max_register() const override;
    std::string show() const override;
  };

  class IrEndFixpoint : public IrInstruction {
  public:
    const std::size_t fixpoint;
    const std::size_t target;

    explicit IrEndFixpoint(
      std::size_t fixpoint,
      std::size_t target,
      const std::shared_ptr<const Node> node
    );
    std::size_t max_register() const override;
    std::string show() const override;
  };

  class IrExit : public IrInstruction {
  public:
    const std::size_t value;

    explicit IrExit(
      std::size_t value,
      const std::shared_ptr<const Node> node
    );
    std::size_t max_register() const override;
    std::string show() const override;
  };

  class IrReturn : public IrInstruction {
  public:
    const std::size_t value;

    explicit IrReturn(
      std::size_t value,
      const std::shared_ptr<const Node> node
    );
    std::size_t max_register() const override;
    std::string show() const override;
  };
}

#endif
