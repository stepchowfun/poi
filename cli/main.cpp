#include <fstream>
#include <iostream>
#include <memory>
#include <poi/ast.h>
#include <poi/error.h>
#include <poi/instruction.h>
#include <poi/parser.h>
#include <poi/string_pool.h>
#include <poi/tokenizer.h>
#include <poi/value.h>
#include <poi/version.h>
#include <sstream>

// These are the actions we can perform on a source file.
enum class CliAction {
  EMIT_TOKENS,
  EMIT_AST,
  EMIT_BYTECODE,
  EVAL
};

// The logic for the command line interface is here.
// Run `poi --help` for instructions.
int main(int argc, char *argv[]) {
  // Print this message if we are unable to parse the input.
  const std::string parse_error = "Try poi --help for more information.\n";

  // Display the help message.
  if (
    argc == 1 || (
      argc == 2 && (
        std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help"
      )
    )
  ) {
    std::cout <<
      "Poi (https://github.com/stepchowfun/poi)\n"
      "----------------------------\n"
      "Usage:\n"
      "  poi -h, --help\n"
      "  poi -v, --version\n"
      "  poi source\n"
      "  poi --emit-tokens source\n"
      "  poi --emit-ast source\n"
      "  poi --emit-bytecode source\n"
      "  poi --eval source\n";
    return 0;
  }

  // Display the version information.
  if (argc == 2 && (
    std::string(argv[1]) == "-v" || std::string(argv[1]) == "--version")
  ) {
    std::cout << "Version: " << Poi::VERSION << "\n";
    if (Poi::COMMIT_HASH) {
      std::cout << "Commit: " << Poi::COMMIT_HASH << "\n";
    }
    std::cout << "Build type: " << Poi::BUILD_TYPE << "\n";
    return 0;
  }

  // Check that we got the right number of arguments.
  if (argc != 2 && argc != 3) {
    std::cerr << parse_error;
    return 1;
  }

  // Determine what the user wants us to do.
  std::string input_path = argv[1];
  auto cli_action = CliAction::EVAL;
  if (argc == 3) {
    if (std::string(argv[1]) == "--emit-tokens") {
      cli_action = CliAction::EMIT_TOKENS;
    } else if (std::string(argv[1]) == "--emit-ast") {
      cli_action = CliAction::EMIT_AST;
    } else if (std::string(argv[1]) == "--emit-bytecode") {
      cli_action = CliAction::EMIT_BYTECODE;
    } else if (std::string(argv[1]) == "--eval") {
      cli_action = CliAction::EVAL;
    } else {
      // We didn't recognize the syntax.
      std::cerr << parse_error;
      return 1;
    }
    input_path = argv[2];
  }

  // Create a string pool to improve performance.
  Poi::StringPool pool;

  // Read the source file.
  std::ifstream input_file(input_path);
  if (!input_file.is_open()) {
    std::cerr << "Unable to open file '" + std::string(input_path) + "'.\n";
    return 1;
  }
  std::stringstream file_buffer;
  file_buffer << input_file.rdbuf();
  input_file.close();
  std::string source_str = file_buffer.str();
  auto source_name = pool.insert(input_path);
  auto source = pool.insert(source_str);

  // Catch any Poi errors and report them.
  try {
    // Perform lexical analysis.
    auto token_stream = Poi::tokenize(source_name, source, pool);
    if (cli_action == CliAction::EMIT_TOKENS) {
      for (auto &token : *(token_stream.tokens)) {
        std::cout << token.show(pool) << "\n";
      }
      return 0;
    }

    // Parse the tokens into an AST.
    auto term = Poi::parse(token_stream, pool);
    if (cli_action == CliAction::EMIT_AST) {
      std::cout << term->show(pool) << "\n";
      return 0;
    }

    // Compile the AST into bytecode.
    std::vector<Poi::Instruction> program;
    std::vector<Poi::Instruction> expression;
    std::unordered_map<size_t, size_t> environment;
    term->emit_instructions(program, expression, environment, 0);
    program.insert(program.end(), expression.begin(), expression.end());
    if (cli_action == CliAction::EMIT_BYTECODE) {
      for (auto &instruction : program) {
        std::cout << instruction.show(pool) << "\n";
      }
      return 0;
    }

    // Evaluate the program.
    std::unordered_map<
      size_t,
      std::shared_ptr<const Poi::Value>
    > interpreter_environment;
    std::vector<std::shared_ptr<const Poi::Term>> stack_trace;
    auto value = Poi::trampoline(
      term->eval(term, interpreter_environment, stack_trace, pool),
      stack_trace,
      pool
    );
    std::cout << value->show(pool) << "\n";
  } catch(Poi::Error &e) {
    // There was an error. Print it and exit.
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
