/*
  This header declares the data types for values.
*/

#ifndef POI_VALUE_H
#define POI_VALUE_H

#include <memory>
#include <poi/ast.h>
#include <unordered_map>

namespace poi {

  class Abstraction; // Declared in gram/ast.h

  class Value {
  public:
    virtual ~Value();
    virtual std::string show() = 0;
  };

  class FunctionValue : public Value {
  public:
    std::shared_ptr<poi::Abstraction> abstraction;
    std::unordered_map<size_t, std::shared_ptr<poi::Value>> captures;

    explicit FunctionValue(
      std::shared_ptr<poi::Abstraction> abstraction,
      std::unordered_map<size_t, std::shared_ptr<poi::Value>> &captures
    );
    std::string show() override;
  };

}

#endif
