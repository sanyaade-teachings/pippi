.PHONY: test test-grains examples build

test:
	python -m unittest discover -s tests -p 'test_*.py' -v

test-grains:
	python -m unittest tests/test_graincloud.py -v

test-wavesets:
	python -m unittest tests/test_wavesets.py -v

clean:
	rm -rf build/
	rm -rf pippi/*.c
	rm -rf pippi/*.so

build:
	python setup.py develop
