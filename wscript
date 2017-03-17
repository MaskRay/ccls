#! /usr/bin/env python
# encoding: utf-8

import urllib2

VERSION='0.0.1'
APPNAME='indexer'

top = '.'
out = 'build'

from waflib.Tools.compiler_cxx import cxx_compiler
cxx_compiler['linux'] = ['clang++', 'g++']

def options(opt):
  opt.load('compiler_cxx')

def configure(conf):
  conf.load('compiler_cxx')
  conf.check(header_name='stdio.h', features='cxx cxxprogram', mandatory=True)

# https://github.com/Andersbakken/rtags/blob/master/scripts/getclang.sh
def download_clang(conf):
  url = 'http://releases.llvm.org/3.9.0/clang+llvm-3.9.0-x86_64-apple-darwin.tar.xz'
  response = urllib2.urlopen(url)
  html = response.read()
  pass

def build(bld):
  cc_files = bld.path.ant_glob(['**/*.cpp', '**/*.cc'],
                               excl=['*tests/*', 'third_party/*'])
  bld.program(
      source=cc_files,
      cxxflags=['-std=c++11'],
      includes=['third_party/rapidjson/include',
                '/usr/local/google/home/jdufault/super-clang-index/clang+llvm-4.0.0-x86_64-linux-gnu-ubuntu-14.04/include'],
      lib=['clang', 'rt', 'pthread'],
      libpath=['/usr/local/google/home/jdufault/super-clang-index/clang+llvm-4.0.0-x86_64-linux-gnu-ubuntu-14.04/lib'],
      rpath=['/usr/local/google/home/jdufault/super-clang-index/clang+llvm-4.0.0-x86_64-linux-gnu-ubuntu-14.04/lib'],
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

