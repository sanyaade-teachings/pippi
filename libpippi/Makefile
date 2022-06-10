.PHONY: all cython examples

default: all

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

astrid:
	echo "Building astrid renderer...";
	cython cython/renderer.pyx
	gcc -g -Wall -Wextra -ldl -lpthread -lm -Isrc -Ivendor -Icython -I/usr/include/python3.9 -lpython3.9 examples/renderer.c $(ALLSOURCES) src/astrid.c cython/renderer.c -o build/renderer

astrid-adc:
	echo "Building astrid adc...";
	gcc -g -Wall -Wextra -ldl -lpthread -lm -lhiredis -Isrc -Ivendor $(ALLSOURCES) src/adc.c -o build/adc

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

buffer-examples:
	mkdir -p build renders

	echo "Building ring_buffer.c example...";
	gcc $(FLAGS) examples/ring_buffer.c $(ALLSOURCES) -o build/ring_buffer

	echo "Building slicing.c example...";
	gcc $(FLAGS) examples/slicing.c $(ALLSOURCES) -o build/slicing

	echo "Building slicing2.c example...";
	gcc $(FLAGS) examples/slicing2.c $(ALLSOURCES) -o build/slicing2

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
	echo "Building convolution.c example...";
	gcc $(FLAGS) examples/convolution.c $(ALLSOURCES) -o build/convolution

	echo "Building convolution2.c example...";
	gcc $(FLAGS) examples/convolution2.c $(ALLSOURCES) -o build/convolution2

soundfile-examples:
	echo "Building readsoundfile.c example...";
	gcc $(FLAGS) examples/readsoundfile.c $(ALLSOURCES) -o build/readsoundfile

render:
	echo "Rendering examples..."
	./scripts/render_examples.sh

examples: soundfile-examples convolution-examples microsound-examples embedded-examples mir-examples buffer-examples osc-examples
all: astrid examples render
