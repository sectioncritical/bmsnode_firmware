# CHANGELOG

**BMSNode Firmware**

<!--- next entry here -->

## 0.8.1
2020-09-16

### Fixes

- use cal constants for cell V calc, update cal value #32 #44 #50 (92601191dac303aa6651154281c67befc17c60d6)
- make bmsutil a submodule [skip ci] (0bc446185f1eef7274c4c8cea69182d6ed73a12e)
- add dumpflash target to Makefile for debug [skip ci] (61b0ae45a693d68ccbb2aaf0c9e8823c94fbf7aa)
- add testmode command to firmware ... (e7625ff7bc19fb77d43c023883b7f174b95fab7b)
- update thermistor table to match current part #38 #43 ... (f7b851eb8cfab0656632c9a65a03f7318fbf9c14)
- add visual ident to PING command #7 (59929b1f7b6f96cc5cc3b2769c13c88f6bbc87cf)

## 0.8.0
2020-09-02

### Features

- **board:** add support for new board bmsnode v1.0.0 (e7bcb5206ca2604bc4f03009bcdec424aaa0cb00)
- **board:** add support for JLC v1.0.0 boards #47 (cfa3f25e4701834b903ab26a07c0eb28c9a965db)

### Fixes

- minor doc fixes for board update [skip ci] (eff6563e879645cfdb4744d773ab87194aa51ef5)
- update bootloader README for half-duplex diffs [skip ci] (dea194e26b3e052c9d260fa49ee774623aa7b07d)

## 0.7.0
2020-05-30

### Features

- **cfg:** add config set/get command, new set of parameters ... (f02b16586df129e5a2be71e425a7bea430e37e8d)
- **shunt:** add shunt config, automation, status reporting #10 (b663812c11168afc410fb09ca5a767e5e84b7e4f)

## 0.6.1
2020-05-20

### Fixes

- add doxygen docs generation, no functional change (#35) (19967ce43d8aff563b3bc85697bfbb9f2820b777)

## 0.6.0
2020-05-15

### Features

- **shunt:** add shunt mode and command #11 (a8a202c0139a4c87c40041e19544bcace7367f80)

### Fixes

- **cmd:** add api to get last command processed ... (a88d2a6f46f03f2021295008bcc9e85d8d4e0b49)

## 0.5.0
2020-05-12

### Features

- **adc:** add ADC module and raw data command #30 (f54f3d8220b40a6d46f51ec2dd0f208da26961f3)
- **adc:** add cell voltage reading and status command #8 (eb28af8a6532122f4918185804e6b2a0c583f189)
- **adc:** add temp sensor reading to status command #9 (99f7ec45b99799685282a7c15080c55ec3ed6728)

## 0.4.0
2020-05-10

### Features

- **power:** add power save mode in main loop #13 (968f9676155ddf031cdf633a50de8d30c2b0b5c0)

### Fixes

- add unit test placeholder for "main", no tests implemented yet (d76d739ad1e0754aa68dcc529531eab296d5786c)
- **tmr:** create timer module (dde75572b6e6b316ccdac620d3e34c547f28b8cb)
- **ser:** rename ser module to match others style (72fff5e29ffe9f1dc3ca6aa91ffa5daa86bcaf3f)
- add DFU detection to keep from sleeping during dfu operation (da8790951c8bde038e78636cd84056974d0a42ee)

## 0.3.1
2020-04-29

### Fixes

- **build:** fix release artifact, add build log #18 (0fe50ed1eeaeae2afe36234e6327d247a9e65f69)

## 0.3.0
2020-04-29

### Features

- **build:** add release packaging and versioning #18 (cc9c6b3544e40e8a5be24620d4a3374198bf4e64)
- **cmd:** add fw version to UID command #17 (3d1663961c50007f55c873aacab98826a602131f)
- **build:** add CI tagging and release #18 (e0b63b7c746175e6f31f3b0f81bf19771873581e)

### Fixes

- **req:** update milestones [skip ci] (011ead185ef31a4277324eb71beb7295316c333b)

## 0.2.0
2020-04-23

Initial release as a platform for testing and feature development. This
release has a minimal implementation that can receive and reply to a limited
command set and perform DFU (boot loader) firmware updates.

### Features

- provisioning script to store unique ID and board type in "permanent" memory
- simple (WIP) python utility to communicate with BMSNode devices via serial
- packet protocol parsing with command dispatching and reply mechanism
- PING command used for device discovery on a bus
- ADDR command to set the bus address for a device
- configuration module to support parameters for future features
  (such as calibration constants)
