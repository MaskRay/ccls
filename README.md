# cquery

cquery is a low-latency language server for C++. It is extremely scalable and
has been designed for and tested on large code bases like
[Chromium](https://chromium.googlesource.com/chromium/src/). It's primary goal
is to make working on large code bases much faster by providing accurate and
fast semantic analysis.

![Demo](/images/demo.png?raw=true)

There are rough edges (especially when editing), but it is already possible to
be productive with cquery. Here's a list of implemented features:
  * code completion
  * references
  * type hierarchy
  * calls to functions, calls to base and derived functions
  * rename
  * goto definition, goto base method
  * document symbol search
  * global symbol search

# Setup

## Building

Eventually, cquery will be published in the vscode extension marketplace and you
will be able to install and run it without any additional steps. To use cquery
you need to clone this repository, build it, and then open vscode in this
folder.

```bash
# Build cquery
$ git clone https://github.com/jacobdufault/cquery --recursive
$ cd cquery
$ pushd third_party/sparsehash # This step will eventually be eliminated
$ ./configure # This step will eventually be eliminated
# sparsehash is header-only; building it is not required
$ popd # This step will eventually be eliminated
$ ./waf configure
$ ./waf build

# Build extension
$ cd vscode-client
$ npm install
$ code .
```

After VSCode is running, you can hit `F5` to launch the extension locally. Make
sure to open up settings and look over the configuration options. You will
probably want to increase the number of indexers that run from 7 to 40 or 50,
depending on how many cores are on your CPUs.

## Project setup

### compile_commands.json (Best)

To get the most accurate index possible, you can give cquery a compilation
database emitted from your build system of choice. For example, here's how to
generate one in ninja. When you sync your code you should regenerate this file.

```bash
$ ninja -t compdb cxx cc > compile_commands.json
```

The `compile_commands.json` file should be in the top-level workspace directory.

### cquery.extraClangArguments

If for whatever reason you cannot generate a `compile_commands.json` file, you
can add the flags to the `cquery.extraClangArguments` configuration option.

### clang_args

If for whatever reason you cannot generate a `compile_commands.json` file, you
can add the flags to a file called `clang_args` located in the top-level
workspace directory.

Each argument in that file is separated by a newline. Lines starting with `#`
are skipped. Here's an example:

```
# Language
-xc++
-std=c++11

# Includes
-I/work/cquery/third_party
```

# Limitations

cquery is able to respond to queries quickly because it caches a huge amount of
information. When a request comes in, cquery just looks it up in the cache
without running many computations. As a result, there's a large memory overhead.
For example, a full index of Chrome will take about 10gb of memory. If you
exclude v8, webkit, and blink, it goes down to about 6.5gb.

# License

MIT
