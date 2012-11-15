Kate Include Helper Plugin
==========================

Information
-----------

This plugin intended to simplify the hard life of C/C++ programmers who use Kate to write code :-)
First of all, I tired to use the file browser to open mine (or system) header files. With this version
0.1 of the plugin one may press F10 to open a header file that has its name under cursor.
Actually, you are not even required to move a cursor to a file name if the current line starts with
``#include`` directive...

Requirements
------------

* [Kate](http://kate-editor.org) editor version >= 2.9.


Installation
------------

* Clone sources into (some) working dir
* To install into your home directory you have to specify a prefix like this::

        $ cd <plugin-sources-dir>
        $ mkdir build && cd build
        $ cmake -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX=~/.kde4 .. && make && make install

* To make a system-wide installation, set the prefix to /usr and become a superuser to ``make install``
* After that u have to enable it from ``Settings->Configure Kate...->Plugins`` and configure the include paths
  globally and/or per session...

Note: One may use ``kde4-config`` utility with option ``--localprefix`` or ``--prefix`` to get
user or system-wide prefix correspondingly.


Some Features in Details
========================

Open Header/Implementation: How it works
----------------------------------------

Kate shipped with a plugin named *Open Header*, but sooner after I started to use it I've found
few cases when it can't helps me. Nowadays I have 2 "real life" examples when it fails:

Often one may find a source tree splitted into ``${project}/src/`` and ``${project}/include`` dirs.
So, when you are at some header from ``include/`` dir, that plugin never will find your source file.
And vise versa.

The second case: sometimes you have a really big class defined in a header file
(let it be ``my_huge_application.hh``). It may consist of few dickers of methods each of which is
hundred lines or so. In that case I prefer to split implementation into several files and name them
aftr a base header like ``my_huge_application_cmd_line.cc`` (for everything related to command line parsing),
``my_huge_application_networking.cc`` (for everything related to network I/O), and so on. As you may guess
"Open Header" plugin will fail to find a corresponding header for that source files.

Starting from version 0.5 Include Helper Plugin can deal with both mentioned cases!
So, you don't need an Open Header anymore! It is capable to "simple" toggle between header and source files,
like Open Header plugin do, but slightly smarter :-)

TBD more details

Some important notes
--------------------

* monitoring too much (nested) directories, for example in ``/usr/include`` configured as
  system directory, may lead to high resources consumption, so ``inotify_add_watch`` would
  return a ``ENOSPC`` error (use ``strace`` to find out and/or check kate's console log for
  **strange** messages from ``DirWatch``).
  So if your system short on resources just try to avoid live ``#include`` files status updates.
  Otherwise one may increase a number of available files/dirs watches by doing this::

    # echo 16384 >/proc/sys/fs/inotify/max_user_watches

  To make it permanent add the following to ``/etc/sysctl.conf`` or ``/etc/sysctl.d/inotify.conf``
  (depending on system)::

    fs.inotify.max_user_watches = 16384

TODO
====

* Add autocompleter for ``#include`` files (done)
* Handle multiple matches (done)
* Passive popups if nothing found (done)
* Handle #include files w/ relative path
* Use Shift+F10 to go back in stack (?)
* Form an ``#include`` directive w/ filename currently active in a clipboard (done)
* List of currently ``#included`` files in a dialog and/or menu (done)
* OpenFile dialog for current ``#include`` line
* Is it possible to use annotations iface somehow to indicate 'not-found' ``#include`` file?
* Add quick open dialog -- like quick document switcher, but allows to find file to open
  based on configured include paths by partial name match...
* Add view to explore a tree of #included files
* Add option(s) to include/exclude files from completion list
* Issue a warning if /proc/sys/fs/inotify/max_user_watches is not high enough


Changes
=======

Version 0.5
-----------

* add an action to switch between header and implementation file, just like an official *Open Header*
  plugin but smarter ;-) See details above.

Version 0.4.5
-------------

* fix nasty bug w/ path remove from config dialog

Version 0.4.4
-------------

* if file going to open is inaccessible for writing, open it in RO mode, so implicit modifications
  (like TAB to space conversions or trailing spaces removal) wouldn't annoy on close

Version 0.4.3
-------------

* make directory monitoring optional and configured via plugin's *Other Settings* configuration page

Version 0.4.2
-------------

* watch configured directories for changes and update ``#include`` files status
* add support to create source tarball

Version 0.4.1
-------------

* open dialog w/ currently ``#included`` files, if unable to open a file under cursor
  or cursor not on a word at all
* remove duplicates from completion list: for out of source builds and if both, source
  and binary dirs are in the search list, it led to duplicates