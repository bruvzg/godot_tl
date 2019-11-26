![libgdtl logo](logo.png)

# libgdtl

BiDi, shaping and basic text layout for Godot Engine.

## Dependencies

- Godot 3.1+
- C++14 compiler
- Meson build system (for gdnative module build only)
- SCons build system

## Compiling (as builtin module)

### Build options

Name | Description | Default value
--- | --- | ---
`builtin_runtime` | Use the built-in libraries | `true`
`use_graphite2` | Enable SIL Graphite 2 complementary shaper | `true`
`use_font_wrapper` | Enable Godot font wrapper for default controls | `false`

If `use_font_wrapper` is enabled, apply `patch_font.diff` from the root of this repository to the Godot engine source first.

### Building `libgdtl` module

Clone this repository (without `--recursive` flag) into Godot's `modules` subfolder as `godot_tl`.
Rebuild Godot engine as usual.

## Compiling (as gdnative module)

### Build options

Name | Description | Default value
--- | --- | ---
`godot-cpp-lib-name` | godot-cpp static library name (without `.a` or `.lib` extension) | `libgodot-cpp`
`static-lib` | Build static library | `false`
`static-runtime` | Link libraries statically for better portability | `false`
`builtin-runtime` | Use the built-in libraries | `false`
`use-graphite2` | Enable SIL Graphite 2 complementary shaper | `true`

### Building `godot-cpp` static library

See https://github.com/GodotNativeTools/godot-cpp/blob/master/README.md#compiling-the-cpp-bindings-library

### Building `libgdtl` module

You can compile this module by executing:
```
meson {Target-Folder} -Dgodot-cpp-lib-name={Godot-CPP-Name} --buildtype=release
ninja -C {Target-Folder}
```

### Exporting projects with `libgdtl` module (iOS)

After exporting Xcode project form the Godot editor, add module static library and its dependencies to the exported project.

Navigate to `Targets` > Exported App Name > `General` > `Frameworks, Libraries, and Embedded Content` and drag-and-drop following files to the list:

* `libgdtl.a`
* `libgodot-cpp.a`
* `libfreetype2.a`
* `libgraphite2.a` (optional)
* `libharfbuzz.a`
* `libicu4c.a`
* `libpng.a`
* `libzlib.a`

![Xcode iOS libs](ios_libs.png)

## Documentation

[Documentation](https://bruvzg.github.io/godot_tl/docs/html/index.html)

[Class reference](https://bruvzg.github.io/godot_tl/docs/html/classes/index.html)

## Screenshots

<details>
  <summary>Screenshots</summary>

![libgdtl all scripts screenshot](scripts_scr.png)

![libgdtl debug view screenshot](debug_view.png)

![iOS simulator screenshot](ios_scr.png)

![Android screenshot](andr_scr.jpg)

</details>

## License
- The source code of the **libgdtl** module is released under unlicense.
For more information, see http://unlicense.org/ or the accompanying UNLICENSE file.

- **Godot** and **GodotNativeTools** are licensed under MIT license.
For more information, see https://github.com/godotengine/godot/blob/master/LICENSE.txt.

- **HarfBuzz** is licensed under MIT-like License.
For more information, see https://github.com/harfbuzz/harfbuzz/blob/master/COPYING

- **ICU4C** is licensed under Unicode, Inc. License.
For more information, see http://www.unicode.org/copyright.html#License

- **FreeType** is licensed under FreeType License (BSD-like) or GNU General Public License (GPL), version 2.
For more information, see https://www.freetype.org/license.html

- **SIL Graphite engine** is licensed under GNU Lesser General Public License (LGPL), version 2.1+ or GNU General Public License (GPL), version 2 or Mozilla Public License.
For more information, see https://github.com/silnrsi/graphite/blob/master/COPYING

## Demo data

[Montserrat](https://github.com/JulietaUla/Montserrat/), [Awami Nastaliq](https://software.sil.org/awami/download/), [Comic Neue](http://comicneue.com/) and [Noto](https://www.google.com/get/noto/) fonts are published under the [SIL Open Font License, Version 1.1](https://scripts.sil.org/cms/scripts/page.php?site_id=nrsi&id=OFL)

[Material Design icons by Google](https://github.com/google/material-design-icons) are published under the [Apache License Version 2.0](https://www.apache.org/licenses/LICENSE-2.0.txt)

Noto Color Emoji font is cut down to single glyph (U+1F604) using [glyphhanger](https://github.com/filamentgroup/glyphhanger).

[![Travis Build Status](https://travis-ci.org/bruvzg/godot_tl.svg?branch=master)](https://travis-ci.org/bruvzg/godot_tl)
