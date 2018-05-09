#pragma once

#include "serializer.h"

#include <memory>
#include <string>

/*
The language client plugin needs to send initialization options in the
`initialize` request to the ccls language server. The only required option is
`cacheDirectory`, which is where index files will be stored.

  {
    "initializationOptions": {
      "cacheDirectory": "/tmp/ccls"
    }
  }

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
  // Cache directory for indexed files.
  std::string cacheDirectory;
  // Cache serialization format.
  //
  // "json" generates `cacheDirectory/.../xxx.json` files which can be pretty
  // printed with jq.
  //
  // "binary" uses a compact binary serialization format.
  // It is not schema-aware and you need to re-index whenever a struct
  // member has changed.
  SerializeFormat cacheFormat = SerializeFormat::Binary;

  struct Clang {
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

    // If true, diagnostics from a full document parse will be reported.
    bool onParse = true;

    // If true, diagnostics from typing will be reported.
    bool onType = true;

    std::vector<std::string> whitelist;
  } diagnostics;

  // Semantic highlighting
  struct Highlight {
    // Like index.{whitelist,blacklist}, don't publish semantic highlighting to
    // blacklisted files.
    std::vector<std::string> blacklist;

    std::vector<std::string> whitelist;
  } highlight;

  struct Index {
    // Attempt to convert calls of make* functions to constructors based on
    // hueristics.
    //
    // For example, this will show constructor calls for std::make_unique
    // invocations. Specifically, ccls will try to attribute a ctor call
    // whenever the function name starts with make (ignoring case).
    bool attributeMakeCallsToCtor = true;

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

    // If true, project paths that were skipped by the whitelist/blacklist will
    // be logged.
    bool logSkippedPaths = false;

    // Allow indexing on textDocument/didChange.
    // May be too slow for big projects, so it is off by default.
    bool onDidChange = false;

    // Number of indexer threads. If 0, 80% of cores are used.
    int threads = 0;

    std::vector<std::string> whitelist;
  } index;

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
MAKE_REFLECT_STRUCT(Config::Clang, extraArgs, resourceDir);
MAKE_REFLECT_STRUCT(Config::ClientCapability, snippetSupport);
MAKE_REFLECT_STRUCT(Config::CodeLens, localVariables);
MAKE_REFLECT_STRUCT(Config::Completion,
                    caseSensitivity,
                    dropOldRequests,
                    detailedLabel,
                    filterAndSort,
                    includeBlacklist,
                    includeMaxPathSize,
                    includeSuffixWhitelist,
                    includeWhitelist);
MAKE_REFLECT_STRUCT(Config::Diagnostics,
                    blacklist,
                    frequencyMs,
                    onParse,
                    onType,
                    whitelist)
MAKE_REFLECT_STRUCT(Config::Highlight, blacklist, whitelist)
MAKE_REFLECT_STRUCT(Config::Index,
                    attributeMakeCallsToCtor,
                    blacklist,
                    comments,
                    enabled,
                    logSkippedPaths,
                    onDidChange,
                    threads,
                    whitelist);
MAKE_REFLECT_STRUCT(Config::WorkspaceSymbol, caseSensitivity, maxNum, sort);
MAKE_REFLECT_STRUCT(Config::Xref, container, maxNum);
MAKE_REFLECT_STRUCT(Config,
                    compilationDatabaseCommand,
                    compilationDatabaseDirectory,
                    cacheDirectory,
                    cacheFormat,

                    clang,
                    client,
                    codeLens,
                    completion,
                    diagnostics,
                    highlight,
                    index,
                    workspaceSymbol,
                    xref);

extern std::unique_ptr<Config> g_config;
thread_local extern int g_thread_id;
