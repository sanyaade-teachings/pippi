#!/usr/bin/env python
from setuptools import setup
from setuptools.extension import Extension
from Cython.Build import cythonize
import numpy as np

ext_modules = cythonize([
        Extension('pippi.defaults', ['pippi/defaults.pyx']), 
        Extension('pippi.dsp', ['pippi/dsp.pyx']), 
        Extension('pippi.fx', ['pippi/fx.pyx']), 
        Extension('pippi.grains', ['pippi/grains.pyx']), 
        Extension('pippi.graph', ['pippi/graph.pyx']), 
        Extension('pippi.interpolation', ['pippi/interpolation.pyx']),
        Extension('pippi.oscs', ['pippi/oscs.pyx']), 
        Extension('pippi.soundbuffer', ['pippi/soundbuffer.pyx']), 
        Extension('pippi.soundpipe', ['pippi/soundpipe.pyx'], 
            libraries=['sndfile', 'soundpipe'], 
            library_dirs=['/usr/local/lib']
        ), 
        Extension('pippi.wavetables', ['pippi/wavetables.pyx']),
    ], 
    include_path=[
        np.get_include(), 
        '/usr/include/sndfile.h'
        '/usr/local/include/soundpipe.h', 
    ], 
    annotate=True
) 

setup(
    name='pippi',
    version='2.0.0-beta-4',
    description='Computer music with Python',
    author='He Can Jog',
    author_email='erik@hecanjog.com',
    url='https://github.com/hecanjog/pippi',
    packages=['pippi', 'pippi.tune'],
    ext_modules=ext_modules, 
    install_requires=[
        'numpy', 
        'Pillow', 
        'PySoundFile'
    ], 
    keywords='algorithmic computer music', 
    python_requires='>=3',
    classifiers=[
        'Development Status :: 4 - Beta', 
        'Intended Audience :: Developers',
        'Intended Audience :: Education',
        'Intended Audience :: Other Audience', # Musicians and audio tinkerers, and of course anyone else...
        'License :: Public Domain', 
        'Natural Language :: English', 
        'Operating System :: POSIX', 
        'Operating System :: POSIX :: Linux', 
        'Operating System :: Microsoft :: Windows', 
        'Operating System :: MacOS', 
        'Programming Language :: Cython', 
        'Programming Language :: Python :: 3 :: Only', 
        'Programming Language :: Python :: 3.6', 
        'Topic :: Artistic Software', 
        'Topic :: Multimedia :: Sound/Audio', 
        'Topic :: Multimedia :: Sound/Audio :: Sound Synthesis', 
    ]
)
