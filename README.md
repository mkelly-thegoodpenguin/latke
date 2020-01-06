# Latke

A small set of C++ wrapper classes designed to make it easier to do
efficient batch image processing with OpenCL. 

## Tests

1. simple: simple program illustrating use of OpenCL images.

1. debayer: convert Bayer mosaic raw image to RGB(A). To run the program, pass in the file name of the raw file, and optionally the bayer pattern. RGGB is the default pattern.

A sample bayer image can be found [here](https://github.com/codeplaysoftware/visioncpp/wiki/Example:-Bayer-Filter-Demosaic)
