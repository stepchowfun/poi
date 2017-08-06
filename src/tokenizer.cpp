#include <poi/error.h>
#include <poi/tokenizer.h>
#include <unordered_map>

enum class LineContinuationStatus {
  LCS_DEFAULT,
  LCS_WAIT_FOR_NEWLINE,
  LCS_WAIT_FOR_TOKEN
};

Poi::TokenStream Poi::tokenize(
  size_t source_name,
  size_t source,
  Poi::StringPool &pool
) {
  // For performance, get a local copy of the source name and content.
  std::string source_name_str = pool.find(source_name);
  std::string source_str = pool.find(source);

  std::vector<Token> tokens;
  size_t pos = 0;
  std::vector<Token> grouping_stack;
  LineContinuationStatus line_continuation_status =
    LineContinuationStatus::LCS_DEFAULT;
  size_t line_continuation_marker_pos = 0;

  while (pos < source_str.size()) {
    // Comments begin with '#' and continue to the end of the line.
    if (source_str[pos] == '#') {
      while (pos < source_str.size() && source_str[pos] != '\n') {
        ++pos;
      }
      continue;
    }

    // Ignore whitespace except for line feeds;
    // it is only used to separate other tokens.
    if (
      source_str[pos] == ' ' ||
      source_str[pos] == '\t' ||
      source_str[pos] == '\r'
    ) {
      ++pos;
      continue;
    }

    // Parse line continuation markers.
    if (source_str[pos] == '\\') {
      if (line_continuation_status != LineContinuationStatus::LCS_DEFAULT) {
        throw Error(
          "Duplicate '\\'.",
          source_name_str, source_str,
          pos, pos + 1
        );
      }
      line_continuation_status = LineContinuationStatus::LCS_WAIT_FOR_NEWLINE;
      line_continuation_marker_pos = pos;
      ++pos;
      continue;
    }

    // For line feeds, insert a SEPARATOR unless
    // there was a line continuation marker.
    if (source_str[pos] == '\n') {
      if (line_continuation_status == LineContinuationStatus::LCS_DEFAULT) {
        std::string literal("");
        tokens.push_back(Token(
          TokenType::SEPARATOR, pool.insert(literal),
          source_name, source,
          pos, pos,
          false
        ));
      }
      if (
        line_continuation_status ==
        LineContinuationStatus::LCS_WAIT_FOR_NEWLINE
      ) {
        line_continuation_status = LineContinuationStatus::LCS_WAIT_FOR_TOKEN;
      }

      line_continuation_marker_pos = source_str.size();
      ++pos;
      continue;
    }

    // If we parsed a line continuation marker, there
    // should have been a subsequent line feed.
    if (
      line_continuation_status ==
      LineContinuationStatus::LCS_WAIT_FOR_NEWLINE
    ) {
      throw Error(
        "Unexpected '\\'.",
        source_name_str, source_str,
        line_continuation_marker_pos, line_continuation_marker_pos + 1
      );
    }

    if (
      line_continuation_status ==
      LineContinuationStatus::LCS_WAIT_FOR_TOKEN
    ) {
      line_continuation_status = LineContinuationStatus::LCS_DEFAULT;
    }

    // IDENTIFIERs and keywords
    // Identifiers consist of ASCII letters, digits, and underscores, and must
    // not start with a letter. We also accept any bytes >= 0x80, which allows
    // for Unicode symbols.
    if (source_str[pos] == '_' ||
      (source_str[pos] >= 'A' && source_str[pos] <= 'Z') ||
      (source_str[pos] >= 'a' && source_str[pos] <= 'z') ||
      (source_str[pos] & 0x80) != 0) {
      size_t end_pos = pos + 1;
      while (end_pos < source_str.size() && (source_str[end_pos] == '_' ||
        (source_str[end_pos] >= 'A' && source_str[end_pos] <= 'Z') ||
        (source_str[end_pos] >= 'a' && source_str[end_pos] <= 'z') ||
        (source_str[end_pos] >= '0' && source_str[end_pos] <= '9') ||
        (source_str[end_pos] & 0x80) != 0)) {
        ++end_pos;
      }
      size_t length = end_pos - pos;
      auto literal = source_str.substr(pos, length);
      if (literal == "data") {
        tokens.push_back(Token(
          TokenType::DATA,
          pool.insert(literal),
          source_name, source,
          pos, end_pos,
          false
        ));
      } else if (literal == "match") {
        tokens.push_back(Token(
          TokenType::MATCH,
          pool.insert(literal),
          source_name, source,
          pos, end_pos,
          false
        ));
      } else {
        tokens.push_back(Token(
          TokenType::IDENTIFIER,
          pool.insert(literal),
          source_name, source,
          pos, end_pos,
          false
        ));
      }
      pos = end_pos;
      continue;
    }

    auto parse_symbol = [&](
      TokenType type,
      std::string literal,
      bool opener,
      bool closer,
      TokenType opener_type
    ) {
      if (
        pos + literal.size() <= source_str.size() &&
        source_str.substr(pos, literal.size()) == literal
      ) {
        if (closer) {
          if (grouping_stack.empty()) {
            throw Error(
              "Unmatched '" + literal + "'.",
              source_name_str, source_str,
              pos, pos + 1
            );
          } else if (grouping_stack.back().type != opener_type) {
            throw Error(
              "Unmatched '" + pool.find(grouping_stack.back().literal) + "'.",
              source_name_str, source_str,
              grouping_stack.back().start_pos, grouping_stack.back().end_pos
            );
          }
          grouping_stack.pop_back();
        }
        Token token(
          type, pool.insert(literal),
          source_name, source,
          pos, pos + literal.size(),
          literal == ","
        );
        if (opener) {
          grouping_stack.push_back(token);
        }
        tokens.push_back(token);
        pos += literal.size();
        return true;
      }
      return false;
    };

    // Parse two-character symbols first to prevent them from
    // being parsed as one-character symbols.

    // ARROW
    if (parse_symbol(
      TokenType::ARROW, "->", false, false, static_cast<TokenType>(0)
    )) {
      continue;
    }

    // Parse one-character symbols.

    // DOT
    if (parse_symbol(
      TokenType::DOT, ".", false, false, static_cast<TokenType>(0)
    )) {
      continue;
    }

    // EQUALS
    if (parse_symbol(
      TokenType::EQUALS, "=", false, false, static_cast<TokenType>(0)
    )) {
      continue;
    }

    // LEFT_PAREN
    if (parse_symbol(
      TokenType::LEFT_PAREN, "(", true, false, static_cast<TokenType>(0)
    )) {
      continue;
    }

    // RIGHT_PAREN
    if (parse_symbol(
      TokenType::RIGHT_PAREN, ")", false, true, TokenType::LEFT_PAREN
    )) {
      continue;
    }

    // SEPARATOR
    if (parse_symbol(
      TokenType::SEPARATOR, ",", false, false, static_cast<TokenType>(0)
    )) {
      continue;
    }

    // If we made it this far, the input wasn't recognized and
    // should be rejected.
    throw Error(
      "Unexpected character '" + source_str.substr(pos, 1) + "'.",
      source_name_str, source_str,
      pos, pos + 1
    );
  }

  // Make sure all braces/brackets have been closed.
  if (!grouping_stack.empty()) {
    throw Error(
      "Unmatched '" + pool.find(grouping_stack.back().literal) + "'.",
      source_name_str, source_str,
      grouping_stack.back().start_pos, grouping_stack.back().end_pos
    );
  }

  // Filter out superfluous implicit SEPARATOR tokens.
  auto filtered_tokens = std::make_shared<std::vector<Token>>();
  for (auto iter = tokens.begin(); iter != tokens.end(); ++iter) {
    // If we have an implicit SEPARATOR token,
    // do some tests to see if we can skip it.
    if (iter->type == TokenType::SEPARATOR && !iter->explicit_separator) {
      // Skip redundant SEPARATOR tokens.
      if (
        std::next(iter) != tokens.end() &&
        std::next(iter)->type == TokenType::SEPARATOR
      ) {
        continue;
      }
      if (
        !filtered_tokens->empty() &&
        filtered_tokens->back().type == TokenType::SEPARATOR
      ) {
        continue;
      }

      // Don't add SEPARATOR tokens at the beginning of the file.
      if (filtered_tokens->empty()) {
        continue;
      }

      // Don't add SEPARATOR tokens at the end of the file.
      if (std::next(iter) == tokens.end()) {
        continue;
      }

      // Don't add SEPARATOR tokens after a group opener.
      auto next_token = *std::next(iter);
      if (next_token.type == TokenType::RIGHT_PAREN) {
        continue;
      }

      // Don't add SEPARATOR tokens before a group closer.
      auto prev_token = filtered_tokens->back();
      if (prev_token.type == TokenType::LEFT_PAREN) {
        continue;
      }
    }

    // Add the token to the filtered vector.
    filtered_tokens->push_back(*iter);
  }

  // Return an std::unique_ptr to the vector.
  return TokenStream(source_name, source, filtered_tokens);
}
