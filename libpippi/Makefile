.PHONY: all wavetables

default: all

ALLSOURCES = src/oscs.sine.c \
	src/oscs.pulsar.c \
	src/oscs.tape.c \
	src/oscs.tukey.c \
	src/microsound.c \
	src/mir.c \
	src/soundfile.c \
	src/scheduler.c \
	src/pippicore.c

FLAGS = -g -std=c89 -Wall -Wsign-compare -pedantic -lm -Isrc -Ivendor

wavetables:
	./scripts/build_static_wavetables.sh

all:
	mkdir -p build renders

	echo "Building pulsarosc.c example...";
	gcc $(FLAGS) examples/pulsarosc.c $(ALLSOURCES) -o build/pulsarosc
	
	echo "Building sineosc.c example...";
	gcc $(FLAGS) examples/sineosc.c $(ALLSOURCES) -o build/sineosc

	echo "Building tukeyosc.c example...";
	gcc $(FLAGS) examples/tukeyosc.c $(ALLSOURCES) -o build/tukeyosc

	echo "Building ring_buffer.c example...";
	gcc $(FLAGS) examples/ring_buffer.c $(ALLSOURCES) -o build/ring_buffer

	echo "Building onset_detector.c example...";
	gcc $(FLAGS) examples/onset_detector.c $(ALLSOURCES) -o build/onset_detector

	echo "Building pitch_tracker.c example...";
	gcc $(FLAGS) examples/pitch_tracker.c $(ALLSOURCES) -o build/pitch_tracker

	echo "Building memory_pool.c example...";
	gcc $(FLAGS) -DLP_FLOAT -DLP_STATIC examples/memory_pool.c src/oscs.sine.c src/soundfile.c src/pippicore.c -o build/memorypool

	echo "Building scheduler.c example...";
	gcc $(FLAGS) examples/scheduler.c $(ALLSOURCES) -o build/scheduler

	echo "Building additive_synthesis.c example...";
	gcc $(FLAGS) examples/additive_synthesis.c $(ALLSOURCES) -o build/additive_synthesis

	echo "Building graincloud.c example...";
	gcc $(FLAGS) examples/graincloud.c $(ALLSOURCES) -o build/graincloud

	echo "Building graincloud2.c example...";
	gcc $(FLAGS) examples/graincloud2.c $(ALLSOURCES) -o build/graincloud2

	echo "Building tapeosc.c example...";
	gcc $(FLAGS) examples/tapeosc.c $(ALLSOURCES) -o build/tapeosc

	echo "Building readsoundfile.c example...";
	gcc $(FLAGS) examples/readsoundfile.c $(ALLSOURCES) -o build/readsoundfile


	echo "Rendering examples..."
	./scripts/render_examples.sh
