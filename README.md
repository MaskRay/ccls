# cquery

[![Join the chat at https://gitter.im/cquery-project/Lobby](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/cquery-project/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

cquery is a highly-scalable, low-latency language server for C/C++/Objective-C. It is tested
and designed for large code bases like
[Chromium](https://chromium.googlesource.com/chromium/src/). cquery provides
accurate and fast semantic analysis without interrupting workflow.

![Demo](https://ptpb.pw/GlSQ.png?raw=true)

cquery implements almost the entire language server protocol and provides
some extra features to boot:

  * code completion (with both signature help and snippets)
  * finding definition/references
  * type hierarchy (parent type, derived types, expandable tree view)
  * finding base/derived methods/classes, call tree
  * symbol rename
  * document and global symbol search
  * hover tooltips showing symbol type
  * diagnostics
  * code actions (clang FixIts)
  * darken/fade code disabled by preprocessor
  * #include auto-complete, undefined type include insertion, include quick-jump
    (goto definition, document links)
  * auto-implement functions without a definition
  * semantic highlighting, including support for [rainbow semantic highlighting](https://medium.com/@evnbr/coding-in-color-3a6db2743a1e)

# Setup - build cquery, install extension, setup project

There are three steps to get cquery up and running. Eventually, cquery will be
published in the vscode extension marketplace which will reduce these three
steps to only project setup.

## [Getting started](https://github.com/jacobdufault/cquery/wiki/Getting-started)

And [wiki/Build](https://github.com/jacobdufault/cquery/wiki/Build).

# Limitations

cquery is able to respond to queries quickly because it caches a huge amount of
information. When a request comes in, cquery just looks it up in the cache
without running many computations. As a result, there's a large memory overhead.
For example, a full index of Chrome will take about 10gb of memory. If you
exclude v8, webkit, and third_party, it goes down to about 6.5gb.

# License

MIT
