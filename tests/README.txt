This directory contains several tests for DPF related things, from graphics to plugin stuff to utilities.
Each *.cpp file is meant to be its own test.

In order to test DPF components individually, these tests do not link against DGL directly,
but will directly import the needed source code.
All files must be self-contained, in order to prevent surprises in regards global state and initialization stuff.
