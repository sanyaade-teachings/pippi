#!/usr/bin/env python
from setuptools import setup
from Cython.Build import cythonize

setup(
    name='pippi',
    version='2.0.0-alpha-3',
    description='Computer music with Python',
    author='He Can Jog',
    author_email='erik@hecanjog.com',
    url='https://github.com/hecanjog/pippi',
    packages=['pippi'],
    ext_modules=cythonize(['pippi/oscs.pyx', 'pippi/interpolation.pyx']), 
)
