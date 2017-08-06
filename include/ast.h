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

namespace poi {

  // Forward declarations to avoid mutually recursive headers
  class Value; // Declared in poi/value.h
  class DataTypeValue; // Declared in poi/value.h

  class Node {
  public:
    size_t source_name;
    size_t source;
    size_t start_pos; // Inclusive
    size_t end_pos; // Exclusive
    std::shared_ptr<std::unordered_set<size_t>> free_variables;

    explicit Node(
      size_t source_name,
      size_t source,
      size_t start_pos,
      size_t end_pos,
      std::shared_ptr<std::unordered_set<size_t>> free_variables
    );
    virtual ~Node();
    virtual std::string show(poi::StringPool &pool) = 0;
  };

  class Term : public Node {
  public:
    explicit Term(
      size_t source_name,
      size_t source,
      size_t start_pos,
      size_t end_pos,
      std::shared_ptr<std::unordered_set<size_t>> free_variables
    );
    virtual ~Term();
    virtual std::string show(poi::StringPool &pool) = 0;
    virtual std::shared_ptr<poi::Value> eval(
      std::shared_ptr<poi::Term> term,
      std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment,
      poi::StringPool &pool
    ) = 0;
  };

  class Variable : public Term {
  public:
    size_t variable;

    explicit Variable(
      size_t source_name,
      size_t source,
      size_t start_pos,
      size_t end_pos,
      std::shared_ptr<std::unordered_set<size_t>> free_variables,
      size_t variable
    );
    std::string show(poi::StringPool &pool) override;
    std::shared_ptr<poi::Value> eval(
      std::shared_ptr<poi::Term> term,
      std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment,
      poi::StringPool &pool
    ) override;
  };

  class Abstraction : public Term {
  public:
    size_t variable;
    std::shared_ptr<poi::Term> body;

    explicit Abstraction(
      size_t source_name,
      size_t source,
      size_t start_pos,
      size_t end_pos,
      std::shared_ptr<std::unordered_set<size_t>> free_variables,
      size_t variable,
      std::shared_ptr<poi::Term> body
    );
    std::string show(poi::StringPool &pool) override;
    std::shared_ptr<poi::Value> eval(
      std::shared_ptr<poi::Term> term,
      std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment,
      poi::StringPool &pool
    ) override;
  };

  class Application : public Term {
  public:
    std::shared_ptr<poi::Term> abstraction;
    std::shared_ptr<poi::Term> operand;

    explicit Application(
      size_t source_name,
      size_t source,
      size_t start_pos,
      size_t end_pos,
      std::shared_ptr<std::unordered_set<size_t>> free_variables,
      std::shared_ptr<poi::Term> abstraction,
      std::shared_ptr<poi::Term> operand
    );
    std::string show(poi::StringPool &pool) override;
    std::shared_ptr<poi::Value> eval(
      std::shared_ptr<poi::Term> term,
      std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment,
      poi::StringPool &pool
    ) override;
  };

  class Let : public Term {
  public:
    size_t variable;
    std::shared_ptr<poi::Term> definition;
    std::shared_ptr<poi::Term> body;

    explicit Let(
      size_t source_name,
      size_t source,
      size_t start_pos,
      size_t end_pos,
      std::shared_ptr<std::unordered_set<size_t>> free_variables,
      size_t variable,
      std::shared_ptr<poi::Term> definition,
      std::shared_ptr<poi::Term> body
    );
    std::string show(poi::StringPool &pool) override;
    std::shared_ptr<poi::Value> eval(
      std::shared_ptr<poi::Term> term,
      std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment,
      poi::StringPool &pool
    ) override;
  };

  class DataType : public Term {
  public:
    std::shared_ptr<std::vector<size_t>> constructor_names;
    std::shared_ptr<
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
    std::string show(poi::StringPool &pool) override;
    std::shared_ptr<poi::Value> eval(
      std::shared_ptr<poi::Term> term,
      std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment,
      poi::StringPool &pool
    ) override;
  };

  // A Member `t.x` can refer to one of two things:
  // a) If `t` is a data type, then `t.x` refers to one of its constructors.
  //    Example: `bool.true`
  // b) If `t` is a data value, then `t.x` refers to one of its members.
  //    Example: `person.name`
  class Member : public Term {
  public:
    std::shared_ptr<poi::Term> object;
    size_t field;

    explicit Member(
      size_t source_name,
      size_t source,
      size_t start_pos,
      size_t end_pos,
      std::shared_ptr<std::unordered_set<size_t>> free_variables,
      std::shared_ptr<poi::Term> object,
      size_t field
    );
    std::string show(poi::StringPool &pool) override;
    std::shared_ptr<poi::Value> eval(
      std::shared_ptr<poi::Term> term,
      std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment,
      poi::StringPool &pool
    ) override;
  };

  // A Data term evaluates to a DataValue value. These terms show up in the
  // automatically generated constructor functions. There is no concrete
  // syntax for Data terms.
  class Data : public Term {
  public:
    std::shared_ptr<poi::DataTypeValue> type;
    size_t constructor;

    explicit Data(
      size_t source_name,
      size_t source,
      size_t start_pos,
      size_t end_pos,
      std::shared_ptr<std::unordered_set<size_t>> free_variables,
      std::shared_ptr<poi::DataTypeValue> type,
      size_t constructor
    );
    std::string show(poi::StringPool &pool) override;
    std::shared_ptr<poi::Value> eval(
      std::shared_ptr<poi::Term> term,
      std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment,
      poi::StringPool &pool
    ) override;
  };

}

#endif
