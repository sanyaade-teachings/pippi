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
LPLIBS = -lm -ldl -lpthread -lrt

clean:
	rm -rf build/*
	rm -f cython/*.c
	rm -f cython/*.h
	rm -rf cython/build/*

	rm -f python/cython/*.c
	rm -f python/cython/*.h
	rm -rf python/src
	rm -f python/*.so


astrid-renderer:
	mkdir -p build

	echo "Building astrid renderer...";
	cython cython/cyrenderer.pyx
	$(CC) -g -std=gnu2x -Wall -Wextra $(LPINCLUDES) `python3-config --cflags` -Icython -l$(PYLIB) $(LPLIBS) src/astrid.c src/renderer.c $(LPSOURCES) cython/cyrenderer.c -o build/astrid-renderer

astrid-midilog:
	mkdir -p build

	echo "Building astrid midilog...";
	python cython/setup.py build_ext --inplace --build-lib=python

astrid-ipc:
	echo "Building astrid ipc tools...";
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/makecounter.c $(LPLIBS) -o build/astrid-makecounter
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/rmcounter.c $(LPLIBS) -o build/astrid-rmcounter
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/getvoiceid.c $(LPLIBS) -o build/astrid-getvoiceid

	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/createsharedbuffer.c $(LPLIBS) -o build/astrid-createsharedbuffer
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/ipccreatevalue.c $(LPLIBS) -o build/astrid-ipccreatevalue
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/ipcgetvalue.c $(LPLIBS) -o build/astrid-ipcgetvalue
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/ipcsetvalue.c $(LPLIBS) -o build/astrid-ipcsetvalue
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/ipcdestroyvalue.c $(LPLIBS) -o build/astrid-ipcdestroyvalue

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

astrid-seq:
	mkdir -p build

	echo "Building astrid seq...";
	gcc $(LPFLAGS) $(LPINCLUDES) -I${LPDIR}/vendor/libpqueue/src $(LPSOURCES) ${LPDIR}/vendor/libpqueue/src/pqueue.c src/astrid.c src/seq.c $(LPLIBS) -o build/astrid-seq

astrid-midimap:
	mkdir -p build

	echo "Building astrid midimap tools...";
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/addnotemap.c $(LPLIBS) -o build/astrid-addnotemap
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/rmnotemap.c $(LPLIBS) -o build/astrid-rmnotemap
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/printnotemap.c $(LPLIBS) -o build/astrid-printnotemap
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/triggernotemap.c $(LPLIBS) -o build/astrid-triggernotemap

astrid-devices:
	mkdir -p build

	echo "Building astrid device tools...";
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/getdeviceids.c $(LPLIBS) -o build/astrid-getdeviceids
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/setdeviceid.c $(LPLIBS) -o build/astrid-setdeviceid

astrid-voicestatus:
	mkdir -p build

	echo "Building astrid voice status...";
	gcc $(LPFLAGS) -DLPSESSIONDB $(LPINCLUDES) $(LPDBINCLUDES) $(LPSOURCES) $(LPDBSOURCES) src/astrid.c src/voicestatus.c $(LPLIBS) -lncurses -o build/astrid-voicestatus

astrid-adc:
	mkdir -p build

	echo "Building astrid adc and tools...";
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/adc.c $(LPLIBS) -o build/astrid-adc
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/lpadccreate.c $(LPLIBS) -o build/astrid-adccreate
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/lpadcwriteval.c $(LPLIBS) -o build/astrid-adcwriteval
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/lpadcwritewav.c $(LPLIBS) -o build/astrid-adcwritewav
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/lpadcsave.c $(LPLIBS) -o build/astrid-adcsave
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/lpadcread1s.c $(LPLIBS) -o build/astrid-adcread1s
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/lpadcdestroy.c $(LPLIBS) -o build/astrid-adcdestroy

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

build: clean astrid-dac astrid-adc astrid-renderer astrid-q astrid-seriallistener astrid-ipc astrid-devices astrid-voicestatus astrid-midilog astrid-seq astrid-midimap

macos: clean astrid-dac astrid-adc astrid-renderer-macos astrid-q astrid-seriallistener astrid-ipc astrid-devices astrid-voicestatus astrid-midilog astrid-seq

