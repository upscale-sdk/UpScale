#K1_LD_VERBOSE    := -Wl,-verbose > ld.txt

# K1_TOOLCHAIN_DIR := $(shell echo $${K1_TOOLCHAIN_DIR:-/usr/local/k1tools/})

#======== THE FOLLOWING HAVE TO BE OVERIDDEN TO CHANGE BUILD's DEFAULT ========#

K1_LIB_PREFIX ?= lib
K1_LIB_SUFFIX ?= .a

K1_ERIKA_FILES    ?= .
K1_ERIKA_PKG2     ?= ${K1_ERIKA_FILES}/rtos/ee/pkg2

K1_OUTPTDIR ?= $(if ${O},${O},output)

#export K1_TOOLCHAIN_DIR := ~/usr/local/k1tools
K1_TOOLCHAIN_DIR := $(shell echo $${K1_TOOLCHAIN_DIR:-/usr/local/k1tools/})

#========= THE ABOVE HAVE TO BE OVERIDDEN TO CHANGE BUILD's DEFAULT ==========#

# Needed for Ubuntu/Debian since sh points to dash instead of bash...
SHELL := /bin/bash

K1_ERIKA_PLATFORM ?= mOS

#board can be tc2, emb01 or developer
board := developer

# emb01 is always a remote target
# tc2 and developer can be both local or remote (local by default)
ifeq (${board}, emb01)
remote := true
else
remote := false
endif

K1_LIBGOMP_DEFS ?= -DBAR_SIGWAIT -DWORK_FIRST_SCHED="0U" -DTASKING_ENABLED\
  -DTASK_BARRIER_BUSY_WAITING -DERIKA_TASK_STACK_SIZE="1024U"\
  -DSTATIC_TDG_ENABLED #-DTASK_CORE_STATIC_SCHED 

# Get the libraryes sub-platform
ifneq (${K1_ERIKA_SUB_PLATFORM},)
K1_ERIKA_PLATFORM := ${K1_ERIKA_SUB_PLATFORM}
else
ifeq (${K1_ERIKA_PLATFORM}, p-socrates)
K1_ERIKA_PLATFORM := mOS
endif # K1_ERIKA_PLATFORM = p-socrates
endif # !K1_ERIKA_SUB_PLATFORM
$(info K1_ERIKA_SUB_PLATFORM=${K1_ERIKA_PLATFORM})

ifeq (${K1_ERIKA_PLATFORM}, bsp)
#arch have to be k1a (Andey)
arch := k1a
hypervisor-opt :=

K1_ERIKA_SRCS := $(addprefix ${K1_ERIKA_PKG2}, \
  /cpu/kalray_k1/hal/ee_k1_asm.S \
  /common/utils/ee_assert.c \
  /cpu/kalray_k1/bsp/ee_k1_irq.c \
  /cpu/kalray_k1/bsp/ee_k1_cluster_init.c \
  /cpu/kalray_k1/bsp/ee_k1_rm_task_server.c \
  /cpu/kalray_k1/bsp/ee_k1_startup_handlers.c \
  /cpu/kalray_k1/bsp/ee_k1_bsp_communication.c \
  /cpu/kalray_k1/bsp/eecfg.c \
  /cpu/kalray_k1/bsp/ee_hal_init.c \
  /hal/kernel/ee_hal_std_change_context.c \
  /kernel/bsp/oo/ee_scheduler.c \
  /kernel/bsp/oo/ee_api_dynamic.c \
  /kernel/bsp/oo/ee_api_extension.c \
  /kernel/bsp/oo/ee_api_osek.c \
  /kernel/bsp/oo/ee_scheduler_entry_points.c \
  /kernel/bsp/k1/ee_api_k1.c \
  /kernel/bsp/k1/ee_kernel_k1.c)

K1_ERIKA_INCLUDE_DIRS := ${K1_ERIKA_APP_DIR} \
  $(addprefix ${K1_ERIKA_PKG2}, /common/compilers /common/compilers/gcc \
    /common/utils /cpu/kalray_k1/bsp /cpu/kalray_k1/hal /hal/kernel\
    /kernel/bsp /kernel/bsp/k1 /kernel/bsp/oo) \
  ${K1_ERIKA_FILES}/libgomp ${K1_ERIKA_FILES}/libpsocoffload\
  ${K1_ERIKA_FILES}/libgomp/config ${K1_ERIKA_FILES}/libgomp/config/k1

K1_CLUSTER_INCLUDE_DIRS_OPT := $(addprefix -I, $(K1_ERIKA_INCLUDE_DIRS) $(K1_ERIKA_APP_INCLUDE_DIRS))

#-mcore=k1io
K1_CFLAGS := -mboard=${board} -mos=bare -march=${arch} -std=c99\
  -Wall -Wno-unused-function -save-temps=obj -Wall -g3 -O\
  -Winit-self -Wswitch-default -Wfloat-equal -Wundef -Wshadow\
  -Wuninitialized -Wno-unused-function\
  -I. $(K1_CLUSTER_INCLUDE_DIRS_OPT) -DK1_ERIKA_PLATFORM_BSP\
  ${K1_LIBGOMP_DEFS}

else
ifeq (${K1_ERIKA_PLATFORM}, mOS)
#arch can be k1a (Andey) or k1b (Bostan)
arch ?= k1b
hypervisor-opt := -mhypervisor

# Useful functions
toupper = ${strip ${shell echo $(1) | tr '[:lower:]' '[:upper:]'-}}
tolower = ${strip ${shell echo $(1) | tr '[:upper:]' '[:lower:]'-}}

K1_ERIKA_SCHEDULER ?= partitioned
${info K1_ERIKA_SCHEDULER=${K1_ERIKA_SCHEDULER}}

K1_ERIKA_SCHEDULER_OPTIONS := -DEE_SCHEDULER_$(call toupper, ${K1_ERIKA_SCHEDULER})

K1_ERIKA_SRCS := $(addprefix ${K1_ERIKA_PKG2}, \
  /cpu/kalray_k1/mOS/ee_k1_cluster_boot.c \
  /cpu/kalray_k1/mOS/ee_k1_wrappers.S \
  /cpu/kalray_k1/mOS/ee_k1_asm.S \
  /cpu/kalray_k1/mOS/ee_k1_irq.c \
  /cpu/kalray_k1/mOS/ee_hal_init.c \
  /cpu/kalray_k1/mOS/eecfg.c \
  /common/utils/ee_assert.c \
  /hal/kernel/ee_hal_std_change_context.c \
  /kernel/mOS/oo/${K1_ERIKA_SCHEDULER}/ee_scheduler.c \
  /kernel/mOS/oo/ee_api_dynamic.c \
  /kernel/mOS/oo/ee_api_extension.c \
  /kernel/mOS/oo/ee_api_osek.c \
  /kernel/mOS/oo/ee_scheduler_entry_points.c \
  /kernel/mOS/k1/ee_api_k1.c \
  /kernel/mOS/k1/${K1_ERIKA_SCHEDULER}/ee_kernel_k1.c \
  )

K1_OMP_SRCS := ${K1_ERIKA_FILES}/libgomp/root.c\
  ${K1_ERIKA_FILES}/libgomp/context.S\
  ${K1_ERIKA_FILES}/libgomp/offload.c
 
K1_ERIKA_INCLUDE_DIRS := ${K1_ERIKA_APP_DIR} \
  $(addprefix ${K1_ERIKA_PKG2}, /common/compilers /common/compilers/gcc\
    /common/utils /cpu/kalray_k1/mOS /hal/kernel /kernel/mOS/k1\
    /kernel/mOS/oo)\
  ${K1_ERIKA_FILES}/libgomp ${K1_ERIKA_FILES}/libpsocoffload\
  ${K1_ERIKA_FILES}/libgomp/config ${K1_ERIKA_FILES}/libgomp/config/k1

K1_CLUSTER_INCLUDE_DIRS_OPT := $(addprefix -I, $(K1_ERIKA_INCLUDE_DIRS) $(K1_ERIKA_APP_INCLUDE_DIRS))
    
K1_CFLAGS := -mcluster=node -mboard=${board} -mhypervisor -std=c99\
  -march=${arch} -mos=bare -save-temps=obj -Wall -g3 -O\
  ${K1_ERIKA_SCHEDULER_OPTIONS}\
  -Winit-self -Wswitch-default -Wfloat-equal -Wundef -Wshadow\
  -Wuninitialized -Wno-unused-function\
  -I. $(K1_CLUSTER_INCLUDE_DIRS_OPT) -DK1_ERIKA_PLATFORM_MOS\
  ${K1_LIBGOMP_DEFS}
else
$(error Unknown Sub-Platform: ${K1_ERIKA_PLATFORM})
endif
endif

# Default Release Folder ERIKA +  libgomp + libpsocoffload
K1_PSOCTOOLS ?= psoctools

# Define the C and ASM object files 
K1_ERIKA_OBJS := $(K1_ERIKA_SRCS:.c=.o)
K1_ERIKA_OBJS := $(K1_ERIKA_OBJS:.S=.o)
K1_ERIKA_OBJS := $(addprefix $(K1_OUTPTDIR)/, $(notdir $(K1_ERIKA_OBJS)))

# Define the C and ASM object files 
K1_OMP_OBJS := $(K1_OMP_SRCS:.c=.o)
K1_OMP_OBJS := $(K1_OMP_OBJS:.S=.o)
K1_OMP_OBJS := $(addprefix $(K1_OUTPTDIR)/, $(notdir $(K1_OMP_OBJS)))

K1_DEP_OPT = -MD -MF $(subst .o,.d,$(@))

K1_SRCS := $(K1_ERIKA_SRCS) $(K1_OMP_SRCS)

K1_OBJS := $(K1_ERIKA_OBJS) $(K1_OUTPTDIR)/root.o ${K1_ERIKA_FILES}/offload.o

#Dependencies
K1_DEPS := $(K1_OBJS:.o=.d)

K1_ERIKA_LIBS_NAME ?= ee

K1_ERIKA_LIB := $(K1_LIB_PREFIX)$(K1_ERIKA_LIBS_NAME)$(K1_LIB_SUFFIX)

K1_IO_LDFLAGS := -mcluster=ioddr -march=${arch} -mboard=${board} -mos=rtems\
  -lm  -lmppapower -lmppanoc -lmpparouting -lpcie_queue -lmppaipc\
  -Wl,-Map="${K1_OUTPTDIR}/build/${K1_IO_APP_NAME}.map

K1_ARFLAGS := rcs

# Add all the sources path to %.c and %.S vpath
vpath %.c $(dir $(K1_SRCS)) libpsocoffload libgomp
vpath %.S $(dir $(K1_SRCS)) libgomp

.PHONY: all clean

all: $(K1_OUTPTDIR) $(K1_OUTPTDIR)/$(K1_ERIKA_LIB) $(K1_OUTPTDIR)/libpsocomp.a\
  $(K1_OUTPTDIR)/libpsocoffload.a $(K1_PSOCTOOLS)

ifneq ($(MAKECMDGOALS),clean)
-include $(K1_DEPS)
endif

$(K1_OUTPTDIR):
	mkdir -p $@

# Objects depend on directories, but they are not remade if directories change
$(K1_OBJS): | $(K1_OUTPTDIR)

$(K1_OUTPTDIR)/$(K1_ERIKA_LIB): $(K1_ERIKA_OBJS)
	$(K1_TOOLCHAIN_DIR)/bin/k1-ar $(K1_ARFLAGS) $@ $^

$(K1_OUTPTDIR)/libpsocomp.a: $(K1_OMP_OBJS)
	$(K1_TOOLCHAIN_DIR)/bin/k1-ar $(K1_ARFLAGS) $@ $^
	
$(K1_OUTPTDIR)/libpsocoffload.a: $(K1_OUTPTDIR)/offload_support.o
	$(K1_TOOLCHAIN_DIR)/bin/k1-ar $(K1_ARFLAGS) $@ $^

$(K1_OUTPTDIR)/offload_support.o: offload_support.c
	$(K1_TOOLCHAIN_DIR)/bin/k1-gcc -mcluster=ioddr -mboard=${board} -mos=rtems\
  ${hypervisor-opt} -march=${arch} -std=c99 -Wall -Wno-unused-function\
  -save-temps=obj -g -O -I. $(K1_CLUSTER_INCLUDE_DIRS_OPT)\
  ${LIBGOMP_DEFS} ${K1_DEP_OPT} -c $< -o $@

$(K1_OUTPTDIR)/%.o: %.c
	$(K1_TOOLCHAIN_DIR)/bin/k1-gcc $(K1_CFLAGS) ${K1_DEP_OPT} -c $< -o $@

$(K1_OUTPTDIR)/%.o: %.S
	$(K1_TOOLCHAIN_DIR)/bin/k1-gcc $(K1_CFLAGS) ${K1_DEP_OPT} -c $< -o $@

clean:
	rm -rf $(K1_OUTPTDIR) $(K1_PSOCTOOLS)

$(K1_PSOCTOOLS): $(K1_OUTPTDIR) $(K1_OUTPTDIR)/$(K1_ERIKA_LIB) $(K1_OUTPTDIR)/libpsocomp.a $(K1_OUTPTDIR)/libpsocoffload.a
	@echo "Creating $@ drop folder"
	@mkdir -p $@
	@mkdir -p $@/include
	@mkdir -p $@/lib
	@cp -r $(K1_OUTPTDIR)/$(K1_ERIKA_LIB) $@/lib
	@cp -r $(K1_OUTPTDIR)/libpsocomp.a $@/lib
	@cp -r $(K1_OUTPTDIR)/libpsocoffload.a $@/lib
	@find ${K1_ERIKA_PKG2} -type d -regex ".*examples/*.*" -prune -o -type d -regex ".*bsp/*.*" -prune -o -regex ".*\.[h][cpx]*" -type f -exec cp {} $@/include \;
	@find ${K1_ERIKA_FILES}/libgomp -type d -regex ".*test.*/*.*" -prune -o -regex ".*\.[h][cpx]*" -type f -exec cp {} $@/include \;
	@find ${K1_ERIKA_FILES}/libpsocoffload -regex ".*\.[h][cpx]*" -type f -exec cp {} $@/include \;
