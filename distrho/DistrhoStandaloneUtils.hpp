/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2022 Filipe Coelho <falktx@falktx.com>
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

#ifndef DISTRHO_STANDALONE_UTILS_HPP_INCLUDED
#define DISTRHO_STANDALONE_UTILS_HPP_INCLUDED

#include "src/DistrhoDefines.h"

START_NAMESPACE_DISTRHO

/* ------------------------------------------------------------------------------------------------------------
 * Standalone plugin related utilities */

/**
   @defgroup PluginRelatedUtilities Plugin related utilities

   @{
 */

/**
   Check if the current standalone supports audio input.
*/
bool supportsAudioInput();

/**
   Check if the current standalone supports dynamic buffer size changes.
*/
bool supportsBufferSizeChanges();

/**
   Check if the current standalone supports MIDI.
*/
bool supportsMIDI();

/**
   Check if the current standalone has audio input enabled.
*/
bool isAudioInputEnabled();

/**
   Check if the current standalone has MIDI enabled.
*/
bool isMIDIEnabled();

/**
   Get the current buffer size.
*/
uint32_t getBufferSize();

/**
   Request permissions to use audio input.
   Only valid to call if audio input is supported but not currently enabled.
*/
bool requestAudioInput();

/**
   Request change to a new buffer size.
*/
bool requestBufferSizeChange(uint32_t newBufferSize);

/**
   Request permissions to use MIDI.
   Only valid to call if MIDI is supported but not currently enabled.
*/
bool requestMIDI();

/** @} */

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_STANDALONE_UTILS_HPP_INCLUDED
