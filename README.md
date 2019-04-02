# ccls

[![Telegram](https://img.shields.io/badge/telegram-@cclsp-blue.svg)](https://telegram.me/cclsp)
[![Gitter](https://img.shields.io/badge/gitter-ccls--project-blue.svg?logo=gitter-white)](https://gitter.im/ccls-project/ccls)

This is a temporary fork of ccls with experimental CUDA support. `ccls` originates from [cquery](https://github.com/cquery-project/cquery), is a C/C++/Objective-C language server.

  * code completion (with both signature help and snippets)
  * [definition](src/messages/textDocument_definition.cc)/[references](src/messages/textDocument_references.cc), and other cross references
  * cross reference extensions: `$ccls/call` `$ccls/inheritance` `$ccls/member` `$ccls/vars` ...
  * formatting
  * hierarchies: [call (caller/callee) hierarchy](src/messages/ccls_call.cc), [inheritance (base/derived) hierarchy](src/messages/ccls_inheritance.cc), [member hierarchy](src/messages/ccls_member.cc)
  * [symbol rename](src/messages/textDocument_rename.cc)
  * [document symbols](src/messages/textDocument_documentSymbol.cc) and approximate search of [workspace symbol](src/messages/workspace_symbol.cc)
  * [hover information](src/messages/textDocument_hover.cc)
  * diagnostics and code actions (clang FixIts)
  * semantic highlighting and preprocessor skipped regions
  * semantic navigation: `$ccls/navigate`

## CUDA quickstart

Your `.ccls` configuration should look something like:
```
%compile_commands.json
%cu --cuda-gpu-arch=sm_70
%cu --cuda-path=/usr/local/cuda-9.2/
```
This fork changes the compile commands from the `compile_commands.json` file that look like:

    /usr/local/cuda/bin/nvcc -ccbin=gcc-6  -I../src -I../external/cutlass -I../external/cub -isystem=../external/googletest/googletest/include   -Xcompiler -fopenmp --expt-extended-lambda --std=c++11 -gencode arch=compute_70,code=sm_70 -gencode arch=compute_70,code=compute_70 -g   -x cu -c /home/max/dev/cuml/ml-prims/test/add.cu -o test/CMakeFiles/mlcommon_test.dir/add.cu.o && /usr/local/cuda/bin/nvcc -ccbin=gcc-6  -I../src -I../external/cutlass -I../external/cub -isystem=../external/googletest/googletest/include   -Xcompiler -fopenmp --expt-extended-lambda --std=c++11 -gencode arch=compute_70,code=sm_70 -gencode arch=compute_70,code=compute_70 -g   -x cu -M /home/max/dev/cuml/ml-prims/test/add.cu -MT test/CMakeFiles/mlcommon_test.dir/add.cu.o -o $DEP_FILE

To something more like:

    clang -I../src -I../external/cutlass -I../external/cub -isystem=../external/googletest/googletest/include --std=c++11 --cuda-gpu-arch=sm_70 --cuda-path=/usr/local/cuda-9.2/ -c add.cu

In other words, it whitelists includes (`-I`) and c++ standard flags, but ignores all the `nvcc` switches that `clang` doesn't understand. Note that clang understands CUDA files by default.

## General Info

It has a global view of the code base and support a lot of cross reference features, see [wiki/FAQ](../../wiki/FAQ).
It starts indexing the whole project (including subprojects if exist) parallelly when you open the first file, while the main thread can serve requests before the indexing is complete.
Saving files will incrementally update the index.

Compared with cquery, it makes use of C++17 features, has less third-party dependencies and slimmed-down code base.
It leverages Clang C++ API as [clangd](https://clang.llvm.org/extra/clangd.html) does, which provides better support for code completion and diagnostics.
Refactoring is a non-goal as it can be provided by clang-include-fixer and other Clang based tools.

The comparison with cquery as noted on 2018-07-15:

|             | cquery                         | ccls                         |
|------------ |--------------------------------|------------------------------|
| third_party | more                           | fewer                        |
| C++         | C++14                          | C++17                        |
| clang API   | libclang (C)                   | clang/llvm C++               |
| Filesystem  | AbsolutePath + custom routines | llvm/Support                 |
| index       | libclang                       | clangIndex, some enhancement |
| pipeline    | index merge+id remapping       | simpler and more robust      |

cquery has system include path detection (through running the compiler driver) while ccls uses clangDriver.

# >>> [Getting started](../../wiki/Home) (CLICK HERE) <<<

* [Build](../../wiki/Build)
* [FAQ](../../wiki/FAQ)

ccls can index itself (~180MiB RSS when idle, noted on 2018-09-01), FreeBSD, glibc, Linux, LLVM (~1800MiB RSS), musl (~60MiB RSS), ... with decent memory footprint. See [wiki/compile_commands.json](../../wiki/compile_commands.json) for examples.
