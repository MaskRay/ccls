#!/usr/bin/env bash
root=$(realpath "$(dirname "$0")/..")
version=$(TZ=UTC date +v%Y%m%d)
cd "$root/build/release"

case $(uname -s) in
  Darwin)
    libclang=(lib/clang+llvm-*/lib/libclang.dylib)
    name=cquery-$version-x86_64-apple-darwin ;;
  FreeBSD)
    libclang=(lib/clang+llvm-*/lib/libclang.so.?)
    name=cquery-$version-x86_64-unknown-freebsd10 ;;
  Linux)
    libclang=(lib/clang+llvm-*/lib/libclang.so.?)
    name=cquery-$version-x86_64-unknown-linux-gnu ;;
  *)
    echo Unsupported >&2
    exit 1 ;;
esac

pkg=$(mktemp -d)
rsync -rtLR bin "./${libclang[0]}" ./lib/clang+llvm-*/lib/clang/*/include "$pkg/"

cd "$pkg"
strip -s bin/cquery "${libclang[0]}"
case $(uname -s) in
  Darwin)
    # FIXME
    ;;
  Linux)
    # ./bin/cquery -> $name/bin/cquery
    tar -Jcf "$root/build/$name.tar.xz" --owner 0 --group 0 --xform "s,^\./,$name/," . ;;
  *)
    tar -Jcf "$root/build/$name.tar.xz" --uid 0 --gid 0 -s ",^\./,$name/," .
esac
rm -r "$pkg"
