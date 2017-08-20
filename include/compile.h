/*
  This header declares the interface to the interpreter.
*/

#ifndef POI_COMPILER_H
#define POI_COMPILER_H

#include <memory>
#include <poi/ast.h>
#include <poi/instruction.h>
#include <vector>

namespace Poi {
  size_t compile(
    std::shared_ptr<const Poi::Term> term,
    std::vector<Poi::Instruction> &program
  );
}

#endif
