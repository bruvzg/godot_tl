.. _doc_about_intro:

libgdtl Introduction
====================

BiDi, shaping and basic text layout for Godot Engine.

Dependencies
------------

- Godot 3.1+
- C++14 compiler
- Meson build system (for gdnative module build only)
- SCons build system

Compiling (as builtin module)
-----------------------------

Build options
^^^^^^^^^^^^^

+------------------+------------------------------------------------+---------------+
| Name             | Description                                    | Default value |
+==================+================================================+===============+
| builtin_runtime  | Use the built-in libraries                     | true          |
+------------------+------------------------------------------------+---------------+
| use_graphite2    | Enable SIL Graphite 2 complementary shaper     | true          |
+------------------+------------------------------------------------+---------------+
| use_font_wrapper | Enable Godot font wrapper for default controls | false         |
+------------------+------------------------------------------------+---------------+

If `use_font_wrapper` is enabled, apply `patch_font.diff` from the root of this repository to the Godot engine source first.

Building `libdgtl` module
^^^^^^^^^^^^^^^^^^^^^^^^^

Clone this repository (without `--recursive` flag) into Godot's `modules` subfolder as `godot_tl`.
Rebuild Godot engine as ususal.

Compiling (as gdnative module)
------------------------------

Build options
^^^^^^^^^^^^^

+--------------------+------------------------------------------------------------------+---------------+
| Name               | Description                                                      | Default value |
+====================+==================================================================+===============+
| godot-cpp-lib-name | godot-cpp static library name (without `.a` or `.lib` extension) | libgodot-cpp  |
+--------------------+------------------------------------------------------------------+---------------+
| static-runtime     | Link libraries statically for better portability                 | false         |
+--------------------+------------------------------------------------------------------+---------------+
| builtin-runtime    | Use the built-in libraries                                       | false         |
+--------------------+------------------------------------------------------------------+---------------+
| use-graphite2      | Enable SIL Graphite 2 complementary shaper                       | true          |
+--------------------+------------------------------------------------------------------+---------------+

Building `godot-cpp` static library
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

See `<https://github.com/GodotNativeTools/godot-cpp/blob/master/README.md#compiling-the-cpp-bindings-library>`_

Building `libdgtl` module
^^^^^^^^^^^^^^^^^^^^^^^^^

You can compile this module by executing:
::

	meson {Targer-Folder} -Dgodot-cpp-lib-name={Godot-CPP-Name} --buildtype=release
	ninja -C {Targer-Folder}

License
-------

- The source code of the **libgdtl** module is released under unlicense.
  
  For more information, see `<http://unlicense.org/>`_ or the accompanying UNLICENSE file.
- **Godot** and **GodotNativeTools** are licensed under MIT license.
  
  For more information, see `<https://github.com/godotengine/godot/blob/master/LICENSE.txt>`_.
- **HarfBuzz** is licensed under MIT-like License.
  
  For more information, see `<https://github.com/harfbuzz/harfbuzz/blob/master/COPYING>`_
- **ICU4C** is licensed under Unicode, Inc. License.
  
  For more information, see `<http://www.unicode.org/copyright.html#License>`_
- **FreeType** is licensed under FreeType License (BSD-like) or GNU General Public License (GPL), version 2.
  
  For more information, see `<https://www.freetype.org/license.html>`_
- **SIL Graphite engine** is licensed under GNU Lesser General Public License (LGPL), version 2.1+ or GNU General Public License (GPL), version 2 or Mozilla Public License.
  
  For more information, see `<https://github.com/silnrsi/graphite/blob/master/COPYING>`_

Demo data
---------

`Montserrat <https://github.com/JulietaUla/Montserrat/>`_, `Awami Nastaliq <https://software.sil.org/awami/download/>`_, `Comic Neue <http://comicneue.com/>`_ and `Noto <https://www.google.com/get/noto/>`_ fonts are published under the `SIL Open Font License, Version 1.1 <https://scripts.sil.org/cms/scripts/page.php?site_id=nrsi&id=OFL>`_

`Material Design icons by Google <https://github.com/google/material-design-icons>`_ are published under the `Apache License Version 2.0 <https://www.apache.org/licenses/LICENSE-2.0.txt>`_

Noto Color Emoji font is cut down to single glyph (U+1F604) using `glyphhanger <https://github.com/filamentgroup/glyphhanger>`_.
