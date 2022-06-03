# t-pose software

These are programs which apply the various t-pose concepts into useable executables.

### triangulate

This program will triangulate a given image. It generates an output file "out.tri" which contains the triangulations at all specified resolutions. Note that the resolutions are given in number of triangles and currently still hardcoded.

    ./main -i image.png

### warp

Two-way consistent warping program, which takes as input two triangulation files containing various resolutions, and two corresponding images, which it warps onto each other. The resulting triangulations are written to the output same files with the warped points.

    ./main -ia imageA.png -ib imageB.png -ta outA.tri -tb outB.tri

### view

This is a triangulation viewer, handling both multiple triangulation sets and warped triangulations. Simply specify a triangulation file and optionally an image file

    ./main -t out.tri [-i image.png]

### mesh

This is currently being overhauled.
