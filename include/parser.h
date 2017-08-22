/*
  This header declares the interface to the parser.
*/

#ifndef POI_PARSER_H
#define POI_PARSER_H

#include <cstddef>
#include <memory>
#include <poi/ast.h>
#include <poi/string_pool.h>
#include <poi/token.h>

namespace Poi {
  // Parse a stream of tokens into an AST.
  std::shared_ptr<const Term> parse(
    const TokenStream &token_stream,
    const StringPool &pool
  );
}

#endif
