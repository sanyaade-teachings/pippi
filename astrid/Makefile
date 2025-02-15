PYLIB := $(shell python -c "from distutils import sysconfig; print(sysconfig.get_config_var('LIBRARY')[3:-2])")

LPDIR = ../libpippi

LPSOURCES = $(LPDIR)/vendor/fft/fft.c \
	${LPDIR}/vendor/linenoise/linenoise.c \
	${LPDIR}/vendor/libpqueue/src/pqueue.c \
	${LPDIR}/vendor/lmdb/libraries/liblmdb/mdb.c \
	${LPDIR}/vendor/lmdb/libraries/liblmdb/midl.c \
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
LPINCLUDES = -Isrc -I${LPDIR}../pippi -I$(LPDIR)/vendor -I$(LPDIR)/src -I${LPDIR}/vendor/linenoise -I${LPDIR}/vendor/libpqueue/src -I${LPDIR}/vendor/lmdb/libraries/liblmdb
LPDBINCLUDES = -I${LPDIR}/vendor/sqlite3
LPFLAGS = -g -std=gnu2x -Wall -Wextra -pedantic -O0 -DNOPYTHON
LPLIBS = -lm -ldl -lpthread -lrt -ljack -lasound

default: littlefield

clean:
	rm -rf build/*
	rm -f cython/*.c
	rm -f cython/*.h
	rm -rf cython/build/*

	rm -f python/cython/*.c
	rm -f python/cython/*.h
	rm -rf python/src
	rm -f python/*.so

#astrid-midilog:
#	mkdir -p build
#
#	echo "Building astrid midilog...";
#	python cython/setup.py build_ext --inplace --build-lib=python

astrid-ipc:
	echo "Building astrid ipc tools...";

	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/getvoiceid.c $(LPLIBS) -o build/astrid-getvoiceid
	#gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/createsharedbuffer.c $(LPLIBS) -o build/astrid-createsharedbuffer
	#gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/ipccreatevalue.c $(LPLIBS) -o build/astrid-ipccreatevalue
	#gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/ipcgetvalue.c $(LPLIBS) -o build/astrid-ipcgetvalue
	#gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/ipcsetvalue.c $(LPLIBS) -o build/astrid-ipcsetvalue
	#gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/ipcdestroyvalue.c $(LPLIBS) -o build/astrid-ipcdestroyvalue
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/sampler.c $(LPLIBS) -o build/astrid-sampler

astrid-q:
	echo "Building astrid queue reader...";
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/qreader.c $(LPLIBS) -o build/astrid-qreader
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/qserver.c $(LPLIBS) -o build/astrid-qserver
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/msg.c $(LPLIBS) -o build/astrid-msg

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
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/setdeviceid.c $(LPLIBS) -o build/astrid-setdeviceid

astrid-voicestatus:
	mkdir -p build

	echo "Building astrid voice status...";
	gcc $(LPFLAGS) -DLPSESSIONDB $(LPINCLUDES) $(LPDBINCLUDES) $(LPSOURCES) $(LPDBSOURCES) src/astrid.c src/voicestatus.c $(LPLIBS) -lncurses -o build/astrid-voicestatus

astrid-serial-tools:
	mkdir -p build

	echo "Building astrid serial listener & friends...";
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/seriallistener.c $(LPLIBS) -o build/astrid-seriallistener
	gcc $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/msgarg.c $(LPLIBS) -o build/astrid-msgarg

	#echo "Building astrid serial parser test program...";
	#$(CC) $(LPFLAGS) -fsanitize=address -fsanitize=leak -fno-omit-frame-pointer $(LPINCLUDES) $(LPSOURCES) src/astrid.c orc/serialparser.c $(LPLIBS) -o build/astrid-serialparser

astrid-bufstr:
	mkdir -p build

	echo "Building astrid bufstr...";
	$(CC) $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c orc/bufstr.c $(LPLIBS) -o build/astrid-bufstr

astrid-paramset:
	mkdir -p build

	echo "Building astrid paramset...";
	$(CC) $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c orc/paramset.c $(LPLIBS) -o build/astrid-paramset

littlefield:
	mkdir -p build

	echo "Building littlefield...";
	#$(CC) $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/getmidideviceid.c $(LPLIBS) -o build/getmidideviceid
	#$(CC) $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c src/hashtest.c $(LPLIBS) -o build/hashtest

	#$(CC) $(LPFLAGS) -fsanitize=address -fsanitize=leak -fno-omit-frame-pointer $(LPINCLUDES) $(LPSOURCES) src/astrid.c orc/littlefield.c $(LPLIBS) -o build/littlefield
	$(CC) $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c orc/littlefield.c $(LPLIBS) -o build/littlefield
	#$(CC) $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c orc/b4wego.c $(LPLIBS) -o build/b4wego
	sudo cp build/littlefield /usr/local/bin/
	#sudo cp build/b4wego /usr/local/bin/


astrid-pulsar:
	mkdir -p build

	echo "Building astrid pulsar...";

	$(CC) $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c orc/pulsar.c $(LPLIBS) -o build/astrid-pulsar

simple:
	mkdir -p build

	$(CC) $(LPFLAGS) -fsanitize=address -fsanitize=leak -fno-omit-frame-pointer $(LPINCLUDES) $(LPSOURCES) src/astrid.c orc/simple.c $(LPLIBS) -o build/simple-san
	$(CC) $(LPFLAGS) $(LPINCLUDES) $(LPSOURCES) src/astrid.c orc/simple.c $(LPLIBS) -o build/simple


build: clean astrid-q astrid-serial-tools astrid-ipc astrid-devices astrid-midimap astrid-pulsar astrid-simple

install: 
	cp build/astrid-* /usr/local/bin/
