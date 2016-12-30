P-SOCRATES Project: compiler flow
----------------------------------

0. Scope of this document

This is a short document explaining how to install the UpScale compiler flow.
This document also describes some basic usage of the compiler.

0.1. Context

Currently the compiler targets the MPPA platform from Kalray in the context of
P-SOCRATES project. Offloading of work from the I/O side is supported by means of the
#pragma omp target. OpenMP is supported in the cluster side.

1. Prerequisites

1.1. Specific prerequisites

An environment with the k1-toolchain as provided by Kalray and erika enterprise
operating system installation.

1.2. Regular prerequisites

Please see

  https://pm.bsc.es/ompss-docs/user-guide/installation.html#mercurium-build-requirements

for a detailed description of the requisites when building Mercurium

2. Installation

2.1 Mercurium 

  $ cd mcxx-psocrates

Generate the configure file

  $ autoreconf -vfi

Then run configure

  $ ./configure --prefix=<<installation-path>> --enable-tl-openmp-gomp \
                --with-erika-enterprise=<<path-to-erika-installation>>

Now compile (this step takes a while)

  $ make

And install to the <<installation-path>> specified in the configure step

  $ make install

2.2 boxer

  $ cd boxer

Compile and install 

  $ make PREFIX=<<installation-path>> install

2.3 psoc_mapper

  $ cd psoc_mapper

Compile and install 

  $ make PREFIX=<<installation-path>> install

IMPORTANT: Add the <<installation-path>> to your PATH environment variable.  
