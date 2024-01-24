# Using STM32 MCU with picolibc standard library

## Why

The standard C library is a given when programming in native environment, but that's not always the case for MCU development.
You can do a lot of MCU programming in C without using the standard library but that's not always the case.
It's easy to write an(unoptimized) replacement for `memcmp` but it's not that easy to write one for `sqrt`.
And that's why there's a high chance that you will need to use the standard C library.
Since MCUs can have small flash and RAM, you will want to use a standard C library that was developed with these constraints in mind.
I chose to use the tiny `picolibc` standard C library implementation which has support for multiple CPU architectures.

This documentation provides instructions on how to build picolibc on your own or use the pre-compiled version(ARM only).

## Building it

The instructions bellow were tested with `picolibc v1.8.5` and `Ubuntu 22.04`.

For building `picolibc` to use it with STM32 or other 32bit ARM CPUs, you will need to have installed:
* arm-none-eabi-gcc
* ninja-build
* meson

```
sudo apt install gcc-arm-none-eabi ninja-build pip
```

Now we need to install meson using `pip` tool.
In order to build the project, meson doesn't need to be installed as root.
But unfortunately in order to install picolibc we need to have meson installed as root:
```
sudo pip install meson
```

Clone the picolibc repo https://github.com/picolibc/picolibc

For building, the path to meson needs to be known by the shell:
```
export PATH="$PATH":/home/$(whoami)/.local/bin
```

Build picolibc:

```
mkdir build-stm32
cd build-stm32
../scripts/do-arm-configure
ninja
```

Install picolibc:

```
ninja install
```
Picolibc should now be installed in `/usr/local/picolibc` folder.

Now that picolibc is installed on our system we can make use of it.

## Building STM32 code and linking with picolibc

Inside this repo you can just type `make`. As a result `test.bin` flashable image will be built.
That's all you need if you want to have a project that builds using picolibc as standard C library.

The info below is not needed for building your project. It's information on what work was needed for building for STM32 using picolibc.

The compiler needs to know where to find the C library and its headers in order to use them when building our code.
Fortunately, picolibc provides a `.spec` file to instruct GCC about that.
In our [Makefile](Makefile), we point gcc to use the spec file by adding `-specs=picolibc.specs` to the compiler invocation line.

Another thing we need to take care of is to use a linker script that will be aware of picolibc related `.sections`(flash and RAM sections where to place code, variables, etc.).
For this, we are setting in the [Makefile](Makefile) the file [stm32f100-picolibc.ld](stm32f100-picolibc.ld) to be used by GCC as linker script. The linker script is copied from `picolibc.ld` file that was generated from picolibc build.
Some small adjustments and additions are made to the contents of the original.

Linker script adjustments to match STM32F100 memory offsets and sizes:
- changed flash start address from 0 to 0x08000000
- changed flash size from 4MB(0x00400000) to 128KB
- changed RAM size from 2MB(0x00200000) to 8KB

As a linker script addition, we need to add `.vectors` section at the beginning of the flash.
This is necessary to match the `int_vectors` array of function pointers, declared with `.vectors` section attribute in [stm32-ints.h](stm32-ints.h) file.
The code entry point address and all the interrupt function addresses need to be placed in the correct order starting from flash offset 0.

MCU needs to start execution at picolibc's code entry point. From the linker script statement `ENTRY(_start)` we find that picolibc's entry point is named `_start`.
This is the picolibc function that takes care of initialization heap, stack, variables, etc. and then starts executing the `main` function.
`_start` is the function to which our MCU should jump after a reset and that's why we add it as the second entry(offset 4) in the `int_vectors` array of function adresses.
MCU is wired to jump after a reset to the address pointed at flash offset 4.

Also in [stm32-ints.h](stm32-ints.h) file, the linker declared `__stack` variable is placed at offset 0 of `int_vectors`. This is the address of the stack base(AKA top of stack) that MCU loads in
stack pointer(SP) register at reset.


## GDB debugging

If you want to run your code through GDB for debug purposes and want to also debug inside the C standard library, you need to point GDB to the picolibc sources.
This is done with command:
`directory <picolibc_sources_path>/newlib`

## Using precompiled picolibc

For ARM architecture, the picolibc creator provides picolibc pre-compiled binaries, so there's no need to compile your own.
Since STM32 is an ARM CPU, these precompiled binaries should work out of the box.
For other architectures there are no provided pre-compiled picolibc binaries.

The picolibc creator kindly provided us the download link and instructions on how to install the ARM pre-compiled picolibc: https://keithp.com/picolibc/

Unfortunately the Linux installation instructions don't work for Ubuntu 22.04 because of different gcc install path.

But this is how I did it for the `1.8.4-12.3` release
1. Download the archive https://keithp.com/picolibc/dist/gnu-arm-embedded/picolibc-1.8.4-12.3.rel1.zip
2. Extract it in a folder of your choice. Mine was `/home/spanceac/Downloads/picolibc-prebuilt-rel`
3. Modify [Makefile](Makefile) so `-specs=picolibc.specs` will point to the specs file from the folder of prebuilt picolibs. In my case the path was expanded to `-specs=/home/spanceac/Downloads/picolibc-prebuilt-rel/lib/gcc/arm-none-eabi/12.3.1/picolibc.specs`
4. Also modify [Makefile](Makefile) to add to `CFLAGS` the argument
```
--sysroot=<absolute_path_to_picolibc_prebuilt_folder>/arm-none-eabi
```
In my case this expanded to
```
--sysroot=/home/spanceac/Downloads/picolibc-prebuilt-rel/arm-none-eabi
```

Now you can run `make` to compile the project using precompiled picolibc.

## Test code

To prove that the picolibc integration goal is achieved I want to test the following:
* calling functions from C standard library works
* interrupts work, proving that linker script adjustments work

The test code will use `memcpy`, `memcmp` and `memset` functions from the C standard library to modify data on the `char test[2]` global array.

A timer interrupt handler sets the contents of the array to `"on"` using `memcpy`, every 1s.

The main loop tests the value inside the global array with `memcmp`.
Every time the main loop detects that the `test` array was set to `"on"` value, it will toggle a LED and clear the contents of the array with `memset`.
As a result, the LED blinks with the period of the timer.


