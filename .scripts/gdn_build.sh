#!/bin/bash

set -ev
set -x #echo on

meson --version
ninja --version
scons --version
clang --version

cd godot-cpp
scons -j2 platform=linux bits=64 target=debug generate_bindings=yes use_custom_api_file=yes custom_api_file="../api_3_1.json"

cd ..

meson ./_build_linux64 -Dgodot-cpp-lib-name=libgodot-cpp.linux.debug.64 -Dbuiltin-runtime=true -Duse-graphite2=true
ninja -C ./_build_linux64
