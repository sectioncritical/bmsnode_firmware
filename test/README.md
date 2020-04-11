BMS Node Firmware Unit Test
===========================

These unit tests are used to exercise and validate various functions of the
BMS Node firmware. This is not a functional or systems level test.

Because the target device is such a constrained MCU it is difficult to run
actual unit tests on the MCU itself. Instead, the unit tests are compiled
as a native program on the host system. This allows the correctness of
individual functions or modules to be verified.

As much as possible, the production firmware is written to use functions that
can be tested in this way.

This does not take into account differences between the compilers and target
processors. For example the AVR compiler int size is 2, while on most desktop
computers it is 4. It is possible that the code may execute differently on
the AVR target than on the test host due to differences like this.

Catch2 Unit Test Framework
--------------------------

These tests use the [Catch2](https://github.com/catchorg/Catch2) unit test
framework. This is a header-only framework and makes it very easy to write
decent unit tests quickly.

The Catch2 header file (`catch.hpp`) is included in this directory and is
under a different license, the "Boost Software License".
