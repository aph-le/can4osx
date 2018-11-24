can4osx
=======
(C) 2018 Alexander Philipp

* [Project Website](http://aph-le.github.io/can4osx/)

Version 0.0.2

## Overview

User-space driver using IOKitLib and IOUSBLib CAN to USB Adapters using the Kvaser canlib API.
Based on Kvaser canlib linux driver.

## Supported Devices

* [Kvaser Leaf Light v2](http://www.kvaser.com/products/kvaser-leaf-light-v2/)
* [Kvaser Leaf Pro v2 (classical CAN and receive CAN-FD)](https://www.kvaser.com/product/kvaser-leaf-pro-hs-v2)

* [IXXAT USB-to-CAN FD Automotive](https://www.ixxat.com/de/produkte/industrie-produkte/pc-interfaces/pc-can-interfaces/pc-can-interfaces-details)


## Usage

can4osx is a user space driver, only supporting usb devices right now. So just include the files in your project.


## Todo

* Almost ervery command has to be reworked
* Overhaul of the internal data structure
* Add new interfaces
