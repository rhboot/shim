
#pragma once

//
// Status codes common to all execution phases
//
typedef UINTN RETURN_STATUS;

//
// The Microsoft* C compiler can removed references to unreferenced data items
//  if the /OPT:REF linker option is used. We defined a macro as this is a
//  a non standard extension
//
#if defined (_MSC_VER) && _MSC_VER < 1800 && !defined (MDE_CPU_EBC)
///
/// Remove global variable from the linked image if there are no references to
/// it after all compiler and linker optimizations have been performed.
///
///
#define GLOBAL_REMOVE_IF_UNREFERENCED  __declspec(selectany)
#else
///
/// Remove the global variable from the linked image if there are no references
///  to it after all compiler and linker optimizations have been performed.
///
///
#define GLOBAL_REMOVE_IF_UNREFERENCED
#endif

/**
  Returns a 16-bit signature built from 2 ASCII characters.

  This macro returns a 16-bit value built from the two ASCII characters specified
  by A and B.

  @param  A    The first ASCII character.
  @param  B    The second ASCII character.

  @return A 16-bit value built from the two ASCII characters specified by A and B.

**/
#ifdef SIGNATURE_16
#undef SIGNATURE_16
#endif
#define SIGNATURE_16(A, B)  ((A) | (B << 8))

/**
  Returns a 32-bit signature built from 4 ASCII characters.

  This macro returns a 32-bit value built from the four ASCII characters specified
  by A, B, C, and D.

  @param  A    The first ASCII character.
  @param  B    The second ASCII character.
  @param  C    The third ASCII character.
  @param  D    The fourth ASCII character.

  @return A 32-bit value built from the two ASCII characters specified by A, B,
          C and D.

**/
#ifdef SIGNATURE_32
#undef SIGNATURE_32
#endif
#define SIGNATURE_32(A, B, C, D)  (SIGNATURE_16 (A, B) | (SIGNATURE_16 (C, D) << 16))

#define MIN(a, b) ({(a) < (b) ? (a) : (b);})
