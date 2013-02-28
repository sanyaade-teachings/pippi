from setuptools import setup

setup(name='pippi',
        version='0.1.2',
        description='A python computer music system',
        url='http://hecanjog.github.com/pippi',
        author='He Can Jog',
        author_email='erik@hecanjog.com',
        license='Public Domain',
        packages=['pippi'],
        install_requires=['distribute'],
        extras_require = {
            'realtime': ['pyalsaaudio', 'termcolor', 'pyliblo'],
        },
        test_suite='nose.collector',
        tests_require=['nose'],
        scripts=['bin/pippi'],
        zip_safe=False)
