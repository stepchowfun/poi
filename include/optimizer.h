/*
  This header declares the interface to the optimizer.
*/

#ifndef POI_OPTIMIZER_H
#define POI_OPTIMIZER_H

#include <memory>
#include <poi/ir.h>

namespace Poi {
  // Optimize a block of IR.
  std::shared_ptr<IrBlock> optimize(std::shared_ptr<IrBlock> block);
}

#endif
