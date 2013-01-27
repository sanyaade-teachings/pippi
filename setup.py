from setuptools import setup

setup(name='pippi',
        version='0.1.1',
        description='A command driven computer music instrument',
        url='http://hecanjog.github.com/pippi',
        author='He Can Jog',
        author_email='erik@hecanjog.com',
        license='Public Domain',
        packages=['pippi'],
        install_requires=[
            'pyalsaaudio',
            'termcolor',
        ],
        test_suite='nose.collector',
        tests_require=['nose'],
        scripts=['bin/pippi'],
        zip_safe=False)
