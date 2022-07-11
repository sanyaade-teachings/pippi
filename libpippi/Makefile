.PHONY: all cython examples

default: all

PYLIB := $(shell python -c "from distutils import sysconfig; print(sysconfig.get_config_var('LIBRARY')[3:-2])")

ALLSOURCES = vendor/fft/fft.c \
	src/oscs.bln.c \
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

FLAGS = -g -std=gnu11 -Werror -Wall -Wextra -pedantic -lm -Isrc -Ivendor

clean:
	rm build/*
	rm renders/*.wav

astrid-renderer:
	mkdir -p build renders

	echo "Building astrid renderer...";
	cython cython/cyrenderer.pyx
	gcc -g -Wall -Wextra -ldl -lpthread -lm -lhiredis -Isrc -Ivendor `python3-config --cflags` -Icython -l$(PYLIB) src/renderer.c $(ALLSOURCES) cython/cyrenderer.c -o build/renderer

astrid-dac:
	mkdir -p build renders

	echo "Building astrid dac...";
	gcc -g -Wall -Wextra -ldl -lpthread -lm -lhiredis -Isrc -Ivendor $(ALLSOURCES) src/astrid.c src/dac.c -o build/dac

astrid-adc:
	mkdir -p build renders

	echo "Building astrid adc...";
	gcc -g -Wall -Wextra -ldl -lpthread -lm -lhiredis -Isrc -Ivendor $(ALLSOURCES) src/adc.c -o build/adc
	
astrid: astrid-dac astrid-adc astrid-renderer

silencer:
	mkdir -p build renders

	echo "Building silencer...";
	gcc $(FLAGS) tools/silencer.c $(ALLSOURCES) -o build/silencer

osc-examples:
	mkdir -p build renders

	echo "Building pulsarosc.c example...";
	gcc $(FLAGS) examples/pulsarosc.c $(ALLSOURCES) -o build/pulsarosc

	echo "Building phasorosc.c example...";
	gcc $(FLAGS) examples/phasorosc.c $(ALLSOURCES) -o build/phasorosc

	echo "Building sineosc.c example...";
	gcc $(FLAGS) examples/sineosc.c $(ALLSOURCES) -o build/sineosc

	echo "Building tukeyosc.c example...";
	gcc $(FLAGS) examples/tukeyosc.c $(ALLSOURCES) -o build/tukeyosc

	echo "Building tableosc.c example...";
	gcc $(FLAGS) examples/tableosc.c $(ALLSOURCES) -o build/tableosc

	echo "Building blnosc.c example...";
	gcc $(FLAGS) examples/blnosc.c $(ALLSOURCES) -o build/blnosc

	echo "Building tapeosc.c example...";
	gcc $(FLAGS) examples/tapeosc.c $(ALLSOURCES) -o build/tapeosc

	echo "Building tapeosc2.c example...";
	gcc $(FLAGS) examples/tapeosc2.c $(ALLSOURCES) -o build/tapeosc2

	echo "Building additive_synthesis.c example...";
	gcc $(FLAGS) examples/additive_synthesis.c $(ALLSOURCES) -o build/additive_synthesis

warble-examples:
	mkdir -p build renders

	echo "Building warble.c example...";
	gcc $(FLAGS) examples/warble.c $(ALLSOURCES) -o build/warble

buffer-examples:
	mkdir -p build renders

	echo "Building ring_buffer.c example...";
	gcc $(FLAGS) examples/ring_buffer.c $(ALLSOURCES) -o build/ring_buffer

	echo "Building slicing.c example...";
	gcc $(FLAGS) examples/slicing.c $(ALLSOURCES) -o build/slicing

	echo "Building slicing2.c example...";
	gcc $(FLAGS) examples/slicing2.c $(ALLSOURCES) -o build/slicing2

	echo "Building resample_buffer.c example...";
	gcc $(FLAGS) examples/resample_buffer.c $(ALLSOURCES) -o build/resample_buffer


mir-examples:
	mkdir -p build renders

	echo "Building onset_detector.c example...";
	gcc $(FLAGS) examples/onset_detector.c $(ALLSOURCES) -o build/onset_detector

	echo "Building pitch_tracker.c example...";
	gcc $(FLAGS) examples/pitch_tracker.c $(ALLSOURCES) -o build/pitch_tracker

embedded-examples:
	mkdir -p build renders

	echo "Building memory_pool.c example...";
	gcc $(FLAGS) -Wdouble-promotion -DLP_FLOAT -DLP_STATIC examples/memory_pool.c src/oscs.sine.c src/soundfile.c src/pippicore.c -o build/memorypool

microsound-examples:
	mkdir -p build renders

	echo "Building crossings.c example...";
	gcc $(FLAGS) examples/crossings.c $(ALLSOURCES) -o build/crossings

	echo "Building crossings2.c example...";
	gcc $(FLAGS) examples/crossings2.c $(ALLSOURCES) -o build/crossings2

	echo "Building scheduler.c example...";
	gcc $(FLAGS) examples/scheduler.c $(ALLSOURCES) -o build/scheduler

	echo "Building grainformation.c example...";
	gcc $(FLAGS) examples/grainformation.c $(ALLSOURCES) -o build/grainformation

	echo "Building grainformation2.c example...";
	gcc $(FLAGS) examples/grainformation2.c $(ALLSOURCES) -o build/grainformation2

convolution-examples:
	mkdir -p build renders

	echo "Building convolution.c example...";
	gcc $(FLAGS) examples/convolution.c $(ALLSOURCES) -o build/convolution

	echo "Building convolution2.c example...";
	gcc $(FLAGS) examples/convolution2.c $(ALLSOURCES) -o build/convolution2

soundfile-examples:
	mkdir -p build renders

	echo "Building readsoundfile.c example...";
	gcc $(FLAGS) examples/readsoundfile.c $(ALLSOURCES) -o build/readsoundfile

render:
	mkdir -p build renders

	echo "Rendering examples..."
	./scripts/render_examples.sh

examples: soundfile-examples convolution-examples microsound-examples embedded-examples mir-examples buffer-examples osc-examples
all: astrid examples
