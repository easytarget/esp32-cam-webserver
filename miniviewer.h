//File: index_ov2640.html
const uint8_t miniviewer_html[] = R"=====(
<!doctype html>
<html>
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width,initial-scale=1">
    <title id="title">ESP32-CAM MiniViewer</title>
    <link rel="icon" type="image/png" sizes="32x32" href="/favicon-32x32.png">
    <link rel="icon" type="image/png" sizes="16x16" href="/favicon-16x16.png">
    <link rel="stylesheet" type="text/css" href="/style.css">
    <style>
    // style overrides here
    </style>
  </head>

  <body>
    <section class="main">
      <div id="logo">
        <label for="nav-toggle-cb" id="nav-toggle" style="float:left;" title="Settings">&#9776;&nbsp;</label>
        <button id="swap-viewer" style="float:left;" title="Swap to full feature viewer">Full</button>
        <button id="get-still" style="float:left;">Get Still</button>
        <button id="toggle-stream" style="float:left;" class="hidden">Start Stream</button>
        <div id="wait-settings" style="float:left;" class="loader" title="Waiting for camera settings to load"></div>
      </div>
      <div id="content">
        <div class="hidden" id="sidebar">
          <input type="checkbox" id="nav-toggle-cb">
            <nav id="menu" style="width:24em;">
              <div class="input-group hidden" id="lamp-group">
                <label for="lamp">Light</label>
                <div class="range-min">Off</div>
                <input type="range" id="lamp" min="0" max="100" value="0" class="action-setting">
                <div class="range-max">Full</div>
              </div>
              <div class="input-group" id="framesize-group">
                <label for="framesize">Resolution</label>
                <select id="framesize" class="action-setting">
                  <option value="10">UXGA(1600x1200)</option>
                  <option value="9">SXGA(1280x1024)</option>
                  <option value="8">XGA(1024x768)</option>
                  <option value="7">SVGA(800x600)</option>
                  <option value="6">VGA(640x480)</option>
                  <option value="5">CIF(400x296)</option>
                  <option value="4">QVGA(320x240)</option>
                  <option value="3">HQVGA(240x176)</option>
                  <option value="0">QQVGA(160x120)</option>
                </select>
              </div>
              <!-- Hide the next entries, they are present in the body so that we
                  can pass settings to/from them for use in the scripting, not for user setting -->
              <div id="rotate" class="action-setting hidden"></div>
              <div id="cam_name" class="action-setting hidden"></div>
              <div id="stream_url" class="action-setting hidden"></div>
              <div id="detect" class="action-setting hidden"></div> 
              <div id="recognize" class="action-setting hidden"></div>
            </nav>
        </div>
        <figure>
          <div id="stream-container" class="image-container hidden">
            <div class="close" id="close-stream">×</div>
            <img id="stream" src="">
          </div>
        </figure>
      </div>
    </section>
  </body>

  <script>
  document.addEventListener('DOMContentLoaded', function (event) {
    var baseHost = document.location.origin;
    var streamURL = 'Undefined';

    const settings = document.getElementById('sidebar')
    const waitSettings = document.getElementById('wait-settings')
    const lampGroup = document.getElementById('lamp-group')
    const rotate = document.getElementById('rotate')
    const view = document.getElementById('stream')
    const viewContainer = document.getElementById('stream-container')
    const stillButton = document.getElementById('get-still')
    const streamButton = document.getElementById('toggle-stream')
    const closeButton = document.getElementById('close-stream')
    const swapButton = document.getElementById('swap-viewer')

    const hide = el => {
      el.classList.add('hidden')
    }
    const show = el => {
      el.classList.remove('hidden')
    }

    const disable = el => {
      el.classList.add('disabled')
      el.disabled = true
    }

    const enable = el => {
      el.classList.remove('disabled')
      el.disabled = false
    }

    const updateValue = (el, value, updateRemote) => {
      updateRemote = updateRemote == null ? true : updateRemote
      let initialValue
      if (el.type === 'checkbox') {
        initialValue = el.checked
        value = !!value
        el.checked = value
      } else {
        initialValue = el.value
        el.value = value
      }
  
      if (updateRemote && initialValue !== value) {
        updateConfig(el);
      } else if(!updateRemote){
        if(el.id === "lamp"){
          if (value == -1) { 
            hide(lampGroup)
          } else {
            show(lampGroup)
          }
        } else if(el.id === "cam_name"){
          window.document.title = value;
          console.log('Name set to: ' + value);
        } else if(el.id === "code_ver"){
          console.log('Firmware Build: ' + value);
        } else if(el.id === "rotate"){
          rotate.value = value;
          applyRotation();
        } else if(el.id === "stream_url"){
          streamURL = value;
          streamButton.setAttribute("title", `You can also browse to '${streamURL}' for a raw stream`);
          console.log('Stream URL set to:' + value);
        } 
      }
    }

    function updateConfig (el) {
      let value
      switch (el.type) {
        case 'checkbox':
          value = el.checked ? 1 : 0
          break
        case 'range':
        case 'select-one':
          value = el.value
          break
        case 'button':
        case 'submit':
          value = '1'
          break
        default:
          return
      }

      const query = `${baseHost}/control?var=${el.id}&val=${value}`

      fetch(query)
        .then(response => {
          console.log(`request to ${query} finished, status: ${response.status}`)
        })
    }

    document
      .querySelectorAll('.close')
      .forEach(el => {
        el.onclick = () => {
          hide(el.parentNode)
        }
      })

    // read initial values
    fetch(`${baseHost}/status`)
      .then(function (response) {
        return response.json()
      })
      .then(function (state) {
        document
          .querySelectorAll('.action-setting')
          .forEach(el => {
            updateValue(el, state[el.id], false)
          })
        hide(waitSettings);
        show(settings);
        show(streamButton);
        startStream();
      })

    // Put some helpful text on the 'Still' button
    stillButton.setAttribute("title", `You can also browse to '${baseHost}/capture' for standalone images`);

    const stopStream = () => {
      window.stop();
      streamButton.innerHTML = 'Start Stream';
      hide(viewContainer);
    }

    const startStream = () => {
      view.src = streamURL;
      view.scrollIntoView(false);
      streamButton.innerHTML = 'Stop Stream';
      show(viewContainer);
    }

    const applyRotation = () => {
      rot = rotate.value;
      if (rot == -90) {
        viewContainer.style.transform = `rotate(-90deg)  translate(-100%)`;
      } else if (rot == 90) {
        viewContainer.style.transform = `rotate(90deg) translate(0, -100%)`
      } else {
        viewContainer.style.transform = `rotate(0deg)`
      }
       console.log('Rotation ' + rot + ' applied');
   }

    // Attach actions to controls

    stillButton.onclick = () => {
      stopStream();
      view.src = `${baseHost}/capture?_cb=${Date.now()}`;
      view.scrollIntoView(false);
      show(viewContainer);
    }

    closeButton.onclick = () => {
      stopStream();
      hide(viewContainer);
    }

    streamButton.onclick = () => {
      const streamEnabled = streamButton.innerHTML === 'Stop Stream'
      if (streamEnabled) {
        stopStream();
      } else {
        startStream();
      }
    }

    // Attach default on change action
    document
      .querySelectorAll('.action-setting')
      .forEach(el => {
        el.onchange = () => updateConfig(el)
      })

    // Custom actions
    // Detection and framesize
    rotate.onchange = () => {
      applyRotation();
      updateConfig(rotate);
    }

    framesize.onchange = () => {
      updateConfig(framesize)
      if (framesize.value > 5) {
        updateValue(detect, false)
        updateValue(recognize, false)
      }
    }

    swapButton.onclick = () => {
      window.open('/','_self');
    }

  })
  </script>
</html>
)=====";

size_t miniviewer_html_len = sizeof(miniviewer_html);
