.. _doc_font_substitution:

Font substitution
=================

Font substitution is used to find a replacement for an unavailable character.

In addition to main list of the substitution fonts, you can specify preferred fonts for the script (writing system) and language.

Use `ISO 639-1 <https://en.wikipedia.org/wiki/ISO_639-1>`_ codes for the language names, and `ISO 15924 <https://en.wikipedia.org/wiki/ISO_15924>`_ for the script names.

Substitution lists have following priority: `Language`, `Script`, `Main List`. Providing script and language information is not required, but can improve shaping speed if large number of fonts used.

.. image:: img/font_inspector.png
