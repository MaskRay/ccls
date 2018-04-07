# ccls

ccls is a fork of cquery (originally written by Jacob Dufault),
a C/C++/Objective-C language server.

  * code completion (with both signature help and snippets)
  * finding [definition](src/messages/text_document_definition.cc)/[references](src/messages/text_document_references.cc)
  * [call (caller/callee) hierarchy](src/messages/ccls_call_hierarchy.cc), [inheritance (base/derived) hierarchy](src/messages/ccls_inheritance_hierarchy.cc), [member hierarchy](src/messages/ccls_member_hierarchy.cc)
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

# >>> [Getting started](https://github.com/MaskRay/ccls/wiki/Getting-started) (CLICK HERE) <<<

# License

MIT
