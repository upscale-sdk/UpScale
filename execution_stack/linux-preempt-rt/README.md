Upscale SDK - PREEMPT_RT support for Kalray MPPA
================================================

This section explains how to build and run a real-time version of the Linux
kernel for [Kalray MPPA](http://www.kalrayinc.com/) on the I/O cores.
In particular, it explains how to patch and run the Linux kernel 3.10 provided
by Kalray with the [PREEMPT_RT patch] (https://wiki.linuxfoundation.org/realtime)
provided by the Linux kernel community.

Building Linux
--------------

1. Install the following packages (they might be already installed):

        sudo yum install libtool autoconf automake kernel-headers libuuid.i686
        libuuid libuuid-devel patch cmake gcc gcc-c++ popt popt.i686 popt-devel
        gtk2 gtk2.i686 gtk2-devel libpcap libpcap.i686 libpcap-devel elfutils
        elfutils.i686 elfutils-libs elfutils-libs.i686 elfutils-libelf-devel
        elfutils-libelf-devel.i686 elfutils-libelf elfutils-libelf.i686 ruby
        glibc-devel.i686 libgcc libgcc.i686 perl-YAML-LibYAML gengetopt
        libstdc++-devel.i686 libstdc++-devel perl-HTML-Table flex flex-static
        libuuid.i686 libuuid glib-devel.i686 glib-devel glib2-devel
        glibc-devel.i686 glibc-devel ncurses-devel ncurses-devel.i686 stow
        texi2html texlive tcl-devel tk-devel texinfo dejagnu opensp libxml2
        libxml2-devel libxml2-devel.i686 glibc-static glpk glpk-devel zlib.i686
        zlib zlib-devel.i686 zlib-devel qt-devel qwt qwt-devel qwt-devel.i686
        bison flex rubygem-sqlite3 jack-audio-connection-kit-devel.x86_64
        jack-audio-connection-kit-devel.i686 dkms expect rubber
        libudev-devel.x86_64 libudev-devel.i686 inkscape parallel
        rubygem-net-ssh rubygem-net-scp openssh-askpass ant expat-devel pandoc

2. Enter the your home directory:

        cd $HOME

3. Download the source code:

        git clone git+ssh://username@scm.gforge.inria.fr//gitroot/manycorelabs/manycorelabs

   where `username` is your account to access the Kalray forge.

4. Restore Linux at commit `36bc1933961910b6e8c08432fdabcb5eb8e4e9b` by typing

        cd $HOME/manycorelabs

        git checkout origin/release-2.4.1 -b local_2.4.1

5. Download the additional sub-components by typing:

        git submodule update --init –recursive

6. Inside the manycorelabs directory add a new target to the build system for compiling PREEMPT_RT:

        cd $HOME/manycorelabs

        patch -p1 -i $HOME/patches/manycorelabs_build.patch

        cp $HOME/patches/k1bio_developer_smp_mmu_rt_defconfig $HOME/manycorelabs/buildroot/configs

7. Apply the patches to add PREEMPT_RT support:

        cd $HOME/manycorelabs/linux/

        patch -p1 -i $HOME/patches/linux_build.patch

        git am $HOME/patches/rt_patches/*

8. Then build:

        cd $HOME/manycorelabs/

        export LC_ALL=C

        ./valid/hudson/build.rb --simroot=/usr/local/k1tools --toolchain=linux

9. After the build (which could need even one hour) the images will be put in the

        $HOME/manycorelabs/buildroot/k1bio_developer_smp_mmu_rt/images/ 

   directory


Running Linux
-------------

To run the Linux kernel compiled according to the above instructions:

1. To deploy the Linux kernel image and filesystem on the I/O cores:

        cd $HOME/manycorelabs/buildroot/k1bio_developer_smp_mmu_rt/images/

        k1-jtag-runner  --march=bostan --jtag-verbose --exec-file=IODDR0:vmlinux mem=512M

2. To create a serial and SSH communication between the host machine and the
   Linux running on the I/O cores, type:

        sudo modprobe mppapcie_tty

        sudo modprobe mppapcie_eth

        sudo ifconfig mppa.0.0.0   10.0.0.5/24

        minicom  -o -D /dev/ttymppa/0/0/0/0 (user: root   password: kalray)


Compiling and running an application
------------------------------------

This section explains how to compile and run an application for Linux on the
I/O cores. Note that the application must be cross-compiled on the development
workstation, then transferred to the I/O cores where it can be finally run
using the standard Linux commands through the Minicom serial console.

To compile a `hello_world.c` file, type

1. `export PATH=$HOME/manycorelabs/devimage/manycorelabs-linux/toolroot/usr/local/k1-linux/bin/:$PATH`

2. `k1-gcc hello_world.c -mcore=k1bio -mboard=developer -mcluster=ioddr -mos=linux –o hello_world`

The binary file can be transferred to the I/O core through the following command:

`sshpass -p kalray scp -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no hello_world root@10.0.0.2:`


Compiling and running cyclictest
--------------------------------

This section explains how to compile and run a real-time benchmark for the
Linux kernel executed on the I/O cores. Note that the benchmark must be
cross-compiled on the development workstation, then transferred to the I/O
cores where it can be finally run using the standard Linux commands through the
Minicom serial console.

The real-time performance of the kernel can be measured through the cyclictest
application:

1. `cd $HOME`
2. `git clone git://git.kernel.org/pub/scm/utils/rt-tests/rt-tests.git`
3. `cd $HOME/rt-tests`
4. `git checkout v0.80 -b local_v0.80`
5. `git am $HOME/patches/rt-tests-patches/*`
6. `export PATH=$HOME/manycorelabs/devimage/manycorelabs-linux/toolroot/usr/local/k1-linux/bin/:$PATH`
7. `make cyclictest`

The cyclictest binary can be transferred to the I/IO cores through the
following command:

`sshpass -p kalray scp -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no cyclictest root@10.0.0.2:`

Once transferred, the cyclictest binary can be run through Minicom by typing:

`./cyclictest -S -p 80 -i 1000 -l 10000`


Customizing Busybox
-------------------

[BusyBox](http://www.busybox.net) is a software located on the Linux filesystem
providing several stripped-down Unix tools (e.g., grep, find, sh, mount, etc.)
in a single executable file. It was specifically created for embedded operating
systems with very limited resources. This section explains how to select and
customize the tools provided. Note that it assumes that you have already
compiled the Linux kernel according to the previous instructions.

1. `cd $HOME/manycorelabs/buildroot/k1bio_developer_smp_mmu_rt/build/busybox-1.19.4`

2. `make ARCH=k1b CROSS_COMPILE=$HOME/manycorelabs/devimage/manycorelabs-linux/toolroot/usr/local/k1-linux/bin/k1-linux- oldconfig`

3. `make ARCH=k1b CROSS_COMPILE=$HOME/manycorelabs/devimage/manycorelabs-linux/toolroot/usr/local/k1-linux/bin/k1-linux- menuconfig`

4. `cp .config $HOME/manycorelabs/buildroot/package/busybox/busybox-1.19.k1b.config`

5. Remove all output directories in manycorelabs/buildroot:

        rm -fr $HOME/ manycorelabs/buildroot/k1bio_developer_smp_mmu_rt 

6. Then rebuild:

        cd $HOME/ manycorelabs

        export LC_ALL=C

        ./valid/hudson/build.rb --simroot=/usr/local/k1tools --toolchain=linux

7. After the build (which could need even one hour) the images will be put in the
`$HOME/manycorelabs/buildroot/k1bio_developer_smp_mmu_rt/images/` directory
