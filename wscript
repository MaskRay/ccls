#! /usr/bin/env python
# encoding: utf-8

from urllib2 import urlopen # Python 2
# from urllib.request import urlopen # Python 3
import os.path
from subprocess import call
import sys

VERSION='0.0.1'
APPNAME='indexer'

top = '.'
out = 'build'


# Example URLs
#   http://releases.llvm.org/4.0.0/clang+llvm-4.0.0-x86_64-linux-gnu-ubuntu-14.04.tar.xz
#   http://releases.llvm.org/4.0.0/clang+llvm-4.0.0-x86_64-apple-darwin.tar.xz
#   http://releases.llvm.org/4.0.0/LLVM-4.0.0-win64.exe
# TODO: windows support (it's an exe!)

global CLANG_PLATFORM_NAME
global CLANG_TARBALL_PLATFORM_NAME

if sys.platform == 'linux' or sys.platform == 'linux2':
  CLANG_PLATFORM_NAME = 'x86_64-linux-gnu-ubuntu-14.04'
  CLANG_TARBALL_PLATFORM_NAME = 'clang+llvm'
elif sys.platform == 'darwin':
  CLANG_PLATFORM_NAME = 'x86_64-apple-darwin'
  CLANG_TARBALL_PLATFORM_NAME = 'clang+llvm'
else:
  sys.stderr.write('ERROR: Unknown platform {0}\n'.format(sys.platform))
  sys.exit(1)

# Version of clang to download and use.
CLANG_VERSION = '4.0.0'
# Tarball name on clang servers that should be used.
CLANG_TARBALL_NAME  = '{0}-{1}-{2}'.format(CLANG_TARBALL_PLATFORM_NAME, CLANG_VERSION, CLANG_PLATFORM_NAME)
# Directory clang has been extracted to.
CLANG_DIRECTORY = '{0}/{1}'.format(out, CLANG_TARBALL_NAME)
# URL of the tarball to download.
CLANG_TARBALL_URL = 'http://releases.llvm.org/{0}/{1}.tar.xz'.format(CLANG_VERSION, CLANG_TARBALL_NAME)
# Path to locally tarball.
CLANG_TARBALL_LOCAL_PATH = '{0}.tar.xz'.format(CLANG_DIRECTORY)


from waflib.Tools.compiler_cxx import cxx_compiler
cxx_compiler['linux'] = ['clang++', 'g++']

def options(opt):
  opt.load('compiler_cxx')

def configure(conf):
  conf.load('compiler_cxx')
  conf.check(header_name='stdio.h', features='cxx cxxprogram', mandatory=True)

  # Download and save the compressed tarball as |compressed_file_name|.
  if not os.path.isfile(CLANG_TARBALL_LOCAL_PATH):
    print('Downloading clang tarball')
    print('   destination: {0}'.format(CLANG_TARBALL_LOCAL_PATH))
    print('   source:      {0}'.format(CLANG_TARBALL_URL))
    # TODO: verify checksum
    response = urlopen(CLANG_TARBALL_URL)
    with open(CLANG_TARBALL_LOCAL_PATH, 'wb') as f:
      f.write(response.read())
  else:
    print('Found clang tarball at {0}'.format(CLANG_TARBALL_LOCAL_PATH))

  # Extract the tarball.
  if not os.path.isdir(CLANG_DIRECTORY):
    print('Extracting clang')
    # TODO: make portable.
    call(['tar', 'xf', CLANG_TARBALL_LOCAL_PATH, '-C', out])
  else:
    print('Found extracted clang at {0}'.format(CLANG_DIRECTORY))

def build(bld):
  # todo: configure vars
  CLANG_INCLUDE_DIR = '{0}/include'.format(CLANG_DIRECTORY)
  CLANG_LIB_DIR = '{0}/lib'.format(CLANG_DIRECTORY)
  CLANG_INCLUDE_DIR = os.path.abspath(CLANG_INCLUDE_DIR)
  CLANG_LIB_DIR = os.path.abspath(CLANG_LIB_DIR)
  print('CLANG_INCLUDE_DIR: {0}'.format(CLANG_INCLUDE_DIR))
  print('CLANG_LIB_DIR:     {0}'.format(CLANG_LIB_DIR))

  cc_files = bld.path.ant_glob(['**/*.cpp', '**/*.cc'],
                               excl=['foo/*',
                                     'libcxx/*',
                                     '*tests/*',
                                     'third_party/*'])

  lib = ['clang']
  if sys.platform == 'linux' or sys.platform == 'linux2':
    lib.append('rt')
    lib.append('pthread')
  elif sys.platform == 'darwin':
    lib.append('pthread')

  bld.program(
      source=cc_files,
      cxxflags=['-g', '-O3', '-std=c++11', '-Wall'],
      includes=[
        'third_party/',
        'third_party/doctest/',
        'third_party/rapidjson/include/',
        'third_party/sparsehash/src/',
        'third_party/sparsepp/',
        CLANG_INCLUDE_DIR],
      lib=lib,
      libpath=[CLANG_LIB_DIR],
      rpath=[CLANG_LIB_DIR],
      target='app')

  #bld.shlib(source='a.cpp', target='mylib', vnum='9.8.7')
  #bld.shlib(source='a.cpp', target='mylib2', vnum='9.8.7', cnum='9.8')
  #bld.shlib(source='a.cpp', target='mylib3')
  #bld.program(source=cc_files, target='app', use='mylib')
  #bld.stlib(target='foo', source='b.cpp')

  # just a test to check if the .c is compiled as c++ when no c compiler is found
  #bld.program(features='cxx cxxprogram', source='main.c', target='app2')

  #if bld.cmd != 'clean':
  #  from waflib import Logs
  #  bld.logger = Logs.make_logger('test.log', 'build') # just to get a clean output
  #  bld.check(header_name='sadlib.h', features='cxx cxxprogram', mandatory=False)
  #  bld.logger = None

