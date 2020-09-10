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
    <style>
      body {
        font-family: Arial,Helvetica,sans-serif;
        background: #181818;
        color: #EFEFEF;
        font-size: 16px
      }

      a {
        color: #EFEFEF;
        text-decoration: underline
      }

      h2 {
        font-size: 18px
      }

      section.main {
        display: flex
      }

      #menu,section.main {
        flex-direction: column
      }

      #menu {
        display: none;
        flex-wrap: nowrap;
        min-width: 340px;
        background: #363636;
        padding: 8px;
        border-radius: 4px;
        margin-top: -10px;
        margin-right: 10px;
      }

 /*     #content {
        display: block;
        flex-wrap: wrap;
        align-items: stretch
      }
*/
      figure {
        padding: 0px;
        margin: 0;
        -webkit-margin-before: 0;
        margin-block-start: 0;
        -webkit-margin-after: 0;
        margin-block-end: 0;
        -webkit-margin-start: 0;
        margin-inline-start: 0;
        -webkit-margin-end: 0;
        margin-inline-end: 0
      }

      figure img {
        display: block;
/*        width: 100%;
          no max-width:  */
        max-width: 100%;
        width: auto;
        height: auto;
        border-radius: 4px;
        margin-top: 8px;
      }

/*      @media (min-width: 800px) and (orientation:landscape) {
        #content {
          display:flex;
          flex-wrap: nowrap;
          align-items: stretch
        }


        figure img {
          display: block;
          max-width: 100%;
          max-height: calc(100vh - 40px);
          width: auto;
          height: auto
        }

        figure {
          padding: 0 0 0 0px;
          margin: 0;
          -webkit-margin-before: 0;
          margin-block-start: 0;
          -webkit-margin-after: 0;
          margin-block-end: 0;
          -webkit-margin-start: 0;
          margin-inline-start: 0;
          -webkit-margin-end: 0;
          margin-inline-end: 0
        }
      }
*/
      section#buttons {
        display: flex;
        flex-wrap: nowrap;
        justify-content: space-between
      }

      #nav-toggle {
        cursor: pointer;
        display: block
      }

      #nav-toggle-cb {
        outline: 0;
        opacity: 0;
        width: 0;
        height: 0
      }

      #nav-toggle-cb:checked+#menu {
        display: flex
      }

      .input-group {
        display: flex;
        flex-wrap: nowrap;
        line-height: 22px;
        margin: 5px 0
      }

      .input-group>label {
        display: inline-block;
        padding-right: 10px;
        min-width: 47%
      }

      .input-group input,.input-group select {
        flex-grow: 1
      }

      .range-max,.range-min {
        display: inline-block;
        padding: 0 5px
      }

      button {
        display: block;
        margin: 5px;
        padding: 0 12px;
        border: 0;
        line-height: 28px;
        cursor: pointer;
        color: #fff;
        background: #ff3034;
        border-radius: 5px;
        font-size: 16px;
        outline: 0
      }

      button:hover {
        background: #ff494d
      }

      button:active {
        background: #f21c21
      }

      button.disabled {
        cursor: default;
        background: #a0a0a0
      }

      input[type=range] {
        -webkit-appearance: none;
        width: 100%;
        height: 22px;
        background: #363636;
        cursor: pointer;
        margin: 0
      }

      input[type=range]:focus {
        outline: 0
      }

      input[type=range]::-webkit-slider-runnable-track {
        width: 100%;
        height: 2px;
        cursor: pointer;
        background: #EFEFEF;
        border-radius: 0;
        border: 0 solid #EFEFEF
      }

      input[type=range]::-webkit-slider-thumb {
        border: 1px solid rgba(0,0,30,0);
        height: 22px;
        width: 22px;
        border-radius: 50px;
        background: #ff3034;
        cursor: pointer;
        -webkit-appearance: none;
        margin-top: -11.5px
      }

      input[type=range]:focus::-webkit-slider-runnable-track {
        background: #EFEFEF
      }

      input[type=range]::-moz-range-track {
        width: 100%;
        height: 2px;
        cursor: pointer;
        background: #EFEFEF;
        border-radius: 0;
        border: 0 solid #EFEFEF
      }

      input[type=range]::-moz-range-thumb {
        border: 1px solid rgba(0,0,30,0);
        height: 22px;
        width: 22px;
        border-radius: 50px;
        background: #ff3034;
        cursor: pointer
      }

      input[type=range]::-ms-track {
        width: 100%;
        height: 2px;
        cursor: pointer;
        background: 0 0;
        border-color: transparent;
        color: transparent
      }

      input[type=range]::-ms-fill-lower {
        background: #EFEFEF;
        border: 0 solid #EFEFEF;
        border-radius: 0
      }

      input[type=range]::-ms-fill-upper {
        background: #EFEFEF;
        border: 0 solid #EFEFEF;
        border-radius: 0
      }

      input[type=range]::-ms-thumb {
        border: 1px solid rgba(0,0,30,0);
        height: 22px;
        width: 22px;
        border-radius: 50px;
        background: #ff3034;
        cursor: pointer;
        height: 2px
      }

      input[type=range]:focus::-ms-fill-lower {
        background: #EFEFEF
      }

      input[type=range]:focus::-ms-fill-upper {
        background: #363636
      }

      input[type=text] {
        border: 1px solid #363636;
        font-size: 14px;
        height: 20px;
        margin: 1px;
        outline: 0;
        border-radius: 5px
      }

      .switch {
        display: block;
        position: relative;
        line-height: 22px;
        font-size: 16px;
        height: 22px
      }

      .switch input {
        outline: 0;
        opacity: 0;
        width: 0;
        height: 0
      }

      .slider {
        width: 50px;
        height: 22px;
        border-radius: 22px;
        cursor: pointer;
        background-color: grey
      }

      .slider,.slider:before {
        display: inline-block;
        transition: .4s
      }

      .slider:before {
        position: relative;
        content: "";
        border-radius: 50%;
        height: 16px;
        width: 16px;
        left: 4px;
        top: 3px;
        background-color: #fff
      }

      input:checked+.slider {
        background-color: #ff3034
      }

      input:checked+.slider:before {
        -webkit-transform: translateX(26px);
        transform: translateX(26px)
      }

      select {
        border: 1px solid #363636;
        font-size: 14px;
        height: 22px;
        outline: 0;
        border-radius: 5px
      }

      .image-container {
        position: relative;
        min-width: 160px;
        transform-origin: top left
      }

      .close {
        position: absolute;
        z-index: 99;
        right: 5px;
        top: 5px;
        background: #ff3034;
        width: 16px;
        height: 16px;
        border-radius: 100px;
        color: #fff;
        text-align: center;
        line-height: 18px;
        cursor: pointer
      }

      .hidden {
        display: none
      }

      .inline-button {
        line-height: 20px;
        margin: 2px;
        padding: 1px 4px 2px 4px;
      }

      .loader {
        border: 0.5em solid #f3f3f3; /* Light grey */
        border-top: 0.5em solid #000000; /* white */
        border-radius: 50%;
        width: 1em;
        height: 1em;
        -webkit-animation: spin 2s linear infinite; /* Safari */
        animation: spin 2s linear infinite;
      }

      @-webkit-keyframes spin {   /* Safari */
        0% { -webkit-transform: rotate(0deg); }
        100% { -webkit-transform: rotate(360deg); }
      }

      @keyframes spin {
        0% { transform: rotate(0deg); }
        100% { transform: rotate(360deg); }
      }

    </style>
  </head>

  <body>
    <section class="main">
      <div id="logo">
        <label for="nav-toggle-cb" id="nav-toggle" style="float:left;" title="Settings">&#9776;&nbsp;</label>
        <button id="get-still" style="float:left;">Get Still</button>
        <button id="toggle-stream" style="float:left;" class="hidden">Start Stream</button>
        <div id="wait-settings" style="float:left;" class="loader" title="Waiting for camera settings to load"></div>
      </div>
      <div id="content">
        <div class="hidden" id="sidebar">
          <input type="checkbox" id="nav-toggle-cb">
          <nav id="menu">
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
            <!-- Hide the next two entries, but they need to be present so that we
                can pass settings to them for use in the script -->
            <div id="rotate" class="action-setting hidden"></div>
            <div id="cam_name" class="action-setting hidden"></div>
            <div id="stream_url" class="action-setting hidden"></div>
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
    const streamGroup = document.getElementById('stream-group')
    const rotate = document.getElementById('rotate')
    const view = document.getElementById('stream')
    const viewContainer = document.getElementById('stream-container')
    const stillButton = document.getElementById('get-still')
    const streamButton = document.getElementById('toggle-stream')
    const closeButton = document.getElementById('close-stream')
  
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
        } else if(el.id === "rotate"){
          rotate.value = value;
          applyRotation();
          console.log('Rotate initial value: ' + rotate.value);
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
      console.log('Rotation changed');
      applyRotation();
      updateConfig(rotate);
    }
  })  // event
  </script>
</html>
)=====";
size_t miniviewer_html_len = sizeof(miniviewer_html);
