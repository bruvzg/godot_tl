.. _doc_compile_ios:

Compiling GDNative module for iOS
=================================

Requirements
^^^^^^^^^^^^

- Meson build system
- SCons build system
- Xcode 10.0 (or later) with the iOS (10.0) SDK and the command line tools.

Building `godot-cpp` static library
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. Open a Terminal and navigate to the `godot-cpp` subdirectory.

2. Apply provided patch for iOS build:

::

    $ git apply ../patch_gdcpp_ios.diff

3. Compile `godot-cpp` static libraries for required platforms (`arm64` and `armv7` for real devices and `x86_64` for simulator), by typing:

::

    $ scons platform=ios target=release ios_arch=arm64 generate_bindings=yes
    $ scons platform=ios target=release ios_arch=armv7 generate_bindings=yes
    $ scons platform=ios target=release ios_arch=x86_64 generate_bindings=yes

Building `libgdtl` static library and built-in dependencies
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. Navigate to the root directory of the source.
2. Edit iOS SDK and toolchain paths in the Meson cross compile files (`toolchains/ios.{arch}.toolchain`).
3. Configure `libgdtl` static libraries, by typing:

::

    $ meson ./_ios_arm8 -Dgodot-cpp-lib-name=libgodot-cpp.ios.release.arm64 --buildtype=release --cross-file ./toolchains/ios.arm64.toolchain
    $ meson ./_ios_arm7 -Dgodot-cpp-lib-name=libgodot-cpp.ios.release.armv7 --buildtype=release --cross-file ./toolchains/ios.armv7.toolchain
    $ meson ./_ios_x86_64 -Dgodot-cpp-lib-name=libgodot-cpp.ios.release.x86_64 --buildtype=release --cross-file ./toolchains/ios.x86_64.toolchain

4. Compile `libgdtl` static libraries, by typing:

::

    $ ninja -C ./_ios_arm8
    $ ninja -C ./_ios_arm7
    $ ninja -C ./_ios_x86_64

5. (Optional) Bundle `libgdtl` and it's dependencies into one file:

::

    $ libtool -static -o ./_ios_arm8/libgdtl_full.a ./_ios_arm8/libgdtl.a ./_ios_arm8/subprojects/freetype2/libfreetype2.a ./_ios_arm8/subprojects/graphite2/libgraphite2.a ./_ios_arm8/subprojects/harfbuzz/libharfbuzz.a ./_ios_arm8/subprojects/icu4c/libicu4c.a ./_ios_arm8/subprojects/libpng/liblibpng.a ./_ios_arm8/subprojects/zlib/libzlib.a ./godot-cpp/bin/libgodot-cpp.ios.release.arm64.a
    $ libtool -static -o ./_ios_arm7/libgdtl_full.a ./_ios_arm7/libgdtl.a ./_ios_arm7/subprojects/freetype2/libfreetype2.a ./_ios_arm7/subprojects/graphite2/libgraphite2.a ./_ios_arm7/subprojects/harfbuzz/libharfbuzz.a ./_ios_arm7/subprojects/icu4c/libicu4c.a ./_ios_arm7/subprojects/libpng/liblibpng.a ./_ios_arm7/subprojects/zlib/libzlib.a ./godot-cpp/bin/libgodot-cpp.ios.release.armv7.a
    $ libtool -static -o ./_ios_x86_64/libgdtl_full.a ./_ios_x86_64/libgdtl.a ./_ios_x86_64/subprojects/freetype2/libfreetype2.a ./_ios_x86_64/subprojects/graphite2/libgraphite2.a ./_ios_x86_64/subprojects/harfbuzz/libharfbuzz.a ./_ios_x86_64/subprojects/icu4c/libicu4c.a ./_ios_x86_64/subprojects/libpng/liblibpng.a ./_ios_x86_64/subprojects/zlib/libzlib.a ./godot-cpp/bin/libgodot-cpp.ios.release.x86_64.a

6. (Optional) Bundle static libs for all platforms into one "fat" file:

::

    $ lipo -create ./_ios_arm8/libgdtl_full.a ./_ios_arm7/libgdtl_full.a ./_ios_x86_64/libgdtl_full.a -output ./libgdtl/bin/libgdtl_ios_fat.a

Adding `libgdtl` static library to the exported Xcode project
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. Copy `libgdtl` static library file(s) into exported project directory (alongside with `{app_name}.xcode` and `{app_name}.a` files).
2. Open exported project in the Xcode.
3. Select `Targets` > Exported App Name > `General` > `Frameworks, Libraries, and Embedded Content`
4. Drag-and-drop (or use `+` button) `libgdtl` fat (or individual platform) file(s) to the list.

.. image:: img/ios_adding_static_lib.png
