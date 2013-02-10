import distribute_setup
distribute_setup.use_setuptools()

from setuptools import setup

setup(name='pippi',
        version='0.1.1-1',
        description='A python computer music system',
        url='http://hecanjog.github.com/pippi',
        author='He Can Jog',
        author_email='erik@hecanjog.com',
        license='Public Domain',
        packages=['pippi'],
        extras_require = {
            'realtime': ['pyalsaaudio', 'termcolor', 'vcosc'],
        },
        test_suite='nose.collector',
        tests_require=['nose'],
        scripts=['bin/pippi'],
        zip_safe=False)
