# CVPort example

This example will show how to modify audio input/output port type in DPF so they can work as CV.<br/>
Take a look at `initAudioPort()` method.<br/>

Worth noting that CV is not supported outside of JACK and LV2 formats.<br/>
While it can be built and used in other formats, these ports will appear as regular audio to the host.

The plugin does sample & hold (S&H) processing.<br/>
3 I/O ports are specified.<br/>

- Input audio port.
- Input CV port to modify Hold Time.
- Output CV port.

The plugin also has a Hold Time parameter. It mixes CV value and parameter value.<br/>
