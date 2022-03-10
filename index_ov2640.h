/*
 * primary HTML for the OV2640 camera module
 */

const uint8_t index_ov2640_html[] = R"=====(<!doctype html>
<html>
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width,initial-scale=1">
    <title>ESP32 OV2640</title>
    <link rel="icon" type="image/png" sizes="32x32" href="/favicon-32x32.png">
    <link rel="icon" type="image/png" sizes="16x16" href="/favicon-16x16.png">
    <link rel="stylesheet" type="text/css" href="/style.css">
    <style>
      @media (min-width: 800px) and (orientation:landscape) {
        #content {
          display:flex;
          flex-wrap: nowrap;
          align-items: stretch
        }
      }
    </style>
  </head>

  <body>
    <section class="main">
      <div id="logo">
        <label for="nav-toggle-cb" id="nav-toggle" style="float:left;">&#9776;&nbsp;&nbsp;Settings&nbsp;&nbsp;&nbsp;&nbsp;</label>
        <button id="swap-viewer" style="float:left;" title="Swap to simple viewer">Simple</button>
        <button id="get-still" style="float:left;">Get Still</button>
        <button id="toggle-stream" style="float:left;" class="hidden">Start Stream</button>
        <div id="wait-settings" style="float:left;" class="loader" title="Waiting for camera settings to load"></div>
      </div>
      <div id="content">
        <div class="hidden" id="sidebar">
          <input type="checkbox" id="nav-toggle-cb" checked="checked">
            <nav id="menu">
              <div class="input-group hidden" id="lamp-group" title="Flashlight LED.&#013;&#013;Warning:&#013;Built-In lamps can be Very Bright! Avoid looking directly at LED&#013;Can draw a lot of power and may cause visual artifacts, affect WiFi or even brownout the camera on high settings">
                <label for="lamp">Light</label>
                <div class="range-min">Off</div>
                <input type="range" id="lamp" min="0" max="100" value="0" class="default-action">
                <div class="range-max"><span style="font-size: 125%;">&#9888;</span>Full</div>
              </div>
              <div class="input-group hidden" id="autolamp-group" title="When enabled the lamp will only turn on while the camera is active">
                <label for="autolamp">Auto Lamp</label>
                <div class="switch">
                  <input id="autolamp" type="checkbox" class="default-action">
                  <label class="slider" for="autolamp"></label>
                </div>
              </div>

              <div class="input-group" id="framesize-group" title="Camera resolution&#013;Higher resolutions will result in lower framerates">
                <label for="framesize">Resolution</label>
                <select id="framesize" class="default-action">
                  <option value="13">UXGA (1600x1200)</option>
                  <option value="12">SXGA (1280x1024)</option>
                  <option value="11">HD (1280x720)</option>
                  <option value="10">XGA (1024x768)</option>
                  <option value="9">SVGA (800x600)</option>
                  <option value="8">VGA (640x480)</option>
                  <option value="7">HVGA (480x320)</option>
                  <option value="6">CIF (400x296)</option>
                  <option value="5">QVGA (320x240)</option>
                  <option value="3">HQVGA (240x176)</option>
                  <option value="1">QQVGA (160x120)</option>
                  <option value="0">THUMB (96x96)</option>
                </select>
              </div>
              <div class="input-group" id="quality-group" title="Camera Image and Stream quality factor&#013;Higher settings will result in lower framerates">
                <label for="quality">Quality</label>
                <div class="range-min">Low</div>
                <!-- Note; the following element is 'flipped' in CSS so that it slides from High to Low
                     As a result the 'min' and 'max' values are reversed here too -->
                <input type="range" id="quality" min="6" max="63" value="10" class="default-action">
                <div class="range-max">High</div>
              </div>
              <div class="input-group" id="set-xclk-group" title="Camera Bus Clock Frequency&#013;Increasing this will raise the camera framerate and capture speed&#013;&#013;Raising too far will result in visual artifacts and/or incomplete frames&#013;This setting can vary a lot between boards, budget boards typically need lower values">
                 <label for="set-xclk">XCLK</label>
                 <div class="text">
                    <input id="xclk" type="number" min="2" max="32" size="3" step="1" class="default-action">
                    <div class="range-max">MHz</div>
                  </div>
              </div>
              <div class="input-group" id="brightness-group">
                <label for="brightness">Brightness</label>
                <div class="range-min">-2</div>
                <input type="range" id="brightness" min="-2" max="2" value="0" class="default-action">
                <div class="range-max">2</div>
              </div>
              <div class="input-group" id="contrast-group">
                <label for="contrast">Contrast</label>
                <div class="range-min">-2</div>
                <input type="range" id="contrast" min="-2" max="2" value="0" class="default-action">
                <div class="range-max">2</div>
              </div>
              <div class="input-group" id="saturation-group">
                <label for="saturation">Saturation</label>
                <div class="range-min">-2</div>
                <input type="range" id="saturation" min="-2" max="2" value="0" class="default-action">
                <div class="range-max">2</div>
              </div>
              <div class="input-group" id="special_effect-group">
                <label for="special_effect">Special Effect</label>
                <select id="special_effect" class="default-action">
                  <option value="0" selected="selected">No Effect</option>
                  <option value="1">Negative</option>
                  <option value="2">Grayscale</option>
                  <option value="3">Red Tint</option>
                  <option value="4">Green Tint</option>
                  <option value="5">Blue Tint</option>
                  <option value="6">Sepia</option>
                </select>
              </div>
              <div class="input-group" id="awb-group">
                <label for="awb">AWB Enable</label>
                <div class="switch">
                  <input id="awb" type="checkbox" class="default-action" checked="checked">
                  <label class="slider" for="awb"></label>
                </div>
              </div>
              <div class="input-group" id="awb_gain-group">
                <label for="awb_gain">Manual AWB Gain</label>
                <div class="switch">
                  <input id="awb_gain" type="checkbox" class="default-action" checked="checked">
                  <label class="slider" for="awb_gain"></label>
                </div>
              </div>
              <div class="input-group" id="wb_mode-group">
                <label for="wb_mode">WB Mode</label>
                <select id="wb_mode" class="default-action">
                  <option value="0" selected="selected">Auto</option>
                  <option value="1">Sunny</option>
                  <option value="2">Cloudy</option>
                  <option value="3">Office</option>
                  <option value="4">Home</option>
                </select>
              </div>
              <div class="input-group" id="aec-group">
                <label for="aec">AEC Sensor Enable</label>
                <div class="switch">
                  <input id="aec" type="checkbox" class="default-action" checked="checked">
                  <label class="slider" for="aec"></label>
                </div>
              </div>
              <div class="input-group" id="aec2-group">
                <label for="aec2">AEC DSP</label>
                <div class="switch">
                  <input id="aec2" type="checkbox" class="default-action" checked="checked">
                  <label class="slider" for="aec2"></label>
                </div>
              </div>
              <div class="input-group" id="ae_level-group">
                <label for="ae_level">AE Level</label>
                <div class="range-min">-2</div>
                <input type="range" id="ae_level" min="-2" max="2" value="0" class="default-action">
                <div class="range-max">2</div>
              </div>
              <div class="input-group" id="aec_value-group">
                <label for="aec_value">Exposure</label>
                <div class="range-min">0</div>
                <input type="range" id="aec_value" min="0" max="1200" value="204" class="default-action">
                <div class="range-max">1200</div>
              </div>
              <div class="input-group" id="agc-group">
                <label for="agc">AGC</label>
                <div class="switch">
                  <input id="agc" type="checkbox" class="default-action" checked="checked">
                  <label class="slider" for="agc"></label>
                </div>
              </div>
              <div class="input-group hidden" id="agc_gain-group">
                <label for="agc_gain">Gain</label>
                <div class="range-min">1x</div>
                <input type="range" id="agc_gain" min="0" max="30" value="5" class="default-action">
                <div class="range-max">31x</div>
              </div>
              <div class="input-group" id="gainceiling-group">
                <label for="gainceiling">Gain Ceiling</label>
                <div class="range-min">2x</div>
                <input type="range" id="gainceiling" min="0" max="6" value="0" class="default-action">
                <div class="range-max">128x</div>
              </div>
              <div class="input-group" id="bpc-group">
                <label for="bpc">BPC</label>
                <div class="switch">
                  <input id="bpc" type="checkbox" class="default-action">
                  <label class="slider" for="bpc"></label>
                </div>
              </div>
              <div class="input-group" id="wpc-group">
                <label for="wpc">WPC</label>
                <div class="switch">
                  <input id="wpc" type="checkbox" class="default-action" checked="checked">
                  <label class="slider" for="wpc"></label>
                </div>
              </div>
              <div class="input-group" id="raw_gma-group">
                <label for="raw_gma">Raw GMA Enable</label>
                <div class="switch">
                  <input id="raw_gma" type="checkbox" class="default-action" checked="checked">
                  <label class="slider" for="raw_gma"></label>
                </div>
              </div>
              <div class="input-group" id="lenc-group">
                <label for="lenc">Lens Correction</label>
                <div class="switch">
                  <input id="lenc" type="checkbox" class="default-action" checked="checked">
                  <label class="slider" for="lenc"></label>
                </div>
              </div>
              <div class="input-group" id="hmirror-group">
                <label for="hmirror">H-Mirror Stream</label>
                <div class="switch">
                  <input id="hmirror" type="checkbox" class="default-action" checked="checked">
                  <label class="slider" for="hmirror"></label>
                </div>
              </div>
              <div class="input-group" id="vflip-group">
                <label for="vflip">V-Flip Stream</label>
                <div class="switch">
                  <input id="vflip" type="checkbox" class="default-action" checked="checked">
                  <label class="slider" for="vflip"></label>
                </div>
              </div>
              <div class="input-group" id="rotate-group">
                <label for="rotate">Rotate in Browser</label>
                <select id="rotate" class="default-action">
                  <option value="90">90&deg; (Right)</option>
                  <option value="0" selected="selected">0&deg; (None)</option>
                  <option value="-90">-90&deg; (Left)</option>
                </select>
              </div>
              <div class="input-group" id="dcw-group">
                <label for="dcw">DCW (Downsize EN)</label>
                <div class="switch">
                  <input id="dcw" type="checkbox" class="default-action" checked="checked">
                  <label class="slider" for="dcw"></label>
                </div>
              </div>
              <div class="input-group" id="colorbar-group">
                <label for="colorbar">Test Pattern</label>
                <div class="switch">
                  <input id="colorbar" type="checkbox" class="default-action">
                  <label class="slider" for="colorbar"></label>
                </div>
              </div>
              <div class="input-group" id="min_frame_time-group" title="Minimum frame time&#013;Higher settings reduce the frame rate&#013;Use this for a smoother stream and to reduce load on the WiFi and browser">
                <label for="min_frame_time">Frame Duration Limit</label>
                <select id="min_frame_time" class="default-action">
                  <option value="3333">3.3s  (0.3fps)</option>
                  <option value="2000">2s    (0.5fps)</option>
                  <option value="1000">1s    (1fps)</option>
                  <option value="500">500ms (2fps)</option>
                  <option value="333">333ms (3fps)</option>
                  <option value="200">200ms (5fps)</option>
                  <option value="100">100ms (10fps)</option>
                  <option value="50">50ms (20fps)</option>
                  <option value="0" selected="selected">Disabled</option>
                </select>
              </div>
              <div class="input-group" id="preferences-group">
                <label for="prefs" style="line-height: 2em;">Preferences</label>
                <button id="reboot" title="Reboot the camera module">Reboot</button>
                <button id="save_prefs" title="Save Preferences on camera module">Save</button>
                <button id="clear_prefs" title="Erase saved Preferences on camera module">Erase</button>
              </div>
              <div class="input-group" id="cam_name-group">
                <label for="cam_name">
                <a href="/dump" title="System Info" target="_blank">Name</a></label>
                <div id="cam_name" class="default-action"></div>
              </div>
              <div class="input-group" id="code_ver-group">
                <label for="code_ver">
                <a href="https://github.com/easytarget/esp32-cam-webserver"
                  title="ESP32 Cam Webserver on GitHub" target="_blank">Firmware</a></label>
                <div id="code_ver" class="default-action"></div>
              </div>
              <div class="input-group hidden" id="stream-group">
                <label for="stream_url" id="stream_link">Stream</label>
                <div id="stream_url" class="default-action">Unknown</div>
              </div>
            </nav>
        </div>
        <figure>
          <div id="stream-container" class="image-container hidden">
            <div class="close close-rot-none" id="close-stream">×</div>
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
    var viewerURL = 'Undefined';

    const header = document.getElementById('logo')
    const settings = document.getElementById('sidebar')
    const waitSettings = document.getElementById('wait-settings')
    const lampGroup = document.getElementById('lamp-group')
    const autolampGroup = document.getElementById('autolamp-group')
    const streamGroup = document.getElementById('stream-group')
    const camName = document.getElementById('cam_name')
    const codeVer = document.getElementById('code_ver')
    const rotate = document.getElementById('rotate')
    const view = document.getElementById('stream')
    const viewContainer = document.getElementById('stream-container')
    const stillButton = document.getElementById('get-still')
    const streamButton = document.getElementById('toggle-stream')
    const closeButton = document.getElementById('close-stream')
    const streamLink = document.getElementById('stream_link')
    const framesize = document.getElementById('framesize')
    const xclk = document.getElementById('xclk')
    const swapButton = document.getElementById('swap-viewer')
    const savePrefsButton = document.getElementById('save_prefs')
    const clearPrefsButton = document.getElementById('clear_prefs')
    const rebootButton = document.getElementById('reboot')
    const minFrameTime = document.getElementById('min_frame_time')

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
        if(el.id === "aec"){
          value ? hide(exposure) : show(exposure)
        } else if(el.id === "agc"){
          if (value) {
            show(gainCeiling)
            hide(agcGain)
          } else {
            hide(gainCeiling)
            show(agcGain)
          }
        } else if(el.id === "awb_gain"){
          value ? show(wb) : hide(wb)
        } else if(el.id === "lamp"){
          if (value == -1) {
            hide(lampGroup)
            hide(autolampGroup)
          } else {
            show(lampGroup)
            show(autolampGroup)
          }
        } else if(el.id === "cam_name"){
          camName.innerHTML = value;
          window.document.title = value;
          console.log('Name set to: ' + value);
        } else if(el.id === "code_ver"){
          codeVer.innerHTML = value;
          console.log('Firmware Build: ' + value);
        } else if(el.id === "rotate"){
          rotate.value = value;
          applyRotation();
        } else if(el.id === "min_frame_time"){
          min_frame_time.value = value;
        } else if(el.id === "stream_url"){
          streamURL = value;
          viewerURL = value + 'view';
          stream_url.innerHTML = value;
          stream_link.setAttribute("title", `Open the standalone stream viewer :: ${viewerURL}`);
          stream_link.style.textDecoration = "underline";
          stream_link.style.cursor = "pointer";
          streamButton.setAttribute("title", `Start the stream :: ${streamURL}`);
          show(streamGroup)
          console.log('Stream URL set to: ' + streamURL);
          console.log('Stream Viewer URL set to: ' + viewerURL);
        }
      }
    }

    var rangeUpdateScheduled = false
    var latestRangeConfig

    function updateRangeConfig (el) {
      latestRangeConfig = el
      if (!rangeUpdateScheduled) {
        rangeUpdateScheduled = true;
        setTimeout(function(){
          rangeUpdateScheduled = false
          updateConfig(latestRangeConfig)
        }, 150);
      }
    }

    function updateConfig (el) {
      let value
      switch (el.type) {
        case 'checkbox':
          value = el.checked ? 1 : 0
          break
        case 'range':
        case 'number':
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
          .querySelectorAll('.default-action')
          .forEach(el => {
            updateValue(el, state[el.id], false)
          })
        hide(waitSettings);
        show(settings);
        show(streamButton);
        //startStream();
      })

    // Put some helpful text on the 'Still' button
    stillButton.setAttribute("title", `Capture a still image :: ${baseHost}/capture`);

    const stopStream = () => {
      window.stop();
      streamButton.innerHTML = 'Start Stream';
      streamButton.setAttribute("title", `Start the stream :: ${streamURL}`);
      hide(viewContainer);
    }

    const startStream = () => {
      view.src = streamURL;
      view.scrollIntoView(false);
      streamButton.innerHTML = 'Stop Stream';
      streamButton.setAttribute("title", `Stop the stream`);
      show(viewContainer);
    }

    const applyRotation = () => {
      rot = rotate.value;
      if (rot == -90) {
        viewContainer.style.transform = `rotate(-90deg)  translate(-100%)`;
        closeButton.classList.remove('close-rot-none');
        closeButton.classList.remove('close-rot-right');
        closeButton.classList.add('close-rot-left');
      } else if (rot == 90) {
        viewContainer.style.transform = `rotate(90deg) translate(0, -100%)`;
        closeButton.classList.remove('close-rot-left');
        closeButton.classList.remove('close-rot-none');
        closeButton.classList.add('close-rot-right');
      } else {
        viewContainer.style.transform = `rotate(0deg)`;
        closeButton.classList.remove('close-rot-left');
        closeButton.classList.remove('close-rot-right');
        closeButton.classList.add('close-rot-none');
      }
      console.log('Rotation ' + rot + ' applied');
    }

    // Attach actions to controls

    streamLink.onclick = () => {
      stopStream();
      window.open(viewerURL, "_blank");
    }

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
      .querySelectorAll('.default-action')
      .forEach(el => {
        el.onchange = () => updateConfig(el)
      })

    // Update range sliders as they are being moved
    document
      .querySelectorAll('input[type="range"]')
      .forEach(el => {
        el.oninput = () => updateRangeConfig(el)
      })

    // Custom actions
    // Gain
    const agc = document.getElementById('agc')
    const agcGain = document.getElementById('agc_gain-group')
    const gainCeiling = document.getElementById('gainceiling-group')
    agc.onchange = () => {
      updateConfig(agc)
      if (agc.checked) {
        show(gainCeiling)
        hide(agcGain)
      } else {
        hide(gainCeiling)
        show(agcGain)
      }
    }

    // Exposure
    const aec = document.getElementById('aec')
    const exposure = document.getElementById('aec_value-group')
    aec.onchange = () => {
      updateConfig(aec)
      aec.checked ? hide(exposure) : show(exposure)
    }

    // AWB
    const awb = document.getElementById('awb_gain')
    const wb = document.getElementById('wb_mode-group')
    awb.onchange = () => {
      updateConfig(awb)
      awb.checked ? show(wb) : hide(wb)
    }

    // Detection and framesize
    rotate.onchange = () => {
      applyRotation();
      updateConfig(rotate);
    }

    framesize.onchange = () => {
      updateConfig(framesize)
    }

    minFrameTime.onchange = () => {
      updateConfig(minFrameTime)
    }

    xclk.onchange = () => {
      console.log("xclk:" , xclk);
      updateConfig(xclk)
    }

    swapButton.onclick = () => {
      window.open('/?view=simple','_self');
    }

    savePrefsButton.onclick = () => {
      if (confirm("Save the current preferences?")) {
        updateConfig(savePrefsButton);
      }
    }

    clearPrefsButton.onclick = () => {
      if (confirm("Remove the saved preferences?")) {
        updateConfig(clearPrefsButton);
      }
    }

    rebootButton.onclick = () => {
      if (confirm("Reboot the Camera Module?")) {
        updateConfig(rebootButton);
        // Some sort of countdown here?
        hide(settings);
        hide(viewContainer);
        header.innerHTML = '<h1>Rebooting!</h1><hr>Page will reload after 30 seconds.';
        setTimeout(function() {
          location.replace(document.URL);
        }, 30000);
      }
    }

  })
  </script>
</html>)=====";

size_t index_ov2640_html_len = sizeof(index_ov2640_html)-1;
