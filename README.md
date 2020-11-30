# Latke

A small set of C++ wrapper classes for efficient batch image processing using OpenCL.

[![badge-license]][link-license]

## Test Applications

### Debayer

The test binaries `debayer_buffer` and `debayer_image` convert a Bayer mosaic raw image to RGB(A),
using either OpenCL buffers or OpenCL images.

To run the programs, pass in an input directory with the raw images, an output directory to store
the debayered images, and optionally the bayer pattern from

`{RGGB,GRBG,GBRG,BGGR} `

`RGGB` is the default pattern. A test raw file can be found in the `test_data` folder.

Example:

`$ debayer_buffer -i /home/FOO  -o /home/BAR  BGGR`

Note: the opencl kernel '.cl' files must be compiled at runtime to create the kernel binaries, so the test binary
must have access to these files. These `.cl` files are copied to the build folder, so the test binary
must be run from this folder.  


[badge-license]: https://img.shields.io/badge/License-LGPL%20v2-blue.svg "LGPL v2"
[link-license]: https://github.com/GrokImageCompression/latke/blob/master/COPYING "LGPLv2"
