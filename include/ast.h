/*
  This header declares the abstract syntax tree.
*/

#ifndef POI_AST_H
#define POI_AST_H

#include <memory>
#include <poi/string_pool.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Poi {

  // Forward declarations to avoid mutually recursive headers
  class Value; // Declared in poi/value.h
  class DataTypeValue; // Declared in poi/value.h

  class Node {
  public:
    const size_t source_name;
    const size_t source;
    const size_t start_pos; // Inclusive
    const size_t end_pos; // Exclusive

    explicit Node(
      size_t source_name,
      size_t source,
      size_t start_pos,
      size_t end_pos
    );
    virtual ~Node();
    virtual std::string show(const Poi::StringPool &pool) const = 0;
  };

  class Pattern : public Node {
  public:
    explicit Pattern(
      size_t source_name,
      size_t source,
      size_t start_pos,
      size_t end_pos
    );
    virtual ~Pattern();
  };

  class VariablePattern : public Pattern {
  public:
    const size_t variable;

    explicit VariablePattern(
      size_t source_name,
      size_t source,
      size_t start_pos,
      size_t end_pos,
      size_t variable
    );
    std::string show(const Poi::StringPool &pool) const override;
  };

  class ConstructorPattern : public Pattern {
  public:
    const size_t constructor;
    const std::shared_ptr<
      std::vector<std::shared_ptr<Poi::Pattern>>
    > parameters;

    explicit ConstructorPattern(
      size_t source_name,
      size_t source,
      size_t start_pos,
      size_t end_pos,
      size_t constructor,
      std::shared_ptr<
        std::vector<std::shared_ptr<Poi::Pattern>>
      > parameters
    );
    std::string show(const Poi::StringPool &pool) const override;
  };

  class Term : public Node {
  public:
    const std::shared_ptr<std::unordered_set<size_t>> free_variables;

    explicit Term(
      size_t source_name,
      size_t source,
      size_t start_pos,
      size_t end_pos,
      std::shared_ptr<std::unordered_set<size_t>> free_variables
    );
    virtual ~Term();
    virtual std::shared_ptr<Poi::Value> eval(
      std::shared_ptr<Poi::Term> term,
      std::unordered_map<size_t, std::shared_ptr<Poi::Value>> &environment,
      Poi::StringPool &pool
    ) const = 0;
  };

  class Variable : public Term {
  public:
    const size_t variable;

    explicit Variable(
      size_t source_name,
      size_t source,
      size_t start_pos,
      size_t end_pos,
      std::shared_ptr<std::unordered_set<size_t>> free_variables,
      size_t variable
    );
    std::string show(const Poi::StringPool &pool) const override;
    std::shared_ptr<Poi::Value> eval(
      std::shared_ptr<Poi::Term> term,
      std::unordered_map<size_t, std::shared_ptr<Poi::Value>> &environment,
      Poi::StringPool &pool
    ) const override;
  };

  class Function : public Term {
  public:
    const std::shared_ptr<Poi::Pattern> pattern;
    const std::shared_ptr<Poi::Term> body;

    explicit Function(
      size_t source_name,
      size_t source,
      size_t start_pos,
      size_t end_pos,
      std::shared_ptr<std::unordered_set<size_t>> free_variables,
      std::shared_ptr<Poi::Pattern> pattern,
      std::shared_ptr<Poi::Term> body
    );
    std::string show(const Poi::StringPool &pool) const override;
    std::shared_ptr<Poi::Value> eval(
      std::shared_ptr<Poi::Term> term,
      std::unordered_map<size_t, std::shared_ptr<Poi::Value>> &environment,
      Poi::StringPool &pool
    ) const override;
  };

  class Application : public Term {
  public:
    const std::shared_ptr<Poi::Term> function;
    const std::shared_ptr<Poi::Term> operand;

    explicit Application(
      size_t source_name,
      size_t source,
      size_t start_pos,
      size_t end_pos,
      std::shared_ptr<std::unordered_set<size_t>> free_variables,
      std::shared_ptr<Poi::Term> function,
      std::shared_ptr<Poi::Term> operand
    );
    std::string show(const Poi::StringPool &pool) const override;
    std::shared_ptr<Poi::Value> eval(
      std::shared_ptr<Poi::Term> term,
      std::unordered_map<size_t, std::shared_ptr<Poi::Value>> &environment,
      Poi::StringPool &pool
    ) const override;
  };

  class Binding : public Term {
  public:
    const std::shared_ptr<Poi::Pattern> pattern;
    const std::shared_ptr<Poi::Term> definition;
    const std::shared_ptr<Poi::Term> body;

    explicit Binding(
      size_t source_name,
      size_t source,
      size_t start_pos,
      size_t end_pos,
      std::shared_ptr<std::unordered_set<size_t>> free_variables,
      std::shared_ptr<Poi::Pattern> pattern,
      std::shared_ptr<Poi::Term> definition,
      std::shared_ptr<Poi::Term> body
    );
    std::string show(const Poi::StringPool &pool) const override;
    std::shared_ptr<Poi::Value> eval(
      std::shared_ptr<Poi::Term> term,
      std::unordered_map<size_t, std::shared_ptr<Poi::Value>> &environment,
      Poi::StringPool &pool
    ) const override;
  };

  class DataType : public Term {
  public:
    const std::shared_ptr<std::vector<size_t>> constructor_names;
    const std::shared_ptr<
      std::unordered_map<size_t, std::vector<size_t>>
    > constructor_params;

    explicit DataType(
      size_t source_name,
      size_t source,
      size_t start_pos,
      size_t end_pos,
      std::shared_ptr<std::unordered_set<size_t>> free_variables,
      std::shared_ptr<std::vector<size_t>> constructor_names,
      std::shared_ptr<
        std::unordered_map<size_t, std::vector<size_t>>
      > constructor_params
    );
    std::string show(const Poi::StringPool &pool) const override;
    std::shared_ptr<Poi::Value> eval(
      std::shared_ptr<Poi::Term> term,
      std::unordered_map<size_t, std::shared_ptr<Poi::Value>> &environment,
      Poi::StringPool &pool
    ) const override;
  };

  // A Member `t.x` can refer to one of two things:
  // a) If `t` is a data type, then `t.x` refers to one of its constructors.
  //    Example: `bool.true`
  // b) If `t` is a data value, then `t.x` refers to one of its members.
  //    Example: `person.name`
  class Member : public Term {
  public:
    const std::shared_ptr<Poi::Term> object;
    const size_t field;

    explicit Member(
      size_t source_name,
      size_t source,
      size_t start_pos,
      size_t end_pos,
      std::shared_ptr<std::unordered_set<size_t>> free_variables,
      std::shared_ptr<Poi::Term> object,
      size_t field
    );
    std::string show(const Poi::StringPool &pool) const override;
    std::shared_ptr<Poi::Value> eval(
      std::shared_ptr<Poi::Term> term,
      std::unordered_map<size_t, std::shared_ptr<Poi::Value>> &environment,
      Poi::StringPool &pool
    ) const override;
  };

  // A Data term evaluates to a DataValue value. These terms show up in the
  // automatically generated constructor functions. There is no concrete
  // syntax for Data terms.
  class Data : public Term {
  public:
    const std::shared_ptr<Poi::DataTypeValue> type;
    const size_t constructor;

    explicit Data(
      size_t source_name,
      size_t source,
      size_t start_pos,
      size_t end_pos,
      std::shared_ptr<std::unordered_set<size_t>> free_variables,
      std::shared_ptr<Poi::DataTypeValue> type,
      size_t constructor
    );
    std::string show(const Poi::StringPool &pool) const override;
    std::shared_ptr<Poi::Value> eval(
      std::shared_ptr<Poi::Term> term,
      std::unordered_map<size_t, std::shared_ptr<Poi::Value>> &environment,
      Poi::StringPool &pool
    ) const override;
  };

  class Match : public Term {
  public:
    const std::shared_ptr<Poi::Term> discriminee;
    const std::shared_ptr<
      std::vector<std::shared_ptr<Poi::Function>>
    > cases;

    explicit Match(
      size_t source_name,
      size_t source,
      size_t start_pos,
      size_t end_pos,
      std::shared_ptr<std::unordered_set<size_t>> free_variables,
      std::shared_ptr<Poi::Term> discriminee,
      std::shared_ptr<
        std::vector<std::shared_ptr<Poi::Function>>
      > cases
    );
    std::string show(const Poi::StringPool &pool) const override;
    std::shared_ptr<Poi::Value> eval(
      std::shared_ptr<Poi::Term> term,
      std::unordered_map<size_t, std::shared_ptr<Poi::Value>> &environment,
      Poi::StringPool &pool
    ) const override;
  };

}

#endif
