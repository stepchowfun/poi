/*
  This header declares the interface to the compiler.
*/

#ifndef POI_COMPILER_H
#define POI_COMPILER_H

#include <memory>
#include <poi/ast.h>
#include <poi/bytecode.h>
#include <vector>

namespace Poi {
  std::size_t compile(
    const Poi::Term &term,
    std::vector<Poi::Bytecode> &program
  );
}

#endif
