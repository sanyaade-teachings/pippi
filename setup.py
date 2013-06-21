from setuptools import setup, Extension

pippic = Extension('_pippic', 
        sources = ['src/pippi.c'], 
        )

setup(name='pippi',
        version='0.2.0',
        description='A python computer music system',
        url='http://hecanjog.github.com/pippi',
        author='He Can Jog',
        author_email='erik@hecanjog.com',
        license='Public Domain',
        packages=['pippi'],
        ext_modules = [ pippic ],
        install_requires=['distribute', 'docopt'],
        extras_require = {
            'realtime': ['pyalsaaudio', 'termcolor', 'pyliblo'],
        },
        test_suite='nose.collector',
        tests_require=['nose'],
        scripts=['bin/pippi'],
        zip_safe=False)
