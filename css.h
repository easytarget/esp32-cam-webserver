/*
 * Master CSS file for the camera pages
 */

const uint8_t style_css[] = R"=====(/*
 * CSS for the esp32 cam webserver
 */

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
  color: #EFEFEF;
  width: 380px;
  background: #363636;
  padding: 8px;
  border-radius: 4px;
  margin-top: -10px;
  margin-right: 10px;
}

/*     #content {
  display: flex;
  flex-wrap: wrap;
  align-items: stretch
}
*/
figure {
  padding: 0px;
  margin: 0;
  margin-block-start: 0;
  margin-block-end: 0;
  margin-inline-start: 0;
  margin-inline-end: 0
}

figure img {
  display: block;
  max-width: 100%;
  width: auto;
  height: auto;
  border-radius: 4px;
  margin-top: 8px;
}

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

#quality {
    transform: rotateY(180deg);
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
  margin: 3px;
  padding: 0 8px;
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
  width: 0;
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
  background: #ff3034;
  width: 16px;
  height: 16px;
  border-radius: 100px;
  color: #fff;
  text-align: center;
  line-height: 18px;
  cursor: pointer
}

.close-rot-none {
  left: 5px;
  top: 5px;
}

.close-rot-left {
  right: 5px;
  top: 5px;
}

.close-rot-right {
  left: 5px;
  bottom: 5px;
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
})=====";

size_t style_css_len = sizeof(style_css)-1;
