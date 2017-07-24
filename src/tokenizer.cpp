#include <poi/error.h>
#include <poi/tokenizer.h>
#include <unordered_map>

enum class LineContinuationStatus {
  LCS_DEFAULT,
  LCS_WAIT_FOR_NEWLINE,
  LCS_WAIT_FOR_TOKEN
};

std::shared_ptr<std::string> intern(
  std::unordered_map<std::string, std::shared_ptr<std::string>> &intern_pool,
  std::string &s
) {
  auto iter = intern_pool.find(s);
  if (iter == intern_pool.end()) {
    auto interned_s = std::make_shared<std::string>(s);
    intern_pool.insert({ s, interned_s });
    return interned_s;
  } else {
    return iter->second;
  }
}

std::unique_ptr<std::vector<poi::Token>> poi::tokenize(
  std::shared_ptr<std::string> source_name,
  std::shared_ptr<std::string> source
) {
  std::vector<Token> tokens;
  size_t pos = 0;
  std::vector<Token> grouping_stack;
  LineContinuationStatus line_continuation_status =
    LineContinuationStatus::LCS_DEFAULT;
  size_t line_continuation_marker_pos = 0;
  std::unordered_map<std::string, std::shared_ptr<std::string>> intern_pool;

  while (pos < source->size()) {
    // Comments begin with '#' and continue to the end of the line.
    if ((*source)[pos] == '#') {
      while (pos < source->size() && (*source)[pos] != '\n') {
        ++pos;
        continue;
      }
    }

    // Ignore whitespace except for line feeds;
    // it is only used to separate other tokens.
    if (
      (*source)[pos] == ' ' ||
      (*source)[pos] == '\t' ||
      (*source)[pos] == '\r'
    ) {
      ++pos;
      continue;
    }

    // Parse line continuation markers.
    if ((*source)[pos] == '\\') {
      if (line_continuation_status != LineContinuationStatus::LCS_DEFAULT) {
        throw Error(
          "Duplicate '\\'.",
          *source, *source_name,
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
    if ((*source)[pos] == '\n') {
      if (line_continuation_status == LineContinuationStatus::LCS_DEFAULT) {
        std::string literal("");
        tokens.push_back(Token(
          TokenType::SEPARATOR, intern(intern_pool, literal),
          source_name, source,
          pos, pos
        ));
        tokens.back().explicit_separator = false;
      }
      if (
        line_continuation_status ==
        LineContinuationStatus::LCS_WAIT_FOR_NEWLINE
      ) {
        line_continuation_status = LineContinuationStatus::LCS_WAIT_FOR_TOKEN;
      }

      line_continuation_marker_pos = source->size();
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
        *source, *source_name,
        line_continuation_marker_pos, line_continuation_marker_pos + 1
      );
    }

    if (
      line_continuation_status ==
      LineContinuationStatus::LCS_WAIT_FOR_TOKEN
    ) {
      line_continuation_status = LineContinuationStatus::LCS_DEFAULT;
    }

    // IDENTIFIER
    // Identifiers consist of ASCII letters, digits, and underscores, and must
    // not start with a letter. We also accept any bytes >= 0x80, which allows
    // for Unicode symbols.
    if ((*source)[pos] == '_' ||
      ((*source)[pos] >= 'A' && (*source)[pos] <= 'Z') ||
      ((*source)[pos] >= 'a' && (*source)[pos] <= 'z') ||
      ((*source)[pos] & 0x80) != 0) {
      size_t end_pos = pos + 1;
      while (end_pos < source->size() && ((*source)[end_pos] == '_' ||
        ((*source)[end_pos] >= 'A' && (*source)[end_pos] <= 'Z') ||
        ((*source)[end_pos] >= 'a' && (*source)[end_pos] <= 'z') ||
        ((*source)[end_pos] >= '0' && (*source)[end_pos] <= '9') ||
        ((*source)[end_pos] & 0x80) != 0)) {
        ++end_pos;
      }
      size_t length = end_pos - pos;
      auto literal = source->substr(pos, length);
      tokens.push_back(Token(
        TokenType::IDENTIFIER,
        intern(intern_pool, literal),
        source_name, source,
        pos, end_pos
      ));
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
        pos + literal.size() <= source->size() &&
        source->substr(pos, literal.size()) == literal
      ) {
        if (closer) {
          if (grouping_stack.empty()) {
            throw Error(
              "Unmatched '" + literal + "'.",
              *source, *source_name,
              pos, pos + 1
            );
          } else if (grouping_stack.back().type != opener_type) {
            throw Error(
              "Unmatched '" + *(grouping_stack.back().literal) + "'.",
              *source, *source_name,
              grouping_stack.back().start_pos, grouping_stack.back().end_pos
            );
          }
          grouping_stack.pop_back();
        }
        Token token(
          type, intern(intern_pool, literal),
          source_name, source,
          pos, pos + literal.size()
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
      tokens.back().explicit_separator = true;
      continue;
    }

    // If we made it this far, the input wasn't recognized and
    // should be rejected.
    throw Error(
      "Unexpected character '" + source->substr(pos, 1) + "'.",
      *source, *source_name,
      pos, pos + 1
    );
  }

  // Make sure all braces/brackets have been closed.
  if (!grouping_stack.empty()) {
    throw Error("Unmatched '" + *(grouping_stack.back().literal) + "'.",
      *source, *source_name,
      grouping_stack.back().start_pos, grouping_stack.back().end_pos
    );
  }

  // Filter out superfluous implicit SEPARATOR tokens.
  auto filtered_tokens = std::unique_ptr<std::vector<Token>>(
    new std::vector<Token>
  );
  for (auto iter = tokens.begin(); iter != tokens.end(); ++iter) {
    // If we have an implicit SEPARATOR token,
    // do some tests to see if we can skip it.
    if (iter->type == TokenType::SEPARATOR && !iter->explicit_separator) {
      // Skip redundant SEPARATOR tokens.
      if (
        (iter + 1) != tokens.end() &&
        (iter + 1)->type == TokenType::SEPARATOR
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
      if ((iter + 1) == tokens.end()) {
        continue;
      }

      // Don't add SEPARATOR tokens after a group opener.
      auto next_token = *(iter + 1);
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
  return filtered_tokens;
}
