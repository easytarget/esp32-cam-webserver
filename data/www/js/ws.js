var urlCreator = window.URL || window.webkitURL;
const stream = document.getElementById('video');
var streamURL = 'ws://' + location.hostname + ':' + location.port + '/ws';

const ws = new WebSocket(streamURL);
ws.binaryType = 'arraybuffer';

ws.onmessage = function(event) {
  var arrayBufferView = new Uint8Array(event.data);
  var blob = new Blob([arrayBufferView], {type: "image/jpeg"});
  var imageUrl = urlCreator.createObjectURL(blob);
  video.src = imageUrl;
}

ws.onopen = function(event) {
  console.log(event.currentTarget.url);
}

ws.onclose = function(event) {
  console.log(event);
}

ws.onerror = function() {
  console.log(error);
}

