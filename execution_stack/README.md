Upscale SDK - Operating System
==============================

This directory contains the operating system support for the Upscale SDK on the
[Kalray MPPA](http://www.kalrayinc.com/) platform.

* [`erika-enterprise-rtems`](./erika-enterprise-rtems): this is the default
  toolchain with [RTEMS](https://www.rtems.org/) on the I/O cores and
  [ERIKA Enterprise](http://erika.tuxfamily.org/) on the computing clusters
* [`erika-enterprise-linux-experimental`](./erika-enterprise-linux-experimental):
  this is an experimental toolchain with Linux on the I/O cores and
  [ERIKA Enterprise](http://erika.tuxfamily.org/) on the computing clusters
* [`linux-preempt-rt`](./linux-preempt-rt): this is the
  [PREEMPT_RT support](https://wiki.linuxfoundation.org/realtime) for the Linux
  kernel 3.10 running on the I/O cores
