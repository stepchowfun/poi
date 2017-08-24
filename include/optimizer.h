/*
  This header declares the interface to the optimizer.
*/

#ifndef POI_OPTIMIZER_H
#define POI_OPTIMIZER_H

#include <memory>
#include <poi/ir.h>

namespace Poi {
  // Optimize a block of IR.
  const std::shared_ptr<const IrBlock> optimize(
    const std::shared_ptr<const IrBlock> block
  );
}

#endif
