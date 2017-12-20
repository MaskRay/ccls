#!/usr/bin/env python
# encoding: utf-8

try:
    from urllib2 import urlopen # Python 2
except ImportError:
    from urllib.request import urlopen # Python 3

import os.path
import string
import subprocess
import sys
import re
import ctypes
import shutil

VERSION = '0.0.1'
APPNAME = 'cquery'

top = '.'
out = 'build'


# Example URLs
#   http://releases.llvm.org/5.0.0/clang+llvm-5.0.0-linux-x86_64-ubuntu16.04.tar.xz
#   http://releases.llvm.org/5.0.0/clang+llvm-5.0.0-linux-x86_64-ubuntu14.04.tar.xz
#   http://releases.llvm.org/5.0.0/clang+llvm-5.0.0-x86_64-apple-darwin.tar.xz

CLANG_TARBALL_EXT = '.tar.xz'
if sys.platform == 'darwin':
  CLANG_TARBALL_NAME = 'clang+llvm-$version-x86_64-apple-darwin'
elif sys.platform.startswith('freebsd'):
  CLANG_TARBALL_NAME = 'clang+llvm-$version-amd64-unknown-freebsd10'
# It is either 'linux2' or 'linux3' before Python 3.3
elif sys.platform.startswith('linux'):
  # These executable depend on libtinfo.so.5
  CLANG_TARBALL_NAME = 'clang+llvm-$version-linux-x86_64-ubuntu14.04'
elif sys.platform == 'win32':
  CLANG_TARBALL_NAME = 'LLVM-$version-win64'
  CLANG_TARBALL_EXT = '.exe'
else:
  sys.stderr.write('ERROR: Unknown platform {0}\n'.format(sys.platform))
  sys.exit(1)

from waflib.Tools.compiler_cxx import cxx_compiler
cxx_compiler['linux'] = ['clang++', 'g++']

if sys.version_info < (3, 0):
  if sys.platform == 'win32':
    kdll = ctypes.windll.kernel32
    def symlink(source, link_name, target_is_directory=False):
      # SYMBOLIC_LINK_FLAG_DIRECTORY: 0x1
      SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE = 0x2
      flags = int(target_is_directory)
      ret = kdll.CreateSymbolicLinkA(link_name, source, flags)
      if ret == 0:
        err = ctypes.WinError()
        ERROR_PRIVILEGE_NOT_HELD = 1314
        # Creating symbolic link on Windows requires a special priviledge SeCreateSymboliclinkPrivilege, 
        # which an non-elevated process lacks. Starting with Windows 10 build 14972, this got relaxed 
        # when Developer Mode is enabled. Triggering this new behaviour requires a new flag. Try again.
        if err[0] == ERROR_PRIVILEGE_NOT_HELD:
          flags |= SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE
          ret = kdll.CreateSymbolicLinkA(link_name, source, flags)
          if ret != 0:
            return
          err = ctypes.WinError()
        raise err
  else:
    # Python 3 compatibility
    real_symlink = os.symlink
    def symlink(source, link_name, target_is_directory=False):
      return real_symlink(source, link_name)
  os.symlink = symlink

def options(opt):
  opt.load('compiler_cxx')
  grp = opt.add_option_group('Configuration options related to use of clang from the system (not recommended)')
  grp.add_option('--use-system-clang', dest='use_system_clang', default=False, action='store_true',
                 help='enable use of clang from the system')
  grp.add_option('--bundled-clang', dest='bundled_clang', default='4.0.0',
                 help='bundled clang version, downloaded from https://releases.llvm.org/ , e.g. 4.0.0 5.0.0')
  grp.add_option('--llvm-config', dest='llvm_config', default='llvm-config',
                 help='specify path to llvm-config for automatic configuration [default: %default]')
  grp.add_option('--clang-prefix', dest='clang_prefix', default='',
                 help='enable fallback configuration method by specifying a clang installation prefix (e.g. /opt/llvm)')
  grp.add_option('--variant', default='release',
                 help='variant name for saving configuration and build results. Variants other than "debug" turn on -O3')

def download_and_extract(destdir, url, ext):
  dest = destdir + ext
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
    if ext == '.exe':
      subprocess.call(['7z', 'x', '-o{0}'.format(destdir), '-xr!$PLUGINSDIR', dest])
    else:
      subprocess.call(['tar', '-x', '-C', out, '-f', dest])
  else:
    print('Found extracted at {0}'.format(destdir))

def configure(ctx):
  ctx.resetenv(ctx.options.variant)

  ctx.load('compiler_cxx')
  cxxflags = ['-g', '-std=c++11', '-Wall', '-Wno-sign-compare', '-Werror']
  # /Zi: -g, /WX: -Werror, /W3: roughly -Wall, there is no -std=c++11 equivalent in MSVC.
  # /wd4722: ignores warning C4722 (destructor never returns) in loguru
  # /wd4267: ignores warning C4267 (conversion from 'size_t' to 'type'), roughly -Wno-sign-compare
  msvcflags = ['/nologo', '/FS', '/EHsc', '/Zi', '/W3', '/WX', '/wd4996', '/wd4722', '/wd4267', '/wd4800']
  if ctx.options.variant != 'debug':
    cxxflags.append('-O3')
    msvcflags.append('/O2') # There is no O3
  if ctx.env.CXX_NAME != 'msvc':
    # If environment variable CXXFLAGS is unset, provide a sane default.
    if not ctx.env.CXXFLAGS:
      ctx.env.CXXFLAGS = cxxflags
  else:
    ctx.env.CXXFLAGS = msvcflags

  ctx.check(header_name='stdio.h', features='cxx cxxprogram', mandatory=True)

  ctx.load('clang_compilation_database', tooldir='.')

  ctx.env['use_system_clang'] = ctx.options.use_system_clang
  ctx.env['bundled_clang'] = ctx.options.bundled_clang
  def libname(lib):
    # Newer MinGW and MSVC both wants full file name
    if sys.platform == 'win32':
      return 'lib' + lib
    return lib
  if ctx.options.use_system_clang:
    # Ask llvm-config for cflags and ldflags
    ctx.find_program(ctx.options.llvm_config, msg='checking for llvm-config', var='LLVM_CONFIG', mandatory=False)
    if ctx.env.LLVM_CONFIG:
      ctx.check_cfg(msg='Checking for clang flags',
                    path=ctx.env.LLVM_CONFIG,
                    package='',
                    uselib_store='clang',
                    args='--cppflags --ldflags')
      # llvm-config does not provide the actual library we want so we check for it
      # using the provided info so far.
      ctx.check_cxx(lib=libname('clang'), uselib_store='clang', use='clang')

    else: # Fallback method using a prefix path
      ctx.start_msg('Checking for clang prefix')
      if not ctx.options.clang_prefix:
        raise ctx.errors.ConfigurationError('not found (--clang-prefix must be specified when llvm-config is not found)')

      prefix = ctx.root.find_node(ctx.options.clang_prefix)
      if not prefix:
        raise ctx.errors.ConfigurationError('clang prefix not found: "%s"'%ctx.options.clang_prefix)

      ctx.end_msg(prefix)

      includes = [ n.abspath() for n in [ prefix.find_node('include') ] if n ]
      libpath  = [ n.abspath() for n in [ prefix.find_node(l) for l in ('lib', 'lib64')] if n ]
      ctx.check_cxx(msg='Checking for library clang', lib=libname('clang'), uselib_store='clang', includes=includes, libpath=libpath)

  else:
    global CLANG_TARBALL_NAME, CLANG_TARBALL_EXT

    # TODO Remove these after dropping clang 4 (after we figure out how to index Chrome)
    if ctx.options.bundled_clang[0] == '4':
      CLANG_TARBALL_EXT = '.tar.xz'
      if sys.platform == 'darwin':
        CLANG_TARBALL_NAME = 'clang+llvm-$version-x86_64-apple-darwin'
      elif sys.platform.startswith('freebsd'):
        CLANG_TARBALL_NAME = 'clang+llvm-$version-amd64-unknown-freebsd10'
      elif sys.platform.startswith('linux'):
        # These executable depend on libtinfo.so.5
        CLANG_TARBALL_NAME = 'clang+llvm-$version-x86_64-linux-gnu-ubuntu-14.04'
      elif sys.platform == 'win32':
        CLANG_TARBALL_NAME = 'LLVM-$version-win64'
        CLANG_TARBALL_EXT = '.exe'
      else:
        sys.stderr.write('ERROR: releases.llvm.org does not provide pre-built binaries for your platform {0}\n'.format(sys.platform))
        sys.exit(1)

    CLANG_TARBALL_NAME = string.Template(CLANG_TARBALL_NAME).substitute(version=ctx.options.bundled_clang)
    # Directory clang has been extracted to.
    CLANG_DIRECTORY = '{0}/{1}'.format(out, CLANG_TARBALL_NAME)
    # URL of the tarball to download.
    CLANG_TARBALL_URL = 'http://releases.llvm.org/{0}/{1}{2}'.format(ctx.options.bundled_clang, CLANG_TARBALL_NAME, CLANG_TARBALL_EXT)

    print('Checking for clang')
    download_and_extract(CLANG_DIRECTORY, CLANG_TARBALL_URL, CLANG_TARBALL_EXT)
    bundled_clang_dir = os.path.join(out, ctx.options.variant, 'lib', CLANG_TARBALL_NAME)
    try:
      os.makedirs(os.path.dirname(bundled_clang_dir))
    except OSError:
      pass
    clang_dir = os.path.normpath('../../' + CLANG_TARBALL_NAME)
    try:
      os.symlink(clang_dir, bundled_clang_dir, target_is_directory=True)
    except NotImplementedError:
      # Copying the whole directory instead.
      shutil.copytree(clang_dir, bundled_clang_dir)
    except OSError:
      pass

    clang_node = ctx.path.find_dir(bundled_clang_dir)
    ctx.check_cxx(uselib_store='clang',
                  includes=clang_node.find_dir('include').abspath(),
                  libpath=clang_node.find_dir('lib').abspath(),
                  lib=libname('clang'))

  ctx.msg('Clang includes', ctx.env.INCLUDES_clang)
  ctx.msg('Clang library dir', ctx.env.LIBPATH_clang)

def build(bld):
  cc_files = bld.path.ant_glob(['src/*.cc', 'src/messages/*.cc'])

  lib = []
  if sys.platform.startswith('linux'):
    lib.append('rt')
    lib.append('pthread')
    lib.append('dl')
  elif sys.platform.startswith('freebsd'):
    # loguru::stacktrace_as_stdstring calls backtrace_symbols
    lib.append('execinfo')
    # sparsepp/spp_memory.h uses libkvm
    lib.append('kvm')

    lib.append('pthread')
    lib.append('thr')
  elif sys.platform == 'darwin':
    lib.append('pthread')

  clang_tarball_name = None
  # Fallback for windows
  default_resource_directory = os.path.join(os.getcwd(), 'clang_resource_dir')
  if bld.env['use_system_clang']:
    if sys.platform == 'darwin':
      rpath = bld.env['LIBPATH_clang'][0]
    else:
      rpath = []

    devnull = '/dev/null' if sys.platform != 'win32' else 'NUL'
    output = subprocess.check_output(['clang', '-###', '-xc', devnull], stderr=subprocess.STDOUT).decode()
    match = re.search(r'"-resource-dir" "([^"]*)"', output, re.M | re.I)
    if match:
        default_resource_directory = match.group(1)
    else:
        print("Failed to found system clang resource directory. Falling back.")
  elif sys.platform.startswith('freebsd') or sys.platform.startswith('linux'):
    clang_tarball_name = os.path.basename(os.path.dirname(bld.env['LIBPATH_clang'][0]))
    rpath = '$ORIGIN/../lib/' + clang_tarball_name + '/lib'
    default_resource_directory = '../lib/{}/lib/clang/{}'.format(clang_tarball_name, bld.env['bundled_clang'])
  elif sys.platform == 'darwin':
    clang_tarball_name = os.path.basename(os.path.dirname(bld.env['LIBPATH_clang'][0]))
    rpath = '@loader_path/../lib/' + clang_tarball_name + '/lib'
    default_resource_directory = '../lib/{}/lib/clang/{}'.format(clang_tarball_name, bld.env['bundled_clang'])
  elif sys.platform == 'win32':
    rpath = [] # Unsupported
    name = os.path.basename(os.path.dirname(bld.env['LIBPATH_clang'][0]))
    # Poor Windows users' RPATH
    out_clang_dll = os.path.join(bld.path.get_bld().abspath(), 'bin', 'libclang.dll')
    try:
      os.makedirs(os.path.dirname(out_clang_dll))
      os.symlink(os.path.join(bld.path.get_bld().abspath(), 'lib', name, 'bin', 'libclang.dll'), out_clang_dll)
    except OSError:
      pass
  else:
    rpath = bld.env['LIBPATH_clang']
  bld.program(
      source=cc_files,
      use='clang',
      includes=[
        'src/',
        'third_party/',
        'third_party/doctest/',
        'third_party/loguru/',
        'third_party/rapidjson/include/',
        'third_party/sparsepp/'],
      defines=['LOGURU_WITH_STREAMS=1',
               'DEFAULT_RESOURCE_DIRECTORY="' + default_resource_directory + '"'],
      lib=lib,
      rpath=rpath,
      target='bin/cquery')

  if clang_tarball_name is not None:
    bld.install_files('${PREFIX}/lib/' + clang_tarball_name + '/lib', bld.path.get_bld().ant_glob('lib/' + clang_tarball_name + '/lib/libclang.(dylib|so.[4-9])', quiet=True))
    if bld.cmd == 'install':
      # TODO This may be cached and cannot be re-triggered. Use proper shell escape.
      bld(rule='rsync -rtR {}/./lib/{}/lib/clang/*/include {}/'.format(bld.path.get_bld(), clang_tarball_name, bld.env['PREFIX']))

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

def init(ctx):
  from waflib.Build import BuildContext, CleanContext, InstallContext, UninstallContext
  for y in (BuildContext, CleanContext, InstallContext, UninstallContext):
    class tmp(y):
      variant = ctx.options.variant

  # This is needed because waf initializes the ConfigurationContext with
  # an arbitrary setenv('') which would rewrite the previous configuration
  # cache for the default variant if the configure step finishes.
  # Ideally ConfigurationContext should just let us override this at class
  # level like the other Context subclasses do with variant
  from waflib.Configure import ConfigurationContext
  class cctx(ConfigurationContext):
    def resetenv(self, name):
      self.all_envs = {}
      self.setenv(name)
