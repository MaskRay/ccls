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
#   http://releases.llvm.org/5.0.0/clang+llvm-5.0.0-linux-x86_64-ubuntu16.04.tar.xz
#   http://releases.llvm.org/5.0.0/clang+llvm-5.0.0-linux-x86_64-ubuntu14.04.tar.xz
#   http://releases.llvm.org/5.0.0/clang+llvm-5.0.0-x86_64-apple-darwin.tar.xz
# TODO: windows support (it's an exe!)

global CLANG_PLATFORM_NAME
global CLANG_TARBALL_PLATFORM_NAME

if sys.platform == 'linux' or sys.platform == 'linux2':
  CLANG_PLATFORM_NAME = 'linux-x86_64-ubuntu14.04'
  CLANG_TARBALL_PLATFORM_NAME = 'clang+llvm'
elif sys.platform == 'darwin':
  CLANG_PLATFORM_NAME = 'x86_64-apple-darwin'
  CLANG_TARBALL_PLATFORM_NAME = 'clang+llvm'
else:
  sys.stderr.write('ERROR: Unknown platform {0}\n'.format(sys.platform))
  sys.exit(1)

# Version of clang to download and use.
CLANG_VERSION = '5.0.0'
# Tarball name on clang servers that should be used.
CLANG_TARBALL_NAME  = '{0}-{1}-{2}'.format(CLANG_TARBALL_PLATFORM_NAME, CLANG_VERSION, CLANG_PLATFORM_NAME)
# Directory clang has been extracted to.
CLANG_DIRECTORY = '{0}/{1}'.format(out, CLANG_TARBALL_NAME)
# URL of the tarball to download.
CLANG_TARBALL_URL = 'http://releases.llvm.org/{0}/{1}.tar.xz'.format(CLANG_VERSION, CLANG_TARBALL_NAME)
# Path to locally tarball.
CLANG_TARBALL_LOCAL_PATH = '{0}.tar.xz'.format(CLANG_DIRECTORY)

# Directory libcxx will be extracted to.
LIBCXX_DIRECTORY = '{0}/libcxx'.format(out)
# URL to download libcxx from.
LIBCXX_URL = 'http://releases.llvm.org/4.0.0/libcxx-4.0.0.src.tar.xz'
# Absolute path for where to download the URL.
LIBCXX_LOCAL_PATH = '{0}/libcxx-4.0.0.src.tar.xz'.format(out)

from waflib.Tools.compiler_cxx import cxx_compiler
cxx_compiler['linux'] = ['clang++', 'g++']

def options(opt):
  opt.load('compiler_cxx')

def download_and_extract(destdir, dest, url):
  # Download and save the compressed tarball as |compressed_file_name|.
  if not os.path.isfile(dest):
    print('Downloading tarball')
    print('   destination: {0}'.format(dest))
    print('   source:      {0}'.format(url))
    # TODO: verify checksum
    response = urlopen(url)
    with open(dest, 'wb') as f:
      f.write(response.read())
  else:
    print('Found tarball at {0}'.format(dest))

  # Extract the tarball.
  if not os.path.isdir(destdir):
    print('Extracting')
    # TODO: make portable.
    call(['tar', 'xf', dest, '-C', out])
  else:
    print('Found extracted at {0}'.format(destdir))

def configure(conf):
  conf.load('compiler_cxx')
  conf.check(header_name='stdio.h', features='cxx cxxprogram', mandatory=True)
  conf.load('clang_compilation_database', tooldir='.')

  print('Checking for clang')
  download_and_extract(CLANG_DIRECTORY, CLANG_TARBALL_LOCAL_PATH, CLANG_TARBALL_URL)
  #print('Checking for libcxx')
  #download_and_extract(LIBCXX_DIRECTORY, LIBCXX_LOCAL_PATH, LIBCXX_URL)

  """
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
  """

def build(bld):
  # todo: configure vars
  CLANG_INCLUDE_DIR = '{0}/include'.format(CLANG_DIRECTORY)
  CLANG_LIB_DIR = '{0}/lib'.format(CLANG_DIRECTORY)
  CLANG_INCLUDE_DIR = os.path.abspath(CLANG_INCLUDE_DIR)
  CLANG_LIB_DIR = os.path.abspath(CLANG_LIB_DIR)
  print('CLANG_INCLUDE_DIR: {0}'.format(CLANG_INCLUDE_DIR))
  print('CLANG_LIB_DIR:     {0}'.format(CLANG_LIB_DIR))

  cc_files = bld.path.ant_glob(['src/**/*.cpp', 'src/**/*.cc'])

  lib = ['clang']
  if sys.platform == 'linux' or sys.platform == 'linux2':
    lib.append('rt')
    lib.append('pthread')
    lib.append('dl')
  elif sys.platform == 'darwin':
    lib.append('pthread')

  bld.program(
      source=cc_files,
      cxxflags=['-g', '-O3', '-std=c++11', '-Wall', '-Werror'],
      includes=[
        'third_party/',
        'third_party/doctest/',
        'third_party/loguru/',
        'third_party/rapidjson/include/',
        'third_party/sparsehash/src/',
        'third_party/sparsepp/',
        CLANG_INCLUDE_DIR],
      defines=['LOGURU_WITH_STREAMS=1'],
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

