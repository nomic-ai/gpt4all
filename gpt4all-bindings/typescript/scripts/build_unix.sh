#!/bin/sh

SYSNAME=$(uname -s)

if [ "$SYSNAME" = "Linux" ]; then
  BASE_DIR="runtimes/linux-x64"
  LIB_EXT="so"
elif [ "$SYSNAME" = "Darwin" ]; then
  BASE_DIR="runtimes/osx"
  LIB_EXT="dylib"
elif [ -n "$SYSNAME" ]; then
  echo "Unsupported system: $SYSNAME" >&2
  exit 1
else
  echo "\"uname -s\" failed" >&2
  exit 1
fi

NATIVE_DIR="$BASE_DIR/native"
BUILD_DIR="$BASE_DIR/build"

rm -rf "$BASE_DIR"
mkdir -p "$NATIVE_DIR" "$BUILD_DIR"

cmake -S ../../gpt4all-backend -B "$BUILD_DIR" &&
cmake --build "$BUILD_DIR" -j --config Release && {
  cp "$BUILD_DIR"/libbert*.$LIB_EXT   "$NATIVE_DIR"/
  cp "$BUILD_DIR"/libgptj*.$LIB_EXT   "$NATIVE_DIR"/
  cp "$BUILD_DIR"/libllama*.$LIB_EXT  "$NATIVE_DIR"/
}
