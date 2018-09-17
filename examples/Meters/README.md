# Meters example

This example will show how parameter outputs can be used for UI meters in DPF.<br/>
The plugin will inspect the host audio buffer but it won't change it in any way.<br/>

In this example the UI will display a simple meter based on the plugin's parameter outputs.<br/>
In order to make drawing easier the UI uses NanoVG instead of raw OpenGL.<br/>
Please see the Parameters and States examples before studying this one.<br/>
