# CHANGELOG

**BMSNode Firmware**

<!--- next entry here -->

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
