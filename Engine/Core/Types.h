#pragma once

// ============================================================================
// Evo::Platform-Independent Fixed-Width Types
// ============================================================================
//
// Provides guaranteed-size integer and floating-point type aliases with
// compile-time size verification.  Definitions are selected per-compiler
// using native intrinsic widths rather than relying on <cstdint> alone.
//
// Naming convention (Unreal-style, widely adopted in engine codebases):
//   uint8  uint16  uint32  uint64   — unsigned integers
//   int8   int16   int32   int64    — signed integers
//   float32  float64                — IEEE-754 floating point

#include <cstddef>   // size_t, ptrdiff_t

namespace Evo {

// ============================================================================
// Compiler-specific type definitions
// ============================================================================

#if defined(_MSC_VER)
	// ---- Microsoft Visual C++ ----
	typedef signed   __int8     int8;
	typedef signed   __int16    int16;
	typedef signed   __int32    int32;
	typedef signed   __int64    int64;
	typedef unsigned __int8     uint8;
	typedef unsigned __int16    uint16;
	typedef unsigned __int32    uint32;
	typedef unsigned __int64    uint64;

#elif defined(__clang__)
	// ---- Clang (check before GCC — clang also defines __GNUC__) ----
	typedef signed   char       int8;
	typedef signed   short      int16;
	typedef signed   int        int32;
	typedef signed   long long  int64;
	typedef unsigned char       uint8;
	typedef unsigned short      uint16;
	typedef unsigned int        uint32;
	typedef unsigned long long  uint64;

#elif defined(__GNUC__)
	// ---- GCC ----
	typedef signed   char       int8;
	typedef signed   short      int16;
	typedef signed   int        int32;
	typedef signed   long long  int64;
	typedef unsigned char       uint8;
	typedef unsigned short      uint16;
	typedef unsigned int        uint32;
	typedef unsigned long long  uint64;

#else
	#error "Unsupported compiler. Evo requires MSVC, GCC, or Clang."
#endif

// ---- Floating point (IEEE-754, same on all supported compilers) ----
typedef float   float32;
typedef double  float64;

// ============================================================================
// Compile-time size guarantees
// ============================================================================

static_assert(sizeof(int8)    == 1, "int8 must be exactly 1 byte");
static_assert(sizeof(int16)   == 2, "int16 must be exactly 2 bytes");
static_assert(sizeof(int32)   == 4, "int32 must be exactly 4 bytes");
static_assert(sizeof(int64)   == 8, "int64 must be exactly 8 bytes");
static_assert(sizeof(uint8)   == 1, "uint8 must be exactly 1 byte");
static_assert(sizeof(uint16)  == 2, "uint16 must be exactly 2 bytes");
static_assert(sizeof(uint32)  == 4, "uint32 must be exactly 4 bytes");
static_assert(sizeof(uint64)  == 8, "uint64 must be exactly 8 bytes");
static_assert(sizeof(float32) == 4, "float32 must be exactly 4 bytes (IEEE-754 single)");
static_assert(sizeof(float64) == 8, "float64 must be exactly 8 bytes (IEEE-754 double)");

} // namespace Evo

