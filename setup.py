#!/uss/bin/env python
from setuptools import setup
from setuptools.extension import Extension
from Cython.Build import cythonize
import numpy as np

dev = False

INCLUDES = ['libpippi/vendor', 'libpippi/src', '/usr/local/include', np.get_include()]
MACROS = [("NPY_NO_DEPRECATED_API", "NPY_1_7_API_VERSION")]
DIRECTIVES = {}

if dev:
    MACROS += [("CYTHON_TRACE_NOGIL", "1")]
    DIRECTIVES['profile'] = True
    DIRECTIVES['linetrace'] = True
    DIRECTIVES['binding'] = True

ext_modules = cythonize([
        Extension('pippi.renderer', [
                'pippi/renderer.pyx',
                'libpippi/vendor/linenoise/linenoise.c',
                'libpippi/vendor/libpqueue/src/pqueue.c',
                'libpippi/vendor/lmdb/libraries/liblmdb/mdb.c',
                'libpippi/vendor/lmdb/libraries/liblmdb/midl.c',
                'libpippi/src/pippicore.c',
                'astrid/src/astrid.c',
            ],
            libraries=['jack', 'rt', 'asound'], 
            include_dirs=INCLUDES+[
                'libpippi/vendor/libpqueue/src', 
                'libpippi/vendor/linenoise', 
                'libpippi/vendor/lmdb/libraries/liblmdb',
                'astrid/src'
            ],           
            define_macros=MACROS
        ),

        Extension('pippi.microcontrollers', ['pippi/microcontrollers.pyx'],
            include_dirs=INCLUDES, 
            define_macros=MACROS
        ),

        Extension('pippi.breakpoints', ['pippi/breakpoints.pyx'],
            include_dirs=INCLUDES, 
        ),
        Extension('pippi.multiband', ['pippi/multiband.pyx'], 
            libraries=['soundpipe'], 
            library_dirs=['/usr/local/lib'],
            include_dirs=INCLUDES,
            define_macros=MACROS
        ), 
        Extension('pippi.events', ['pippi/events.pyx'], include_dirs=INCLUDES, define_macros=MACROS),

        Extension('pippi.defaults', ['pippi/defaults.pyx']), 
        Extension('pippi.dsp', ['pippi/dsp.pyx'], 
            include_dirs=INCLUDES,
            define_macros=MACROS
        ), 
        Extension('pippi.fx', [
                'libpippi/vendor/fft/fft.c',
                'libpippi/src/pippicore.c',
                'libpippi/src/fx.softclip.c',
                'pippi/fx.pyx'
            ],
            libraries=['soundpipe'], 
            library_dirs=['/usr/local/lib'],
            include_dirs=INCLUDES + ['libpippi/vendor/fft'],
            define_macros=MACROS
        ),
        Extension('pippi.grains', ['pippi/grains.pyx'],
            libraries=['soundpipe'], 
            library_dirs=['/usr/local/lib'],
            include_dirs=INCLUDES,
            define_macros=MACROS
        ),
        Extension('pippi.grains2', [
                'libpippi/src/pippicore.c',
                'libpippi/src/oscs.tape.c',
                'libpippi/src/microsound.c',
                'pippi/grains2.pyx'
            ],
            include_dirs=INCLUDES,
            define_macros=MACROS
        ),

        Extension('pippi.lists', ['pippi/lists.pyx'],
            include_dirs=INCLUDES, 
            define_macros=MACROS
        ),
        Extension('pippi.sampler', ['pippi/sampler.pyx'],
            libraries=['soundpipe'], 
            library_dirs=['/usr/local/lib'],
            include_dirs=INCLUDES,
            define_macros=MACROS
        ),
        Extension('pippi.fft', [
                'libpippi/vendor/fft/fft.c',
                'pippi/fft.pyx'
            ], 
            include_dirs=INCLUDES + ['libpippi/vendor'],
            define_macros=MACROS
        ), 

        Extension('pippi.hyperupic', ['pippi/hyperupic.pyx'],
            include_dirs=INCLUDES, 
            define_macros=MACROS
        ), 
        Extension('pippi.graph', ['pippi/graph.pyx'], 
            include_dirs=INCLUDES, 
            define_macros=MACROS
        ), 
        Extension('pippi.interpolation', ['pippi/interpolation.pyx'], 
            include_dirs=INCLUDES,
            define_macros=MACROS
        ),
        Extension('pippi.noise', [
                'libpippi/src/pippicore.c',
                'libpippi/src/oscs.bln.c',
                'pippi/noise/noise.pyx'
            ], 
            include_dirs=INCLUDES,
            define_macros=MACROS
        ), 
        Extension('pippi.shapes', ['pippi/shapes.pyx'], 
            include_dirs=INCLUDES, 
            define_macros=MACROS
        ), 

        Extension('pippi.sounddb', ['pippi/sounddb.pyx'],
            include_dirs=INCLUDES, 
            define_macros=MACROS
        ), 
        Extension('pippi.mir', [
                'libpippi/src/pippicore.c', 
                'libpippi/src/mir.c', 
                'pippi/mir.pyx'
            ],
            include_dirs=INCLUDES, 
            define_macros=MACROS
        ),

        # Oscs
        Extension('pippi.oscs', ['pippi/oscs/oscs.pyx'], 
            include_dirs=INCLUDES,
            define_macros=MACROS
        ), 

        Extension('pippi.alias', ['pippi/oscs/alias.pyx'], 
            include_dirs=INCLUDES,
            define_macros=MACROS
        ), 
        Extension('pippi.bar', ['pippi/oscs/bar.pyx'],
            libraries=['soundpipe'], 
            library_dirs=['/usr/local/lib'],
            include_dirs=INCLUDES, 
            define_macros=MACROS
        ),
        Extension('pippi.dss', ['pippi/oscs/dss.pyx'], 
            include_dirs=INCLUDES,
            define_macros=MACROS
        ), 
        Extension('pippi.drunk', ['pippi/oscs/drunk.pyx'], 
            include_dirs=INCLUDES,
            define_macros=MACROS
        ), 
        Extension('pippi.fold', ['pippi/oscs/fold.pyx'], 
            include_dirs=INCLUDES,
            define_macros=MACROS
        ), 
        Extension('pippi.fm', ['pippi/oscs/fm.pyx'], 
            include_dirs=INCLUDES,
            define_macros=MACROS
        ), 
        Extension('pippi.osc', ['pippi/oscs/osc.pyx'], 
            include_dirs=INCLUDES,
            define_macros=MACROS
        ), 
        Extension('pippi.osc2d', ['pippi/oscs/osc2d.pyx'], 
            include_dirs=INCLUDES,
            define_macros=MACROS
        ), 
        Extension('pippi.pulsar', ['pippi/oscs/pulsar.pyx'], 
            include_dirs=INCLUDES,
            define_macros=MACROS
        ), 
        Extension('pippi.pulsar2d', ['pippi/oscs/pulsar2d.pyx'], 
            include_dirs=INCLUDES,
            define_macros=MACROS
        ), 
        Extension('pippi.pluck', ['pippi/oscs/pluck.pyx'], 
            include_dirs=INCLUDES,
            define_macros=MACROS
        ), 
        Extension('pippi.sineosc', ['pippi/oscs/sineosc.pyx'], 
            include_dirs=INCLUDES,
            define_macros=MACROS
        ), 
        Extension('pippi.tukey', ['pippi/oscs/tukey.pyx'], 
            include_dirs=INCLUDES,
            define_macros=MACROS
        ), 

        Extension('pippi.midi', ['pippi/midi.pyx'], 
            include_dirs=INCLUDES,
            define_macros=MACROS
        ), 
        Extension('pippi.rhythm', ['pippi/rhythm.pyx'], 
            include_dirs=INCLUDES,
            define_macros=MACROS
        ), 
        Extension('pippi.rand', [
                'libpippi/src/pippicore.c',
                'pippi/rand.pyx',
            ], 
            include_dirs=INCLUDES, 
            define_macros=MACROS
        ), 

        Extension('pippi.soundbuffer', [
                'libpippi/vendor/fft/fft.c',
                'pippi/soundbuffer.pyx'
            ], 
            include_dirs=INCLUDES + ['libpippi/vendor/fft'],
            define_macros=MACROS
        ), 
        Extension('pippi.soundfont', ['pippi/soundfont.pyx'], 
            include_dirs= INCLUDES + ['modules/TinySoundFont'], 
            define_macros=MACROS
        ), 
        Extension('pippi.soundpipe', ['pippi/soundpipe.pyx'], 
            libraries=['soundpipe'], 
            library_dirs=['/usr/local/lib'],
            include_dirs=['/usr/local/include'],
        ), 
        Extension('pippi.wavesets', ['pippi/wavesets.pyx'], 
            include_dirs=INCLUDES, 
            define_macros=MACROS
        ), 
        Extension('pippi.buffers', [
                'libpippi/vendor/fft/fft.c',
                'libpippi/src/pippicore.c',
                'libpippi/src/spectral.c',
                'libpippi/src/soundfile.c',
                'libpippi/src/fx.softclip.c',
                'pippi/buffers.pyx',
            ],
            include_dirs=INCLUDES + ['libpippi/vendor/fft'],
            define_macros=MACROS,
            extra_compile_args=['-g3'],
        ),
        Extension('pippi.wavetables', [
                'libpippi/src/pippicore.c',
                'pippi/wavetables.pyx',
            ],
            include_dirs=INCLUDES, 
            define_macros=MACROS
        ),

        # Tuning / scales / harmony / melody
        Extension('pippi.scales', ['pippi/tune/scales.pyx'], 
            include_dirs=INCLUDES,
            define_macros=MACROS
        ), 
        Extension('pippi.intervals', ['pippi/tune/intervals.pyx'], 
            include_dirs=INCLUDES,
            define_macros=MACROS
        ), 
        Extension('pippi.frequtils', ['pippi/tune/frequtils.pyx'], 
            include_dirs=INCLUDES,
            define_macros=MACROS
        ), 
        Extension('pippi.chords', ['pippi/tune/chords.pyx'], 
            include_dirs=INCLUDES,
            define_macros=MACROS
        ), 
        Extension('pippi.old', ['pippi/tune/old.pyx'], 
            include_dirs=INCLUDES,
            define_macros=MACROS
        ), 
        Extension('pippi.slonimsky', ['pippi/tune/slonimsky.pyx'], 
            include_dirs=[np.get_include()], 
            define_macros=MACROS
        ),
        Extension('pippi.tune', ['pippi/tune/tune.pyx'], 
            include_dirs=INCLUDES,
            define_macros=MACROS
        ), 
        Extension('pippi.ugens', [
                'libpippi/src/pippicore.c',
                'libpippi/src/ugens.utils.c',
                'libpippi/src/oscs.sine.c',
                'libpippi/src/ugens.sine.c',
                'libpippi/src/oscs.tape.c',
                'libpippi/src/ugens.tape.c',
                'libpippi/src/oscs.pulsar.c',
                'libpippi/src/ugens.pulsar.c',
                'pippi/ugens.pyx'
            ],
            include_dirs=INCLUDES, 
            define_macros=MACROS
        ), 
    ], 
    annotate=dev, 
    compiler_directives=DIRECTIVES,
    gdb_debug=dev,
) 

with open('README.md') as f:
    readme = f.read()

setup(
    name='pippi',
    version='2.0.0-beta-5',
    description='Computer music with Python',
    long_description=readme,
    long_description_content_type='text/markdown',
    author='He Can Jog',
    author_email='erik@hecanjog.com',
    url='https://github.com/hecanjog/pippi',
    packages=['pippi', 'pippi.tune'],
    ext_modules=ext_modules, 
    install_requires=[
        'Cython',
        'numpy', 
        'Pillow', 
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
        'Programming Language :: Python :: 3.7', 
        'Programming Language :: Python :: 3.8', 
        'Programming Language :: Python :: 3.9', 
        'Programming Language :: Python :: 3.10', 
        'Programming Language :: Python :: 3.11', 
        'Topic :: Artistic Software', 
        'Topic :: Multimedia :: Sound/Audio', 
        'Topic :: Multimedia :: Sound/Audio :: Sound Synthesis', 
    ]
)
