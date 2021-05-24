# CHANGELOG

**BMSNode Firmware**

<!--- next entry here -->

## 0.11.0
2021-05-24

### Breaking changes

#### Switch to attiny1614 MCU ... (2a55aa46d9d8e92b8f2dc927e6d3d6e5ab17419f)

Port to attiny1614 for new v3 boards.

#### large update throughout to support v3 boards with ATtiny1614 (e30d046c8e1ebe2f0c9a8ca7753367f0e07b334d)

port to ATtiny1614 for new v3 boards

- update firmware for new 1614 peripherals
- update unit test code to match
- update host utilities and provisioning
- update docker image for new tools
- runs on v3 board

### Features

- add ext and mcu temp readings #59 #63 (13a85c2701a2d53838f47fefd0708d9c6a62a956)
- **test:** update unit tests for new firmware (234fbc9a661e0e5253d416ebc3021d7cd6a5060e)

### Fixes

- bmsloader dont fail verify due to boot vector [skip ci] (54ebcb713d245d6ee85e9a71c3c48d365cf2591d)
- update doxygen theme, no functional changes (59a458143eadd1e3a4ab2efe0a7428d2f54642e6)
- parser 5 sec timeout if partial packet rcvd #53 (1ef0f593e5e0951020332dc2a4b3f6ad89fb35cb)
- add watchdog for all states #57 (ec0142371ed20600758fafd520ef471fcb175d23)
- add code size check and todo report #16 #23 (03cab9428c2535adda2ccccb1ad3ce2c8b56be7c)
- **bootloader:** add optiboot_x bootloader for 1614 mcu [skip ci] (93403db634dcb227a4340eefd0ffbf9192e46182)
- **bootloader:** clean up bootloader files location [skip ci] (3dea763c7cf635d99827d9fe64ddd64f67260f21)
- **bootloader:** reorg bl files, add optiboot_x prep for upcoming 1614 (d636d9f268b892582a42edac20d26f987fbf282e)
- **build:** update docker to support attiny1614 build tools (f8632120b6fafb8a97b092e977a2c38ddf63a43f)
- update provisioning for 1614, update bmsutil for optiboot_x (0c4dd1c2d61cd927362e1cc5e51e32b5cdf68130)
- **adc:** update ADC for v3 board/new MCU (175ae308bd40bbbf4978699331f658d559c7543c)
- adjust pwm freq and set gpio for lower power (15b64e6fb7bd172e4cc5029d2963972530b66641)
- update docs for new v3 design (a6a2718c1aaa3abd8cf070987b76fcebbab45fe1)

## 0.10.3
2020-12-13

### Fixes

- clarify SETPARM/ADDR behavior #49 (1ff1358f7e3bff6137f6228bb37dbe225f90a680)
- move ADC timing to ADC module #60 (8825875b91f37304cfcf5e08ffd34e2c457a6136)
- **cfg:** add factory reset command #39 (9f9788570d55152e838db2c6d798e3dbfc92f58c)
- **shunt:** shunt PWM adjustment smoothing #56 (9bd68c4ddca2049f27124d9d1d428418fa753e0e)
- **adc:** add ADC smoothing #55 (7e6515c5aa090293eaef1981e48ef302ce49cd6b)

## 0.10.2
2020-11-25

### Fixes

- error in ADC sample timing (7d2f162bb28aeae67cbf1d377a4c83568e7696ef)

## 0.10.1
2020-11-24

### Fixes

- reduce PWM freq to about 500 Hz (12f52fc66e7e79d5671e6a52fa1c08ec3890619c)

## 0.10.0
2020-11-20

### Features

- add LED abstraction (83b7cf23c54677f906d4fe7183a3ea36e00363f9)
- add list helper module (192158985f4fd856bf4a817ad04fa97d9401f342)
- add scheduled timers (b97bfb179dad8c550958e9efee8824fbba3d0272)
- change rate to 9600 [skip ci] (4c648204e52240b196c83ebb81f44e276505d6af)
- **shunt:** use PWM for shunt with temp limit #46 ... (6b1fc0c2f4c6c314012abe22d0b141fecc64ca70)

### Fixes

- add new v2.0 boards to utils (5ea10c7d8377e02a46e3715a97ae2d6ccb02eb1d)
- update bootloader hex files with latest optiboot version [skip ci] (786e85ef6c7f703e1141ca28171034af07e43c82)
- parameters not being stored #54 (707c34b3d10cb25dc1c8858e91fb9a1d7935cb54)
- reduce app startup blink timeout from 5 to 1 seconds (850e9c3c3ec15ff5742023f1b71a9fe7285a3fa9)
- convert main loop to state machine (00803927f8f014e3f7a9dac955dcea90e9b61e93)
- clean up and improve documentation, add gitlab pages (1f392fb8b6c6dc6547fc4d928a29b888be3119ee)
- clean up some cppcheck warnings (36d594b48561e57a2503d5ad2428e3d98ad5218b)

## 0.9.0
2020-09-17

### Features

- change serial rate to 2400, update tools and docs (05e7fe85ff92970f6a9ae47a97dd1c3b53f979d3)

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
