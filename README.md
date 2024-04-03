# The Skybox4mc project

## About

The Skybox4mc project provides a simple CLI tool to convert UV maps to cubemaps. Additionally, it creates a Minecraft ressource pack to use this UV map in game so you don't have to build it yourself. The ressource pack is compatible with [OptiFine](https://optifine.net/home) and [FabricSkyboxes](https://github.com/AMereBagatelle/fabricskyboxes).

## Built with

* Compiled using [MinGW-w64](https://www.mingw-w64.org/)
* Built with [SDL](https://www.libsdl.org/)
* Built with [SDL_image](https://github.com/libsdl-org/SDL_image)
* Built with [libzip](https://libzip.org/)

## How to use it ?

### Installation

* Download the latest release
* Extract the files
* Done

### Usage

General case: call `<path to the Skybox4mc executable> <additional parameters>`.

The tool has up to 6 optional parameters:
* `inputImagePath` specifies the path to the input image. Default: `./inputImage.jpg`.
* `outputArchivePath` specifies the path to the output archive. Default: `./outputArchive.zip`.
* `outputImageTileDimensions` specifies the dimensions of the output image tiles. Default: `2048`.
* `previewImageDimensions` specifies the dimensions of the preview image. Default: `256`.
* `packFormat` specifies the Minecraft ressource pack format. Default: `1` corresponds to a pack made for Minecraft `1.8.9`. More information available on the [wiki](https://minecraft.wiki/w/Pack_format).
* `packDescription` specifies a custom pack description.

!!! The additional parameters need to be specified in the right order.
