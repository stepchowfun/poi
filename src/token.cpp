#include <poi/token.h>
#include <type_traits>

Poi::Token::Token(
  Poi::TokenType type,
  std::size_t literal,
  std::size_t source_name,
  std::size_t source,
  std::size_t start_pos,
  std::size_t end_pos,
  bool explicit_separator) :
  type(type),
  literal(literal),
  source_name(source_name),
  source(source),
  start_pos(start_pos),
  end_pos(end_pos),
  explicit_separator(explicit_separator) {
}

std::string Poi::Token::show(Poi::StringPool &pool) const {
  return std::string(
    TokenTypeName[
      static_cast<typename std::underlying_type<TokenType>::type>(type)
    ]
  ) + ": '" + pool.find(literal) + "'";
}

Poi::TokenStream::TokenStream(
  std::size_t source_name,
  std::size_t source,
  std::shared_ptr<const std::vector<Poi::Token>> tokens
) :
  source_name(source_name),
  source(source),
  tokens(tokens) {
}
