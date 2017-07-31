/*
  This header declares the interface to the lexical analyzer.
*/

#ifndef POI_TOKENIZER_H
#define POI_TOKENIZER_H

#include <memory>
#include <poi/string_pool.h>
#include <poi/token.h>
#include <vector>

namespace poi {

  // Perform lexical analysis.
  // The tokenizer guarantees that all LEFT_*/RIGHT_* tokens will be matched
  // in the returned stream.
  std::unique_ptr<std::vector<poi::Token>> tokenize(
    size_t source_name,
    size_t source,
    poi::StringPool &pool
  );

}

#endif
