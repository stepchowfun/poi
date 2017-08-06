/*
  This header declares the lexical tokens.
*/

#ifndef POI_TOKENS_H
#define POI_TOKENS_H

#include <poi/string_pool.h>
#include <string>

namespace poi {

  enum class TokenType {
    ARROW,
    DATA,
    DOT,
    EQUALS,
    IDENTIFIER,
    LEFT_PAREN,
    MATCH,
    RIGHT_PAREN,
    SEPARATOR
  };

  const char * const TokenTypeName[] = {
    "ARROW",
    "DATA",
    "DOT",
    "EQUALS",
    "IDENTIFIER",
    "LEFT_PAREN",
    "MATCH",
    "RIGHT_PAREN",
    "SEPARATOR"
  };

  class Token {
  public:
    poi::TokenType type;
    size_t literal;
    size_t source_name;
    size_t source;
    size_t start_pos; // Inclusive
    size_t end_pos; // Exclusive

    bool explicit_separator; // Only used for SEPARATOR tokens

    explicit Token(
      poi::TokenType type, size_t literal,
      size_t source_name, size_t source,
      size_t start_pos, size_t end_pos
    );
    std::string show(poi::StringPool &pool);
  };

}

#endif
