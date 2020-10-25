"""Linux2v README
"""
from linux2v import version

# Always prefer setuptools over distutils
from setuptools import setup, find_packages
# To use a consistent encoding
# from pathlib import Path
# here = Path(__file__).parent.resolve()

requires = [
    'click==6.6',
    'ZODB==4.4.3',
    'ZEO==4.3.0',
    'zodburi==2.0',
    'psutil==4.3.1',
    'pyramid==1.7.3',
    'pyramid_zodbconn==0.7',
    'arrow==0.8.0',
    'redis==2.10.5',
    'hiredis==0.2.0',
    'raven==5.24.3',
    'huey==1.2.0',
    'alog==0.9.9',
    'pyfunctional==0.7.1',
    'Cython==0.25.1',
    'thriftpy>=0.3.9',
]


setup(
    name='linux2v',

    # Versions should comply with PEP440.  For a discussion on single-sourcing
    # the version across setup.py and the project code, see
    # https://packaging.python.org/en/latest/single_source_version.html
    version=version,

    description='A Python project',
    long_description='',

    url='https://bitbucket.org/keithss/linux2v',

    author='Albert Wei',
    author_email='albert@saasame.com',

    # Python Package Classifiers
    # See https://pypi.python.org/pypi?%3Aaction=list_classifiers
    classifiers=[
        'NO-UPLOAD',
        'Development Status :: 5 - Production',
        'Intended Audience :: Developers',
        'License :: Other/Proprietary License'
        'Programming Language :: Python :: 3.5',
    ],

    # What does your project relate to?
    keywords='saasame linux v2v',

    # You can just specify the packages manually here if your project is
    # simple. Or you can use find_packages().
    packages=find_packages(
        exclude=['contrib', 'docs', 'tests*', 'deploy*', 'data*'
                 'linux2v.fixtures*']),

    zip_safe=False,

    # List run-time dependencies here.  These will be installed by pip when
    # your project is installed. For an analysis of "install_requires" vs pip's
    # requirements files see:
    # https://packaging.python.org/en/latest/requirements.html
    install_requires=requires,
    tests_require=requires,
    test_suite="linux2v",

    # List additional groups of dependencies here (e.g. development
    # dependencies). You can install these using the following syntax,
    # for example:
    # $ pip install -e .[dev,test]
    extras_require={
        'dev': ['check-manifest'],
        'testing': ['pytest-cov'],
    },

    # If there are data files included in your packages that need to be
    # installed, specify them here.  If using Python 2.6 or less, then these
    # have to be included in MANIFEST.in as well.
    # package_data={
    #     'sample': ['package_data.dat'],
    # },
    package_data={
       'linux2v': ['*.so'],
       'linux2v.lib': ['*.so'],
       'linux2v.lib.macho': ['*.so'],
       'linux2v.db': ['*.so'],
       'linux2v.launcher': ['*.so'],
       'linux2v.scripts': ['*.so', '*.sh', '*.template',
                           'runonced/*.py', 'runonced/*.sh'],
    },

    # Although 'package_data' is the preferred approach, in some case you may
    # need to place data files outside of your packages. See:
    # http://docs.python.org/3.4/distutils/setupscript.html#installing-additional-files # noqa
    # In this case, 'data_file' will be installed into '<sys.prefix>/my_data'
    # data_files=[('my_data', ['data/data_file'])],

    # To provide executable scripts, use entry points in preference to the
    # "scripts" keyword. Entry points provide cross-platform support and allow
    # pip to create the appropriate form of executable for the target platform.
    entry_points={
        'paste.app_factory': [
            'main=linux2v:main'
        ],
        'console_scripts': [
            'linux2v=linux2v.scripts.maincmd:main',
        ],
    },
)
