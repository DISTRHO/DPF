# States example

This example will show how states work in DPF.<br/>
The plugin will not do any audio processing.<br/>

In this example the UI will display a 3x3 grid of colors which can *not* be changed by the host.<br/>
There are 2 colors: blue and orange. Blue means off, orange means on.<br/>
When a grid block is clicked its color will change and the host will receive a message which is auto-saved.<br/>
When the UI is opened it will receive messages indicating the last used block states.<br/>
