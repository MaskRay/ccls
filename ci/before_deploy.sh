#!/bin/bash
root=$(realpath "$(dirname "$0")/..")
version=$(TZ=UTC date +v%Y%m%d)

case "$TRAVIS_OS_NAME" in
  osx)
    SO=dylib
    name=cquery-$version-x86_64-apple-darwin ;;
  *)
    SO=so
    name=cquery-$version-x86_64-unknown-linux-gnu ;;
esac

pkg=$(mktemp -d)

cd "$root/build/release"
rsync -rtLR bin ./lib/clang+llvm-*/lib/libclang.$SO.? ./lib/clang+llvm-*/lib/clang/*/include "$pkg/"

cd "$pkg"
strip -s bin/cquery lib/clang+llvm*/lib/libclang.$SO.?
if [[ $(uname) == Linux ]]; then
  # ./bin/cquery -> $name/bin/cquery
  tar -Jcf "$root/build/$name.tar.xz" --owner 0 --group 0 --xform "s,^\./,$name/," .
else
  tar -zcf "$root/build/$name.tar.gz" --uid 0 --gid 0 -s ",^\./,$name/," .
fi
rm -r "$pkg"
