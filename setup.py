#!/usr/bin/env python
from setuptools import setup

try:
    from Cython.Build import cythonize
except ImportError:
    ext_modules = cythonize([
        'pippi/oscs.pyx', 
        'pippi/soundbuffer.pyx', 
        'pippi/interpolation.pyx',
        'pippi/wavetables.pyx'
    ]) 
else:
    from setuptools.extension import Extension
    ext_modules = [
        Extension('pippi.oscs', ['pippi/oscs.c']), 
        Extension('pippi.soundbuffer', ['pippi/soundbuffer.c']), 
        Extension('pippi.interpolation', ['pippi/interpolation.c']),
        Extension('pippi.wavetables', ['pippi/wavetables.c']),
    ]

setup(
    name='pippi',
    version='2.0.0-a8',
    description='Computer music with Python',
    author='He Can Jog',
    author_email='erik@hecanjog.com',
    url='https://github.com/hecanjog/pippi',
    packages=['pippi'],
    ext_modules=ext_modules, 
    install_requires=[
        'numpy', 
        'Pillow-SIMD', 
        'PySoundFile'
    ], 
    keywords='algorithmic computer music', 
    python_requires='>=3',
    classifiers=[
        'Development Status :: 3 - Alpha', 
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
