#!/usr/bin/env python
from setuptools import setup
from Cython.Build import cythonize
#from Cython.Compiler.Options import get_directive_defaults

#directive_defaults = get_directive_defaults()
#directive_defaults['linetrace'] = True
#directive_defaults['binding'] = True

setup(
    name='pippi',
    version='2.0.0-alpha-5',
    description='Computer music with Python',
    author='He Can Jog',
    author_email='erik@hecanjog.com',
    url='https://github.com/hecanjog/pippi',
    packages=['pippi'],
    ext_modules=cythonize([
        'pippi/oscs.pyx', 
        'pippi/soundbuffer.pyx', 
        'pippi/interpolation.pyx',
        'pippi/wavetables.pyx'
    ]), 
)
