#include <Arduino.h>

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
  html {
    font-family: Arial, Helvetica, sans-serif;
    text-align: center;
  }
  h1 {
    font-size: 1.8rem;
    color: white;
  }
  h2{
    font-size: 1.5rem;
    font-weight: bold;
    color: #143642;
  }
  .topnav {
    overflow: hidden;
    background-color: #143642;
  }
  body {
    margin: 0;
  }
  .content {
    padding: 30px;
    max-width: 600px;
    margin: 0 auto;
  }
  .card {
    background-color: #F8F7F9;;
    box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);
    padding-top:10px;
    padding-bottom:20px;
  }
  .button {
    padding: 15px 50px;
    font-size: 24px;
    text-align: center;
    outline: none;
    color: #fff;
    background-color: #0f8b8d;
    border: none;
    border-radius: 5px;
    -webkit-touch-callout: none;
    -webkit-user-select: none;
    -khtml-user-select: none;
    -moz-user-select: none;
    -ms-user-select: none;
    user-select: none;
    -webkit-tap-highlight-color: rgba(0,0,0,0);
  }
  /*.button:hover {background-color: #0f8b8d}*/
  .button:active {
    background-color: #0f8b8d;
    box-shadow: 2 2px #CDCDCD;
    transform: translateY(2px);
  }
  .state, .brightness{
    font-size: 1.5rem;
    color:#8c8c8c;
    font-weight: bold;
  }

body {
--bg-range-color: rgba(0, 0, 0, 0.5);
--bg-value-color: rgba(15, 139, 141,1);
}

.range{
    height: 120px;
}

.vertical-slider {
    transform: rotateZ(270deg) translateX(-30px);
    touch-action: none;
    display: inline-block;
    width: 110px;
    height: 50px;
    -webkit-appearance: none;
    appearance: none;
    outline: none;
    border-radius: 8px;
    background-color: transparent;
    -webkit-backdrop-filter: blur(10px);
    backdrop-filter: blur(10px);
    background-image: linear-gradient(
            90deg,
            var(--bg-value-color) 0%,
            var(--bg-value-color) 30%,
            var(--bg-range-color) 30%,
            var(--bg-range-color) 100%
    );
}

.vertical-slider::-webkit-slider-runnable-track {
    -webkit-appearance: none;
    appearance: none;
}

.vertical-slider::-webkit-slider-thumb {
    -webkit-appearance: none;
    appearance: none;
    width: 25px;
    height: 50px;
    background: transparent;
    border: none;
    cursor: pointer;
}
  </style>
<title>ESP Web Server</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="icon" href="data:,">
</head>
<body>
  <div class="topnav">
    <h1>ESP WebSocket Server</h1>
  </div>
  <div class="content">
    <div class="card">
      <h2>Ledstrip</h2>
      <p class="brightness">brightness: <span id="brightness">%BRIGHTNESS%</span></p>
      <div class="range">
        <input class="vertical-slider" name="blur" id="blur" type="range" min="0" max="100" value="5" oninput="changeRange(this)" />
      </div>      
      <p class="state">state: <span id="state">%STATE%</span></p>
                      
      <p class="switch">
          <input type="range" onchange="updateSliderPWM(this)" id="slider2" min="50" max="100" step="1" value ="0" class="slider">
      </p>
      <p class="state">Max Brightness: <span id="sliderValue2"></span> &percnt;</p>
      
      <p><button id="button" class="button">Toggle</button></p>

      
    </div>
  </div>
<script>
  var gateway = `ws://${window.location.hostname}/ws`;
  var websocket;
  window.addEventListener('load', onLoad);
  function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage; // <-- add this line
  }
  function onOpen(event) {
    console.log('Connection opened');
  }
  function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
  }
  function onMessage(event) {
    var state;
    //console.log(event.data)
    var ledstrip = JSON.parse(event.data).ledstrip
    console.log(ledstrip);
    if (ledstrip.state == "1"){
      state = "ON";
    }
    else{
      state = "OFF";
    }

    document.getElementById('state').innerHTML = state;
    document.getElementById('brightness').innerHTML = Math.trunc(ledstrip.brightness) + '%';
    document.getElementById("slider2").value = Math.trunc(ledstrip.maxBrightness100);

    if (document.getElementById("blur").value != Math.trunc(ledstrip.targetBrightness100) && document.getElementById("blur") !== document.activeElement){
      document.getElementById("blur").style.backgroundImage = getSliderBgCss(Math.trunc(ledstrip.targetBrightness100));
      document.getElementById("blur").value = Math.trunc(ledstrip.targetBrightness100);
    }
    
  }
  function onLoad(event) {
    initWebSocket();
    initButton();
  }
  function initButton() {
    document.getElementById('button').addEventListener('click', toggle);
  }
  function toggle(){
    websocket.send('toggle');
  }
  function updateSliderPWM(element) {
  var sliderNumber = element.id.charAt(element.id.length-1);
  var sliderValue = document.getElementById(element.id).value;
  var sliderText = document.getElementById("sliderValue"+sliderNumber)
  if (sliderText != null) {sliderText.innerHTML = sliderValue}
  console.log(sliderValue);
  websocket.send(sliderNumber+"s"+sliderValue.toString()+";");
  }

  function getSliderBgCss(percent){
    return `linear-gradient(90deg, var(--bg-value-color) 0\u0025, var(--bg-value-color) ${percent}\u0025, var(--bg-range-color) ${percent}\u0025, var(--bg-range-color) 100\u0025)`;
  }

  function changeRange(_this, _value) {
    if (_value !== undefined) {
        _this.value = _value;
    }
    console.log(_this.value);
    const percent = (+_this.value / +_this.max) * 100;
    // const percent100 = percent * 100;
    // _this.style.backgroundImage = getSliderBgCss(percent, percent100);
    _this.style.backgroundImage = getSliderBgCss(percent);
    websocket.send("1s"+_this.value.toString()+";");
  }
</script>
</body>
</html>
)rawliteral";