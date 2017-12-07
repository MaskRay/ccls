def FlagsForFile( filename, **kwargs ):
  return {
    'flags': [
      '-xc++',
      '-std=c++11',
      '-DLOGURU_WITH_STREAMS=1',
      '-Isrc/',
      '-Ithird_party/',
      '-Ithird_party/doctest',
      '-Ithird_party/rapidjson/include',
      '-Ithird_party/sparsepp',
      '-Ithird_party/loguru',
      '-Ibuild/clang+llvm-4.0.0-x86_64-linux-gnu-ubuntu-14.04/include'
    ]
  }
