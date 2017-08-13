/*
  This header declares the data types for values.
*/

#ifndef POI_VALUE_H
#define POI_VALUE_H

#include <memory>
#include <poi/string_pool.h>
#include <string>
#include <unordered_map>

namespace Poi {
  // Forward declarations to avoid mutually recursive headers
  class Function; // Declared in poi/ast.h
  class DataType; // Declared in poi/ast.h

  class Value {
  public:
    virtual ~Value();
    virtual std::string show(const Poi::StringPool &pool) const = 0;
  };

  class FunctionValue : public Value {
  public:
    const std::shared_ptr<const Poi::Function> function;
    const std::shared_ptr<
      const std::unordered_map<size_t, std::shared_ptr<const Poi::Value>>
    > captures;

    explicit FunctionValue(
      std::shared_ptr<const Poi::Function> function,
      std::shared_ptr<
        const std::unordered_map<size_t, std::shared_ptr<const Poi::Value>>
      > captures
    );
    std::string show(const Poi::StringPool &pool) const override;
  };

  class DataTypeValue : public Value {
  public:
    const std::shared_ptr<const Poi::DataType> data_type;

    explicit DataTypeValue(
      std::shared_ptr<const Poi::DataType> data_type
    );
    std::string show(const Poi::StringPool &pool) const override;
  };

  class DataValue : public Value {
  public:
    const std::shared_ptr<const Poi::DataType> type;
    const std::size_t constructor;
    const std::shared_ptr<
      const std::unordered_map<size_t, std::shared_ptr<const Poi::Value>>
    > captures;

    explicit DataValue(
      std::shared_ptr<const Poi::DataType> type,
      std::size_t constructor,
      std::shared_ptr<
        const std::unordered_map<size_t, std::shared_ptr<const Poi::Value>>
      > captures
    );
    std::string show(const Poi::StringPool &pool) const override;
  };

  // Used to "tie the knot" for recursive bindings
  class ProxyValue : public Value {
  public:
    const std::shared_ptr<const Poi::Value> value;

    explicit ProxyValue(
      std::shared_ptr<const Poi::Value> value
    );
    std::string show(const Poi::StringPool &pool) const override;
  };
}

#endif
