from setuptools import Extension, setup
from Cython.Build import cythonize

setup(ext_modules=cythonize([
    Extension(
        'cymidi_statuslog', 
        ['../libpippi/src/pippicore.c', 'src/astrid.c', 'cython/cymidi_statuslog.pyx'],
        include_dirs=['../libpippi/vendor', '../libpippi/src', 'src'],           
    ),
], build_dir='python'))
