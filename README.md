# ccls

[![Telegram](https://img.shields.io/badge/telegram-@cclsp-blue.svg)](https://telegram.me/cclsp)
[![Gitter](https://img.shields.io/badge/gitter-ccls--project-blue.svg?logo=gitter-white)](https://gitter.im/ccls-project/ccls)

ccls, which originates from [cquery](https://github.com/cquery-project/cquery), is a C/C++/Objective-C language server.

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
* [Client feature table](../../wiki/Client-feature-table)
* [FAQ](../../wiki/FAQ)

ccls can index itself (~180MiB RSS when idle, noted on 2018-09-01), FreeBSD, glibc, Linux, LLVM (~1800MiB RSS), musl (~60MiB RSS), ... with decent memory footprint. See [wiki/compile_commands.json](../../wiki/compile_commands.json) for examples.
