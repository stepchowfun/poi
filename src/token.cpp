#include <poi/token.h>
#include <type_traits>

poi::Token::Token(
  poi::TokenType type, size_t literal,
  size_t source_name, size_t source,
  size_t start_pos, size_t end_pos) :
  type(type), literal(literal),
  source_name(source_name), source(source),
  start_pos(start_pos), end_pos(end_pos) {
}

std::string poi::Token::show(poi::StringPool &pool) {
  return std::string(
    TokenTypeName[
      static_cast<typename std::underlying_type<TokenType>::type>(type)
    ]
  ) + ": '" + pool.find(literal) + "'";
}
