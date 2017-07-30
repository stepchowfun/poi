/*
  This header declares the abstract syntax tree.
*/

#ifndef POI_AST_H
#define POI_AST_H

#include <memory>
#include <poi/value.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace poi {

  class Value; // Declared in gram/value.h

  class Term {
  public:
    std::shared_ptr<std::string> source_name;
    std::shared_ptr<std::string> source;
    size_t start_pos; // Inclusive
    size_t end_pos; // Exclusive
    std::unordered_set<size_t> free_variables;

    virtual ~Term();
    virtual std::string show() = 0;
    virtual std::shared_ptr<poi::Value> eval(
      std::shared_ptr<poi::Term> term,
      std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment
    ) = 0;
  };

  class Variable : public Term {
  public:
    std::shared_ptr<std::string> name;
    size_t variable_id;

    explicit Variable(
      std::shared_ptr<std::string> name,
      size_t variable_id
    );
    std::string show() override;
    std::shared_ptr<poi::Value> eval(
      std::shared_ptr<poi::Term> term,
      std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment
    ) override;
  };

  class Abstraction : public Term {
  public:
    std::shared_ptr<std::string> variable;
    size_t variable_id;
    std::shared_ptr<poi::Term> body;

    explicit Abstraction(
      std::shared_ptr<std::string> variable,
      size_t variable_id,
      std::shared_ptr<poi::Term> body
    );
    std::string show() override;
    std::shared_ptr<poi::Value> eval(
      std::shared_ptr<poi::Term> term,
      std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment
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
    std::string show() override;
    std::shared_ptr<poi::Value> eval(
      std::shared_ptr<poi::Term> term,
      std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment
    ) override;
  };

  class Let : public Term {
  public:
    std::shared_ptr<std::string> variable;
    size_t variable_id;
    std::shared_ptr<poi::Term> definition;
    std::shared_ptr<poi::Term> body;

    explicit Let(
      std::shared_ptr<std::string> variable,
      size_t variable_id,
      std::shared_ptr<poi::Term> definition,
      std::shared_ptr<poi::Term> body
    );
    std::string show() override;
    std::shared_ptr<poi::Value> eval(
      std::shared_ptr<poi::Term> term,
      std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment
    ) override;
  };

  class DataConstructor {
  public:
    std::shared_ptr<std::string> name;
    size_t name_id;
    std::shared_ptr<std::vector<std::shared_ptr<std::string>>> params;
    std::shared_ptr<std::vector<size_t>> param_ids;

    explicit DataConstructor(
      std::shared_ptr<std::string> name,
      size_t name_id,
      std::shared_ptr<std::vector<std::shared_ptr<std::string>>> params,
      std::shared_ptr<std::vector<size_t>> param_ids
    );
    std::string show();
  };

  class DataType : public Term {
  public:
    std::shared_ptr<std::vector<poi::DataConstructor>> constructors;

    explicit DataType(
      std::shared_ptr<std::vector<poi::DataConstructor>> constructors
    );
    std::string show() override;
    std::shared_ptr<poi::Value> eval(
      std::shared_ptr<poi::Term> term,
      std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment
    ) override;
  };

  class Group : public Term {
  public:
    std::shared_ptr<poi::Term> body;

    explicit Group(std::shared_ptr<poi::Term> body);
    std::string show() override;
    std::shared_ptr<poi::Value> eval(
      std::shared_ptr<poi::Term> term,
      std::unordered_map<size_t, std::shared_ptr<poi::Value>> &environment
    ) override;
  };

}

#endif
