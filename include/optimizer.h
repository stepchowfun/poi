/*
  This header declares the interface to the optimizer.
*/

#ifndef POI_OPTIMIZER_H
#define POI_OPTIMIZER_H

#include <poi/ast.h>

namespace poi {

  // Optimize an AST.
  std::shared_ptr<poi::Term> optimize(std::shared_ptr<poi::Term> term);

}

#endif
