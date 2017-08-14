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
    virtual std::string show(const StringPool &pool) const = 0;
  };

  class FunctionValue : public Value {
  public:
    const std::shared_ptr<const Function> function;
    const std::shared_ptr<
      const std::unordered_map<size_t, std::shared_ptr<const Value>>
    > captures;

    explicit FunctionValue(
      std::shared_ptr<const Function> function,
      std::shared_ptr<
        const std::unordered_map<size_t, std::shared_ptr<const Value>>
      > captures
    );
    std::string show(const StringPool &pool) const override;
  };

  class DataTypeValue : public Value {
  public:
    const std::shared_ptr<const DataType> data_type;
    const std::shared_ptr<
      const std::unordered_map<size_t, std::shared_ptr<const Value>>
    > constructors;

    explicit DataTypeValue(
      std::shared_ptr<const DataType> data_type,
      const std::shared_ptr<
        const std::unordered_map<size_t, std::shared_ptr<const Value>>
      > constructors
    );
    std::string show(const StringPool &pool) const override;
  };

  class DataValue : public Value {
  public:
    const std::shared_ptr<const DataType> data_type;
    const std::size_t constructor;
    const std::shared_ptr<
      const std::unordered_map<size_t, std::shared_ptr<const Value>>
    > members;

    explicit DataValue(
      std::shared_ptr<const DataType> type,
      std::size_t constructor,
      std::shared_ptr<
        const std::unordered_map<size_t, std::shared_ptr<const Value>>
      > members
    );
    std::string show(const StringPool &pool) const override;
  };

  // Used to "tie the knot" for recursive bindings
  class ProxyValue : public Value {
  public:
    const std::shared_ptr<const Value> value;

    explicit ProxyValue(std::shared_ptr<const Value> value);
    std::string show(const StringPool &pool) const override;
  };

  // Used to implement tail recursion
  class ThunkValue : public Value {
  public:
    const std::shared_ptr<const Term> term;
    const std::shared_ptr<
      const std::unordered_map<size_t, std::shared_ptr<const Value>>
    > environment;

    explicit ThunkValue(
      std::shared_ptr<const Term> term,
      std::shared_ptr<
        const std::unordered_map<size_t, std::shared_ptr<const Value>>
      > environment
    );
    std::string show(const StringPool &pool) const override;
  };

  // Repeatedly evaluate a Value until it is no longer a ThunkValue.
  std::shared_ptr<const Value> trampoline(
    std::shared_ptr<const Value> value,
    std::vector<std::shared_ptr<const Term>> &stack_trace,
    const StringPool &pool
  );
}

#endif
