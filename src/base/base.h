// base.h - the base layer's only entry point. Include this, never an
// individual base_*.h.
//
// The base_*.h files depend on each other. Strings allocate from arenas, arenas
// call the OS layer, and the OS layer takes string paths. Each file includes
// what its own implementation needs. That only resolves when base_types.h and
// base_string.h are seen first, which the order below guarantees. Including
// base_os.h on its own does not compile.
//
// Exactly one file in the program defines BASE_IMPLEMENTATION before including
// this header, and that file is src/unity_build.c. Everyone else gets
// declarations only.
//
// See STYLE.md for conventions.

#ifndef BASE_H
#define BASE_H

#include "base_types.h"
#include "base_string.h"
#include "base_arena.h"
#include "base_os.h"
#include "base_array.h"
#include "base_log.h"

#endif // BASE_H
