#include <poi/error.h>

namespace Poi {
  void get_location_info(
    const std::string &source_name,
    const std::string &source,
    std::size_t start_pos, // Inclusive
    std::size_t end_pos, // Exclusive
    std::size_t &start_line,
    std::size_t &start_col,
    std::size_t &end_line,
    std::size_t &end_col,
    std::size_t &context_start_pos,
    std::size_t &context_end_pos
  ) {
    start_line = 0;
    start_col = 0;
    end_line = 0;
    end_col = 0;
    context_start_pos = 0;
    context_end_pos = source.size();
    std::size_t line_number = 0;
    std::size_t col_number = 0;
    bool found_start = false;
    bool found_end = false;
    for (std::size_t pos = 0; pos <= source.size(); ++pos) {
      if (pos == start_pos) {
        start_line = line_number;
        start_col = col_number;
        found_start = true;
      }
      if (pos == end_pos) {
        end_line = line_number;
        end_col = col_number;
        found_end = true;
      }
      if (pos == source.size() || source[pos] == '\n') {
        ++line_number;
        col_number = 0;
        if (!found_start) {
          context_start_pos = pos + 1;
        }
        if (found_end) {
          context_end_pos = pos;
          break;
        }
      } else {
        ++col_number;
      }
    }
  }
}

std::string Poi::get_location(
  const std::string &source_name,
  const std::string &source,
  std::size_t start_pos,
  std::size_t end_pos
) {
  std::size_t start_line = 0;
  std::size_t start_col = 0;
  std::size_t end_line = 0;
  std::size_t end_col = 0;
  std::size_t context_start_pos = 0;
  std::size_t context_end_pos = 0;
  get_location_info(
    source_name,
    source,
    start_pos,
    end_pos,
    start_line,
    start_col,
    end_line,
    end_col,
    context_start_pos,
    context_end_pos
  );
  if (end_pos == start_pos || end_pos == start_pos + 1) {
    return source_name +
      " @ " + std::to_string(start_line + 1) +
      ":" + std::to_string(start_col + 1);
  } else {
    return source_name +
      " @ " + std::to_string(start_line + 1) +
      ":" + std::to_string(start_col + 1) +
      " - " + std::to_string(end_line + 1) +
      ":" + std::to_string(end_col);
  }
}

Poi::Error::Error() {
}

Poi::Error::Error(const std::string &message) : message(message) {
}

Poi::Error::Error(
  const std::string &message,
  const std::string &source_name,
  const std::string &source
) {
  // Output the source name and message.
  this->message = message + "\nLocation: " + source_name;
}

Poi::Error::Error(
  const std::string &message,
  const std::string &source_name,
  const std::string &source,
  std::size_t start_pos,
  std::size_t end_pos
) {
  // Report the location of the error.
  std::size_t start_line = 0;
  std::size_t start_col = 0;
  std::size_t end_line = 0;
  std::size_t end_col = 0;
  std::size_t context_start_pos = 0;
  std::size_t context_end_pos = 0;
  get_location_info(
    source_name,
    source,
    start_pos,
    end_pos,
    start_line,
    start_col,
    end_line,
    end_col,
    context_start_pos,
    context_end_pos
  );
  this->message = message + "\nLocation: " + get_location(
    source_name,
    source,
    start_pos,
    end_pos
  );

  // Extract the context from the source.
  auto context = source.substr(
    context_start_pos,
    context_end_pos - context_start_pos
  );

  // Check if the context has only whitespace.
  auto only_whitespace = true;
  for (auto c : context) {
    if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
      only_whitespace = false;
      break;
    }
  }

  // If the context has nothing but whitespace, don't bother printing it.
  if (!only_whitespace) {
    // Output the context.
    this->message += "\n\n" + context + "\n";

    // If there was only one line, highlight the offending section with carets.
    if (end_line == start_line) {
      // Before printing the carets, indent until we are at the right column.
      for (std::size_t i = 0; i < start_col; ++i) {
        if (source[start_pos + i] == '\t') {
          // Assume a tab takes up 8 spaces in whatever program this error
          // message is being displayed in. Not ideal, but what else can we do?
          this->message += "        ";
        } else {
          this->message += " ";
        }
      }

      // Add the carets to highlight the problematic code.
      for (std::size_t j = 0; j < end_col - start_col; ++j) {
        if (source[start_pos + start_col + j] == '\t') {
          // Assume a tab takes up 8 carets in whatever program this error
          // message is being displayed in. Not ideal, but what else can we do?
          this->message += "^^^^^^^^";
        } else {
          this->message += "^";
        }
      }

      // We have a line feed before the context, so we add one after for
      // symmetry.
      this->message += "\n";
    }
  }
}

Poi::Error::~Error() {
}

std::string Poi::Error::what() const {
  return message;
}
