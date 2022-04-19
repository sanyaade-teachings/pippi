# Project Name
TARGET = chime

# Sources
CPP_SOURCES = chime.cpp

# libpippi sources
C_SOURCES += ../../../libpippi/src/pippicore.c
C_SOURCES += ../../../libpippi/src/oscs.phasor.c
C_SOURCES += ../../../libpippi/src/oscs.table.c

# libpippi flags: enable static memory alloc 
# & use single precision floats.
C_DEFS += -DLP_STATIC
C_DEFS += -DLP_FLOAT

# Library Locations
LIBDAISY_DIR = ../../libDaisy

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

