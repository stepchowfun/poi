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

  class Term {
  public:
    size_t source_name;
    size_t source;
    size_t start_pos; // Inclusive
    size_t end_pos; // Exclusive
    std::unordered_set<size_t> free_variables;

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

  class DataConstructor {
  public:
    size_t name;
    std::shared_ptr<std::vector<size_t>> params;

    explicit DataConstructor(
      size_t name,
      std::shared_ptr<std::vector<size_t>> params
    );
    std::string show(poi::StringPool &pool);
  };

  class DataType : public Term {
  public:
    std::shared_ptr<std::vector<poi::DataConstructor>> constructors;

    explicit DataType(
      std::shared_ptr<std::vector<poi::DataConstructor>> constructors
    );
    std::string show(poi::StringPool &pool) override;
    std::shared_ptr<poi::Value> eval(
      std::shared_ptr<poi::Term> term,
      std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment,
      poi::StringPool &pool
    ) override;
  };

  class Member : public Term {
  public:
    std::shared_ptr<poi::Term> data;
    size_t field;

    explicit Member(
      std::shared_ptr<poi::Term> data,
      size_t field
    );
    std::string show(poi::StringPool &pool) override;
    std::shared_ptr<poi::Value> eval(
      std::shared_ptr<poi::Term> term,
      std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment,
      poi::StringPool &pool
    ) override;
  };

  class Data : public Term {
  public:
    std::shared_ptr<poi::DataTypeValue> type;

    explicit Data(std::shared_ptr<poi::DataTypeValue> type);
    std::string show(poi::StringPool &pool) override;
    std::shared_ptr<poi::Value> eval(
      std::shared_ptr<poi::Term> term,
      std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment,
      poi::StringPool &pool
    ) override;
  };

  class Group : public Term {
  public:
    std::shared_ptr<poi::Term> body;

    explicit Group(std::shared_ptr<poi::Term> body);
    std::string show(poi::StringPool &pool) override;
    std::shared_ptr<poi::Value> eval(
      std::shared_ptr<poi::Term> term,
      std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment,
      poi::StringPool &pool
    ) override;
  };

}

#endif
