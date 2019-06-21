.PHONY: test test-grains examples build

test:
	python -m unittest discover -s tests -p 'test_*.py' -v

test-grains:
	python -m unittest tests/test_graincloud.py -v

test-wavesets:
	python -m unittest tests/test_wavesets.py -v

test-fx:
	python -m unittest tests/test_fx.py -v

test-noise:
	python -m unittest tests/test_noise.py -v

test-shapes:
	python -m unittest tests/test_shapes.py -v

test-oscs:
	python -m unittest tests/test_oscs.py -v

test-soundbuffer:
	python -m unittest tests/test_soundbuffer.py -v

clean:
	rm -rf build/
	rm -rf pippi/*.c
	rm -rf pippi/*.so

build:
	python setup.py develop
