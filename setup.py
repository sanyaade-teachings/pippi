from setuptools import setup, Extension

pippic = Extension('_pippic', 
        sources = ['src/pippi.c'], 
        )

setup(name='pippi',
        version='1.0.0b1',
        description='Computer music with python',
        url='http://hecanjog.github.com/pippi',

        author='He Can Jog',
        author_email='erik@hecanjog.com',
        license='Public Domain',

        classifiers = [
            'Development Status :: 4 - Beta',
            'Programming Language :: Python :: 2.7',  
        ],

        keywords = 'music dsp',

        packages=['pippi'],
        ext_modules=[ pippic ],

        test_suite='nose.collector',
        tests_require=['nose'],
        zip_safe=False
    )
