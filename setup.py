from setuptools import setup, Extension

pippic = Extension('_pippic', 
        sources = ['src/pippi.c'], 
        )

setup(name='pippi',
        version='0.3.1',
        description='A python computer music system',
        url='http://hecanjog.github.com/pippi',
        author='He Can Jog',
        author_email='erik@hecanjog.com',
        license='Public Domain',
        packages=['pippi'],
        ext_modules = [ pippic ],
        install_requires=['distribute'],
        test_suite='nose.collector',
        tests_require=['nose'],
        zip_safe=False)
