#!/usr/bin/env bash
root=$(cd "$(dirname "$0")/.."; pwd)
version=$(TZ=UTC date +v%Y%m%d)
cd "$root/build/release"

case $(uname -s) in
  Darwin)
    libclang=(lib/clang+llvm-*/lib/libclang.dylib)
    strip_option="-x"
    name=cquery-$version-x86_64-apple-darwin ;;
  FreeBSD)
    libclang=(lib/clang+llvm-*/lib/libclang.so.?)
    strip_option="-s"
    name=cquery-$version-x86_64-unknown-freebsd10 ;;
  Linux)
    libclang=(lib/clang+llvm-*/lib/libclang.so.?)
    strip_option="-s"
    name=cquery-$version-x86_64-unknown-linux-gnu ;;
  *)
    echo Unsupported >&2
    exit 1 ;;
esac

pkg=$(mktemp -d)
mkdir "$pkg/$name"
rsync -rtLR bin "./${libclang[-1]}" ./lib/clang+llvm-*/lib/clang/*/include "$pkg/$name"

cd "$pkg"
strip "$strip_option" "$name/bin/cquery" "$name/${libclang[-1]}"
case $(uname -s) in
  Darwin)
    # https://developer.apple.com/legacy/library/documentation/Darwin/Reference/ManPages/man1/tar.1.html
    # macOS's bsdtar is lack of flags to set uid/gid.
    # First, we generate a list of file in mtree format.
    tar -cf filelist --format=mtree --options="!all,time,mode,type" "$name"
    # Then add a line "/set uid=0 gid=0" after the first line "#mtree".
    awk '/#mtree/{print;print "/set uid=0 gid=0";next}1' filelist > newflielist
    # Finally, use the list to generate the tarball.
    tar -zcf "$root/build/$name.tar.gz" @newflielist ;;
  Linux)
    tar -Jcf "$root/build/$name.tar.xz" --owner 0 --group 0 $name ;;
  *)
    tar -Jcf "$root/build/$name.tar.xz" --uid 0 --gid 0 $name ;;
esac
rm -r "$pkg"
