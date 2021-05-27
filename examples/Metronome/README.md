# Metronome example

This example will show tempo sync in DPF.<br/>

This plugin will output a sine wave at the start of every beat.<br/>
The pitch of sine wave is 1 octave higher at the start of every bar.<br/>

4 parameters are avaialble:

- Gain
- Decay time
- Semitone
- Cent

To calculate frames to the next beat from the start of current audio buffer, following implementation is used.<br/>

```c++
const TimePosition& timePos(getTimePosition());

if (timePos.bbt.valid)
{
    double secondsPerBeat = 60.0 / timePos.bbt.beatsPerMinute;
    double framesPerBeat  = sampleRate * secondsPerBeat;
    double beatFraction   = timePos.bbt.tick / timePos.bbt.ticksPerBeat;

    uint32_t framesToNextBeat = d_isZero(beatFraction)
                              ? 0
                              : static_cast<uint32_t>(framesPerBeat * (1.0 - beatFraction));
    // ...
}
```

Reference:
- [DISTRHO Plugin Framework: TimePosition::BarBeatTick Struct Reference](https://distrho.github.io/DPF/structTimePosition_1_1BarBeatTick.html)
