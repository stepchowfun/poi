/*
  This header declares the interface to the lexical analyzer.
*/

#ifndef POI_TOKENIZER_H
#define POI_TOKENIZER_H

#include <cstddef>
#include <poi/string_pool.h>
#include <poi/token.h>

namespace Poi {
  // Perform lexical analysis. The tokenizer guarantees that all
  // LEFT_*/RIGHT_* tokens will be matched in the returned stream.
  TokenStream tokenize(
    std::size_t source_name,
    std::size_t source,
    StringPool &pool
  );
}

#endif
