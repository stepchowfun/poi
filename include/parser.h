/*
  This header declares the interface to the parser.
*/

#ifndef POI_PARSER_H
#define POI_PARSER_H

#include <memory>
#include <poi/ast.h>
#include <poi/token.h>
#include <vector>

namespace poi {

  // Parse a stream of tokens.
  std::shared_ptr<poi::Term> parse(
    std::vector<poi::Token> &tokens,
    poi::StringPool &pool
  );

}

#endif
