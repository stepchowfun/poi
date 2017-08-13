#include <poi/error.h>
#include <poi/util.h>

void Poi::variables_from_pattern(
  std::unordered_set<size_t> &variables,
  std::shared_ptr<const Poi::Pattern> pattern,
  const Poi::StringPool &pool
) {
  auto variable_pattern = std::dynamic_pointer_cast<
    const Poi::VariablePattern
  >(pattern);
  if (variable_pattern) {
    if (variables.find(variable_pattern->variable) != variables.end()) {
      throw Poi::Error(
        "Duplicate variable '" +
          pool.find(variable_pattern->variable) +
          "' in pattern.",
        pool.find(pattern->source_name),
        pool.find(pattern->source),
        pattern->start_pos,
        pattern->end_pos
      );
    }
    variables.insert(variable_pattern->variable);
  }

  auto constructor_pattern = std::dynamic_pointer_cast<
    const Poi::ConstructorPattern
  >(pattern);
  if (constructor_pattern) {
    for (auto &parameter : *(constructor_pattern->parameters)) {
      variables_from_pattern(variables, parameter, pool);
    }
  }
}
