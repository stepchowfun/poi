#include <poi/ast.h>
#include <poi/error.h>
#include <poi/instruction.h>
#include <poi/value.h>

///////////////////////////////////////////////////////////////////////////////
// Configuration                                                             //
///////////////////////////////////////////////////////////////////////////////

const size_t max_evaluation_depth = 1024;
const size_t tail_call_elimination_min_depth = 100;

///////////////////////////////////////////////////////////////////////////////
// Error handling                                                            //
///////////////////////////////////////////////////////////////////////////////

// This is used to signal errors in pattern matching.
class MatchError : public Poi::Error {
public:
  explicit MatchError(
    const std::string &message // No trailing line break
  ) : Error(message) {
  };
};

// This is used to signal errors in evaluation.
class EvaluationError : public Poi::Error {
public:
  explicit EvaluationError(
    const std::string &message, // No trailing line break
    const std::vector<std::shared_ptr<const Poi::Term>> &stack_trace,
    const Poi::StringPool &pool
  ) {
    std::string new_message = message + "\n";
    for (
      auto iter = stack_trace.rbegin();
      iter != stack_trace.rend();
      ++iter
    ) {
      if (std::dynamic_pointer_cast<const Poi::Binding>(*iter)) {
        // Don't show frames corresponding to Binding expressions, because
        // those are not very helpful.
        continue;
      }

      new_message += "\n" + Poi::get_location(
        pool.find((*iter)->source_name),
        pool.find((*iter)->source),
        (*iter)->start_pos,
        (*iter)->end_pos
      );
    }
    update(new_message);
  };
};

///////////////////////////////////////////////////////////////////////////////
// Helpers                                                                   //
///////////////////////////////////////////////////////////////////////////////

void pattern_match(
  std::unordered_map<
    size_t, std::shared_ptr<const Poi::Value>
  > &environment,
  const std::vector<std::shared_ptr<const Poi::Term>> &stack_trace,
  const Poi::StringPool &pool,
  std::shared_ptr<const Poi::Pattern> pattern,
  std::shared_ptr<const Poi::Value> value
) {
  auto variable_pattern = std::dynamic_pointer_cast<
    const Poi::VariablePattern
  >(pattern);
  if (variable_pattern) {
    auto iter = environment.find(variable_pattern->variable);
    if (iter != environment.end()) {
      auto proxy_value = std::dynamic_pointer_cast<const Poi::ProxyValue>(
        iter->second
      );
      if (proxy_value) {
        const_cast<
          std::shared_ptr<const Poi::Value> &
        >(proxy_value->value) = value;
      }
      environment.erase(iter);
    }
    environment.insert({ variable_pattern->variable, value });
    return;
  }

  auto constructor_pattern = std::dynamic_pointer_cast<
    const Poi::ConstructorPattern
  >(pattern);
  if (constructor_pattern) {
    auto value_data = std::dynamic_pointer_cast<const Poi::DataValue>(value);
    if (value_data) {
      if (
        value_data->data_type->constructor_params->find(
          constructor_pattern->constructor
        ) == value_data->data_type->constructor_params->end()
      ) {
        throw EvaluationError(
          "'" + pool.find(constructor_pattern->constructor) +
            "' is not a constructor of " +
            value_data->data_type->show(pool) + ".",
          stack_trace,
          pool
        );
      }
      if (constructor_pattern->constructor == value_data->constructor) {
        if (
          constructor_pattern->parameters->size() ==
          value_data->data_type->constructor_params->at(
            value_data->constructor
          ).size()
        ) {
          size_t member_index = 0;
          for (auto &parameter : *(constructor_pattern->parameters)) {
            pattern_match(
              environment,
              stack_trace,
              pool,
              parameter,
              value_data->members->at(
                value_data->data_type->constructor_params->at(
                  value_data->constructor
                ).at(member_index)
              )
            );
            ++member_index;
          }
          return;
        }
      }
    }
    throw MatchError(
      "Unable to match " + value->show(pool) + " to this pattern."
    );
  }
}

std::shared_ptr<const Poi::Value> tail_call(
  std::shared_ptr<const Poi::Term> term,
  std::vector<std::shared_ptr<const Poi::Term>> &stack_trace,
  std::shared_ptr<
    const std::unordered_map<size_t, std::shared_ptr<const Poi::Value>>
  > environment,
  const Poi::StringPool &pool
) {
  if (stack_trace.size() >= tail_call_elimination_min_depth) {
    return std::static_pointer_cast<const Poi::Value>(
      std::make_shared<const Poi::ThunkValue>(term, environment)
    );
  } else {
    return term->eval(term, *environment, stack_trace, pool);
  }
}

void enter_frame(
  std::shared_ptr<const Poi::Term> term,
  std::vector<std::shared_ptr<const Poi::Term>> &stack_trace,
  const Poi::StringPool &pool
) {
  stack_trace.push_back(term);
  if (stack_trace.size() >= max_evaluation_depth) {
    throw EvaluationError(
      "Maximum evaluation depth exceeded.",
      stack_trace,
      pool
    );
  }
}

void leave_frame(std::vector<std::shared_ptr<const Poi::Term>> &stack_trace) {
  stack_trace.pop_back();
}

///////////////////////////////////////////////////////////////////////////////
// Node                                                                      //
///////////////////////////////////////////////////////////////////////////////

Poi::Node::Node(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos
) :
  source_name(source_name),
  source(source),
  start_pos(start_pos),
  end_pos(end_pos) {
}

Poi::Node::~Node() {
}

///////////////////////////////////////////////////////////////////////////////
// Pattern                                                                   //
///////////////////////////////////////////////////////////////////////////////

Poi::Pattern::Pattern(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<const std::unordered_set<size_t>> variables
) : Node(
    source_name,
    source,
    start_pos,
    end_pos
  ), variables(variables) {
}

Poi::Pattern::~Pattern() {
}

///////////////////////////////////////////////////////////////////////////////
// VariablePattern                                                           //
///////////////////////////////////////////////////////////////////////////////

Poi::VariablePattern::VariablePattern(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  size_t variable,
  std::shared_ptr<const std::unordered_set<size_t>> variables
) : Pattern(
    source_name,
    source,
    start_pos,
    end_pos,
    variables
  ), variable(variable) {
}

std::string Poi::VariablePattern::show(const Poi::StringPool &pool) const {
  return pool.find(variable);
}

///////////////////////////////////////////////////////////////////////////////
// ConstructorPattern                                                        //
///////////////////////////////////////////////////////////////////////////////

Poi::ConstructorPattern::ConstructorPattern(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  size_t constructor,
  std::shared_ptr<
    const std::vector<std::shared_ptr<const Poi::Pattern>>
  > parameters,
  std::shared_ptr<const std::unordered_set<size_t>> variables
) : Pattern(
    source_name,
    source,
    start_pos,
    end_pos,
    variables
  ),
  constructor(constructor),
  parameters(parameters) {
}

std::string Poi::ConstructorPattern::show(const Poi::StringPool &pool) const {
  std::string result = "{" + pool.find(constructor);
  for (
    auto iter = parameters->begin();
    iter != parameters->end();
    ++iter
  ) {
    result += " " + (*iter)->show(pool);
  }
  result += "}";
  return result;
}

///////////////////////////////////////////////////////////////////////////////
// Term                                                                      //
///////////////////////////////////////////////////////////////////////////////

Poi::Term::Term(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<const std::unordered_set<size_t>> free_variables
) : Node(
    source_name,
    source,
    start_pos,
    end_pos
  ), free_variables(free_variables) {
}

Poi::Term::~Term() {
}

///////////////////////////////////////////////////////////////////////////////
// Variable                                                                  //
///////////////////////////////////////////////////////////////////////////////

Poi::Variable::Variable(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<const std::unordered_set<size_t>> free_variables,
  size_t variable
) : Term(
    source_name,
    source,
    start_pos,
    end_pos,
    free_variables
  ), variable(variable) {
}

std::string Poi::Variable::show(const Poi::StringPool &pool) const {
  return pool.find(variable);
}

std::shared_ptr<const Poi::Value> Poi::Variable::eval(
  std::shared_ptr<const Poi::Term> term,
  const std::unordered_map<
    size_t, std::shared_ptr<const Poi::Value>
  > &environment,
  std::vector<std::shared_ptr<const Poi::Term>> &stack_trace,
  const Poi::StringPool &pool
) const {
  enter_frame(term, stack_trace, pool);
  auto value = environment.at(variable);
  while (true) {
    auto proxy_value = std::dynamic_pointer_cast<const ProxyValue>(value);
    if (proxy_value) {
      value = proxy_value->value;
      if (!value) {
        throw EvaluationError(
          "Recursive references must occur in function bodies.",
          stack_trace,
          pool
        );
      }
    } else {
      break;
    }
  }
  leave_frame(stack_trace);
  return value;
}

size_t Poi::Variable::emit_instructions(
  std::vector<Poi::Instruction> &program,
  std::vector<Poi::Instruction> &expression,
  const std::unordered_map<size_t, size_t> &environment,
  size_t destination,
  bool tail_position
) const {
  Instruction copy;
  copy.node = static_cast<const Poi::Node *>(this);
  copy.type = Poi::InstructionType::COPY;
  copy.copy_args.destination = destination;
  copy.copy_args.source = environment.find(variable)->second;
  expression.push_back(copy);
  return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Function                                                                  //
///////////////////////////////////////////////////////////////////////////////

Poi::Function::Function(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<const std::unordered_set<size_t>> free_variables,
  std::shared_ptr<const Poi::Pattern> pattern,
  std::shared_ptr<const Poi::Term> body
) : Term(
    source_name,
    source,
    start_pos,
    end_pos,
    free_variables
  ), pattern(pattern), body(body) {
}

std::string Poi::Function::show(const Poi::StringPool &pool) const {
  return "(" + pattern->show(pool) + " -> " + body->show(pool) + ")";
}

std::shared_ptr<const Poi::Value> Poi::Function::eval(
  std::shared_ptr<const Poi::Term> term,
  const std::unordered_map<
    size_t, std::shared_ptr<const Poi::Value>
  > &environment,
  std::vector<std::shared_ptr<const Poi::Term>> &stack_trace,
  const Poi::StringPool &pool
) const {
  enter_frame(term, stack_trace, pool);
  auto captures = std::make_shared<
    const std::unordered_map<size_t, std::shared_ptr<const Value>>
  >();
  for (auto iter : *free_variables) {
    std::const_pointer_cast<
      std::unordered_map<size_t, std::shared_ptr<const Value>>
    >(captures)->insert({ iter, environment.at(iter) });
  }
  leave_frame(stack_trace);
  return std::static_pointer_cast<const Value>(
    std::make_shared<FunctionValue>(
      std::dynamic_pointer_cast<const Function>(term),
      captures
    )
  );
}

size_t Poi::Function::emit_instructions(
  std::vector<Poi::Instruction> &program,
  std::vector<Poi::Instruction> &expression,
  const std::unordered_map<size_t, size_t> &environment,
  size_t destination,
  bool tail_position
) const {
  Instruction function;
  function.node = static_cast<const Poi::Node *>(this);
  function.type = Poi::InstructionType::CREATE_FUNCTION;
  function.create_function_args.destination = destination;
  function.create_function_args.body = program.size();

  std::unordered_map<size_t, size_t> body_environment;
  auto variable = std::dynamic_pointer_cast<
    const VariablePattern
  >(pattern)->variable;

  function.create_function_args.captures = new size_t[free_variables->size()];
  body_environment.insert({ variable, 2 });
  size_t index = 3;
  for (auto iter : *free_variables) {
    body_environment.insert({ iter, index });
    function.create_function_args.captures[index - 3] = environment.at(iter);
    index++;
  }

  function.create_function_args.num_captures = free_variables->size();
  function.create_function_args.frame_size = index + body->emit_instructions(
    program,
    program,
    body_environment,
    index,
    true
  );

  if (program.back().type != Poi::InstructionType::CALL_TAIL) {
    Instruction return_instruction;
    return_instruction.node = static_cast<const Poi::Node *>(this);
    return_instruction.type = Poi::InstructionType::RETURN;
    return_instruction.return_args.value = index;
    program.push_back(return_instruction);
  }

  expression.push_back(function);
  return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Application                                                               //
///////////////////////////////////////////////////////////////////////////////

Poi::Application::Application(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<const std::unordered_set<size_t>> free_variables,
  std::shared_ptr<const Poi::Term> function,
  std::shared_ptr<const Poi::Term> operand
) : Term(
    source_name,
    source,
    start_pos,
    end_pos,
    free_variables
), function(function), operand(operand) {
}

std::string Poi::Application::show(const Poi::StringPool &pool) const {
  return "(" + function->show(pool) + " " + operand->show(pool) + ")";
}

std::shared_ptr<const Poi::Value> Poi::Application::eval(
  std::shared_ptr<const Poi::Term> term,
  const std::unordered_map<
    size_t, std::shared_ptr<const Poi::Value>
  > &environment,
  std::vector<std::shared_ptr<const Poi::Term>> &stack_trace,
  const Poi::StringPool &pool
) const {
  enter_frame(term, stack_trace, pool);
  auto function_value = trampoline(
    function->eval(function, environment, stack_trace, pool),
    stack_trace,
    pool
  );
  auto operand_value = trampoline(
    operand->eval(operand, environment, stack_trace, pool),
    stack_trace,
    pool
  );
  auto function_value_fun = std::dynamic_pointer_cast<
    const FunctionValue
  >(function_value);
  if (!function_value_fun) {
    throw EvaluationError(
      function_value->show(pool) + " is not a function.",
      stack_trace,
      pool
    );
  }
  auto new_environment = std::make_shared<
    const std::unordered_map<size_t, std::shared_ptr<const Value>>
  >();
  for (auto iter : *(function_value_fun->captures)) {
    std::const_pointer_cast<
      std::unordered_map<size_t, std::shared_ptr<const Value>>
    >(new_environment)->insert(iter);
  }
  try {
    pattern_match(
      *std::const_pointer_cast<
        std::unordered_map<size_t, std::shared_ptr<const Value>>
      >(new_environment),
      stack_trace,
      pool,
      function_value_fun->function->pattern,
      operand_value
    );
  } catch (MatchError &e) {
    throw EvaluationError(e.what(), stack_trace, pool);
  }
  auto result = tail_call(
    function_value_fun->function->body,
    stack_trace,
    new_environment,
    pool
  );
  leave_frame(stack_trace);
  return result;
}

size_t Poi::Application::emit_instructions(
  std::vector<Poi::Instruction> &program,
  std::vector<Poi::Instruction> &expression,
  const std::unordered_map<size_t, size_t> &environment,
  size_t destination,
  bool tail_position
) const {
  auto function_footprint = function->emit_instructions(
    program,
    expression,
    environment,
    destination,
    false
  );
  auto operand_footprint = operand->emit_instructions(
    program,
    expression,
    environment,
    destination + function_footprint,
    false
  );

  Instruction call;
  call.node = this;
  if (tail_position) {
    call.type = Poi::InstructionType::CALL_TAIL;
    call.call_tail_args.destination = destination;
    call.call_tail_args.function = destination;
    call.call_tail_args.argument = destination + function_footprint;
  } else {
    call.type = Poi::InstructionType::CALL_NON_TAIL;
    call.call_non_tail_args.destination = destination;
    call.call_non_tail_args.function = destination;
    call.call_non_tail_args.argument = destination + function_footprint;
  }

  expression.push_back(call);
  return function_footprint + operand_footprint;
}

///////////////////////////////////////////////////////////////////////////////
// Binding                                                                   //
///////////////////////////////////////////////////////////////////////////////

Poi::Binding::Binding(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<const std::unordered_set<size_t>> free_variables,
  std::shared_ptr<const Poi::Pattern> pattern,
  std::shared_ptr<const Poi::Term> definition,
  std::shared_ptr<const Poi::Term> body
) : Term(
    source_name,
    source,
    start_pos,
    end_pos,
    free_variables
), pattern(pattern), definition(definition), body(body) {
}

std::string Poi::Binding::show(const Poi::StringPool &pool) const {
  return
    "(" +
    pattern->show(pool) +
    " = " +
    definition->show(pool) +
    ", " +
    body->show(pool) +
    ")";
}

std::shared_ptr<const Poi::Value> Poi::Binding::eval(
  std::shared_ptr<const Poi::Term> term,
  const std::unordered_map<
    size_t, std::shared_ptr<const Poi::Value>
  > &environment,
  std::vector<std::shared_ptr<const Poi::Term>> &stack_trace,
  const Poi::StringPool &pool
) const {
  enter_frame(term, stack_trace, pool);
  auto new_environment = std::make_shared<
    const std::unordered_map<size_t, std::shared_ptr<const Value>>
  >(environment.begin(), environment.end());
  for (auto &variable : *(pattern->variables)) {
    std::const_pointer_cast<
      std::unordered_map<size_t, std::shared_ptr<const Value>>
    >(new_environment)->insert({
      variable,
      std::static_pointer_cast<Value>(
        std::make_shared<ProxyValue>(nullptr)
      )
    });
  }
  auto definition_value = trampoline(
    definition->eval(definition, *new_environment, stack_trace, pool),
    stack_trace,
    pool
  );
  try {
    pattern_match(
      *std::const_pointer_cast<
        std::unordered_map<size_t, std::shared_ptr<const Value>>
      >(new_environment),
      stack_trace,
      pool,
      pattern,
      definition_value
    );
  } catch (MatchError &e) {
    throw EvaluationError(e.what(), stack_trace, pool);
  }
  auto result = tail_call(body, stack_trace, new_environment, pool);
  leave_frame(stack_trace);
  return result;
}

size_t Poi::Binding::emit_instructions(
  std::vector<Poi::Instruction> &program,
  std::vector<Poi::Instruction> &expression,
  const std::unordered_map<size_t, size_t> &environment,
  size_t destination,
  bool tail_position
) const {
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// DataType                                                                  //
///////////////////////////////////////////////////////////////////////////////

Poi::DataType::DataType(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<const std::unordered_set<size_t>> free_variables,
  std::shared_ptr<const std::vector<size_t>> constructor_names,
  std::shared_ptr<
    const std::unordered_map<size_t, std::vector<size_t>>
  > constructor_params,
  std::shared_ptr<
    const std::unordered_map<size_t, std::shared_ptr<const Poi::Term>>
  > constructors
) : Term(
    source_name,
    source,
    start_pos,
    end_pos,
    free_variables
  ),
  constructor_names(constructor_names),
  constructor_params(constructor_params),
  constructors(constructors) {
}

std::string Poi::DataType::show(const Poi::StringPool &pool) const {
  std::string result = "{";
  for (
    auto iter = constructor_names->begin();
    iter != constructor_names->end();
    ++iter
  ) {
    result += pool.find(*iter);
    for (auto param : constructor_params->at(*iter)) {
      result += " " + pool.find(param);
    }
    if (std::next(iter) != constructor_names->end()) {
      result += ", ";
    }
  }
  result += "}";
  return result;
}

std::shared_ptr<const Poi::Value> Poi::DataType::eval(
  std::shared_ptr<const Poi::Term> term,
  const std::unordered_map<
    size_t, std::shared_ptr<const Poi::Value>
  > &environment,
  std::vector<std::shared_ptr<const Poi::Term>> &stack_trace,
  const Poi::StringPool &pool
) const {
  enter_frame(term, stack_trace, pool);
  auto constructor_values = std::make_shared<
    const std::unordered_map<size_t, std::shared_ptr<const Value>>
  >();
  for (auto &constructor : *constructors) {
    auto constructor_function = std::dynamic_pointer_cast<const Function>(
      constructor.second
    );
    if (constructor_function) {
      std::const_pointer_cast<
        std::unordered_map<size_t, std::shared_ptr<const Value>>
      >(constructor_values)->insert({
        constructor.first,
        std::static_pointer_cast<const Value>(
          std::make_shared<FunctionValue>(
            constructor_function,
            std::make_shared<
              std::unordered_map<size_t, std::shared_ptr<const Value>>
            >()
          )
        )
      });
    } else {
      std::const_pointer_cast<
        std::unordered_map<size_t, std::shared_ptr<const Value>>
      >(constructor_values)->insert({
        constructor.first,
        std::static_pointer_cast<const Value>(
          std::make_shared<DataValue>(
            std::dynamic_pointer_cast<const Data>(
              constructor.second
            )->data_type.lock(),
            constructor.first,
            std::make_shared<
              std::unordered_map<size_t, std::shared_ptr<const Value>>
            >()
          )
        )
      });
    }
  }
  leave_frame(stack_trace);
  return std::static_pointer_cast<const Value>(
    std::make_shared<DataTypeValue>(
      std::dynamic_pointer_cast<const DataType>(term),
      constructor_values
    )
  );
}

size_t Poi::DataType::emit_instructions(
  std::vector<Poi::Instruction> &program,
  std::vector<Poi::Instruction> &expression,
  const std::unordered_map<size_t, size_t> &environment,
  size_t destination,
  bool tail_position
) const {
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Data                                                                      //
///////////////////////////////////////////////////////////////////////////////

Poi::Data::Data(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<const std::unordered_set<size_t>> free_variables,
  std::weak_ptr<const Poi::DataType> data_type,
  size_t constructor
) : Term(
    source_name,
    source,
    start_pos,
    end_pos,
    free_variables
), data_type(data_type), constructor(constructor) {
}

std::string Poi::Data::show(const Poi::StringPool &pool) const {
  return
    "<" +
    data_type.lock()->show(pool) +
    "." +
    pool.find(constructor) +
    ">";
}

std::shared_ptr<const Poi::Value> Poi::Data::eval(
  std::shared_ptr<const Poi::Term> term,
  const std::unordered_map<
    size_t, std::shared_ptr<const Poi::Value>
  > &environment,
  std::vector<std::shared_ptr<const Poi::Term>> &stack_trace,
  const Poi::StringPool &pool
) const {
  enter_frame(term, stack_trace, pool);
  auto members = std::make_shared<
    std::unordered_map<size_t, std::shared_ptr<const Value>>
  >();
  for (auto iter : *free_variables) {
    members->insert({ iter, environment.at(iter) });
  }
  leave_frame(stack_trace);
  return std::static_pointer_cast<const Value>(
    std::make_shared<DataValue>(data_type.lock(), constructor, members)
  );
}

size_t Poi::Data::emit_instructions(
  std::vector<Poi::Instruction> &program,
  std::vector<Poi::Instruction> &expression,
  const std::unordered_map<size_t, size_t> &environment,
  size_t destination,
  bool tail_position
) const {
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Member                                                                    //
///////////////////////////////////////////////////////////////////////////////

Poi::Member::Member(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<const std::unordered_set<size_t>> free_variables,
  std::shared_ptr<const Poi::Term> object,
  size_t field
) : Term(
    source_name,
    source,
    start_pos,
    end_pos,
    free_variables
), object(object), field(field) {
}

std::string Poi::Member::show(const Poi::StringPool &pool) const {
  return "(" + object->show(pool) + "." + pool.find(field) + ")";
}

std::shared_ptr<const Poi::Value> Poi::Member::eval(
  std::shared_ptr<const Poi::Term> term,
  const std::unordered_map<
    size_t, std::shared_ptr<const Poi::Value>
  > &environment,
  std::vector<std::shared_ptr<const Poi::Term>> &stack_trace,
  const Poi::StringPool &pool
) const {
  enter_frame(term, stack_trace, pool);
  auto object_value = trampoline(
    object->eval(object, environment, stack_trace, pool),
    stack_trace,
    pool
  );
  auto data_type_value = std::dynamic_pointer_cast<const DataTypeValue>(
    object_value
  );
  if (data_type_value) {
    auto constructor = data_type_value->constructors->find(field);
    if (constructor == data_type_value->constructors->end()) {
      throw EvaluationError(
        "'" + pool.find(field) + "' is not a constructor of " +
          data_type_value->show(pool),
        stack_trace,
        pool
      );
    }
    leave_frame(stack_trace);
    return constructor->second;
  } else {
    auto data_value = std::dynamic_pointer_cast<const DataValue>(
      object_value
    );
    if (data_value) {
      auto members = data_value->members;
      auto member = members->find(field);
      if (member == members->end()) {
        throw EvaluationError(
          object_value->show(pool) +
            " has no member '" + pool.find(field) + "'",
          stack_trace,
          pool
        );
      } else {
        leave_frame(stack_trace);
        return member->second;
      }
    } else {
      throw EvaluationError(
        object_value->show(pool) + " has no member '" + pool.find(field) + "'",
        stack_trace,
        pool
      );
    }
  }
}

size_t Poi::Member::emit_instructions(
  std::vector<Poi::Instruction> &program,
  std::vector<Poi::Instruction> &expression,
  const std::unordered_map<size_t, size_t> &environment,
  size_t destination,
  bool tail_position
) const {
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Match                                                                     //
///////////////////////////////////////////////////////////////////////////////

Poi::Match::Match(
  size_t source_name,
  size_t source,
  size_t start_pos,
  size_t end_pos,
  std::shared_ptr<const std::unordered_set<size_t>> free_variables,
  std::shared_ptr<const Poi::Term> discriminee,
  std::shared_ptr<
    const std::vector<std::shared_ptr<const Poi::Function>>
  > cases
) : Term(
    source_name,
    source,
    start_pos,
    end_pos,
    free_variables
), discriminee(discriminee), cases(cases) {
}

std::string Poi::Match::show(const Poi::StringPool &pool) const {
  std::string result = "match " + discriminee->show(pool) + " {";
  bool first = true;
  for (auto &c : *cases) {
    if (!first) {
      result += ", ";
    }
    result += c->show(pool);
    first = false;
  }
  result += "}";
  return result;
}

std::shared_ptr<const Poi::Value> Poi::Match::eval(
  std::shared_ptr<const Poi::Term> term,
  const std::unordered_map<
    size_t, std::shared_ptr<const Poi::Value>
  > &environment,
  std::vector<std::shared_ptr<const Poi::Term>> &stack_trace,
  const Poi::StringPool &pool
) const {
  enter_frame(term, stack_trace, pool);
  auto discriminee_value = trampoline(
    discriminee->eval(discriminee, environment, stack_trace, pool),
    stack_trace,
    pool
  );

  std::shared_ptr<const Value> result;
  for (auto &c : *cases) {
    auto new_environment = std::make_shared<
      const std::unordered_map<size_t, std::shared_ptr<const Value>>
    >(environment.begin(), environment.end());
    try {
      pattern_match(
        *std::const_pointer_cast<
          std::unordered_map<size_t, std::shared_ptr<const Value>>
        >(new_environment),
        stack_trace,
        pool,
        c->pattern,
        discriminee_value
      );
    } catch (MatchError &e) {
      continue;
    }
    if (!result) {
      result = tail_call(c->body, stack_trace, new_environment, pool);
    }
  }
  if (result) {
    leave_frame(stack_trace);
    return result;
  }
  throw EvaluationError(
    discriminee_value->show(pool) + " didn't match any of the cases here.",
    stack_trace,
    pool
  );
}

size_t Poi::Match::emit_instructions(
  std::vector<Poi::Instruction> &program,
  std::vector<Poi::Instruction> &expression,
  const std::unordered_map<size_t, size_t> &environment,
  size_t destination,
  bool tail_position
) const {
  return 0;
}
