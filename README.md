libbmd
======

C wrapper over Blackmagic Devices Decklink C++ api

Status
------

A simple high level capture api a quite reduced bmdcapture leveraging it
are provided. bmdcapture.cpp and the other tools will be converted later.

Build
-----

You need autotools, recent Decklink drivers and library and, optionally, 
Libav for the test programs. 

   ./configure --with-sdkdir=/path/to/the/sdk/include
   make
   make install

Integration with Libav
----------------------

The is an experimental [branch](http://github.com/lu-zero/libav/tree/bmd)
that leverages libbmd capture to provide an avdevice. In order to enable it:

    ./configure --enable-libbmd

is enough.

To use it the normal avdevice way to configure the input is enough

    ./avconv -analyzeduration 0 -f bmd -video_mode 8 -audio_connection 2 -video_connection 4 -video_format 1 -i default ${output options}

Keep in mind that the code is in flux and the options are bound to change.

ToDo
----

* Add some high level api for probing/enumerating devices.
* Add the equivalent simple high level api for playback.
* Provide a thin wrapper over the decklink classes.
* Document the whole thing properly
