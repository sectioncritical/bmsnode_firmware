# CHANGELOG

**BMSNode Firmware**

<!--- next entry here -->

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
