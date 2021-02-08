# double8scan

Around 1990 me and a friend did some double 8 mm (or regular/standard 8) films using two old cameras (I think we broke at least one of them, an Eumig C3). Kodachrome isn't available in D8 anymore, which is a shame.
We never had a decent projector, only a film-shredding original 1932 Kodak double 8 with a 110/220 volt converter. Every time we tried to project something the film stuck on edits, and the metal cog wheels trashed
the perforations. Around 2004 the strips had been laying around for almost 15 years and was it's time do something.

double8scan is a simple C hack that tries to detect the perforation on a scanned double 8 or super 8 strip and spit out the frames as individual jpeg images. You may have to tweak the white and black levels (the switches
`-b` and `-w`, you can also select what color channel to use with `-c`) and maybe restrict perforation and frame sizes (`-p` and `-f`).

Because the strip may be stretched (or the scanner non-perfect) the frame size must be set by the user, but if no size is given the program will probe and display the median frame heights (evaluated both using the full width of the perf, and the single "best" column the program decided). The frame width is automatically adjusted so a minimum of perforation is visible, hopefully without losing frame information. The output width can be overriden, as well as setting a fixed X offset to start the frames from. Once you've decided on a frame height, this program can batch convert the scanned strips.

Build using CMake, for instance:

`mkdir build ; cd build ; cmake .. ; make`

[libjpeg-turbo](https://github.com/libjpeg-turbo/libjpeg-turbo/) is included as a submodule and is built and linked to. Also, on Windows a [getopt substitute](https://github.com/Chunde/getopt-for-windows/) is built automatically.

The movie can then be assembled from the individual frames in various programs. For instance, with [ffmeg](https://trac.ffmpeg.org/wiki/Slideshow) it can be as easy as:

`ffmpeg -framerate 16 -i strip.jpg.%03d.jpg strip.mp4`

given that your original JPEG file was named `strip.jpg`.

Improvements that can be made:

- Try to find white/black level automatically using histogram
- Center frame between perfs, now it just uses frame start (center of perf) + height
- Handle slightly rotated strips (by rotating)
- Crop image
- Output PNGs to avoid 2nd compression
- Add direct movie generation
- ...
