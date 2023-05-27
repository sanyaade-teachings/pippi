.PHONY: cython astrid follow-log

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
	$(LPDIR)/src/spectral.c \
	$(LPDIR)/src/pippicore.c

LPDBSOURCES = $(LPDIR)/vendor/sqlite3/sqlite3.c
LPINCLUDES = -Isrc -I$(LPDIR)/vendor -I$(LPDIR)/src
LPDBINCLUDES = -I${LPDIR}/vendor/sqlite3
LPFLAGS = -g -std=gnu2x -Wall -Wextra -pedantic
LPLIBS = -lm -ldl -lpthread

clean:
	rm -rf build/*

astrid-renderer:
	mkdir -p build

	echo "Building astrid renderer...";
	cython cython/cyrenderer.pyx
	$(CC) -g -std=gnu2x -Wall -Wextra $(LPINCLUDES) `python3-config --cflags` -Icython -l$(PYLIB) $(LPLIBS) src/astrid.c src/renderer.c $(LPSOURCES) cython/cyrenderer.c -o build/astrid-renderer

astrid-ipc:
	echo "Building astrid ipc tools...";
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/makecounter.c $(LPLIBS) -o build/astrid-makecounter
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/rmcounter.c $(LPLIBS) -o build/astrid-rmcounter
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/getvoiceid.c $(LPLIBS) -o build/astrid-getvoiceid

	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/createsharedbuffer.c $(LPLIBS) -o build/astrid-createsharedbuffer

astrid-sessiondb:
	echo "Building astrid session db tools...";
	gcc $(LPFLAGS) -DLPSESSIONDB $(LPINCLUDES) $(LPDBINCLUDES) $(LPSOURCES) $(LPDBSOURCES) src/astrid.c src/createsessiondb.c $(LPLIBS) -o build/astrid-createsessiondb

astrid-q:
	echo "Building astrid queue reader...";
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/qreader.c $(LPLIBS) -o build/astrid-qreader
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/qserver.c $(LPLIBS) -o build/astrid-qserver
	gcc $(LPFLAGS) -DLPSESSIONDB $(LPINCLUDES) $(LPDBINCLUDES) $(LPSOURCES) $(LPDBSOURCES) src/astrid.c src/qmessage.c $(LPLIBS) -o build/astrid-qmessage

astrid-renderer-macos:
	mkdir -p build

	echo "Building astrid renderer...";
	cython cython/cyrenderer.pyx
	$(CC) -g -std=gnu2x -Wall -Wextra $(LPINCLUDES) `python3-config --cflags --ldflags` -Icython src/astrid.c src/renderer.c $(LPSOURCES) cython/cyrenderer.c -l$(PYLIB) $(LPLIBS) -o build/astrid-renderer

astrid-dac:
	mkdir -p build

	echo "Building astrid dac...";
	gcc $(LPFLAGS) -DLPSESSIONDB $(LPINCLUDES) $(LPDBINCLUDES) $(LPSOURCES) $(LPDBSOURCES) src/astrid.c src/dac.c $(LPLIBS) -lhiredis -o build/astrid-dac

astrid-devices:
	mkdir -p build

	echo "Building astrid device tools...";
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/getdeviceids.c $(LPLIBS) -o build/astrid-getdeviceids
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/setdeviceid.c $(LPLIBS) -o build/astrid-setdeviceid

astrid-voice-status:
	mkdir -p build

	echo "Building astrid voice status...";
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/voicestatus.c $(LPLIBS) -lncurses -o build/astrid-voice-status

astrid-adc:
	mkdir -p build

	echo "Building astrid adc...";
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/adc.c $(LPLIBS) -o build/astrid-adc

astrid-seriallistener:
	mkdir -p build

	echo "Building astrid serial listener...";
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/seriallistener.c $(LPLIBS) -o build/astrid-seriallistener

follow-log:
ifeq ($(shell uname),Darwin)
	log stream --predicate 'subsystem == "astrid"'
else
	journalctl -xf --output=json | python -u python/colorize_logs.py
endif

build: astrid-dac astrid-adc astrid-renderer astrid-q astrid-seriallistener astrid-counter astrid-devices

macos: astrid-dac astrid-adc astrid-renderer-macos astrid-q astrid-seriallistener astrid-counter astrid-devices

