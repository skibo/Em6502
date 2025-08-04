# Em6502

This repository isn't really meant for public consumption.  Many years ago I
starting developing a Commodore PET emulator for MacOs.  The "core" emulator,
which emulated the 6502, peripheral chips, memory, etc., was written in C
for portability and the user interface and display bits were written in
Objective-C.  I got pretty far along with that emulator but one day I
discovered an Apple II emulator written in Javascript and I wondered why it
didn't occur to me that Javascript was indeed fast enough to emulate a 1Mhz
computer.  I converted the emulator "core" from C to Javascript and then
learned more about JS user interface development than I cared to.  The
results are at https://skibo.github.io/6502/pet2001/

During the pandemic, I got the idea to revisit my C implementation of the
PET emulator.  The idea was to come up with a cycle-accurate model for the
PET, accurate enough to emulate some tricks used in demos and perhaps even
accurately model the old PETs' "snow" which would occur if you wrote to
video RAM at the wrong times.  Also, I thought I could come up with a decent
development environment in a desktop application.  So, now I have converted
the C parts to C++ and I am using gtkmm on Linux for the GUI.

Thomas Skibo
Nov 2022

