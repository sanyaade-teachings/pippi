# Project Name
TARGET = pulsar3

# Sources
CPP_SOURCES = pulsar3.cpp

# libpippi sources
C_SOURCES += ../../libpippi/src/pippicore.c
C_SOURCES += ../../libpippi/src/oscs.sine.c
C_SOURCES += ../../libpippi/src/oscs.pulsar.c

C_DEFS += -DLP_STATIC
C_DEFS += -DLP_FLOAT

# Library Locations
LIBDAISY_DIR = ../DaisyExamples/libdaisy
DAISYSP_DIR = ../DaisyExamples/DaisySP

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

