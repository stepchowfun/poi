#include <cstddef>
#include <fstream>
#include <iostream>
#include <poi/ast.h>
#include <poi/bytecode.h>
#include <poi/compiler.h>
#include <poi/error.h>
#include <poi/interpreter.h>
#include <poi/ir.h>
#include <poi/parser.h>
#include <poi/string_pool.h>
#include <poi/token.h>
#include <poi/tokenizer.h>
#include <poi/value.h>
#include <poi/version.h>
#include <sstream>

// These are the actions we can perform on a source file.
enum class CliAction {
  EMIT_TOKENS,
  EMIT_AST,
  EMIT_IR,
  EMIT_BC,
  RUN
};

// The logic for the command line interface is here.
// Run `poi --help` for instructions.
int main(int argc, char * argv[]) {
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
      "  poi --emit-ir source\n"
      "  poi --emit-bc source\n"
      "  poi --run source\n";
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

  // Determine what the user wants us to do with the given source file.
  if (argc != 2 && argc != 3) {
    std::cerr << parse_error;
    return 1;
  }
  std::string input_path = argv[1];
  auto cli_action = CliAction::RUN;
  if (argc == 3) {
    if (std::string(argv[1]) == "--emit-tokens") {
      cli_action = CliAction::EMIT_TOKENS;
    } else if (std::string(argv[1]) == "--emit-ast") {
      cli_action = CliAction::EMIT_AST;
    } else if (std::string(argv[1]) == "--emit-ir") {
      cli_action = CliAction::EMIT_IR;
    } else if (std::string(argv[1]) == "--emit-bc") {
      cli_action = CliAction::EMIT_BC;
    } else if (std::string(argv[1]) == "--run") {
      cli_action = CliAction::RUN;
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
    // Perform lexical analysis to convert the source file into tokens.
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

    // Compile the AST into IR.
    auto block = Poi::compile_ast_to_ir(term);
    if (cli_action == CliAction::EMIT_IR) {
      std::cout << block->show();
      return 0;
    }

    // Compile the IR into BC.
    auto bytecode = compile_ir_to_bc(*block);
    if (cli_action == CliAction::EMIT_BC) {
      for (std::size_t i = 0; i < bytecode->size(); ++i) {
        std::cout << i << " " << bytecode->at(i).show() << "\n";
      }
      return 0;
    }

    // Run the program by interpreting the BC.
    auto result = Poi::interpret(&bytecode->at(0), block->frame_size());
    std::cout << result->show(0) << "\n";
  } catch(Poi::Error &e) {
    // There was an error. Print it and exit.
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
