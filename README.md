# Latke

A small set of C++ wrapper classes designed to make it easier to do
efficient batch image processing with OpenCL.

[![badge-license]][link-license]

## Tests

1. debayer_buffer and debayer_image: convert Bayer mosaic raw image to RGB(A), using either OpenCL buffers or OpenCL images. To run the program, pass in the file name of the raw file, and optionally the bayer pattern. RGGB is the default pattern. A test file can be found in the `test_data` folder.


[badge-license]: https://img.shields.io/badge/License-LGPL%20v2-blue.svg "LGPL v2"
[link-license]: https://github.com/GrokImageCompression/latke/blob/master/COPYING "LGPLv2"
