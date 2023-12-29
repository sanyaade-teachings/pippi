.PHONY: examples render

default: examples render

LPSOURCES = vendor/fft/fft.c \
    src/fx.softclip.c \
	src/oscs.bln.c \
	src/oscs.node.c \
	src/oscs.phasor.c \
	src/oscs.sine.c \
	src/oscs.fract.c \
	src/oscs.pulsar.c \
	src/oscs.shape.c \
	src/oscs.tape.c \
	src/oscs.table.c \
	src/oscs.tukey.c \
	src/ugens.sine.c \
	src/ugens.tape.c \
	src/ugens.utils.c \
	src/microsound.c \
	src/mir.c \
	src/soundfile.c \
	src/spectral.c \
	src/pippicore.c

LPFLAGS = -g -std=gnu2x -Werror -Wall -Wextra -pedantic -Isrc -Ivendor
LPLIBS = -lm

clean:
	rm -rf build/*
	rm -rf renders/*.wav

silencer:
	mkdir -p build renders

	echo "Building silencer...";
	gcc $(LPFLAGS) tools/silencer.c $(LPSOURCES) $(LPLIBS) -o build/silencer
	
fx-examples:
	mkdir -p build renders

	echo "Building fx.softclip.c example...";
	gcc $(LPFLAGS) examples/fx.softclip.c $(LPSOURCES) $(LPLIBS) -o build/fxsoftclip

	echo "Building fx.crush.c example...";
	gcc $(LPFLAGS) examples/fx.crush.c $(LPSOURCES) $(LPLIBS) -o build/crush

osc-examples:
	mkdir -p build renders

	echo "Building pulsarosc.c example...";
	gcc $(LPFLAGS) examples/pulsarosc.c $(LPSOURCES) $(LPLIBS) -o build/pulsarosc

	echo "Building pulsarosc2.c example...";
	gcc $(LPFLAGS) examples/pulsarosc2.c $(LPSOURCES) $(LPLIBS) -o build/pulsarosc2

	echo "Building phasorosc.c example...";
	gcc $(LPFLAGS) examples/phasorosc.c $(LPSOURCES) $(LPLIBS) -o build/phasorosc

	echo "Building nodeosc.c example...";
	gcc $(LPFLAGS) examples/nodeosc.c $(LPSOURCES) $(LPLIBS) -o build/nodeosc

	echo "Building sineosc.c example...";
	gcc $(LPFLAGS) examples/sineosc.c $(LPSOURCES) $(LPLIBS) -o build/sineosc

	echo "Building fractosc.c example...";
	gcc $(LPFLAGS) examples/fractosc.c $(LPSOURCES) $(LPLIBS) -o build/fractosc

	echo "Building tukeyosc.c example...";
	gcc $(LPFLAGS) examples/tukeyosc.c $(LPSOURCES) $(LPLIBS) -o build/tukeyosc

	echo "Building tableosc.c example...";
	gcc $(LPFLAGS) examples/tableosc.c $(LPSOURCES) $(LPLIBS) -o build/tableosc

	echo "Building blnosc.c example...";
	gcc $(LPFLAGS) examples/blnosc.c $(LPSOURCES) $(LPLIBS) -o build/blnosc

	echo "Building tapeosc.c example...";
	gcc $(LPFLAGS) examples/tapeosc.c $(LPSOURCES) $(LPLIBS) -o build/tapeosc

	echo "Building tapeosc2.c example...";
	gcc $(LPFLAGS) examples/tapeosc2.c $(LPSOURCES) $(LPLIBS) -o build/tapeosc2

	echo "Building additive_synthesis.c example...";
	gcc $(LPFLAGS) examples/additive_synthesis.c $(LPSOURCES) $(LPLIBS) -o build/additive_synthesis

warble-examples:
	mkdir -p build renders

	echo "Building warble.c example...";
	gcc $(LPFLAGS) examples/warble.c $(LPSOURCES) $(LPLIBS) -o build/warble

buffer-examples:
	mkdir -p build renders

	echo "Building ring_buffer.c example...";
	gcc $(LPFLAGS) examples/ring_buffer.c $(LPSOURCES) $(LPLIBS) -o build/ring_buffer

	echo "Building slicing.c example...";
	gcc $(LPFLAGS) examples/slicing.c $(LPSOURCES) $(LPLIBS) -o build/slicing

	echo "Building slicing2.c example...";
	gcc $(LPFLAGS) examples/slicing2.c $(LPSOURCES) $(LPLIBS) -o build/slicing2

	echo "Building resample_buffer.c example...";
	gcc $(LPFLAGS) examples/resample_buffer.c $(LPSOURCES) $(LPLIBS) -o build/resample_buffer

	echo "Building reverse_buffer.c example...";
	gcc $(LPFLAGS) examples/reverse_buffer.c $(LPSOURCES) $(LPLIBS) -o build/reverse_buffer

	echo "Building taper_buffer.c example...";
	gcc $(LPFLAGS) examples/taper_buffer.c $(LPSOURCES) $(LPLIBS) -o build/taper_buffer

	echo "Building pad_buffer.c example...";
	gcc $(LPFLAGS) examples/pad_buffer.c $(LPSOURCES) $(LPLIBS) -o build/pad_buffer

	echo "Building plotbuffer.c example...";
	gcc $(LPFLAGS) examples/plotbuffer.c $(LPSOURCES) $(LPLIBS) -o build/plotbuffer


mir-examples:
	mkdir -p build renders

	echo "Building onset_detector.c example...";
	gcc $(LPFLAGS) examples/onset_detector.c $(LPSOURCES) $(LPLIBS) -o build/onset_detector

	echo "Building pitch_tracker.c example...";
	gcc $(LPFLAGS) examples/pitch_tracker.c $(LPSOURCES) $(LPLIBS) -o build/pitch_tracker

embedded-examples:
	mkdir -p build renders

	echo "Building memory_pool.c example...";
	gcc $(LPFLAGS) -Wdouble-promotion -DLP_FLOAT -DLP_STATIC examples/memory_pool.c src/oscs.sine.c src/soundfile.c src/pippicore.c $(LPLIBS) -o build/memorypool

microsound-examples:
	mkdir -p build renders

	echo "Building crossings.c example...";
	gcc $(LPFLAGS) examples/crossings.c $(LPSOURCES) $(LPLIBS) -o build/crossings

	#echo "Building crossings2.c example...";
	#gcc $(LPFLAGS) examples/crossings2.c $(LPSOURCES) $(LPLIBS) -o build/crossings2

	echo "Building grainformation.c example...";
	gcc $(LPFLAGS) examples/grainformation.c $(LPSOURCES) $(LPLIBS) -o build/grainformation

	echo "Building grainformation2.c example...";
	gcc $(LPFLAGS) examples/grainformation2.c $(LPSOURCES) $(LPLIBS) -o build/grainformation2

	echo "Building grainformation3.c example...";
	gcc $(LPFLAGS) examples/grainformation3.c $(LPSOURCES) $(LPLIBS) -o build/grainformation3

convolution-examples:
	mkdir -p build renders

	echo "Building convolution.c example...";
	gcc $(LPFLAGS) examples/convolution.c $(LPSOURCES) $(LPLIBS) -o build/convolution

	echo "Building convolution2.c example...";
	gcc $(LPFLAGS) examples/convolution2.c $(LPSOURCES) $(LPLIBS) -o build/convolution2

soundfile-examples:
	mkdir -p build renders

	echo "Building readsoundfile.c example...";
	gcc $(LPFLAGS) examples/readsoundfile.c $(LPSOURCES) $(LPLIBS) -o build/readsoundfile

	echo "Building readrawfile.c example...";
	gcc $(LPFLAGS) examples/readrawfile.c $(LPSOURCES) $(LPLIBS) -o build/readrawfile

wavetable-examples:
	mkdir -p build renders

	echo "Building wavetables.c example...";
	gcc $(LPFLAGS) examples/wavetables.c $(LPSOURCES) $(LPLIBS) -o build/wavetables

	echo "Building windows.c example...";
	gcc $(LPFLAGS) examples/windows.c $(LPSOURCES) $(LPLIBS) -o build/windows

ugens:
	gcc $(LPFLAGS) src/ugens/sineosc.c $(LPSOURCES) $(LPLIBS) -o build/ugen_sineosc
	gcc $(LPFLAGS) src/ugens/mult.c $(LPSOURCES) $(LPLIBS) -o build/ugen_mult
	gcc $(LPFLAGS) src/ugens/tape.c $(LPSOURCES) $(LPLIBS) -o build/ugen_tape

render:
	mkdir -p build renders

	echo "Rendering examples..."
	./scripts/render_examples.sh

examples: soundfile-examples convolution-examples microsound-examples embedded-examples mir-examples buffer-examples osc-examples fx-examples
