#include <memory>
#include <poi/optimizer.h>

std::shared_ptr<poi::Term> poi::optimize(std::shared_ptr<poi::Term> term) {
  // Currently, the only optimization pass is replacing Groups with their
  // bodies.
  auto variable = std::dynamic_pointer_cast<poi::Variable>(term);
  if (variable) {
    return variable;
  }
  auto abstraction = std::dynamic_pointer_cast<poi::Abstraction>(term);
  if (abstraction) {
    auto free_variables = std::make_shared<std::unordered_set<size_t>>();
    free_variables->insert(
      abstraction->free_variables->begin(),
      abstraction->free_variables->end()
    );
    return std::make_shared<poi::Abstraction>(
      abstraction->source_name,
      abstraction->source,
      abstraction->start_pos,
      abstraction->end_pos,
      free_variables,
      abstraction->variable,
      optimize(abstraction->body)
    );
  }
  auto application = std::dynamic_pointer_cast<poi::Application>(term);
  if (application) {
    auto free_variables = std::make_shared<std::unordered_set<size_t>>();
    free_variables->insert(
      application->free_variables->begin(),
      application->free_variables->end()
    );
    return std::make_shared<poi::Application>(
      application->source_name,
      application->source,
      application->start_pos,
      application->end_pos,
      free_variables,
      optimize(application->abstraction),
      optimize(application->operand)
    );
  }
  auto let = std::dynamic_pointer_cast<poi::Let>(term);
  if (let) {
    auto free_variables = std::make_shared<std::unordered_set<size_t>>();
    free_variables->insert(
      let->free_variables->begin(),
      let->free_variables->end()
    );
    return std::make_shared<poi::Let>(
      let->source_name,
      let->source,
      let->start_pos,
      let->end_pos,
      free_variables,
      let->variable,
      optimize(let->definition),
      optimize(let->body)
    );
  }
  auto group = std::dynamic_pointer_cast<poi::Group>(term);
  if (group) {
    return group->body;
  }
  return term;
}
