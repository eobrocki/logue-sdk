# logue-sdk 

[日本語](./README_ja.md)

This repository contains all the files and tools needed to build custom oscillators and effects for the [prologue](https://www.korg.com/products/synthesizers/prologue), [minilogue xd](https://www.korg.com/products/synthesizers/minilogue_xd) and [Nu:Tekt NTS-1 digital kit](https://www.korg.com/products/dj/nts_1) synthesizers.

#### Ready to Use Content

To download ready to use oscillators and effects, refer to the [Unit Index](https://korginc.github.io/logue-sdk/unit-index/) and follow instructions on the developer's website.

#### Compatibility Notes

In order to run user units built with SDK version 1.1-0, the following firmware versions are required:
* prologue: >= v2.00
* minilogue xd: >= v2.00
* Nu:Tekt NTS-1 digital: >= v1.02

## Quick Start
### Clone repository
$ git clone https://github.com/korginc/logue-sdk.git
cd logue-sdk
### Check out submodules (CMSIS)
git submodule update --init --recursive
### Build tool dependencies
cd tools/logue-cli
make
cd ../gcc
make
### Build sample oscillator
cd ../../prologue/demos/waves/
make


#### Overall Structure:
* [platform/prologue/](platform/prologue/) : prologue specific files, templates and demo projects.
* [platform/minilogue-xd/](platform/minilogue-xd/) : minilogue xd specific files, templates and demo projects.
* [platform/nutekt-digital/](platform/nutekt-digital/) : Nu:Tekt NTS-1 digital kit specific files, templates and demo projects.
* [platform/ext/](platform/ext/) : External dependencies and submodules.
* [tools/](tools/) : Installation location and documentation for tools required to build projects and manipulate built products.
* [devboards/](devboards/) : Information and files related to limited edition development boards.

## Sharing your Oscillators/Effects with us

To show us your work please reach out to *logue-sdk@korg.co.jp*.

## Support

The SDK is provided as-is, no technical support will be provided by KORG.

<!-- ## Troubleshooting -->





