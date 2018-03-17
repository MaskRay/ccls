[![Join the chat at https://gitter.im/cquery-project/Lobby](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/cquery-project/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

# cquery

cquery is a highly-scalable, low-latency language server for C/C++/Objective-C. It is tested
and designed for large code bases like
[Chromium](https://chromium.googlesource.com/chromium/src/). cquery provides
accurate and fast semantic analysis without interrupting workflow.

![Demo](https://ptpb.pw/GlSQ.png?raw=true)

cquery implements almost the entire language server protocol and provides
some extra features to boot:

  * code completion (with both signature help and snippets)
  * finding [definition](src/messages/text_document_definition.cc)/[references](src/messages/text_document_references.cc)
  * [call (caller/callee) hierarchy](src/messages/cquery_call_hierarchy.cc), [inheritance (base/derived) hierarchy](src/messages/cquery_inheritance_hierarchy.cc), [member hierarchy](src/messages/cquery_member_hierarchy.cc)
  * [symbol rename](src/messages/text_document_rename.cc)
  * [document symbols](src/messages/text_document_document_symbol.cc) and approximate search of [workspace symbol](src/messages/workspace_symbol.cc)
  * [hover information](src/messages/text_document_hover.cc)
  * diagnostics
  * code actions (clang FixIts)
  * preprocessor skipped regions
  * #include auto-complete, undefined type include insertion, include quick-jump
    (goto definition, document links)
  * auto-implement functions without a definition
  * semantic highlighting, including support for [rainbow semantic highlighting](https://medium.com/@evnbr/coding-in-color-3a6db2743a1e)

# >>> [Getting started](https://github.com/jacobdufault/cquery/wiki/Getting-started) (CLICK HERE) <<<

<a href="https://repology.org/metapackage/cquery">
  <img src="https://repology.org/badge/vertical-allrepos/cquery.svg" alt="Packaging status" align="right">
</a>

# Limitations

cquery is able to respond to queries quickly because it caches a huge amount of
information. When a request comes in, cquery just looks it up in the cache
without running many computations. As a result, there's a large memory overhead.
For example, a full index of Chrome will take about 10gb of memory. If you
exclude v8, webkit, and third_party, it goes down to about 6.5gb.

# License

MIT
