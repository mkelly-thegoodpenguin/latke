# Latke

A small set of C++ wrapper classes for efficient batch image processing using OpenCL.

[![badge-license]][link-license]

## Tests

### Debayer

The test binaries `debayer_buffer` and `debayer_image` convert a Bayer mosaic raw image to RGB(A),
using either OpenCL buffers or OpenCL images. 

To run the programs, pass in the file name of the raw file, and optionally the bayer pattern from

`{RGGB,GRBG,GBRG,BGGR} `

`RGGB` is the default pattern. A test raw file can be found in the `test_data` folder.

Example:

`$ debayer_buffer FOO.png BGGR`

Note: the opencl kernel '.cl' files must be compiled at runtime to create the kernel binaries, so the test binary
must have access to these files. These `.cl` files are copied to the build folder, so the test binary
must be run from this folder.  


[badge-license]: https://img.shields.io/badge/License-LGPL%20v2-blue.svg "LGPL v2"
[link-license]: https://github.com/GrokImageCompression/latke/blob/master/COPYING "LGPLv2"
