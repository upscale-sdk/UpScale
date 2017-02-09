.DEFAULT_GOAL := all

ifneq (${MAKEFILE_CUSTOM_RULES},)
include ${MAKEFILE_CUSTOM_RULES}
endif
$(info MAKEFILE_CUSTOM_RULES=${MAKEFILE_CUSTOM_RULES})

ifeq (${K1_ERIKA_FILES},)
export K1_ERIKA_FILES := $(shell dirname $(realpath Makefile))
endif
$(info K1_ERIKA_FILES=${K1_ERIKA_FILES})

ifeq (${K1_ERIKA_PLATFORM},)
K1_ERIKA_PLATFORM ?= p-socrates
endif
$(info K1_ERIKA_PLATFORM=${K1_ERIKA_PLATFORM})

-include $(K1_ERIKA_PLATFORM).mk
