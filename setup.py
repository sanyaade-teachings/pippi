#!/usr/bin/env python
from setuptools import setup

try:
    from Cython.Build import cythonize
    import numpy as np
    ext_modules = cythonize([
        'pippi/defaults.pyx', 
        'pippi/dsp.pyx', 
        'pippi/filters.pyx', 
        'pippi/fx.pyx', 
        'pippi/grains.pyx',
        'pippi/graph.pyx', 
        'pippi/interpolation.pyx',
        'pippi/oscs.pyx', 
        'pippi/soundbuffer.pyx', 
        'pippi/wavetables.pyx'
    ], include_path=[np.get_include()], annotate=True) 

except ImportError:
    from setuptools.extension import Extension
    ext_modules = [
        Extension('pippi.defaults', ['pippi/defaults.c']), 
        Extension('pippi.dsp', ['pippi/dsp.c']), 
        Extension('pippi.filters', ['pippi/filters.c']), 
        Extension('pippi.fx', ['pippi/fx.c']), 
        Extension('pippi.grains', ['pippi/grains.c']), 
        Extension('pippi.graph', ['pippi/graph.c']), 
        Extension('pippi.interpolation', ['pippi/interpolation.c']),
        Extension('pippi.oscs', ['pippi/oscs.c']), 
        Extension('pippi.soundbuffer', ['pippi/soundbuffer.c']), 
        Extension('pippi.wavetables', ['pippi/wavetables.c']),
    ]

setup(
    name='pippi',
    version='2.0.0-beta-3',
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
