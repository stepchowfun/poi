/*
  This header declares the lexical tokens.
*/

#ifndef POI_TOKENS_H
#define POI_TOKENS_H

#include <memory>
#include <poi/string_pool.h>
#include <string>
#include <vector>

namespace Poi {

  enum class TokenType {
    ARROW,
    DATA,
    DOT,
    EQUALS,
    IDENTIFIER,
    LEFT_CURLY,
    LEFT_PAREN,
    RIGHT_CURLY,
    RIGHT_PAREN,
    SEPARATOR
  };

  const char * const TokenTypeName[] = {
    "ARROW",
    "DATA",
    "DOT",
    "EQUALS",
    "IDENTIFIER",
    "LEFT_CURLY",
    "LEFT_PAREN",
    "RIGHT_CURLY",
    "RIGHT_PAREN",
    "SEPARATOR"
  };

  class Token {
  public:
    const Poi::TokenType type;
    const size_t literal;
    const size_t source_name;
    const size_t source;
    const size_t start_pos; // Inclusive
    const size_t end_pos; // Exclusive

    const bool explicit_separator; // Only used for SEPARATOR tokens

    explicit Token(
      Poi::TokenType type, size_t literal,
      size_t source_name, size_t source,
      size_t start_pos, size_t end_pos,
      bool explicit_separator
    );
    std::string show(Poi::StringPool &pool) const;
  };

  class TokenStream {
  public:
    const size_t source_name;
    const size_t source;
    const std::shared_ptr<std::vector<Poi::Token>> tokens;

    explicit TokenStream(
      size_t source_name,
      size_t source,
      std::shared_ptr<std::vector<Poi::Token>> tokens
    );
  };

}

#endif
