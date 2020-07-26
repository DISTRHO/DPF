# Metronome example

This example will show tempo sync in DPF.<br/>

This plugin will output impulse the start of every beat.<br/>
To calculate exact frames to the next beat from the start of current audio buffer, `TimePosition::BarBeatTick.barBeat` is used.<br/>

Reference:
- [DISTRHO Plugin Framework: TimePosition::BarBeatTick Struct Reference](https://distrho.github.io/DPF/structTimePosition_1_1BarBeatTick.html)
