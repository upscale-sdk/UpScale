Upscale SDK - Operating System
==============================

This section explains how to build and run [ERIKA
Enterprise](http://www.erika-enterprise.com/) on the computing clusters when the I/O
cores execute the [RTEMS operating system](https://www.rtems.org/).

The structure of the directories is as follows:
* **`rtos/`**: core of ERIKA Enterprise + services for libpsocomp (see below)
* **`libgomp/`**: library libpsocomp, the P-SOCRATES version of libgomp for the clusters
* **`libpsocoffload/`**: library for off-loading


Building ERIKA for the toolchain
--------------------------------

The default target of the `Makefile` in the main directory allows to compile
the SDK runtime libraries (i.e. `libee.a`, `libpsocomp.a`, `libpsocoffload.a`)
that must be then manually copied in the P-SOCRATES toolchain. It acts as a
dispatcher by calling the `p-socrates.mk` sub-Makefile.

To build these libraries it is sufficient to type

        make

The Kalray toolchain must be k1-elf-gcc (GCC) 4.9.3 20141104 (prerelease)
[Kalray Compiler unknown 25f00d3-dirty] or higher for proper support of the
hypervisor.


Building and running a native ERIKA application
-----------------------------------------------

When the Makefile's `K1_ERIKA_PLATFORM` variable is set to `mOS`, the build
system allows to compile a native ERIKA application outside the P-SOCRATES SDK.
The application is built for the Kalray hypervisor (it can target Andey or
Bostan with the option `arch=k1a` or `arch=k1b`, respectively).

Usually the application is compiled by typing:

        make MAKEFILE_CUSTOM_RULES=${project_name}/makefile.config

where makefile.config is the application-specific makefile containing

        K1_ERIKA_PLATFORM := mOS
        O := ${K1_ERIKA_APP_DIR}/output

plus a subset of the following variables:

* `K1_ERIKA_FILES`: location of the subdirectory containing ERIKA Enterprise
  (default: `rtos/ee`)
* `K1_ERIKA_APP_NAME`: name of the cluster-side application ELF file (default:
   `erika`)
* `K1_ERIKA_APP_DIR`: directory containing cluster-side application header
   files, if any
* `K1_ERIKA_APP_SRCS`: list of cluster-side application source files; the file
   names must contain an absolute path or a path relative to the directory
   where make is run
* `K1_IO_APP_NAME`: name of I/O-side application ELF file
* `K1_IO_APP_DIR`: optional directory containing I/O-side application header
  files, if any
* `K1_IO_APP_SRCS`: list of I/O-side application source files; the file names
   must contain an absolute path or a path relative to the directory where make
   is run
* `K1_MULTIBIN_APP_NAME`: name of the multi-binary application file containing
  both the application I/O and cluster ELF files (default: `mppa.mpk`)
* `K1_OUTPTDIR`: output directory (build and bin) (default output).

Alternatively, these variables can be also set through environment variables
(e.g. through the standard Linux shell export command) or through the Makefile
command-line arguments.

To run the compiled application, you have the following options:
* Type `make run-erika-sim` to run the ERIKA Enterprise application on the cluster simulator
* Type `make run-erika-hw` to run the ERIKA Enterprise application on the many-core hardware

