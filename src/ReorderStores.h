#ifndef HALIDE_REORDER_STORES
#define HALIDE_REORDER_STORES

#include "IR.h"

/** \file Defines a pass that reorders stores to minimize stack
 * lifetime and simplify trivial store-load-store sequences.
 */

namespace Halide {
namespace Internal {

/** Reorder stores to minimize stack lifetime. */
Stmt reorder_stores(Stmt s);

}
}

#endif
