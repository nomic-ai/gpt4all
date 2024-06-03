#!/bin/sh
# Build script for Unix-like systems (Linux, macOS).
# Script assumes the current working directory is the bindings project root.

SYSNAME=$(uname -s)
PLATFORM=$(uname -m)

# Allows overriding target sysname and platform via args
# If not provided, the current system's sysname and platform will be used

while [ $# -gt 0 ]; do
  case "$1" in
    --sysname=*)
      SYSNAME="${1#*=}"
      shift
      ;;
    --platform=*)
      PLATFORM="${1#*=}"
      shift
      ;;
    *)
      echo "Unknown argument: $1" >&2
      exit 1
      ;;
  esac
done

if [ "$SYSNAME" = "Linux" ]; then
  if [ "$PLATFORM" = "x86_64" ]; then
    BASE_DIR="runtimes/linux-x64"
  elif [ "$PLATFORM" = "aarch64" ]; then
    BASE_DIR="runtimes/linux-arm64"
  else
    echo "Unsupported platform: $PLATFORM" >&2
    exit 1
  fi
  LIB_EXT="so"
elif [ "$SYSNAME" = "Darwin" ]; then
  BASE_DIR="runtimes/darwin"
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

if [ "$PLATFORM" = "x86_64" ]; then
  echo "Building for x86_64"
  cmake -S ../../gpt4all-backend -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=RelWithDebInfo
fi

if [ "$PLATFORM" = "aarch64" ]; then
  if [ "$(uname -m)" != "aarch64" ]; then
    echo "Cross-compiling for aarch64"
    cmake -S ../../gpt4all-backend \
      -B "$BUILD_DIR" \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCMAKE_TOOLCHAIN_FILE="./toolchains/linux-arm64-toolchain.cmake"
  else
    cmake -S ../../gpt4all-backend -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=RelWithDebInfo
  fi
fi

cmake --build "$BUILD_DIR" --parallel && {
  cp "$BUILD_DIR"/libgptj*.$LIB_EXT   "$NATIVE_DIR"/
  cp "$BUILD_DIR"/libllama*.$LIB_EXT  "$NATIVE_DIR"/
}