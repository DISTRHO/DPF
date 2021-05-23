This directory contains several tests for DPF related things, from graphics to plugin stuff to utilities.
Each *.cpp file is meant to be its own test.

In order to test DPF components individually, some of these tests do not link against DGL but import/include its files.
All test files must be self-contained, in order to prevent surprises in regards global state and initialization stuff.

The following tests are present:

 - Application
 Verifies that creating an application instance and its event loop is working correctly.
 This test should automatically close itself without errors after a few seconds

 - Circle
 TODO

 - Color
 Runs a few unit-tests on top of the Color class. Mostly complete but still WIP.

 - Demo
 A full window with widgets to verify that contents are being drawn correctly, window can be resized and events work.
 Can be used in both Cairo and OpenGL modes, the Vulkan variant does not work right now.

 - Line
 TODO

 - NanoSubWidgets
 Verifies that NanoVG subwidgets are being drawn properly, and that hide/show calls work as intended.
 There should be a grey background with 3 squares on top, one of hiding every half second in a sequence.

 - Point
 Runs a few unit-tests on top of the Point class. Mostly complete but still WIP.

 - Rectangle
 TODO

 - Triangle
 TODO

 - Window
 Runs a few basic tests with Window showing, hiding and event loop.
 Will try to create a window on screen.
 Should automatically close after a few seconds.
