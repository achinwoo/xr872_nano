# XRADIO Skylark SDK

![image](https://github.com/achinwoo/xr872_nano/blob/master/doc/top.jpg)

XRADIO Skylark SDK supports XR872/XR808 series wireless MCUs.


## Configuration

- Edit "gcc.mk" to define GCC path to your own path, eg.
```
  CC_DIR = ~/tools/gcc-arm-none-eabi-4_9-2015q2/bin
```


## Building

- Building commands
```
  $ cd ${prj_gcc_path}     # eg. cd project/demo/wlan_demo/gcc
  $ make config            # run `./configure.sh` to select SDK configuration
  $ make config_clean      # remove files generated by `make config`
  $ make lib               # build libraries and copy them to "lib"
  $ make lib_clean         # remove files in "src" generated by `make lib`
  $ make lib_install_clean # Remove libraries in "lib" generated by `make lib`
  $ make                   # build the executable binary
  $ make clean             # remove files generated by `make`
  $ make image             # create the image file
  $ make image_clean       # remove files generated by `make image`
  $ make objdump           # generate the disassembled file
  $ make build             # same as `make lib && make && make image`
  $ make build_clean       # same as `make image_clean clean lib_clean lib_install_clean`
```


## Links

- SDK: https://github.com/XradioTech/xradio-skylark-sdk.git
- WiKi: https://github.com/XradioTech/xradiotech-wiki.git
- DOC: https://docs.xradiotech.com
