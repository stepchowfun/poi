/*
  This header declares the data types for values.
*/

#ifndef POI_VALUE_H
#define POI_VALUE_H

#include <memory>
#include <poi/ast.h>
#include <poi/string_pool.h>
#include <string>
#include <unordered_map>

namespace Poi {
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
    const std::shared_ptr<
      const std::unordered_map<size_t, std::shared_ptr<const Poi::Value>>
    > constructors;

    explicit DataTypeValue(
      std::shared_ptr<const Poi::DataType> data_type,
      const std::shared_ptr<
        const std::unordered_map<size_t, std::shared_ptr<const Poi::Value>>
      > constructors
    );
    std::string show(const Poi::StringPool &pool) const override;
  };

  class DataValue : public Value {
  public:
    const std::shared_ptr<const Poi::DataType> data_type;
    const std::size_t constructor;
    const std::shared_ptr<
      const std::unordered_map<size_t, std::shared_ptr<const Poi::Value>>
    > members;

    explicit DataValue(
      std::shared_ptr<const Poi::DataType> type,
      std::size_t constructor,
      std::shared_ptr<
        const std::unordered_map<size_t, std::shared_ptr<const Poi::Value>>
      > members
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

  // Used to implement tail recursion
  class ThunkValue : public Value {
  public:
    const std::shared_ptr<const Poi::Term> term;
    const std::shared_ptr<
      const std::unordered_map<size_t, std::shared_ptr<const Poi::Value>>
    > environment;

    explicit ThunkValue(
      std::shared_ptr<const Poi::Term> term,
      std::shared_ptr<
        const std::unordered_map<size_t, std::shared_ptr<const Poi::Value>>
      > environment
    );
    std::string show(const Poi::StringPool &pool) const override;
  };

  // Repeatedly evaluate a Poi::Value until it is no longer a Poi::ThunkValue.
  std::shared_ptr<const Poi::Value> trampoline(
    std::shared_ptr<const Poi::Value> value,
    const Poi::StringPool &pool,
    size_t depth
  );
}

#endif
