ccextract
=========

Here is some old code for parsing and displaying [EIA-608][] ("line 21") closed captions.

Compiling
---------

This is only ever tested on GNU/Linux, though I confirmed that it still compiles and runs on 2017 editions of GNU/Linux:

```
apt install build-essential libdv4-dev libquicktime-dev libncursesw5-dev
make
```

Possibly you can also get it to work on OS X or other unix-like operating systems? I don't know.

Running
-------

* Obtain a [DV][] file with line-21 caption data embedded in it. "Raw" DV files, as well as DV data embedded in a quicktime container, are acceptable. If you don't have such a file handy, [here is one][mov] that I found on my hard drive and don't have rights to.

* Play the video at the same time as running the `./tst` program, maybe by doing something like this:

    `mpv -quiet Demo_DV_720x480_CC.mov > /dev/null & ( sleep 0.3 ; ./tst Demo_DV_720x480_CC.mov )`

* Watch and be amazed as the closed captions get decoded and shown in your terminal as the video plays in the video player.

Hacking
-------

* `eia608.c` is the file of the most potential interest; it implements the closed caption decoder.

* `tst.c` uses that decoder, [libdv][], and [libquicktime][] to render closed captions to the screen.

* `smpte.c` is some dumb utility for 30000/1001 fps timecode.

* `references.txt` and `TODO` are documentation and contain what you'd expect.

License
-------

The code comments say this is licensed under [GPL v2][], but if you have an actual use case in mind and need a different license, get in touch; I don't care.

[EIA-608]: https://en.wikipedia.org/wiki/EIA-608
[DV]: https://en.wikipedia.org/wiki/DV
[mov]: https://makeinstallnotwar.org/video/Demo_DV_720x480_CC.mov
[libdv]: http://libdv.sourceforge.net/
[libquicktime]: http://libquicktime.sourceforge.net/
[GPL v2]: https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html
