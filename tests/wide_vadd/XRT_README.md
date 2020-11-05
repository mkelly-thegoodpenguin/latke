# XRT

XRT is the Xilinx open-source runtime for data centre cards such as the U250, the card we will use in this example.
XRT can be found [here](https://github.com/Xilinx/XRT) on Github.

Various Linux distros are supported. For Ubuntu, 18.04 is the latest supported version.

## Install

Install Ubuntu 18.04.5 LTS, and then run

`$ sudo apt update && sudo apt upgrade`

`U250` works with <= `5.4.0-51` kernel.


1. add the following lines to your `.bashrc` file
```
source /opt/xilinx/xrt/setup.sh
source /tools/Xilinx/Vitis/2020.1/settings64.sh
export PATH=$PATH:/tools/Xilinx/Vitis/2020.1/bin:/tools/Xilinx/Vivado/2020.1/bin:/tools/Xilinx/DocNav
export LIBRARY_PATH=$LIBRARY_PATH:/usr/lib/x86_64-linux-gnu
```
1. install [Vitis](https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/vitis.html)
   Download full install tarball, extract, then change into directory:
```
    $ sudo ./xsetup -b ConfigGen
    $ sudo ./xsetup --agree XilinxEULA,3rdPartyEULA,WebTalkTerms --batch Install --config /root/.Xilinx/install_config.txt
```


## U250

#### XRT and U250 Drivers

https://www.xilinx.com/products/boards-and-kits/alveo/u250.html#dsabin_1804_qdma_v2

Install the latest release of XRT, and then the U250 deployment and dev packages :

`$ sudo dpkg -i xilinx-u250-xdma-201830.2-2580015_18.04.deb`


#### Flash Card

`$ sudo /opt/xilinx/xrt/bin/xbmgmt flash --update --shell  xilinx_u250_xdma_201830_2 --card 0000:4a:00.0`

After flash is complete, shut the PC down, remove the PC power plug and wait a few minutes
before re-plugging and re-starting.


#### Validate

`$ xbutil validate`


#### Prerequisites

1. Install Git:

`$ sudo apt install git`

2. (optional) install GitExtensions Git client
`$ sudo apt install mono-complete`

Install [Git Extensions](https://github.com/gitextensions/gitextensions/releases/download/v2.51.05/GitExtensions-2.51.05-Mono.zip)

and add this line to your `.bashrc` file :

`alias gext='mono PATH/TO/GITEXTIONS/GitExtensions.exe'`

3. build and install `opencv` 4
  1. On Ubuntu we follow [this guide](https://docs.opencv.org/master/d7/d9f/tutorial_linux_install.html)
  1. `$ sudo snap install cmake --classic`
  1. `$ sudo apt-get install libgtk2.0-dev pkg-config libavcodec-dev libavformat-dev libswscale-dev`
  1. `$ git clone https://github.com/opencv/opencv.git && cd opencv`
  1. `$ git checkout -b 4.4.0`
  1. `$ mkdir build && cd build`
  1. `$ cmake -D OPENCV_GENERATE_PKGCONFIG=ON -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=/usr/local ..`
  1. `$ make -j48 && sudo make install && sudo ldconfig`
1. install `OpenJDK 11`:
  1. `$ sudo apt install openjdk-11-jdk`
1. install latest [`Eclipse CDT`](https://www.eclipse.org/cdt/downloads.php)


#### Configure Project

```
$ sudo rm -rf /tools/Xilinx/Vitis/2020.1/tps/lnx64/cmake-3.3.2/

$ sudo add-apt-repository ppa:ubuntu-toolchain-r/test
$ sudo apt-get update
$ sudo apt install g++-10
$ sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-10 50

$ sudo apt install valgrind

$ sudo snap install cmake --classic

$ sudo apt install libomp-dev


#### Build/Run Kernel

U250

```
$ v++ -t hw --platform xilinx_u250_xdma_201830_2 -c -k wide_vadd -I'./' -I'/tools/Xilinx/Vivado/2020.1/include/' ./wide_vadd.cpp -o wide_vadd_250HW.xo
$ v++ -t hw --platform xilinx_u250_xdma_201830_2 --link --config connectivity_u250.cfg  --profile_kernel data:wide_vadd:all:all wide_vadd_250HW.xo -o'wide_vadd_HW.xilinx_u250_xdma_201830_2'

```

Now copy `wide_vadd_HW.xilinx_u250_xdma_201830_2` xclbin binary to latke build directory, and run `wide_vadd` executable.


### Profiling

https://www.xilinx.com/html_docs/xilinx2020_1/vitis_doc/profilingapplication.html#xmv1511400547463

`xrt.ini` file must be placed in the same directory as the host executable in order to generate
profile reports


### Emulation

There are two other vitis compile modes:

1. hardware emulation:  `hw_emu`
1. software emulation:  `sw_emu`

Simply replace `hw` above with one of these two other modes:

##### HW EMU

```
$ v++ -t hw_emu --platform xilinx_u250_xdma_201830_2 -c -k wide_vadd -I'./' ./wide_vadd.cpp -o wide_vadd_250HWEMU.xo
$ v++ -t hw_emu --platform xilinx_u250_xdma_201830_2 --link --config connectivity_u250.cfg wide_vadd_250HWEMU.xo -o'wide_vadd_250HWEMU.xilinx_u250_xdma_201830_2'
```

##### SW EMU

```
$ v++ -t sw_emu --platform xilinx_u250_xdma_201830_2 -c -k wide_vadd -I'./' ./wide_vadd.cpp -o wide_vadd_250SWEMU.xo
$ v++ -t sw_emu --platform xilinx_u250_xdma_201830_2 --link --config connectivity_u250.cfg wide_vadd_250SWEMU.xo -o'wide_vadd_250SWEMU.xilinx_u250_xdma_201830_2'
```

To run the emulation, you will need to generate an `emconfig.json` file:

`$ emconfigutil --platform xilinx_u250_xdma_201830_2`

and place it in the working directory of the demo executable.

You will also need to set the `XCL_EMULATION_MODE` environement variable to `sw_emu` or `hw_emu`
software emulation or hardware emulation respectively.
