/*
  This header declares the abstract syntax tree.
*/

#ifndef POI_AST_H
#define POI_AST_H

#include <cstddef>
#include <memory>
#include <poi/string_pool.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Poi {
  // A forward declaration to avoid mutually recursive headers
  class BasicBlock; // Declared in poi/ir.h

  class VariableInfo {
  public:
    const std::size_t stack_location; // Relative to the top of the stack
    const bool is_fixpoint;

    explicit VariableInfo(std::size_t stack_location, bool is_fixpoint);
  };

  class Node {
  public:
    const std::size_t source_name;
    const std::size_t source;
    const std::size_t start_pos; // Inclusive
    const std::size_t end_pos; // Exclusive

    explicit Node(
      std::size_t source_name,
      std::size_t source,
      std::size_t start_pos,
      std::size_t end_pos
    );
    virtual ~Node();
    virtual std::string show(const StringPool &pool) const = 0;
  };

  class Pattern : public Node {
  public:
    const std::shared_ptr<const std::unordered_set<std::size_t>> variables;

    explicit Pattern(
      std::size_t source_name,
      std::size_t source,
      std::size_t start_pos,
      std::size_t end_pos,
      std::shared_ptr<const std::unordered_set<std::size_t>> variables
    );
    virtual ~Pattern();
  };

  class VariablePattern : public Pattern {
  public:
    const std::size_t variable;

    explicit VariablePattern(
      std::size_t source_name,
      std::size_t source,
      std::size_t start_pos,
      std::size_t end_pos,
      std::size_t variable,
      std::shared_ptr<const std::unordered_set<std::size_t>> variables
    );
    std::string show(const StringPool &pool) const override;
  };

  class ConstructorPattern : public Pattern {
  public:
    const std::size_t constructor;
    const std::shared_ptr<
      const std::vector<std::shared_ptr<const Pattern>>
    > parameters;

    explicit ConstructorPattern(
      std::size_t source_name,
      std::size_t source,
      std::size_t start_pos,
      std::size_t end_pos,
      std::size_t constructor,
      std::shared_ptr<
        const std::vector<std::shared_ptr<const Pattern>>
      > parameters,
      std::shared_ptr<const std::unordered_set<std::size_t>> variables
    );
    std::string show(const StringPool &pool) const override;
  };

  class Term : public Node {
  public:
    const std::shared_ptr<
      const std::unordered_set<std::size_t>
    > free_variables;

    explicit Term(
      std::size_t source_name,
      std::size_t source,
      std::size_t start_pos,
      std::size_t end_pos,
      std::shared_ptr<const std::unordered_set<std::size_t>> free_variables
    );
    virtual ~Term();
    virtual std::size_t emit_ir(
      std::shared_ptr<const Poi::Term> term,
      BasicBlock &current_block,
      std::size_t destination,
      bool tail_position,
      const std::unordered_map<std::size_t, VariableInfo> &environment
    ) const = 0;
  };

  class Variable : public Term {
  public:
    const std::size_t variable;

    explicit Variable(
      std::size_t source_name,
      std::size_t source,
      std::size_t start_pos,
      std::size_t end_pos,
      std::shared_ptr<const std::unordered_set<std::size_t>> free_variables,
      std::size_t variable
    );
    std::string show(const StringPool &pool) const override;
    std::size_t emit_ir(
      std::shared_ptr<const Poi::Term> term,
      BasicBlock &current_block,
      std::size_t destination,
      bool tail_position,
      const std::unordered_map<std::size_t, VariableInfo> &environment
    ) const override;
  };

  class Function : public Term {
  public:
    const std::shared_ptr<const Pattern> pattern;
    const std::shared_ptr<const Term> body;

    explicit Function(
      std::size_t source_name,
      std::size_t source,
      std::size_t start_pos,
      std::size_t end_pos,
      std::shared_ptr<const std::unordered_set<std::size_t>> free_variables,
      std::shared_ptr<const Pattern> pattern,
      std::shared_ptr<const Term> body
    );
    std::string show(const StringPool &pool) const override;
    std::size_t emit_ir(
      std::shared_ptr<const Poi::Term> term,
      BasicBlock &current_block,
      std::size_t destination,
      bool tail_position,
      const std::unordered_map<std::size_t, VariableInfo> &environment
    ) const override;
  };

  class Application : public Term {
  public:
    const std::shared_ptr<const Term> function;
    const std::shared_ptr<const Term> operand;

    explicit Application(
      std::size_t source_name,
      std::size_t source,
      std::size_t start_pos,
      std::size_t end_pos,
      std::shared_ptr<const std::unordered_set<std::size_t>> free_variables,
      std::shared_ptr<const Term> function,
      std::shared_ptr<const Term> operand
    );
    std::string show(const StringPool &pool) const override;
    std::size_t emit_ir(
      std::shared_ptr<const Poi::Term> term,
      BasicBlock &current_block,
      std::size_t destination,
      bool tail_position,
      const std::unordered_map<std::size_t, VariableInfo> &environment
    ) const override;
  };

  class Binding : public Term {
  public:
    const std::shared_ptr<const Pattern> pattern;
    const std::shared_ptr<const Term> definition;
    const std::shared_ptr<const Term> body;

    explicit Binding(
      std::size_t source_name,
      std::size_t source,
      std::size_t start_pos,
      std::size_t end_pos,
      std::shared_ptr<const std::unordered_set<std::size_t>> free_variables,
      std::shared_ptr<const Pattern> pattern,
      std::shared_ptr<const Term> definition,
      std::shared_ptr<const Term> body
    );
    std::string show(const StringPool &pool) const override;
    std::size_t emit_ir(
      std::shared_ptr<const Poi::Term> term,
      BasicBlock &current_block,
      std::size_t destination,
      bool tail_position,
      const std::unordered_map<std::size_t, VariableInfo> &environment
    ) const override;
  };

  class DataType : public Term {
  public:
    const std::shared_ptr<const std::vector<std::size_t>> constructor_names;
    const std::shared_ptr<
      const std::unordered_map<std::size_t, std::vector<std::size_t>>
    > constructor_params;
    const std::shared_ptr<
      const std::unordered_map<std::size_t, std::shared_ptr<const Term>>
    > constructors;

    explicit DataType(
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
        const std::unordered_map<std::size_t, std::shared_ptr<const Term>>
      > constructors
    );
    std::string show(const StringPool &pool) const override;
    std::size_t emit_ir(
      std::shared_ptr<const Poi::Term> term,
      BasicBlock &current_block,
      std::size_t destination,
      bool tail_position,
      const std::unordered_map<std::size_t, VariableInfo> &environment
    ) const override;
  };

  // A Data term evaluates to a DataValue value. These terms show up in the
  // automatically generated constructor functions. There is no concrete
  // syntax for Data terms.
  class Data : public Term {
  public:
    const std::weak_ptr<const DataType> data_type;
    const std::size_t constructor;

    explicit Data(
      std::size_t source_name,
      std::size_t source,
      std::size_t start_pos,
      std::size_t end_pos,
      std::shared_ptr<const std::unordered_set<std::size_t>> free_variables,
      std::weak_ptr<const DataType> data_type,
      std::size_t constructor
    );
    std::string show(const StringPool &pool) const override;
    std::size_t emit_ir(
      std::shared_ptr<const Poi::Term> term,
      BasicBlock &current_block,
      std::size_t destination,
      bool tail_position,
      const std::unordered_map<std::size_t, VariableInfo> &environment
    ) const override;
  };

  // A Member `t.x` can refer to one of two things:
  // a) If `t` is a data type, then `t.x` refers to one of its constructors.
  //    Example: `bool.true`
  // b) If `t` is a data value, then `t.x` refers to one of its members.
  //    Example: `person.name`
  class Member : public Term {
  public:
    const std::shared_ptr<const Term> object;
    const std::size_t field;

    explicit Member(
      std::size_t source_name,
      std::size_t source,
      std::size_t start_pos,
      std::size_t end_pos,
      std::shared_ptr<const std::unordered_set<std::size_t>> free_variables,
      std::shared_ptr<const Term> object,
      std::size_t field
    );
    std::string show(const StringPool &pool) const override;
    std::size_t emit_ir(
      std::shared_ptr<const Poi::Term> term,
      BasicBlock &current_block,
      std::size_t destination,
      bool tail_position,
      const std::unordered_map<std::size_t, VariableInfo> &environment
    ) const override;
  };

  class Match : public Term {
  public:
    const std::shared_ptr<const Term> discriminee;
    const std::shared_ptr<
      const std::vector<std::shared_ptr<const Function>>
    > cases;

    explicit Match(
      std::size_t source_name,
      std::size_t source,
      std::size_t start_pos,
      std::size_t end_pos,
      std::shared_ptr<const std::unordered_set<std::size_t>> free_variables,
      std::shared_ptr<const Term> discriminee,
      std::shared_ptr<
        const std::vector<std::shared_ptr<const Function>>
      > cases
    );
    std::string show(const StringPool &pool) const override;
    std::size_t emit_ir(
      std::shared_ptr<const Poi::Term> term,
      BasicBlock &current_block,
      std::size_t destination,
      bool tail_position,
      const std::unordered_map<std::size_t, VariableInfo> &environment
    ) const override;
  };
}

#endif
