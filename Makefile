.PHONY: test examples build

test:
	python -m unittest discover -s tests -p 'test_*.py' -v

clean:
	rm -rf build/
	rm -rf pippi/*.c
	rm -rf pippi/*.so

build:
	python setup.py develop
