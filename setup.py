from setuptools import setup

setup(name='pippi',
        version='0.1',
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
        zip_safe=False)
