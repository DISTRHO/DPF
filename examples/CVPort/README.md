# CVPort example

This example will show how to modify input/output port type in DPF.<br/>
Take a look at `initAudioPort()` method.<br/>

The plugin does sample & hold (S&H) processing.<br/>
3 I/O ports are specified.<br/>

- Input audio port.
- Input CV port to modify Hold Time.
- Output CV port.

The plugin also has a Hold Time parameter. It mixes CV value and parameter value.<br/>
