# Latke

A small set of C++ wrapper classes for efficient batch image processing using OpenCL.

[![badge-license]][link-license]

## Test Applications

### Debayer

The test binaries `debayer_buffer` and `debayer_image` convert a Bayer mosaic raw image to RGB(A),
using either OpenCL buffers or OpenCL images.

To run one of these programs, pass in an input directory with the raw images, an output directory to store
the debayered images, and optionally one of the follow bayer patterns:

`{RGGB,GRBG,GBRG,BGGR} `

`RGGB` is the default pattern. 


Example:

`$ debayer_buffer -i /home/FOO  -o /home/BAR  -p BGGR`


A set of test raw files can be found in the `test_data` folder.

Note: the opencl kernel '.cl' files must be compiled at runtime to create the kernel binaries, so the test binary
must have access to these files. These `.cl` files are copied to the build folder, so the test binary
must be run from this folder.  


### Building

This project uses `cmake` to manage its build.


#### Dependencies

The binaries require an OpenCL 1.2 runtime.

For Intel CPUs, an [OpenCL driver](https://software.intel.com/content/www/us/en/develop/articles/opencl-drivers.html) can be installed.
For Intel iGPUs on Linux, the Intel [comput runtime])https://github.com/intel/compute-runtime) may be used.
And, for AMD GPUs on Linux, [ROCm](https://github.com/RadeonOpenCompute/ROCm) may be used.

Note: NVidia cards are not supported due to their poor OpenCL 1.2 support.


[badge-license]: https://img.shields.io/badge/License-LGPL%20v2-blue.svg "LGPL v2"
[link-license]: https://github.com/GrokImageCompression/latke/blob/master/COPYING "LGPLv2"
