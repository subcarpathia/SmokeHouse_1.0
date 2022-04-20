/******************************************
    Data Submit
*******************************************/

function submitForm(){

  var SetTempMain = document.getElementById("setmaintemp").value;
  websocket.send("SetTempMain."+SetTempMain);
}

/******************************************
    GPIOS and SoftButton Control
*******************************************/

var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
window.addEventListener('load', onLoad);

function onLoad(event) {
    initWebSocket();
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
    websocket.send("states");
}
  
function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
} 



function onMessage(event) {
    var myObj = JSON.parse(event.data);
            console.log(myObj);
            for (i in myObj.gpios){
                var output = myObj.gpios[i].output;
                var state = myObj.gpios[i].state;
                console.log(output);
                console.log(state);
                if (state == "1"){
                    document.getElementById(output).checked = true;
                    /*document.getElementById(output+"s").innerHTML = "ON";*/
                }
                else{
                    document.getElementById(output).checked = false;
                    /*document.getElementById(output+"s").innerHTML = "OFF";*/
                }
            }

           //var softmyObj = JSON.parse(event.data);
           // console.log(softmyObj);
           // for (i in softmyObj.softgpios){
           //     var softoutput = softmyObj.softgpios[i].output;
           //     var state = softmyObj.softgpios[i].state;
           //     console.log(softoutput);
           //     console.log(state);
           //     if (state == "1"){
           //         document.getElementById(softoutput+"soft").checked = true;
           //         document.getElementById(softoutput+"ssoft").innerHTML = "ON";
           //     }
           //     else{
           //         document.getElementById(softoutput+"soft").checked = false;
           //         document.getElementById(softoutput+"ssoft").innerHTML = "OFF";
           //     }
           // }

    console.log(event.data);
}

// Send Requests to Control GPIOs
function toggleCheckbox (element) {
    console.log(element.id);
    websocket.send(element.id);
    if (element.checked){
        document.getElementById(element.id+"s").innerHTML = "ON";
    }
    else {
        document.getElementById(element.id+"s").innerHTML = "OFF"; 
    }
}


// Send Requests to Control Soft GPIOs
  //function toggleSoftCheckbox (element) {
   //console.log(element.id);
   // websocket.send(element.id);
   //if (element.checked){
   //     document.getElementById(element.id+"ssoft").innerHTML = "ON";
   // }
   // else {
   //     document.getElementById(element.id+"ssoft").innerHTML = "OFF"; 
   // }
//}


/******************************************
            READINGS CONTROL
*******************************************/

// Get current sensor readings when the page loads  
window.addEventListener('load', getReadings);


// Function to get current readings on the webpage when it loads for the first time
function getReadings(){
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var myObj = JSON.parse(this.responseText);
      console.log(myObj);
      var temp = myObj.temperature;
      var setpt = myObj.setpoint;
      var powr = myObj.power;
      var status = myObj.statusmsg;
      //var hum = myObj.humidity;
      //console.log("temperature", e.data);
      document.getElementById("temp").innerHTML = temp;
      document.getElementById("setp").innerHTML = setpt;
      document.getElementById("power").innerHTML = powr;
      document.getElementById("status").innerHTML = status;
    }
  }; 
  xhr.open("GET", "/readings", true);
  xhr.send();
}

if (!!window.EventSource) {
  var source = new EventSource('/events');
  
  source.addEventListener('open', function(e) {
    console.log("Events Connected");
  }, false);

  source.addEventListener('error', function(e) {
    if (e.target.readyState != EventSource.OPEN) {
      console.log("Events Disconnected");
    }
  }, false);
  
  source.addEventListener('message', function(e) {
    console.log("message", e.data);
  }, false);
  
  source.addEventListener('new_readings', function(e) {
    console.log("new_readings", e.data);
    var myObj = JSON.parse(e.data);
    console.log(myObj);
    var temp = myObj.temperature;
    var setpt = myObj.setpoint;
    var powr = myObj.power;
    var status = myObj.statusmsg;
      //console.log("temperature", e.data);
      document.getElementById("temp").innerHTML = temp;
      document.getElementById("setp").innerHTML = setpt;
      document.getElementById("power").innerHTML = powr;
      document.getElementById("status").innerHTML = status;
      }, false);
}