# This is not a real module, it's simply an introductory text.

"""
The Blender Python API Reference
================================

 Top Module:
 -----------

  - L{Blender} (*)

 Submodules:
 -----------
  - L{Armature}
     - L{Bone}
     - L{NLA}
  - L{BGL}
  - L{Camera} (*)
  - L{Curve}
  - L{Draw} (*)
  - L{Effect}
  - L{Image} (*)
  - L{Ipo}
  - L{Lamp} (*)
  - L{Lattice}
  - L{Library}
  - L{Material} (*)
  - L{Mathutils}
  - L{Metaball} (*)
  - L{NMesh}
  - L{Noise}
  - L{Object} (*)
  - L{Registry}
  - L{Scene} (*)
     - L{Radio} (new)
     - L{Render}
  - L{Text}
  - L{Texture}
  - L{Types}
  - L{Window} (* important: L{Window.EditMode})
  - L{World} (*)
  - L{sys<Sys>} (*)

 (*) - marks updated.

Introduction:
=============

 This reference documents the Blender Python API, a growing collection of
 Python modules (libraries) that give access to part of the program's internal
 data and functions.
 
 Through scripting Blender can be extended in realtime via
 U{Python <www.python.org>}, an impressive high level, multi-paradigm, open
 source language.  Newcomers are recommended to start with the tutorial that
 comes with it.

 This opens many interesting possibilities, ranging from automating repetitive
 tasks to adding new functionality to the program: procedural models,
 importers and exporters, even complex applications and so on.  Blender itself
 comes with some scripts, but many others can be found in the Scripts & Plugins
 sections and forum posts at the Blender-related sites listed below.

Scripting and Blender:
======================

There are four basic ways to execute scripts in Blender:

 1. They can be loaded or typed as text files in the Text Editor window, then
 executed with ALT+P.
 2. Via command line: 'blender -P <scriptname>' will start Blender and executed
 the given script.  <scriptname> can be a filename in the user's file system or
 the name of a text saved in a .blend Blender file:
 'blender myfile.blend -P textname'.
 3. Properly registered scripts can be selected directly from the program's
 menus.
 4. Scriptlinks: these are also loaded or typed in the Text Editor window and
 can be linked to objects, materials or scenes using the Scriptlink buttons
 tab.  Script links get executed automatically when their events (ONLOAD,
 REDRAW, FRAMECHANGED) are triggered.  Normal scripts can create (L{Text}) and
 link other scripts to objects and events, see L{Object.Object.addScriptLink},
 for example.

Registering scripts:
--------------------
 To be registered a script needs two things:
   - be either in the default scripts dir or in the user defined scripts path
     (see Info window, paths tab);
   - have a proper header.

 Try 'blender -d' to know where your default dir for scripts is, it will
 inform either the dir or the file with that info already parsed, which is
 in the same dir of the scripts folder.

 The header should be like this one (all double and single apostrophes below
 are required)::
  #!BPY
  \"\"\"
  Name: 'Script Name'
  Blender: 233
  Group: 'Export'
  Submenu: 'All' all
  Submenu: 'Selected' sel
  Submenu: 'Configure (gui)' gui
  Tooltip: 'Export to some format.'
  \"\"\"

 where:
  - B{Name} is the string that will appear in the menu;
  - B{Blender} is the minimum program version required to run the script;
  - B{Group} defines where the script will be put, see all groups in the
    Scripts Window's header, menu "Scripts";
  - B{Submenu} adds optional submenus for further control;
  - B{Tooltip} is the (short) tooltip string for the menu entry.

 Submenu lines are not required, use them if you want to provide extra
 options.  To see which submenu the user chose, check the "__script__"
 dictionary in your code: __script__['arg'] has the defined keyword (the word
 after the submenu string name: all, sel or gui in the example above) of the
 chosen submenu.  For example, if the user clicked on submenu 'Selected' above,
 __script__['arg'] will be "sel".

 If your script requires extra data or configuration files, there is a special
 folder where they can be saved: see 'datadir' in L{Blender.Get}.

Interaction with users:
-----------------------

 Scripts can:
  - simply run and exit;
  - grab the main input event queue and process (or pass to Blender) selected
    keyboard, mouse, redraw events;
  - pop messages, menus and small number and text input boxes;
  - draw graphical user interfaces (guis) with OpenGL calls and native
    program buttons, which stay there accepting user input like any other
    Blender window until the user closes them;
  - make changes to the 3D View (set visible layer(s), view point, etc);
  - use external Python libraries, if available.

 You can read the documentation for the L{Window}, L{Draw} and L{BGL} modules
 for more information and also check Python's site for external modules that
 might be useful to you.  Note though that any imported module will become a
 requirement of your script, since Blender itself does not bundle external
 modules.

Command line mode:
------------------

 Python was embedded in Blender, so to access bpython modules you need to
 run scripts from the program itself: you can't import the Blender module
 into an external Python interpreter.  But with "OnLoad" script links, the
 "-b" background mode and additions like the "-P" command line switch,
 L{Blender.Save}, L{Blender.Load}, L{Blender.Quit} and the L{Library} module,
 for many tasks it's possible to control Blender via some automated process
 using scripts.

Demo mode:
----------

 Blender has a demo mode, where once started it can work without user
 intervention, "showing itself off".  Demos can render stills and animations,
 play rendered or realtime animations, calculate radiosity simulations and
 do many other nifty things.  If you want to turn a .blend file into a demo,
 write a script to run the show and link it as a scene "OnLoad" scriptlink.
 The demo will then be played automatically whenever this .blend file is
 opened, B{unless Blender was started with the "-y" parameter}.

The Game Engine API:
--------------------

 Blender has a game engine for users to create and play 3d games.  This
 engine lets programmers add scripts to improve game AI, control, etc, making
 more complex interaction and tricks possible.  The game engine API is
 separate from the Blender Python API this document references and you can
 find its own ref doc in the docs section of the main sites below.

A note to newbie script writers:
--------------------------------

 Interpreted languages are known to be much slower than compiled code, but for
 many applications the difference is negligible or acceptable.  Also, with
 profiling to identify slow areas and well thought optimizations, the speed
 can be I{considerably} improved in many cases.  Try some of the best bpython
 scripts to get an idea of what can be done, you may be surprised.

@author: The Blender Python Team
@requires: Blender 2.34 or newer.
@version: 2.34
@see: U{www.blender3d.org<http://www.blender3d.org>}: main site
@see: U{www.blender.org<http://www.blender.org>}: documentation and forum
@see: U{www.elysiun.com<http://www.elysiun.com>}: user forum
@see: U{projects.blender.org<http://projects.blender.org>}
@see: U{www.python.org<http://www.python.org>}
@see: U{www.python.org/doc<http://www.python.org/doc>}
@note: this documentation was generated by epydoc, which can output html and
   pdf.  For pdf it requires a working LaTeX environment.
"""
