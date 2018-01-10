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
  * references
  * type hierarchy (parent type, derived types, expandable tree view)
  * calls to functions, calls to base and derived functions, call tree
  * symbol rename
  * goto definition, goto base method
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

## Build cquery

Building cquery is simple. The external dependencies are few:

- relatively modern c++11 compiler (ie, clang 3.4 or greater)
- python
- git

```bash
$ clang --version  # if missing, sudo apt-get install clang
$ git clone https://github.com/jacobdufault/cquery --single-branch
$ cd cquery
$ git submodule update --init
$ ./waf configure --prefix ~/.local/stow/cquery  # --prefix is optional, it specifies install directory
$ ./waf build    # -g -O3, built build/release/bin/cquery
$ ./waf install  # optional, copies the executable to $PREFIX/bin/cquery
```

For a debug build:

```bash
$ ./waf configure --variant=debug
$ ./waf build --variant=debug  # -g -O0, built build/debug/bin/cquery
```

See [wiki](https://github.com/jacobdufault/cquery/wiki) for more build instructions
(e.g. using system clang instead of bundled clang+llvm) and other topics.

## Install extension

cquery includes a vscode extension; it is released in <https://github.com/jacobdufault/cquery/releases>. Launch vscode
and install the `vscode-extension.vsix` extension. To do this:

- Hit `F1`; execute the command `Install from VSIX`.
- Select `vscode-extension.vsix` in the file chooser.

**IMPORTANT:** Please reinstall the extension when you download it - it is
still being developed.

To tell the extension where to find cquery, set "cquery.launch.workingDirectory" in User Settings to the location where cquery is installed.

It's probably also worth mentioning that "cquery.launch.command" may need to be customized as well, especially if during building you used a prefix and installed into that prefix, and set "cquery.launch.workingDirectory" to the prefix (since then "cquery.launch.command" will need to be bin/cquery instead of the default release/bin/cquery).

If you run into issues, you can view debug output by running the
(`F1`) `View: Toggle Output` command and opening the `cquery` output section.

## Project setup

### `compile_commands.json` (Best)

See [wiki](https://github.com/jacobdufault/cquery/wiki) for how to generate `compile_commands.json` with CMake, Build EAR, Ninja, ...

If the `compile_commands.json` is not in the top-level workspace directory,
then the `cquery.misc.compilationDatabaseDirectory` setting can be used to
specify its location.

### `cquery.index.extraClangArguments`

If for whatever reason you cannot generate a `compile_commands.json` file, you
can add the flags to the `cquery.index.extraClangArguments` configuration
option.

### `.cquery`

If for whatever reason you cannot generate a `compile_commands.json` file, you
can add the flags to a file called `.cquery` located in the top-level
workspace directory.

Each argument in that file is separated by a newline. Lines starting with `#`
are skipped. The first line can optionally be the path to the intended compiler,
which can help if the standard library paths are relative to the binary.
Here's an example:

```
# Driver
/usr/bin/clang++-4.0

# Language
-xc++
-std=c++11

# Includes
-I/work/cquery/third_party
```

# Building extension

If you wish to modify the vscode extension, you will need to build it locally.
Luckily, it is pretty easy - the only dependency is npm.

```bash
# Build extension
$ cd vscode-client
$ npm install
$ code .
```

When VSCode is running, you can hit `F5` to build and launch the extension
locally.

# Limitations

cquery is able to respond to queries quickly because it caches a huge amount of
information. When a request comes in, cquery just looks it up in the cache
without running many computations. As a result, there's a large memory overhead.
For example, a full index of Chrome will take about 10gb of memory. If you
exclude v8, webkit, and third_party, it goes down to about 6.5gb.

# Wiki

For Emacs/Vim/other editors integration and some additional tips, see [wiki](https://github.com/jacobdufault/cquery/wiki).

# Chromium tips

Chromium is a very large codebase, so cquery benefits from a bit of tuning.
Optionally add these to your settings:

```js
  // Set slightly lower than your CPU core count to keep other tools responsive.
  "cquery.misc.indexerCount": 50,
  // Remove uncommonly used directories with large numbers of files.
  "cquery.index.blacklist": [
   ".*/src/base/third_party/.*",
   ".*/src/native_client/.*",
   ".*/src/native_client_sdk/.*",
   ".*/src/third_party/.*",
   ".*/src/v8/.*",
   ".*/src/webkit/.*"
  ]
```

# License

MIT
