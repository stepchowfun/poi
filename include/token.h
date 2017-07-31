/*
  This header declares the lexical tokens.
*/

#ifndef POI_TOKENS_H
#define POI_TOKENS_H

#include <memory>
#include <string>

namespace poi {

  enum class TokenType {
    ARROW,
    DATA,
    DOT,
    EQUALS,
    IDENTIFIER,
    LEFT_PAREN,
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
    "RIGHT_PAREN",
    "SEPARATOR"
  };

  class Token {
  public:
    poi::TokenType type;
    std::shared_ptr<std::string> literal;
    std::shared_ptr<std::string> source_name;
    std::shared_ptr<std::string> source;
    size_t start_pos; // Inclusive
    size_t end_pos; // Exclusive

    bool explicit_separator; // Only used for SEPARATOR tokens

    explicit Token(
      poi::TokenType type,
      std::shared_ptr<std::string> literal,
      std::shared_ptr<std::string> source_name,
      std::shared_ptr<std::string> source,
      size_t start_pos, size_t end_pos
    );
    std::string show();
  };

}

#endif
