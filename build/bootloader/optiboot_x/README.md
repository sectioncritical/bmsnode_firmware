OPTIBOOT_X
==========

optiboot_x is the name of the optiboot boot loader used for the '1614. It
comes from the Arduino core package `megaTinyCore` maintained by GitHub user
"SpenceKonde". Here is the original repo:

https://github.com/SpenceKonde/megaTinyCore

I needed to make some modification to use it with BMSNode (v3), so I made my
own fork, here:

https://github.com/SpenceKonde/megaTinyCore

And the branch with the modifications is `feature/bmsnodev3`.

**NOTE:** the megaTinyCore repo has a lot of stuff in it for Arduino support
for these AVR cores. I am only using the boot loader out of this repo, not any
of the other Arduino stuff.

Using
-----

There is already a version of the boot loader here that is commited in this
repo. It is used by the higher level build utilities whenever the boot loader
needs to be loaded on a board. Therefore, normally it is not necessary to do
anything here unless you need to rebuild the boot loader for some reason.

Building
--------

In case it is necessary to rebuild the boot loader (maybe it needs a
modification?), you can use the Makefile to build it. You can see all the
available Makefile targets by using:

    make help

But to build it you just need to do:

    make optiboot_x

This will clone the repo, build the boot loader, and place a copy at the same
location as this README file.

In case you need to make modifications to the boot laoder that need to be
pushed back to the boot loader repo, use:

    make dev

This will clone the repo using ssh which makes it easier to push back changes.
