BMS Cell Node Firmware
======================

## 2025 update

I moved this project from gitlab. It used gitlab CI and there are various links
to gitlab stuff (like the badge links below). I have not updated any of this
yet. So the CI is not working right now and you may run across bad links. I did
update the API docs to be hosted at github so that link below is correct.

* * * * *

[![](https://gitlab.com/kroesche/bmsnode/badges/master/pipeline.svg)](https://gitlab.com/kroesche/bmsnode/pipelines)
[![](https://gitlab.com/kroesche/bmsnode/badges/master/coverage.svg)](https://gitlab.com/kroesche/bmsnode/-/jobs/artifacts/master/file/test/reports/bmstest-coverage.html?job=test)
[![](https://gitlab.com/kroesche/bmsnode/-/jobs/artifacts/master/raw/test/reports/check-badge.svg?job=test)](https://gitlab.com/kroesche/bmsnode/-/jobs/artifacts/master/file/test/reports/check/index.html?job=test)

* * * * *

This repository holds firmware for **BMSNode** BMS cell boards. BMSNode boards
are used to monitor a single cell or set of cells connected in parallel, in a
battery pack. It also includes a shunt resistor that can be used to help with
cell balancing.

This device was heavily inspired by the
[diyBMSv4 board](https://github.com/stuartpittaway/diyBMSv4). However this
firmware was written completely from scratch and does not use any of the
original design or protocol.

The BMSNode board design repository
[can be found here](https://gitlab.com/kroesche/bmsnode_board).

License
-------

[![](doc/img/license-badge.svg)](https://opensource.org/licenses/MIT)

This project uses the [MIT license](https://opensource.org/licenses/MIT).
See the [LICENSE.txt](LICENSE.txt) file for the license terms.

Repo Layout
-----------

```
.
├── build   (scripts for building and other automation)
├── doc     (various documentation and API docs generation)
├── src     (firmware source code)
├── test    (test code and scripts)
└── util    (tools to help with usage and test)
```

Documentation
-------------

The `doc` directory contains most project documentation such as specifications.
Otherwise see the various README files and as a last resort see comments in
script files. Most Makefiles have a `make help` target.

The firmware source code is documented using doxygen-style comments. Generated
documentation [can be found here](https://sectioncritical.github.io/bmsnode_firmware/index.html).

Tools and Environment
---------------------

Assumes a POSIX-ish command line environment and shell like bash. Almost
everything is done from the command line, and using Makefiles. The firmware is
compiled with `avr-gcc`.

