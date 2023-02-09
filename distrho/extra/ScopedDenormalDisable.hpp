/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2023 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef DISTRHO_SCOPED_DENORMAL_DISABLE_HPP_INCLUDED
#define DISTRHO_SCOPED_DENORMAL_DISABLE_HPP_INCLUDED

#include "../DistrhoUtils.hpp"

#ifdef __SSE2_MATH__
# include <xmmintrin.h>
#endif

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------
// ScopedDenormalDisable class definition

/**
   ScopedDenormalDisable is a handy class for disabling denormal numbers during a function scope.
   Denormal numbers can happen in IIR or other types of filters, they are often very slow.

   Use this class with care! Messing up with the global state is bound to make some hosts unhappy.
 */
class ScopedDenormalDisable {
public:
    /*
     * Constructor.
     * Current cpu flags will saved, then denormals-as-zero and flush-to-zero set on top.
     */
    inline ScopedDenormalDisable() noexcept;

    /*
     * Destructor.
     * CPU flags will be restored to the value obtained in the constructor.
     */
    inline ~ScopedDenormalDisable() noexcept
    {
        setFlags(oldflags);
    }

private:
   #if defined(__SSE2_MATH__)
    typedef uint cpuflags_t;
   #elif defined(__aarch64__)
    typedef uint64_t cpuflags_t;
   #elif defined(__arm__) && !defined(__SOFTFP__)
    typedef uint32_t cpuflags_t;
   #else
    typedef char cpuflags_t;
   #endif

    // retrieved on constructor, reset to it on destructor
    cpuflags_t oldflags;

    // helper function to set cpu flags
    inline void setFlags(cpuflags_t flags) noexcept;

    DISTRHO_DECLARE_NON_COPYABLE(ScopedDenormalDisable)
    DISTRHO_PREVENT_HEAP_ALLOCATION
};

// --------------------------------------------------------------------------------------------------------------------
// ScopedDenormalDisable class implementation

inline ScopedDenormalDisable::ScopedDenormalDisable() noexcept
    : oldflags(0)
{
   #if defined(__SSE2_MATH__)
    oldflags = _mm_getcsr();
    setFlags(oldflags | 0x8040);
   #elif defined(__aarch64__)
    __asm__ __volatile__("mrs %0, fpcr" : "=r" (oldflags));
    setFlags(oldflags | 0x1000000);
    __asm__ __volatile__("isb");
   #elif defined(__arm__) && !defined(__SOFTFP__)
    __asm__ __volatile__("vmrs %0, fpscr" : "=r" (oldflags));
    setFlags(oldflags | 0x1000000);
   #endif
}

inline void ScopedDenormalDisable::setFlags(const cpuflags_t flags) noexcept
{
   #if defined(__SSE2_MATH__)
    _mm_setcsr(flags);
   #elif defined(__aarch64__)
    __asm__ __volatile__("msr fpcr, %0" :: "r" (flags));
   #elif defined(__arm__) && !defined(__SOFTFP__)
    __asm__ __volatile__("vmsr fpscr, %0" :: "r" (flags));
   #else
    // unused
    (void)flags;
   #endif
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_SCOPED_DENORMAL_DISABLE_HPP_INCLUDED
