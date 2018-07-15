# ccls

ccls is a rewrite of cquery (originally written by Jacob Dufault),
a C/C++/Objective-C language server.

  * code completion (with both signature help and snippets)
  * [definition](src/messages/text_document_definition.cc)/[references](src/messages/text_document_references.cc), and other cross references
  * [call (caller/callee) hierarchy](src/messages/ccls_call_hierarchy.cc), [inheritance (base/derived) hierarchy](src/messages/ccls_inheritance_hierarchy.cc), [member hierarchy](src/messages/ccls_member_hierarchy.cc)
  * [symbol rename](src/messages/text_document_rename.cc)
  * [document symbols](src/messages/text_document_document_symbol.cc) and approximate search of [workspace symbol](src/messages/workspace_symbol.cc)
  * [hover information](src/messages/text_document_hover.cc)
  * diagnostics
  * code actions (clang FixIts)
  * preprocessor skipped regions
  * semantic highlighting, including support for [rainbow semantic highlighting](https://medium.com/@evnbr/coding-in-color-3a6db2743a1e)

It makes use of C++17 features, has less third-party dependencies and slimmed-down code base. Cross reference features are strenghened, (see [wiki/FAQ](../../wiki/FAQ). It currently uses libclang to index C++ code but will switch to Clang C++ API. Refactoring and formatting are non-goals as they can be provided by clang-format, clang-include-fixer and other Clang based tools.

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

# >>> [Getting started](../../wiki/Getting-started) (CLICK HERE) <<<

* [Build](../../wiki/Build)
* [Emacs](../../wiki/Emacs)
* [FAQ](../../wiki/FAQ)
