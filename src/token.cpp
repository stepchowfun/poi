#include <poi/token.h>

poi::Token::Token(
  poi::TokenType type,
  std::shared_ptr<std::string> literal,
  std::shared_ptr<std::string> source_name,
  std::shared_ptr<std::string> source,
  size_t start_pos, size_t end_pos) :
  type(type), literal(literal),
  source_name(source_name), source(source),
  start_pos(start_pos), end_pos(end_pos) {
}

std::string poi::Token::show() {
  return std::string(
    TokenTypeName[
      static_cast<typename std::underlying_type<TokenType>::type>(type)
    ]
  ) + ": '" + *literal + "'";
}
