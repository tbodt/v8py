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
                           'v8py/convert.cpp',
                           'v8py/pyfunction.cpp',
                           'v8py/pyclass.cpp',
                           'v8py/pyclasshandlers.cpp',
                           'v8py/jsobject.cpp'
                           ],
                  include_dirs=['v8py', 'v8/include'],
                  library_dirs=['v8/out/native', 'v8/out/native/obj.target/src'],
                  libraries=['v8_nosnapshot', 'v8_base', 
                             'v8_libbase', 'v8_libplatform', 'v8_libsampler',
                             'icuuc', 'icui18n'],
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
