# Favicons 

Source: A logo I created from the espressif logo, using inkscape 

![logo image](../Docs/logo.svg)

## The 16x16 and 32x32 png images were extracted from a Favicon Package

This package was generated with [RealFaviconGenerator](https://realfavicongenerator.net/)

A very handy site, dont forget to select compression options in the 'html5' section, they are in a hard to spot tab. Doing this reduced the `.png`sizes by ~74% :-)

## The favicon.ico itself came from the command line

The [Imagemagick](https://imagemagick.org/) tool provides a simple image converter that can create `.ico` files from a source image in another format. I simply needed to use this on the 32x32 png icon to make a suitably high-definition icon file.
```
$ convert favicon-32x32.png favicon.ico
```
 
## favicons.h
The icon files were packed into the `favicons.h` header using `xxd -i <file>` to generate the C compatible data structures, and then editing that with comments and adding PROGMEM directives to save on ram use. They should be stable and unlikely to change in the future.
