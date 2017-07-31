#include <poi/ast.h>
#include <poi/error.h>
#include <poi/value.h>

///////////////////////////////////////////////////////////////////////////////
// Value                                                                     //
///////////////////////////////////////////////////////////////////////////////

poi::Value::~Value() {
}

///////////////////////////////////////////////////////////////////////////////
// FunctionValue                                                             //
///////////////////////////////////////////////////////////////////////////////

poi::FunctionValue::FunctionValue(
  std::shared_ptr<poi::Abstraction> abstraction,
  std::shared_ptr<
    std::unordered_map<size_t, std::shared_ptr<poi::Value>>
  > captures
) : abstraction(abstraction), captures(captures) {
}

std::string poi::FunctionValue::show(poi::StringPool &pool) {
  return abstraction->show(pool);
}

///////////////////////////////////////////////////////////////////////////////
// DataTypeValue                                                             //
///////////////////////////////////////////////////////////////////////////////

poi::DataTypeValue::DataTypeValue(
  std::shared_ptr<poi::DataType> data_type
) : data_type(data_type) {
}

std::string poi::DataTypeValue::show(poi::StringPool &pool) {
  return data_type->show(pool);
}

///////////////////////////////////////////////////////////////////////////////
// DataValue                                                                 //
///////////////////////////////////////////////////////////////////////////////

poi::DataValue::DataValue(
  std::shared_ptr<poi::DataTypeValue> type,
  std::size_t constructor,
  std::shared_ptr<
    std::unordered_map<size_t, std::shared_ptr<poi::Value>>
  > captures
) : type(type), constructor(constructor), captures(captures) {
}

std::string poi::DataValue::show(poi::StringPool &pool) {
  std::string result = "(" + pool.find(constructor);
  for (auto &capture : *captures) {
    result += " " + capture.second->show(pool);
  }
  result += ")";
  return result;
}
