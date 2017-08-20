/*
  This header declares the lexical tokens.
*/

#ifndef POI_TOKENS_H
#define POI_TOKENS_H

#include <cstddef>
#include <memory>
#include <poi/string_pool.h>
#include <string>
#include <vector>

namespace Poi {
  enum class TokenType {
    ARROW,
    DOT,
    EQUALS,
    IDENTIFIER,
    LEFT_CURLY,
    LEFT_PAREN,
    MATCH,
    RIGHT_CURLY,
    RIGHT_PAREN,
    SEPARATOR
  };

  const char * const TokenTypeName[] = {
    "ARROW",
    "DOT",
    "EQUALS",
    "IDENTIFIER",
    "LEFT_CURLY",
    "LEFT_PAREN",
    "MATCH",
    "RIGHT_CURLY",
    "RIGHT_PAREN",
    "SEPARATOR"
  };

  class Token {
  public:
    const TokenType type;
    const std::size_t literal;
    const std::size_t source_name;
    const std::size_t source;
    const std::size_t start_pos; // Inclusive
    const std::size_t end_pos; // Exclusive

    const bool explicit_separator; // Only used for SEPARATOR tokens

    explicit Token(
      TokenType type,
      std::size_t literal,
      std::size_t source_name,
      std::size_t source,
      std::size_t start_pos,
      std::size_t end_pos,
      bool explicit_separator
    );
    std::string show(StringPool &pool) const;
  };

  class TokenStream {
  public:
    const std::size_t source_name;
    const std::size_t source;
    const std::shared_ptr<const std::vector<Token>> tokens;

    explicit TokenStream(
      std::size_t source_name,
      std::size_t source,
      std::shared_ptr<const std::vector<Token>> tokens
    );
  };
}

#endif
