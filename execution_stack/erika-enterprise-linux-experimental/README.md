Upscale SDK - Operating System
==============================

The current version of the P-SOCRATES SDK does not allow to build an
application when Linux is running on the I/O cores.

To build [ERIKA Enterprise](http://erika.tuxfamily.org/) for the clusters,
overwrite the contents of the directories `libgomp/` and `libpsocoffload/` with
the ones available inside these directories.
Then, follow the instructions available [here](../erika-enterprise-rtems/README.md)

On the Linux side, the application must be manually cross-compiled and linked
as explained [here](../linux-preempt-rt).

