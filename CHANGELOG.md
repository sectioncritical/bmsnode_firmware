# CHANGELOG

**BMSNode Firmware**

<!--- next entry here -->

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
