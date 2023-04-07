.PHONY: cython astrid

default: build

PYLIB := $(shell python -c "from distutils import sysconfig; print(sysconfig.get_config_var('LIBRARY')[3:-2])")

LPDIR = ../libpippi

LPSOURCES = $(LPDIR)/vendor/fft/fft.c \
    $(LPDIR)/src/fx.softclip.c \
	$(LPDIR)/src/oscs.bln.c \
	$(LPDIR)/src/oscs.node.c \
	$(LPDIR)/src/oscs.phasor.c \
	$(LPDIR)/src/oscs.sine.c \
	$(LPDIR)/src/oscs.pulsar.c \
	$(LPDIR)/src/oscs.shape.c \
	$(LPDIR)/src/oscs.tape.c \
	$(LPDIR)/src/oscs.table.c \
	$(LPDIR)/src/oscs.tukey.c \
	$(LPDIR)/src/microsound.c \
	$(LPDIR)/src/mir.c \
	$(LPDIR)/src/soundfile.c \
	$(LPDIR)/src/scheduler.c \
	$(LPDIR)/src/spectral.c \
	$(LPDIR)/src/pippicore.c

LPINCLUDES = -Isrc -I$(LPDIR)/vendor -I$(LPDIR)/src
LPFLAGS = -g -std=gnu2x -Wall -Wextra -pedantic
LPLIBS = -lm -ldl -lpthread

clean:
	rm -rf build/*

astrid-renderer:
	mkdir -p build

	echo "Building astrid renderer...";
	cython cython/cyrenderer.pyx
	$(CC) -g -std=gnu2x -Wall -Wextra $(LPINCLUDES) `python3-config --cflags` -Icython -l$(PYLIB) $(LPLIBS) src/astrid.c src/renderer.c $(LPSOURCES) cython/cyrenderer.c -o build/renderer

astrid-q:
	echo "Building astrid queue reader...";
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/qreader.c $(LPLIBS) -o build/qreader
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/qserver.c $(LPLIBS) -o build/qserver
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/qmessage.c $(LPLIBS) -o build/qmessage

astrid-renderer-macos:
	mkdir -p build

	echo "Building astrid renderer...";
	cython cython/cyrenderer.pyx
	$(CC) -g -std=gnu2x -Wall -Wextra $(LPINCLUDES) `python3-config --cflags --ldflags` -Icython src/astrid.c src/renderer.c $(LPSOURCES) cython/cyrenderer.c -l$(PYLIB) $(LPLIBS) -o build/renderer

astrid-dac:
	mkdir -p build

	echo "Building astrid dac...";
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/dac.c $(LPLIBS) -lhiredis -o build/dac

astrid-adc:
	mkdir -p build

	echo "Building astrid adc...";
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/adc.c $(LPLIBS) -o build/adc

astrid-seriallistener:
	mkdir -p build

	echo "Building astrid serial listener...";
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/seriallistener.c $(LPLIBS) -o build/seriallistener
	
build: astrid-dac astrid-adc astrid-renderer astrid-q astrid-seriallistener

macos: astrid-dac astrid-adc astrid-renderer-macos astrid-q astrid-seriallistener

