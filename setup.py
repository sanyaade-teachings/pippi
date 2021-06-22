#!/usr/bin/env python
from setuptools import setup
from setuptools.extension import Extension
from Cython.Build import cythonize
import numpy as np

INCLUDES = ['libpippi/src', '/usr/local/include', np.get_include()]
CSOURCES = ['libpippi/src/pippicore.c']

ext_modules = cythonize([
        Extension('pippi.breakpoints', ['pippi/breakpoints.pyx'],
            include_dirs=INCLUDES, 
        ),
        Extension('pippi.multiband', ['pippi/multiband.pyx'], 
            libraries=['soundpipe'], 
            library_dirs=['/usr/local/lib'],
            include_dirs=INCLUDES,
        ), 
        Extension('pippi.events', ['pippi/events.pyx'], include_dirs=INCLUDES),

        Extension('pippi.defaults', ['pippi/defaults.pyx']), 
        Extension('pippi.dsp', ['pippi/dsp.pyx'], include_dirs=INCLUDES), 
        Extension('pippi.fx', ['pippi/fx.pyx'],
            libraries=['soundpipe'], 
            library_dirs=['/usr/local/lib'],
            include_dirs=INCLUDES + ['modules/fft']
        ),
        Extension('pippi.grains', ['pippi/grains.pyx'],
            libraries=['soundpipe'], 
            library_dirs=['/usr/local/lib'],
            include_dirs=INCLUDES
        ),
        Extension('pippi.lists', ['pippi/lists.pyx'],
            include_dirs=INCLUDES, 
        ),
        Extension('pippi.sampler', ['pippi/sampler.pyx'],
            libraries=['soundpipe'], 
            library_dirs=['/usr/local/lib'],
            include_dirs=INCLUDES
        ),
        Extension('pippi.fft', ['modules/fft/fft.c', 'pippi/fft.pyx'], 
            include_dirs=INCLUDES + ['modules/fft'], 
        ), 

        Extension('pippi.hyperupic', ['pippi/hyperupic.pyx'],
            include_dirs=INCLUDES, 
        ), 
        Extension('pippi.graph', ['pippi/graph.pyx'], include_dirs=INCLUDES), 
        Extension('pippi.interpolation', ['pippi/interpolation.pyx'], include_dirs=INCLUDES),
        Extension('pippi.noise', ['pippi/noise/noise.pyx'], include_dirs=INCLUDES), 
        Extension('pippi.shapes', ['pippi/shapes.pyx'], 
            include_dirs=INCLUDES, 
        ), 

        Extension('pippi.sounddb', ['pippi/sounddb.pyx'],
            include_dirs=INCLUDES, 
        ), 
        Extension('pippi.mir', ['pippi/mir.pyx'],
            include_dirs=INCLUDES, 
        ),

        # Oscs
        Extension('pippi.oscs', ['pippi/oscs/oscs.pyx'], include_dirs=INCLUDES), 

        Extension('pippi.alias', ['pippi/oscs/alias.pyx'], include_dirs=INCLUDES), 
        Extension('pippi.bar', ['pippi/oscs/bar.pyx'],
            libraries=['soundpipe'], 
            library_dirs=['/usr/local/lib'],
            include_dirs=INCLUDES, 
        ),
        Extension('pippi.dss', ['pippi/oscs/dss.pyx'], include_dirs=INCLUDES), 
        Extension('pippi.drunk', ['pippi/oscs/drunk.pyx'], include_dirs=INCLUDES), 
        Extension('pippi.fold', ['pippi/oscs/fold.pyx'], include_dirs=INCLUDES), 
        Extension('pippi.fm', ['pippi/oscs/fm.pyx'], include_dirs=INCLUDES), 
        Extension('pippi.osc', ['pippi/oscs/osc.pyx'], include_dirs=INCLUDES), 
        Extension('pippi.osc2d', ['pippi/oscs/osc2d.pyx'], include_dirs=INCLUDES), 
        Extension('pippi.pulsar', ['pippi/oscs/pulsar.pyx'], include_dirs=INCLUDES), 
        Extension('pippi.pulsar2d', ['pippi/oscs/pulsar2d.pyx'], include_dirs=INCLUDES), 
        Extension('pippi.pluck', ['pippi/oscs/pluck.pyx'], include_dirs=INCLUDES), 
        Extension('pippi.sineosc', ['pippi/oscs/sineosc.pyx'], include_dirs=INCLUDES), 
        Extension('pippi.tukey', ['pippi/oscs/tukey.pyx'], include_dirs=INCLUDES), 

        Extension('pippi.midi', ['pippi/midi.pyx'], include_dirs=INCLUDES), 
        Extension('pippi.rhythm', ['pippi/rhythm.pyx'], include_dirs=INCLUDES), 
        Extension('pippi.rand', [
                'libpippi/src/pippicore.c',
                'pippi/rand.pyx',
            ], 
            include_dirs=INCLUDES, 
        ), 

        Extension('pippi.soundbuffer', ['pippi/soundbuffer.pyx'], 
            include_dirs= INCLUDES + ['modules/fft'], 
        ), 
        Extension('pippi.soundfont', ['pippi/soundfont.pyx'], 
            include_dirs= INCLUDES + ['modules/TinySoundFont'], 
        ), 
        Extension('pippi.soundpipe', ['pippi/soundpipe.pyx'], 
            libraries=['soundpipe'], 
            library_dirs=['/usr/local/lib'],
            include_dirs=['/usr/local/include']
        ), 
        Extension('pippi.wavesets', ['pippi/wavesets.pyx'], 
            include_dirs=INCLUDES, 
        ), 
        Extension('pippi.wavetables', [
                'libpippi/src/pippicore.c',
                'pippi/wavetables.pyx',
            ],
            include_dirs=INCLUDES, 
        ),

        # Tuning / scales / harmony / melody
        Extension('pippi.scales', ['pippi/tune/scales.pyx'], include_dirs=INCLUDES), 
        Extension('pippi.intervals', ['pippi/tune/intervals.pyx'], include_dirs=INCLUDES), 
        Extension('pippi.frequtils', ['pippi/tune/frequtils.pyx'], include_dirs=INCLUDES), 
        Extension('pippi.chords', ['pippi/tune/chords.pyx'], include_dirs=INCLUDES), 
        Extension('pippi.old', ['pippi/tune/old.pyx'], include_dirs=INCLUDES), 
        Extension('pippi.slonimsky', ['pippi/tune/slonimsky.pyx'], 
            include_dirs=[np.get_include()], 
        ),
        Extension('pippi.tune', ['pippi/tune/tune.pyx'], include_dirs=INCLUDES), 
    ], 
    annotate=True, 
    compiler_directives={'profile': False},
) 

with open('README.md') as f:
    readme = f.read()

setup(
    name='pippi',
    version='2.0.0-beta-4',
    description='Computer music with Python',
    long_description=readme,
    long_description_content_type='text/markdown',
    author='He Can Jog',
    author_email='erik@hecanjog.com',
    url='https://github.com/hecanjog/pippi',
    packages=['pippi', 'pippi.tune'],
    ext_modules=ext_modules, 
    install_requires=[
        'aubio', 
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
