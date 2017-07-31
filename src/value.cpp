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
  std::unordered_map<size_t, std::shared_ptr<poi::Value>> &captures
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
  std::shared_ptr<std::vector<std::shared_ptr<poi::Value>>> members
) : type(type), constructor(constructor), members(members) {
}

std::string poi::DataValue::show(poi::StringPool &pool) {
  std::string result =
    "(" +
    pool.find((*type->data_type->constructors)[constructor].name);
  for (auto &member : *members) {
    result += " " + member->show(pool);
  }
  result += " : " + type->show(pool) + ")";
  return result;
}
