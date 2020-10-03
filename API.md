# Basic HTTP Commands
## It's an API Jim, but not as we know it

The WebUI and camera server communicate entirely via HTTP requests and responses; this makes controlling all functions of the camera via GET requests possible. An API in effect.

## URI's
### Http Port
`/` Root URI; primary WebUI
`/capture` Return a Jpeg snapshot image
`/view` Simplified Viewer
`/status` Returns a JSON string with all camera status <key>/<value> pairs listed
`/control?var=<key>&val=<val>` Set <key> to <val>
`/dump` Status page

### Stream Port
`/` Root URI; Raw stream
`/view` Stream viewer

## <key> / <val> settings and commands
.. thie list.. the list is the thing.

## Examples
Flash light: on/off
http://<IP-ADDRESS>/control?var=lamp&val=100
http://<IP-ADDRESS>/control?var=lamp&val=0
