# Project Name
TARGET = bells

# Sources
CPP_SOURCES = bells.cpp

# libpippi sources
C_SOURCES += ../../../libpippi/src/pippicore.c
C_SOURCES += ../../../libpippi/src/oscs.pulsar.c

C_DEFS += -DLP_STATIC
C_DEFS += -DLP_FLOAT

# Library Locations
LIBDAISY_DIR = ../../libDaisy

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

CPPFLAGS += -Wdouble-promotion
