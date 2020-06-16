#!/bin/bash

set -ev

meson --version
ninja --version
scons --version
clang --version

cd godot-cpp
scons -j2 platform=linux bits=64 target=debug generate_bindings=yes

cd ..

mkdir _build_linux64
meson _build_linux64 -Dgodot-cpp-lib-name=libgodot-cpp.linux.debug.64 -Dbuiltin-runtime=true -Duse-graphite2=true
ninja -C _build_linux64
