/*
  This header declares the interface to the parser.
*/

#ifndef POI_PARSER_H
#define POI_PARSER_H

#include <memory>
#include <poi/ast.h>
#include <poi/string_pool.h>
#include <poi/token.h>

namespace Poi {
  // Parse a stream of tokens.
  std::shared_ptr<const Poi::Term> parse(
    const Poi::TokenStream &token_stream,
    const Poi::StringPool &pool
  );
}

#endif
