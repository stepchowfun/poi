/*
  This header declares the abstract syntax tree.
*/

#ifndef POI_AST_H
#define POI_AST_H

#include <memory>
#include <string>

namespace poi {

  class Term {
  public:
    std::shared_ptr<std::string> source_name;
    std::shared_ptr<std::string> source;
    size_t start_pos; // Inclusive
    size_t end_pos; // Exclusive

    virtual ~Term();
    virtual std::unique_ptr<Term> clone() = 0;
    virtual std::string show() = 0;
  };

  class Variable : public Term {
  public:
    std::shared_ptr<std::string> name;
    size_t de_bruijn_index;

    explicit Variable(
      std::shared_ptr<std::string> name,
      size_t de_bruijn_index
    );
    std::unique_ptr<Term> clone() override;
    std::string show() override;
  };

  class Abstraction : public Term {
  public:
    std::shared_ptr<std::string> variable;
    std::shared_ptr<poi::Term> body;

    Abstraction(
      std::shared_ptr<std::string> variable,
      std::shared_ptr<poi::Term> body
    );
    std::unique_ptr<Term> clone() override;
    std::string show() override;
  };

  class Application : public Term {
  public:
    std::shared_ptr<poi::Term> abstraction;
    std::shared_ptr<poi::Term> operand;

    Application(
      std::shared_ptr<poi::Term> abstraction,
      std::shared_ptr<poi::Term> operand
    );
    std::unique_ptr<Term> clone() override;
    std::string show() override;
  };

  class Let : public Term {
  public:
    std::shared_ptr<std::string> variable;
    std::shared_ptr<poi::Term> definition;
    std::shared_ptr<poi::Term> body;

    Let(
      std::shared_ptr<std::string> variable,
      std::shared_ptr<poi::Term> definition,
      std::shared_ptr<poi::Term> body
    );
    std::unique_ptr<Term> clone() override;
    std::string show() override;
  };

  class Group : public Term {
  public:
    std::shared_ptr<poi::Term> body;

    explicit Group(std::shared_ptr<poi::Term> body);
    std::unique_ptr<Term> clone() override;
    std::string show() override;
  };

}

#endif
