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

std::string poi::FunctionValue::show() {
  return abstraction->show();
}
