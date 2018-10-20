.PHONY: test examples build

test:
	python -m unittest discover -s tests -p 'test_*.py' -v

clean:
	rm -rf build/
	rm -rf pippi/*.c
	rm -rf pippi/*.so

examples:
	ls examples/*.py | xargs -n 1 -P `nproc --all` python

build:
	python setup.py develop
