#include <poi/ast.h>
#include <poi/error.h>
#include <poi/value.h>

///////////////////////////////////////////////////////////////////////////////
// Helpers                                                                   //
///////////////////////////////////////////////////////////////////////////////

std::shared_ptr<const Poi::Value> Poi::trampoline(
  std::shared_ptr<const Poi::Value> value,
  const Poi::StringPool &pool,
  size_t depth
) {
  auto result = value;
  while (true) {
    auto thunk_value = std::dynamic_pointer_cast<
      const Poi::ThunkValue
    >(result);
    if (thunk_value) {
      result = thunk_value->term->eval(
        thunk_value->term,
        *(thunk_value->environment),
        pool,
        depth
      );
    } else {
      break;
    }
  }
  return result;
}

///////////////////////////////////////////////////////////////////////////////
// Value                                                                     //
///////////////////////////////////////////////////////////////////////////////

Poi::Value::~Value() {
}

///////////////////////////////////////////////////////////////////////////////
// FunctionValue                                                             //
///////////////////////////////////////////////////////////////////////////////

Poi::FunctionValue::FunctionValue(
  std::shared_ptr<const Poi::Function> function,
  std::shared_ptr<
    const std::unordered_map<size_t, std::shared_ptr<const Poi::Value>>
  > captures
) : function(function), captures(captures) {
}

std::string Poi::FunctionValue::show(const Poi::StringPool &pool) const {
  return function->show(pool);
}

///////////////////////////////////////////////////////////////////////////////
// DataTypeValue                                                             //
///////////////////////////////////////////////////////////////////////////////

Poi::DataTypeValue::DataTypeValue(
  std::shared_ptr<const Poi::DataType> data_type,
  const std::shared_ptr<
    const std::unordered_map<size_t, std::shared_ptr<const Poi::Value>>
  > constructors
) : data_type(data_type), constructors(constructors) {
}

std::string Poi::DataTypeValue::show(const Poi::StringPool &pool) const {
  return data_type->show(pool);
}

///////////////////////////////////////////////////////////////////////////////
// DataValue                                                                 //
///////////////////////////////////////////////////////////////////////////////

Poi::DataValue::DataValue(
  std::shared_ptr<const Poi::DataType> data_type,
  std::size_t constructor,
  std::shared_ptr<
    const std::unordered_map<size_t, std::shared_ptr<const Poi::Value>>
  > members
) : data_type(data_type), constructor(constructor), members(members) {
}

std::string Poi::DataValue::show(const Poi::StringPool &pool) const {
  std::string result = "(" + pool.find(constructor);
  for (auto &member : *members) {
    result += " " + member.second->show(pool);
  }
  result += ")";
  return result;
}

///////////////////////////////////////////////////////////////////////////////
// ProxyValue                                                                //
///////////////////////////////////////////////////////////////////////////////

Poi::ProxyValue::ProxyValue(
  std::shared_ptr<const Poi::Value> value
) : value(value) {
}

std::string Poi::ProxyValue::show(const Poi::StringPool &pool) const {
  if (value) {
    return value->show(pool);
  } else {
    throw Poi::Error("Undefined.");
  }
}

///////////////////////////////////////////////////////////////////////////////
// ThunkValue                                                                //
///////////////////////////////////////////////////////////////////////////////

Poi::ThunkValue::ThunkValue(
  std::shared_ptr<const Poi::Term> term,
  std::shared_ptr<
    const std::unordered_map<size_t, std::shared_ptr<const Poi::Value>>
  > environment
) : term(term), environment(environment) {
}

std::string Poi::ThunkValue::show(const Poi::StringPool &pool) const {
  return "<" + term->show(pool) + ">";
}
