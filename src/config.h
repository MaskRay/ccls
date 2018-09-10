/* Copyright 2017-2018 ccls Authors

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#pragma once

#include "serializer.h"

#include <string>

/*
The language client plugin needs to send initialization options in the
`initialize` request to the ccls language server.

If necessary, the command line option --init can be used to override
initialization options specified by the client. For example, in shell syntax:

  '--init={"index": {"comments": 2, "whitelist": ["."]}}'
*/
struct Config {
  // Root directory of the project. **Not available for configuration**
  std::string projectRoot;
  // If specified, this option overrides compile_commands.json and this
  // external command will be executed with an option |projectRoot|.
  // The initialization options will be provided as stdin.
  // The stdout of the command should be the JSON compilation database.
  std::string compilationDatabaseCommand;
  // Directory containing compile_commands.json.
  std::string compilationDatabaseDirectory;
  // Cache directory for indexed files, either absolute or relative to the
  // project root.
  // If empty, cache will be stored in memory.
  std::string cacheDirectory = ".ccls-cache";
  // Cache serialization format.
  //
  // "json" generates `cacheDirectory/.../xxx.json` files which can be pretty
  // printed with jq.
  //
  // "binary" uses a compact binary serialization format.
  // It is not schema-aware and you need to re-index whenever an internal struct
  // member has changed.
  SerializeFormat cacheFormat = SerializeFormat::Binary;

  struct Clang {
    // Arguments that should be excluded, e.g. ["-fopenmp", "-Wall"]
    //
    // e.g. If your project is built by GCC and has an option thag clang does not understand.
    std::vector<std::string> excludeArgs;

    // Additional arguments to pass to clang.
    std::vector<std::string> extraArgs;

    // Value to use for clang -resource-dir if not specified.
    //
    // This option defaults to clang -print-resource-dir and should not be
    // specified unless you are using an esoteric configuration.
    std::string resourceDir;
  } clang;

  struct ClientCapability {
    // TextDocumentClientCapabilities.completion.completionItem.snippetSupport
    bool snippetSupport = false;
  } client;

  struct CodeLens {
    // Enables code lens on parameter and function variables.
    bool localVariables = true;
  } codeLens;

  struct Completion {
    // 0: case-insensitive
    // 1: case-folded, i.e. insensitive if no input character is uppercase.
    // 2: case-sensitive
    int caseSensitivity = 2;

    // Some completion UI, such as Emacs' completion-at-point and company-lsp,
    // display completion item label and detail side by side.
    // This does not look right, when you see things like:
    //     "foo" "int foo()"
    //     "bar" "void bar(int i = 0)"
    // When this option is enabled, the completion item label is very detailed,
    // it shows the full signature of the candidate.
    // The detail just contains the completion item parent context.
    // Also, in this mode, functions with default arguments,
    // generates one more item per default argument
    // so that the right function call can be selected.
    // That is, you get something like:
    //     "int foo()" "Foo"
    //     "void bar()" "Foo"
    //     "void bar(int i = 0)" "Foo"
    // Be wary, this is quickly quite verbose,
    // items can end up truncated by the UIs.
    bool detailedLabel = false;

    // On large projects, completion can take a long time. By default if ccls
    // receives multiple completion requests while completion is still running
    // it will only service the newest request. If this is set to false then all
    // completion requests will be serviced.
    bool dropOldRequests = true;

    // If true, filter and sort completion response. ccls filters and sorts
    // completions to try to be nicer to clients that can't handle big numbers
    // of completion candidates. This behaviour can be disabled by specifying
    // false for the option. This option is the most useful for LSP clients
    // that implement their own filtering and sorting logic.
    bool filterAndSort = true;

    // Regex patterns to match include completion candidates against. They
    // receive the absolute file path.
    //
    // For example, to hide all files in a /CACHE/ folder, use ".*/CACHE/.*"
    std::vector<std::string> includeBlacklist;

    // Maximum path length to show in completion results. Paths longer than this
    // will be elided with ".." put at the front. Set to 0 or a negative number
    // to disable eliding.
    int includeMaxPathSize = 30;

    // Whitelist that file paths will be tested against. If a file path does not
    // end in one of these values, it will not be considered for
    // auto-completion. An example value is { ".h", ".hpp" }
    //
    // This is significantly faster than using a regex.
    std::vector<std::string> includeSuffixWhitelist = {".h", ".hpp", ".hh"};

    std::vector<std::string> includeWhitelist;
  } completion;

  struct Diagnostics {
    // Like index.{whitelist,blacklist}, don't publish diagnostics to
    // blacklisted files.
    std::vector<std::string> blacklist;

    // How often should ccls publish diagnostics in completion?
    //  -1: never
    //   0: as often as possible
    //   xxx: at most every xxx milliseconds
    int frequencyMs = 0;

    // If true, diagnostics will be reported for textDocument/didChange.
    bool onChange = true;

    // If true, diagnostics will be reported for textDocument/didOpen.
    bool onOpen = true;

    // If true, diagnostics will be reported for textDocument/didSave.
    bool onSave = false;

    bool spellChecking = true;

    std::vector<std::string> whitelist;
  } diagnostics;

  // Semantic highlighting
  struct Highlight {
    // true: LSP line/character; false: position
    bool lsRanges = false;

    // Like index.{whitelist,blacklist}, don't publish semantic highlighting to
    // blacklisted files.
    std::vector<std::string> blacklist;

    std::vector<std::string> whitelist;
  } highlight;

  struct Index {
    // If a translation unit's absolute path matches any EMCAScript regex in the
    // whitelist, or does not match any regex in the blacklist, it will be
    // indexed. To only index files in the whitelist, add ".*" to the blacklist.
    // `std::regex_search(path, regex, std::regex_constants::match_any)`
    //
    // Example: `ash/.*\.cc`
    std::vector<std::string> blacklist;

    // 0: none, 1: Doxygen, 2: all comments
    // Plugin support for clients:
    // - https://github.com/emacs-lsp/lsp-ui
    // - https://github.com/autozimu/LanguageClient-neovim/issues/224
    int comments = 2;

    // If false, the indexer will be disabled.
    bool enabled = true;

    // If not 0, a file will be indexed in each tranlation unit that includes it.
    int multiVersion = 0;

    // If multiVersion != 0, files that match blacklist but not whitelist will
    // still only be indexed for one version.
    std::vector<std::string> multiVersionBlacklist;
    std::vector<std::string> multiVersionWhitelist;

    // Allow indexing on textDocument/didChange.
    // May be too slow for big projects, so it is off by default.
    bool onChange = false;

    // Whether to reparse a file if write times of its dependencies have
    // changed. The file will always be reparsed if its own write time changes.
    // 0: no, 1: only after initial load of project, 2: yes
    int reparseForDependency = 2;

    // Number of indexer threads. If 0, 80% of cores are used.
    int threads = 0;

    std::vector<std::string> whitelist;
  } index;

  // Disable semantic highlighting for files larger than the size.
  int64_t largeFileSize = 2 * 1024 * 1024;

  struct WorkspaceSymbol {
    int caseSensitivity = 1;
    // Maximum workspace search results.
    int maxNum = 1000;
    // If true, workspace search results will be dynamically rescored/reordered
    // as the search progresses. Some clients do their own ordering and assume
    // that the results stay sorted in the same order as the search progresses.
    bool sort = true;
  } workspaceSymbol;

  struct Xref {
    // If true, |Location[]| response will include lexical container.
    bool container = false;
    // Maximum number of definition/reference/... results.
    int maxNum = 2000;
  } xref;
};
MAKE_REFLECT_STRUCT(Config::Clang, excludeArgs, extraArgs, resourceDir);
MAKE_REFLECT_STRUCT(Config::ClientCapability, snippetSupport);
MAKE_REFLECT_STRUCT(Config::CodeLens, localVariables);
MAKE_REFLECT_STRUCT(Config::Completion, caseSensitivity, dropOldRequests,
                    detailedLabel, filterAndSort, includeBlacklist,
                    includeMaxPathSize, includeSuffixWhitelist,
                    includeWhitelist);
MAKE_REFLECT_STRUCT(Config::Diagnostics, blacklist, frequencyMs, onChange,
                    onOpen, onSave, spellChecking, whitelist)
MAKE_REFLECT_STRUCT(Config::Highlight, lsRanges, blacklist, whitelist)
MAKE_REFLECT_STRUCT(Config::Index, blacklist, comments, enabled, multiVersion,
                    multiVersionBlacklist, multiVersionWhitelist, onChange,
                    reparseForDependency, threads, whitelist);
MAKE_REFLECT_STRUCT(Config::WorkspaceSymbol, caseSensitivity, maxNum, sort);
MAKE_REFLECT_STRUCT(Config::Xref, container, maxNum);
MAKE_REFLECT_STRUCT(Config, compilationDatabaseCommand,
                    compilationDatabaseDirectory, cacheDirectory, cacheFormat,

                    clang, client, codeLens, completion, diagnostics, highlight,
                    index, largeFileSize, workspaceSymbol, xref);

extern Config *g_config;
thread_local extern int g_thread_id;
