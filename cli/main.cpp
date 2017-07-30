#include <fstream>
#include <iostream>
#include <memory>
#include <poi/ast.h>
#include <poi/error.h>
#include <poi/optimizer.h>
#include <poi/parser.h>
#include <poi/tokenizer.h>
#include <poi/value.h>
#include <poi/version.h>
#include <sstream>

enum class CliAction {
  EMIT_TOKENS,
  EMIT_RAW_AST,
  EMIT_OPTIMIZED_AST,
  EVAL
};

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
      "  poi --emit-raw-ast source\n"
      "  poi --emit-optimized-ast source\n"
      "  poi --eval source\n";
    return 0;
  }

  // Display the version information.
  if (argc == 2 && (
    std::string(argv[1]) == "-v" || std::string(argv[1]) == "--version")
  ) {
    std::cout << "Version: " << poi::VERSION << "\n";
    if (poi::COMMIT_HASH) {
      std::cout << "Commit: " << poi::COMMIT_HASH << "\n";
    }
    std::cout << "Build type: " << poi::BUILD_TYPE << "\n";
    return 0;
  }

  // Check that we got the right number of arguments.
  if (argc != 2 && argc != 3) {
    std::cout << parse_error;
    return 1;
  }

  // Determine what the user wants us to do.
  auto input_path = argv[1];
  auto cli_action = CliAction::EVAL;
  if (argc == 3) {
    if (std::string(argv[1]) == "--emit-tokens") {
      cli_action = CliAction::EMIT_TOKENS;
    } else if (std::string(argv[1]) == "--emit-raw-ast") {
      cli_action = CliAction::EMIT_RAW_AST;
    } else if (std::string(argv[1]) == "--emit-optimized-ast") {
      cli_action = CliAction::EMIT_OPTIMIZED_AST;
    } else if (std::string(argv[1]) == "--eval") {
      cli_action = CliAction::EVAL;
    } else {
      // We didn't recognize the syntax.
      std::cout << parse_error;
      return 1;
    }
    input_path = argv[2];
  }

  // Read the source file.
  std::ifstream input_file(input_path);
  if (!input_file.is_open()) {
    std::cout << "Unable to open file '" + std::string(input_path) + "'.\n";
    return 1;
  }
  std::stringstream file_buffer;
  file_buffer << input_file.rdbuf();
  input_file.close();
  auto source = std::make_shared<std::string>(file_buffer.str());
  auto source_name = std::make_shared<std::string>(input_path);

  // Catch any Poi errors and report them.
  try {
    // Perform lexical analysis.
    auto tokens = poi::tokenize(source_name, source);
    if (cli_action == CliAction::EMIT_TOKENS) {
      for (auto &token : *tokens) {
        std::cout << token.show() << "\n";
      }
      return 0;
    }

    // Parse the tokens into an AST.
    auto term = poi::parse(*tokens);
    if (cli_action == CliAction::EMIT_RAW_AST) {
      if (term) {
        std::cout << term->show() << "\n";
      }
      return 0;
    }

    // Optimize the program.
    auto optimized_term = poi::optimize(term);
    if (cli_action == CliAction::EMIT_OPTIMIZED_AST) {
      if (optimized_term) {
        std::cout << optimized_term->show() << "\n";
      }
      return 0;
    }

    // Evaluate the program.
    std::unordered_map<size_t, std::shared_ptr<poi::Value>> environment;
    auto value = term->eval(term, environment);
    std::cout << value->show() << "\n";
  } catch(poi::Error &e) {
    std::cout << "Error: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
