.PHONY: cython examples

default: astrid examples

PYLIB := $(shell python -c "from distutils import sysconfig; print(sysconfig.get_config_var('LIBRARY')[3:-2])")

ALLSOURCES = vendor/fft/fft.c \
    src/fx.softclip.c \
	src/oscs.bln.c \
	src/oscs.node.c \
	src/oscs.phasor.c \
	src/oscs.sine.c \
	src/oscs.pulsar.c \
	src/oscs.shape.c \
	src/oscs.tape.c \
	src/oscs.table.c \
	src/oscs.tukey.c \
	src/microsound.c \
	src/mir.c \
	src/soundfile.c \
	src/scheduler.c \
	src/spectral.c \
	src/pippicore.c

FLAGS = -g -std=gnu2x -Werror -Wall -Wextra -pedantic -Isrc -Ivendor
LDFLAGS = -lm

clean:
	rm -rf build/*
	rm renders/*.wav

astrid-renderer:
	mkdir -p build renders

	echo "Building astrid renderer...";
	cython cython/cyrenderer.pyx
	gcc -g -Wall -Wextra -ldl -lpthread -lm -Isrc -Ivendor `python3-config --cflags` -Icython -l$(PYLIB) src/astrid.c src/renderer.c $(ALLSOURCES) cython/cyrenderer.c -o build/renderer

astrid-q:
	echo "Building astrid queue reader...";
	gcc -g -Wall -Wextra -Isrc -Ivendor $(ALLSOURCES) src/astrid.c src/qreader.c -ldl -lpthread -lm -o build/qreader
	gcc -g -Wall -Wextra -Isrc -Ivendor $(ALLSOURCES) src/astrid.c src/qserver.c -ldl -lpthread -lm -o build/qserver
	gcc -g -Wall -Wextra -Isrc -Ivendor $(ALLSOURCES) src/astrid.c src/qmessage.c -ldl -lpthread -lm -o build/qmessage

astrid-renderer-macos:
	mkdir -p build renders

	echo "Building astrid renderer...";
	cython cython/cyrenderer.pyx
	gcc -g -Wall -Wextra -Isrc -Ivendor `python3-config --cflags --ldflags` -Icython -l$(PYLIB) src/astrid.c src/renderer.c $(ALLSOURCES) cython/cyrenderer.c -ldl -lpthread -lm -o build/renderer

astrid-dac:
	mkdir -p build renders

	echo "Building astrid dac...";
	gcc -g -Wall -Wextra -Isrc -Ivendor $(ALLSOURCES) src/astrid.c src/dac.c -ldl -lpthread -lm -lhiredis -o build/dac

astrid-adc:
	mkdir -p build renders

	echo "Building astrid adc...";
	gcc -g -Wall -Wextra -Isrc -Ivendor $(ALLSOURCES) src/astrid.c src/adc.c -ldl -lpthread -lm -o build/adc

astrid-seriallistener:
	mkdir -p build renders

	echo "Building astrid serial listener...";
	gcc -g -Wall -Wextra -Isrc -Ivendor $(ALLSOURCES) src/astrid.c src/seriallistener.c -ldl -lpthread -lm -o build/seriallistener
	
astrid: astrid-dac astrid-adc astrid-renderer astrid-q astrid-seriallistener

astrid-macos: astrid-dac astrid-adc astrid-renderer-macos astrid-q astrid-seriallistener

silencer:
	mkdir -p build renders

	echo "Building silencer...";
	gcc $(FLAGS) tools/silencer.c $(ALLSOURCES) $(LDFLAGS) -o build/silencer
	
fx-examples:
	mkdir -p build renders

	echo "Building fx.softclip.c example...";
	gcc $(FLAGS) examples/fx.softclip.c $(ALLSOURCES) $(LDFLAGS) -o build/fxsoftclip

osc-examples:
	mkdir -p build renders

	echo "Building pulsarosc.c example...";
	gcc $(FLAGS) examples/pulsarosc.c $(ALLSOURCES) $(LDFLAGS) -o build/pulsarosc

	echo "Building phasorosc.c example...";
	gcc $(FLAGS) examples/phasorosc.c $(ALLSOURCES) $(LDFLAGS) -o build/phasorosc

	echo "Building nodeosc.c example...";
	gcc $(FLAGS) examples/nodeosc.c $(ALLSOURCES) $(LDFLAGS) -o build/nodeosc

	echo "Building sineosc.c example...";
	gcc $(FLAGS) examples/sineosc.c $(ALLSOURCES) $(LDFLAGS) -o build/sineosc

	echo "Building tukeyosc.c example...";
	gcc $(FLAGS) examples/tukeyosc.c $(ALLSOURCES) $(LDFLAGS) -o build/tukeyosc

	echo "Building tableosc.c example...";
	gcc $(FLAGS) examples/tableosc.c $(ALLSOURCES) $(LDFLAGS) -o build/tableosc

	echo "Building blnosc.c example...";
	gcc $(FLAGS) examples/blnosc.c $(ALLSOURCES) $(LDFLAGS) -o build/blnosc

	echo "Building tapeosc.c example...";
	gcc $(FLAGS) examples/tapeosc.c $(ALLSOURCES) $(LDFLAGS) -o build/tapeosc

	echo "Building tapeosc2.c example...";
	gcc $(FLAGS) examples/tapeosc2.c $(ALLSOURCES) $(LDFLAGS) -o build/tapeosc2

	echo "Building additive_synthesis.c example...";
	gcc $(FLAGS) examples/additive_synthesis.c $(ALLSOURCES) $(LDFLAGS) -o build/additive_synthesis

warble-examples:
	mkdir -p build renders

	echo "Building warble.c example...";
	gcc $(FLAGS) examples/warble.c $(ALLSOURCES) $(LDFLAGS) -o build/warble

buffer-examples:
	mkdir -p build renders

	echo "Building ring_buffer.c example...";
	gcc $(FLAGS) examples/ring_buffer.c $(ALLSOURCES) $(LDFLAGS) -o build/ring_buffer

	echo "Building slicing.c example...";
	gcc $(FLAGS) examples/slicing.c $(ALLSOURCES) $(LDFLAGS) -o build/slicing

	echo "Building slicing2.c example...";
	gcc $(FLAGS) examples/slicing2.c $(ALLSOURCES) $(LDFLAGS) -o build/slicing2

	echo "Building resample_buffer.c example...";
	gcc $(FLAGS) examples/resample_buffer.c $(ALLSOURCES) $(LDFLAGS) -o build/resample_buffer

	echo "Building reverse_buffer.c example...";
	gcc $(FLAGS) examples/reverse_buffer.c $(ALLSOURCES) $(LDFLAGS) -o build/reverse_buffer

	echo "Building plotbuffer.c example...";
	gcc $(FLAGS) examples/plotbuffer.c $(ALLSOURCES) $(LDFLAGS) -o build/plotbuffer


mir-examples:
	mkdir -p build renders

	echo "Building onset_detector.c example...";
	gcc $(FLAGS) examples/onset_detector.c $(ALLSOURCES) $(LDFLAGS) -o build/onset_detector

	echo "Building pitch_tracker.c example...";
	gcc $(FLAGS) examples/pitch_tracker.c $(ALLSOURCES) $(LDFLAGS) -o build/pitch_tracker

embedded-examples:
	mkdir -p build renders

	echo "Building memory_pool.c example...";
	gcc $(FLAGS) -Wdouble-promotion -DLP_FLOAT -DLP_STATIC examples/memory_pool.c src/oscs.sine.c src/soundfile.c src/pippicore.c $(LDFLAGS) -o build/memorypool

microsound-examples:
	mkdir -p build renders

	echo "Building crossings.c example...";
	gcc $(FLAGS) examples/crossings.c $(ALLSOURCES) $(LDFLAGS) -o build/crossings

	echo "Building crossings2.c example...";
	gcc $(FLAGS) examples/crossings2.c $(ALLSOURCES) $(LDFLAGS) -o build/crossings2

	echo "Building scheduler.c example...";
	gcc $(FLAGS) examples/scheduler.c $(ALLSOURCES) $(LDFLAGS) -o build/scheduler

	echo "Building grainformation.c example...";
	gcc $(FLAGS) examples/grainformation.c $(ALLSOURCES) $(LDFLAGS) -o build/grainformation

	echo "Building grainformation2.c example...";
	gcc $(FLAGS) examples/grainformation2.c $(ALLSOURCES) $(LDFLAGS) -o build/grainformation2

convolution-examples:
	mkdir -p build renders

	echo "Building convolution.c example...";
	gcc $(FLAGS) examples/convolution.c $(ALLSOURCES) $(LDFLAGS) -o build/convolution

	echo "Building convolution2.c example...";
	gcc $(FLAGS) examples/convolution2.c $(ALLSOURCES) $(LDFLAGS) -o build/convolution2

soundfile-examples:
	mkdir -p build renders

	echo "Building readsoundfile.c example...";
	gcc $(FLAGS) examples/readsoundfile.c $(ALLSOURCES) $(LDFLAGS) -o build/readsoundfile

	echo "Building readrawfile.c example...";
	gcc $(FLAGS) examples/readrawfile.c $(ALLSOURCES) $(LDFLAGS) -o build/readrawfile

render:
	mkdir -p build renders

	echo "Rendering examples..."
	./scripts/render_examples.sh

examples: soundfile-examples convolution-examples microsound-examples embedded-examples mir-examples buffer-examples osc-examples fx-examples
