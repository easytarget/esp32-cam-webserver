# Favicons 

Source: ![A logo I created from the espressif logo, using inkscape](../Docs/logo.svg)

## The 16x16 and 32x32 png images were extracted from a Favicon Package

This package was generated with [RealFaviconGenerator](https://realfavicongenerator.net/)

A very handy site, dont forget to select compression options in the 'html5' section, they are in a hard to spot tab. Doing this reduced the `.png`sizes by ~74% :-)

## The favicon.ico itself came from https://www.favicon.cc/

This is another [Really handy site](https://www.favicon.cc/), it produces correctly formatted .ico files.
* The `.ico` file in the package from RealFaviconGenerator was quite large (64x64, I believe) and I dont want to be putting a 15K file into the build. So I wanted a small (16x16) image for this.
 
## favicons.h
The icon files were packed into the `favicons.h` header using `xxd -i <file>` to generate the C compatible data structures, and then editing that with comments and adding PROGMEM directives to save on ram use. They should be stable and unlikely to change in the future.
