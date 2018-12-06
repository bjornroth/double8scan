# double8scan

Around 1990 me and a friend did some double 8 mm (or regular/standard 8) films using two old cameras (I think we broke at least one of them, an Eumig C3). Kodachrome isn't available in D8 anymore, which is a shame.
We never had a decent projector, only a film-shredding original 1932 Kodak double 8 with a 110/220 volt converter. Every time we tried to project something the film stuck on edits, and the metal cog wheels trashed the perforations. Around 2004 the strips had been laying around for almost 15 years and was it's time do something.

double8scan is a simple C hack that tries to detect the perforation on a scanned double 8 strip and spit out the frames as individual jpeg images. You may have to tweak the white and black levels (the switches -b and -w, you can also select what color channel to use with -c) and maybe restrict perforation and frame sizes (-p and -f).

Because the strip may be stretched (or the scanner non-perfect) the frame size must be set by the user, but if no size is given the program will probe and display min, max and mean frame heights. It's probably best to use the minimum frame size, otherwise you will get too much border in the image. Once you've decided on a frame height, this program can batch convert the scanned strips.

Link against the IJG JPEG library. The movie can then be assembled from the individual frames in various programs.

Improvements that can be made:

- Try to find white/black level automatically using histogram
- Adapt to work for super 8 and other formats
- Center frame between perfs, now it just uses frame start (center of perf) + height
- Handle slightly rotated strips (using x displacement)
- Handle slightly rotated strips (by rotating)
- Crop image
- Add direct movie generation
- ...
