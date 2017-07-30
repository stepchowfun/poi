#include <memory>
#include <poi/optimizer.h>

std::shared_ptr<poi::Term> poi::optimize(std::shared_ptr<poi::Term> term) {
  auto variable = std::dynamic_pointer_cast<poi::Variable>(term);
  if (variable) {
    return variable;
  }
  auto abstraction = std::dynamic_pointer_cast<poi::Abstraction>(term);
  if (abstraction) {
    return std::make_shared<poi::Abstraction>(
      abstraction->variable,
      abstraction->variable_id,
      optimize(abstraction->body)
    );
  }
  auto application = std::dynamic_pointer_cast<poi::Application>(term);
  if (application) {
    return std::make_shared<poi::Application>(
      optimize(application->abstraction),
      optimize(application->operand)
    );
  }
  auto let = std::dynamic_pointer_cast<poi::Let>(term);
  if (let) {
    return std::make_shared<poi::Let>(
      let->variable,
      let->variable_id,
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
