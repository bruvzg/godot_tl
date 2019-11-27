.. _doc_compile_android:

Compiling GDNative module for Android
=====================================

Requirements
^^^^^^^^^^^^

- Meson build system
- SCons build system
- Android NDK 21+.

Building `godot-cpp` static library
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. Open a Terminal and navigate to the `godot-cpp` subdirectory.

2. Apply provided patch for Android build:

::

    $ git apply ../patch_gdcpp_android.diff

3. Compile `godot-cpp` static libraries for required platforms, by typing:

::

    $ scons platform=android target=release android_arch=arm64v8 generate_bindings=yes
    $ scons platform=android target=release android_arch=armv7 generate_bindings=yes
    $ scons platform=android target=release android_arch=x86_64 generate_bindings=yes

Building `libgdtl` static library and built-in dependencies
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. Navigate to the root directory of the source.
2. Edit Android NDK path in the Meson cross compile files (`toolchains/android.{arch}.toolchain`).
3. Configure `libgdtl` static libraries, by typing:

::

    $ meson ./_android_arm8 -Dgodot-cpp-lib-name=libgodot-cpp.android.release.arm64v8 --buildtype=release --cross-file ./toolchains/android.arm64.toolchain
    $ meson ./_android_arm7 -Dgodot-cpp-lib-name=libgodot-cpp.android.release.armv7 --buildtype=release --cross-file ./toolchains/android.armv7.toolchain
    $ meson ./_android_x86_64 -Dgodot-cpp-lib-name=libgodot-cpp.android.release.x86_64 --buildtype=release --cross-file ./toolchains/android.x86_64.toolchain

4. Compile `libgdtl` static libraries, by typing:

::

    $ ninja -C ./_android_arm8
    $ ninja -C ./_android_arm7
    $ ninja -C ./_android_x86_64

5. Copy `.so` files to the `libgdtl/bin/android.{arch}/`.