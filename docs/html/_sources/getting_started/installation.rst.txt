.. _doc_installation:

Installation
============

Module
------

Use Godot editor and export templates compiled with the module.

Module is ready to use, no modification to the project is required.

GDNative
--------

GDNative plugin is intended to be used with official Godot editor and export templates.

To install plugin:

1. Create `addons` folder in the root of your project.

2. Copy the contents of archive to the `addons` folder. (Do not drag-and-drop it into `FileSystem` tab of the editor. Right-click `addons` folder, select `Open in File Manager` and use your OS file manager to copy/extract files.)

3. After installing plugin, your file system should look like this:

* [addons]

  * [libgdtl]

    * [bin]
    * [classes]
    * [icons]
    * constants.gd
    * gdtl.gd
    * libgdtl.gdnlib
    * plugin.cfg
    * tl_font_family_edit.gd
    * tl_font_family_preview.gd

4. Go to `Project Settings`, click on the `Plugins` tab and activate `gdtl` plugin.
