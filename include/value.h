/*
  This header declares the data types for values.
*/

#ifndef POI_VALUE_H
#define POI_VALUE_H

#include <memory>
#include <unordered_map>

namespace poi {

  // Forward declarations to avoid mutually recursive headers
  class Abstraction; // Declared in poi/ast.h
  class DataType; // Declared in poi/ast.h

  class Value {
  public:
    virtual ~Value();
    virtual std::string show(poi::StringPool &pool) = 0;
  };

  class FunctionValue : public Value {
  public:
    std::shared_ptr<poi::Abstraction> abstraction;
    std::shared_ptr<
      std::unordered_map<size_t, std::shared_ptr<poi::Value>>
    > captures;

    explicit FunctionValue(
      std::shared_ptr<poi::Abstraction> abstraction,
      std::shared_ptr<
        std::unordered_map<size_t, std::shared_ptr<poi::Value>>
      > captures
    );
    std::string show(poi::StringPool &pool) override;
  };

  class DataTypeValue : public Value {
  public:
    std::shared_ptr<poi::DataType> data_type;

    explicit DataTypeValue(
      std::shared_ptr<poi::DataType> data_type
    );
    std::string show(poi::StringPool &pool) override;
  };

  class DataValue : public Value {
  public:
    std::shared_ptr<poi::DataTypeValue> type;
    std::size_t constructor;
    std::shared_ptr<
      std::unordered_map<size_t, std::shared_ptr<poi::Value>>
    > captures;

    explicit DataValue(
      std::shared_ptr<poi::DataTypeValue> type,
      std::size_t constructor,
      std::shared_ptr<
        std::unordered_map<size_t, std::shared_ptr<poi::Value>>
      > captures
    );
    std::string show(poi::StringPool &pool) override;
  };

}

#endif
