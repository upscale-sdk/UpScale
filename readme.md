UpScale Software Development Kit (UpScale SDK)
===================
This directory contains the first release of the UpScale Software Development Kit -– a framework for the development of real-time high-performance applications in many-core platforms.

The UpScale SDK targets systems that demand more and more computational performance to process large amounts of data from multiple data sources, whilst requiring guarantees on processing response times. Many­core processor architectures allow these performance requirements to be achieved, by integrating dozens or hundreds of cores, interconnected with complex networks on chip, paving the way for parallel computing. Unfortunately, parallelization brings many challenges, by drastically affecting the system’s timing behavior: providing guarantees becomes harder, because the behavior of the system running on a multi­core processor depends on interactions that are usually not known by the system designer. This causes system analysts to struggle to provide timing guarantees for such platforms. 

UpScale tackles this challenge by including technologies from different computing segments to successfully exploit the performance opportunities brought by parallel programming models used in the high­performance domain and timing analysis from the embedded real-time domain, for newest manycore embedded processors available.
 
UpScale is released by the P-SOCRATES project, addressing the challenges of predictability and performance of current and future applications with high-performance and real-time requirements. The consortium is led by Instituto Superior de Engenharia do Porto, Portugal, and also comprises: Barcelona Supercomputing Centre, Spain, University of Modena and Regio Emilia, Italy, Swiss Federal Institute of Technology Zurich, Switzerland, Evidence SRL, Italy, Active Technologies SRL, Italy and ATOS, Spain.

More information at https://p-socrates.github.io/. 
 
 
Structure
-------------
The UpScale SDK is divided into three folders related to the three different flows: development, execution and analysis:
- **compilation_flow** provides the tools to compile and map the applications.
- **execution_stack** provides the runtime and operating systems.
- **analysis_flow** provides the Analyzer which can be used to develop automatic timing analysis profiles.

