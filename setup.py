#!/usr/bin/env python
from setuptools import setup, find_packages, Extension

setup(
    name='v8py',

    author='Theodore Dubois',
    author_email='tblodt@icloud.com',

    license='LGPLv3',

    packages=find_packages(),
    ext_modules=[
        Extension('v8py',
                  sources=['v8py/v8py.cpp',
                           'v8py/context.cpp',
                           'v8py/template.cpp',
                           'v8py/convert.cpp',
                           'v8py/jsobject.cpp',
                           'v8py/classtemplate.cpp'],
                  include_dirs=['v8py', '/usr/local/include'],
                  library_dirs=['/usr/local/lib'],
                  libraries=['v8', 'v8_libplatform', 'v8_libbase'],
                  extra_compile_args=[
                      '-std=c++11', 
                      '-Wno-writable-strings', # fucking warnings
                      # '-g', '-O0', # debugging
                      '-fsanitize=address', '-fno-omit-frame-pointer', # make it possible to fix segfaults
                  ],
                  extra_link_args=[
                      '-fsanitize=address', '-fno-omit-frame-pointer', # make it possible to fix segfaults
                  ]),
    ],

    setup_requires=['pytest-runner'],
    tests_require=['pytest'],
)
