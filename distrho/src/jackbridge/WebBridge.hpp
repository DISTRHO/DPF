/*
 * Web Audio + MIDI Bridge for DPF
 * Copyright (C) 2021-2022 Filipe Coelho <falktx@falktx.com>
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

#ifndef WEB_BRIDGE_HPP_INCLUDED
#define WEB_BRIDGE_HPP_INCLUDED

#include "NativeBridge.hpp"

#include <emscripten/emscripten.h>

struct WebBridge : NativeBridge {
#if DISTRHO_PLUGIN_NUM_INPUTS > 0
    bool captureAvailable = false;
#endif
#if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
    bool playbackAvailable = false;
#endif
    bool active = false;
    double timestamp = 0;

    WebBridge()
    {
       #if DISTRHO_PLUGIN_NUM_INPUTS > 0
        captureAvailable = EM_ASM_INT({
            if (typeof(navigator.mediaDevices) !== 'undefined' && typeof(navigator.mediaDevices.getUserMedia) !== 'undefined')
                return 1;
            if (typeof(navigator.webkitGetUserMedia) !== 'undefined')
                return 1;
            return false;
        }) != 0;
       #endif

       #if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        playbackAvailable = EM_ASM_INT({
            if (typeof(AudioContext) !== 'undefined')
                return 1;
            if (typeof(webkitAudioContext) !== 'undefined')
                return 1;
            return 0;
        }) != 0;
       #endif

       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT || DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
        midiAvailable = EM_ASM_INT({
            return typeof(navigator.requestMIDIAccess) === 'function' ? 1 : 0;
        }) != 0;
       #endif
    }

    bool open(const char*) override
    {
        // early bail out if required features are not supported
       #if DISTRHO_PLUGIN_NUM_INPUTS > 0
        if (!captureAvailable)
        {
           #if DISTRHO_PLUGIN_NUM_OUTPUTS == 0
            d_stderr2("Audio capture is not supported");
            return false;
           #else
            if (!playbackAvailable)
            {
                d_stderr2("Audio capture and playback are not supported");
                return false;
            }
            d_stderr2("Audio capture is not supported, but can still use playback");
           #endif
        }
       #endif

       #if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        if (!playbackAvailable)
        {
            d_stderr2("Audio playback is not supported");
            return false;
        }
       #endif

        const bool initialized = EM_ASM_INT({
            if (typeof(Module['WebAudioBridge']) === 'undefined') {
                Module['WebAudioBridge'] = {};
            }

            var WAB = Module['WebAudioBridge'];
            if (!WAB.audioContext) {
                if (typeof(AudioContext) !== 'undefined') {
                    WAB.audioContext = new AudioContext();
                } else if (typeof(webkitAudioContext) !== 'undefined') {
                    WAB.audioContext = new webkitAudioContext();
                }
            }

            return WAB.audioContext === undefined ? 0 : 1;
        }) != 0;
        
        if (!initialized)
        {
            d_stderr2("Failed to initialize web audio");
            return false;
        }

        bufferSize = EM_ASM_INT({
            var WAB = Module['WebAudioBridge'];
            return WAB['minimizeBufferSize'] ? 256 : 2048;
        });
        sampleRate = EM_ASM_INT_V({
            var WAB = Module['WebAudioBridge'];
            return WAB.audioContext.sampleRate;
        });

        allocBuffers(true, true);

        EM_ASM({
            var numInputs = $0;
            var numOutputs = $1;
            var bufferSize = $2;
            var WAB = Module['WebAudioBridge'];

            var realBufferSize = WAB['minimizeBufferSize'] ? 2048 : bufferSize;
            var divider = realBufferSize / bufferSize;

            // main processor
            WAB.processor = WAB.audioContext['createScriptProcessor'](realBufferSize, numInputs, numOutputs);
            WAB.processor['onaudioprocess'] = function (e) {
                // var timestamp = performance.now();
                for (var k = 0; k < divider; ++k) {
                    for (var i = 0; i < numInputs; ++i) {
                        var buffer = e['inputBuffer']['getChannelData'](i);
                        for (var j = 0; j < bufferSize; ++j) {
                            // setValue($3 + ((bufferSize * i) + j) * 4, buffer[j], 'float');
                            HEAPF32[$3 + (((bufferSize * i) + j) << 2) >> 2] = buffer[bufferSize * k + j];
                        }
                    }
                    dynCall('vi', $4, [$5]);
                    for (var i = 0; i < numOutputs; ++i) {
                        var buffer = e['outputBuffer']['getChannelData'](i);
                        var offset = bufferSize * (numInputs + i);
                        for (var j = 0; j < bufferSize; ++j) {
                            buffer[bufferSize * k + j] = HEAPF32[$3 + ((offset + j) << 2) >> 2];
                        }
                    }
                }
            };

            // connect to output
            WAB.processor['connect'](WAB.audioContext['destination']);

            // resume/start playback on first click
            document.addEventListener('click', function(e) {
                var WAB = Module['WebAudioBridge'];
                if (WAB.audioContext.state === 'suspended')
                    WAB.audioContext.resume();
            });
        }, DISTRHO_PLUGIN_NUM_INPUTS, DISTRHO_PLUGIN_NUM_OUTPUTS, bufferSize, audioBufferStorage, WebAudioCallback, this);

        return true;
    }

    bool close() override
    {
        freeBuffers();
        return true;
    }

    bool activate() override
    {
        active = true;
        return true;
    }

    bool deactivate() override
    {
        active = false;
        return true;
    }

    bool supportsAudioInput() const override
    {
       #if DISTRHO_PLUGIN_NUM_INPUTS > 0
        return captureAvailable;
       #else
        return false;
       #endif
    }

    bool isAudioInputEnabled() const override
    {
       #if DISTRHO_PLUGIN_NUM_INPUTS > 0
        return EM_ASM_INT({ return Module['WebAudioBridge'].captureStreamNode ? 1 : 0 }) != 0;
       #else
        return false;
       #endif
    }

    bool requestAudioInput() override
    {
        DISTRHO_SAFE_ASSERT_RETURN(DISTRHO_PLUGIN_NUM_INPUTS > 0, false);

        EM_ASM({
            var numInputs = $0;
            var WAB = Module['WebAudioBridge'];

            var constraints = {};
            // we need to use this weird awkward way for objects, otherwise build fails
            constraints['audio'] = true;
            constraints['video'] = false;
            constraints['autoGainControl'] = {};
            constraints['autoGainControl']['exact'] = false;
            constraints['echoCancellation'] = {};
            constraints['echoCancellation']['exact'] = false;
            constraints['noiseSuppression'] = {};
            constraints['noiseSuppression']['exact'] = false;
            constraints['channelCount'] = {};
            constraints['channelCount']['min'] = 0;
            constraints['channelCount']['ideal'] = numInputs;
            constraints['latency'] = {};
            constraints['latency']['min'] = 0;
            constraints['latency']['ideal'] = 0;
            constraints['sampleSize'] = {};
            constraints['sampleSize']['min'] = 8;
            constraints['sampleSize']['max'] = 32;
            constraints['sampleSize']['ideal'] = 16;
            // old property for chrome
            constraints['googAutoGainControl'] = false;

            var success = function(stream) {
                WAB.captureStreamNode = WAB.audioContext['createMediaStreamSource'](stream);
                WAB.captureStreamNode.connect(WAB.processor);
            };
            var fail = function() {
            };

            if (navigator.mediaDevices !== undefined && navigator.mediaDevices.getUserMedia !== undefined) {
                navigator.mediaDevices.getUserMedia(constraints).then(success).catch(fail);
            } else if (navigator.webkitGetUserMedia !== undefined) {
                navigator.webkitGetUserMedia(constraints, success, fail);
            }
        }, DISTRHO_PLUGIN_NUM_INPUTS);

        return true;
    }

    bool supportsBufferSizeChanges() const override
    {
        return true;
    }

    bool requestBufferSizeChange(const uint32_t newBufferSize) override
    {
        // try to create new processor first
        bool success = EM_ASM_INT({
            var numInputs = $0;
            var numOutputs = $1;
            var newBufferSize = $2;
            var WAB = Module['WebAudioBridge'];

            try {
                WAB.newProcessor = WAB.audioContext['createScriptProcessor'](newBufferSize, numInputs, numOutputs);
            } catch (e) {
                return 0;
            }

            // got new processor, disconnect old one
            WAB.processor['disconnect'](WAB.audioContext['destination']);

            if (WAB.captureStreamNode)
                WAB.captureStreamNode.disconnect(WAB.processor);

            return 1;
        }, DISTRHO_PLUGIN_NUM_INPUTS, DISTRHO_PLUGIN_NUM_OUTPUTS, newBufferSize) != 0;

        if (!success)
            return false;

        bufferSize = newBufferSize;
        freeBuffers();
        allocBuffers(true, true);

        if (bufferSizeCallback != nullptr)
            bufferSizeCallback(newBufferSize, jackBufferSizeArg);

        EM_ASM({
            var numInputs = $0;
            var numOutputs = $1;
            var bufferSize = $2;
            var WAB = Module['WebAudioBridge'];

            // store the new processor
            delete WAB.processor;
            WAB.processor = WAB.newProcessor;
            delete WAB.newProcessor;

            // setup new processor the same way as old one
            WAB.processor['onaudioprocess'] = function (e) {
                // var timestamp = performance.now();
                for (var i = 0; i < numInputs; ++i) {
                    var buffer = e['inputBuffer']['getChannelData'](i);
                    for (var j = 0; j < bufferSize; ++j) {
                        // setValue($3 + ((bufferSize * i) + j) * 4, buffer[j], 'float');
                        HEAPF32[$3 + (((bufferSize * i) + j) << 2) >> 2] = buffer[j];
                    }
                }
                dynCall('vi', $4, [$5]);
                for (var i = 0; i < numOutputs; ++i) {
                    var buffer = e['outputBuffer']['getChannelData'](i);
                    var offset = bufferSize * (numInputs + i);
                    for (var j = 0; j < bufferSize; ++j) {
                        buffer[j] = HEAPF32[$3 + ((offset + j) << 2) >> 2];
                    }
                }
            };

            // connect to output
            WAB.processor['connect'](WAB.audioContext['destination']);

            // and input, if available
            if (WAB.captureStreamNode)
                WAB.captureStreamNode.connect(WAB.processor);

        }, DISTRHO_PLUGIN_NUM_INPUTS, DISTRHO_PLUGIN_NUM_OUTPUTS, bufferSize, audioBufferStorage, WebAudioCallback, this);

        return true;
    }

    bool isMIDIEnabled() const override
    {
       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT || DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
        return EM_ASM_INT({ return Module['WebAudioBridge'].midi ? 1 : 0 }) != 0;
       #else
        return false;
       #endif
    }

    bool requestMIDI() override
    {
       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT || DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
        if (midiAvailable)
        {
            EM_ASM({
                var useInput = !!$0;
                var useOutput = !!$1;
                var maxSize = $2;
                var WAB = Module['WebAudioBridge'];

                var offset = Module._malloc(maxSize);

                var inputCallback = function(event) {
                    if (event.data.length > maxSize)
                        return;
                    var buffer = new Uint8Array(Module.HEAPU8.buffer, offset, maxSize);
                    buffer.set(event.data);
                    dynCall('viiif', $3, [$4, buffer.byteOffset, event.data.length, event.timeStamp]);
                };
                var stateCallback = function(event) {
                    if (event.port.state === 'connected' && event.port.connection === 'open') {
                        if (useInput && event.port.type === 'input') {
                            if (event.port.name.indexOf('Midi Through') < 0)
                                event.port.onmidimessage = inputCallback;
                        } else if (useOutput && event.port.type === 'output') {
                            event.port.open();
                        }
                    }
                };

                var success = function(midi) {
                    WAB.midi = midi;
                    midi.onstatechange = stateCallback;
                    if (useInput) {
                        midi.inputs.forEach(function(port) {
                            if (port.name.indexOf('Midi Through') < 0)
                                port.onmidimessage = inputCallback;
                        });
                    }
                    if (useOutput) {
                        midi.outputs.forEach(function(port) {
                            port.open();
                        });
                    }
                };
                var fail = function(why) {
                    console.log("midi access failed:", why);
                };

                navigator.requestMIDIAccess().then(success, fail);
            }, DISTRHO_PLUGIN_WANT_MIDI_INPUT, DISTRHO_PLUGIN_WANT_MIDI_OUTPUT, kMaxMIDIInputMessageSize, WebMIDICallback, this);

            return true;
        }
        else
       #endif
        {
            d_stderr2("MIDI is not supported");
            return false;
        }
    }

    static void WebAudioCallback(void* const userData /* , const double timestamp */)
    {
        WebBridge* const self = static_cast<WebBridge*>(userData);
        // self->timestamp = timestamp;

        const uint numFrames = self->bufferSize;

        if (self->jackProcessCallback != nullptr && self->active)
        {
            self->jackProcessCallback(numFrames, self->jackProcessArg);

           #if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
            if (self->midiAvailable && self->midiOutBuffer.isDataAvailableForReading())
            {
                static_assert(kMaxMIDIInputMessageSize + 1u == 4, "change code if bumping this value");
                uint32_t offset = 0;
                uint8_t bytes[4] = {};
                double timestamp = EM_ASM_DOUBLE({ return performance.now(); });

                while (self->midiOutBuffer.isDataAvailableForReading() &&
                       self->midiOutBuffer.readCustomData(bytes, ARRAY_SIZE(bytes)))
                {
                    offset = self->midiOutBuffer.readUInt();

                    EM_ASM({
                        var WAB = Module['WebAudioBridge'];
                        if (WAB.midi) {
                            var timestamp = $5 + $0;
                            var size = $1;
                            WAB.midi.outputs.forEach(function(port) {
                                if (port.state !== 'disconnected') {
                                    port.send(size == 3 ? [ $2, $3, $4 ] :
                                              size == 2 ? [ $2, $3 ] :
                                              [ $2 ], timestamp);
                                }
                            });
                        }
                    }, offset, bytes[0], bytes[1], bytes[2], bytes[3], timestamp);
                }

                self->midiOutBuffer.clearData();
            }
           #endif
        }
        else
        {
            for (uint i=0; i<DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
                std::memset(self->audioBuffers[DISTRHO_PLUGIN_NUM_INPUTS + i], 0, sizeof(float)*numFrames);
        }
    }

   #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    static void WebMIDICallback(void* const userData, uint8_t* const data, const int len, const double timestamp)
    {
        DISTRHO_SAFE_ASSERT_RETURN(len > 0 && len <= (int)kMaxMIDIInputMessageSize,);

        WebBridge* const self = static_cast<WebBridge*>(userData);

        // TODO timestamp handling
        self->midiInBufferPending.writeByte(static_cast<uint8_t>(len));
        self->midiInBufferPending.writeCustomData(data, kMaxMIDIInputMessageSize);
        self->midiInBufferPending.commitWrite();
    }
   #endif
};

#endif // WEB_BRIDGE_HPP_INCLUDED
