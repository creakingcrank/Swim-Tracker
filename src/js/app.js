// Store the data we get in these arrays
var len = [];
var int = [];
var stt = [];
var end = [];
var str = [];

var toComputerIndex = 0; // the next array member to send to the computer

var userID; // Put UserID info here
var deviceID; // Put DeviceID info here


function restoreData () {
  
  // localStorage.clear(); // Debug only: Reset local storage after model change
  
  
  if (localStorage.getItem('lengths')) { 
    len = JSON.parse(localStorage.getItem('lengths'));
    int = JSON.parse(localStorage.getItem('intervals'));
    stt = JSON.parse(localStorage.getItem('starts'));
    end = JSON.parse(localStorage.getItem('ends'));
    str = JSON.parse(localStorage.getItem('strokes'));
  }
  
  
}

function saveData () {
  
  var numberOfItems;
  var maxNumberOfItems = 255; // Only keep this number of lengths in local storage
  
  // trim older data
  numberOfItems=len.length;
  if (numberOfItems > maxNumberOfItems) {
    len.splice(0,numberOfItems-maxNumberOfItems);
    int.splice(0,numberOfItems-maxNumberOfItems);
    stt.splice(0,numberOfItems-maxNumberOfItems);
    end.splice(0,numberOfItems-maxNumberOfItems);
    str.splice(0,numberOfItems-maxNumberOfItems);
  }
  
  localStorage.setItem('lengths',JSON.stringify(len));
  localStorage.setItem('intervals',JSON.stringify(int));
  localStorage.setItem('starts',JSON.stringify(stt));
  localStorage.setItem('ends',JSON.stringify(end));
  localStorage.setItem('strokes',JSON.stringify(str));
  
}


// Function to send a message to the Pebble using AppMessage API
// We are currently only sending a message using the "status" appKey defined in appinfo.json/Settings
function sendMessage() {
	Pebble.sendAppMessage({"Status": 1}, messageSuccessHandler, messageFailureHandler);
}

// Called when the message send attempt succeeds
function messageSuccessHandler() {
  console.log("Message send succeeded.");  
}

// Called when the message send attempt fails
function messageFailureHandler() {
  console.log("Message send failed.");
  sendMessage();
}

// Called when JS is ready
Pebble.addEventListener("ready", function(e) {
  console.log("JS is ready!");
  restoreData ();
  userID = Pebble.getAccountToken();
  deviceID = Pebble.getWatchToken();
  sendMessage();
});
												
// Called when incoming message from the Pebble is received
Pebble.addEventListener("appmessage", function(e) {
  
  var last; //What is this for?
  
  len.push(e.payload.Length);
  int.push(e.payload.Interval);
  stt.push(e.payload.Start);
  end.push(e.payload.End);
  str.push(e.payload.Strokes);
  
  last = len.length - 1;
  
  console.log("Storage loc:" + last);
  console.log("Stored Length: " + len[last] );
  console.log("Stored Interval: " + int[last] );
  console.log("Stored Start: " + stt[last] );
  console.log("Stored End: " + end[last] );
  console.log("Stored Strokes: " + str[last] );
 
  if (e.payload.Status == 200) {
    console.log("End of batch received");
    saveData ();
    sendToComputer();
  }
  
});
/*
// delete a length from the array 
function dropLength(index) {
   
  if ( ( index < len.length ) && ( index >= 0 ) ) {
    len.splice(index,1);
    int.splice(index,1);
    stt.splice(index,1);
    end.splice(index,1);
    str.splice(index,1);
   }
 }
*/ 

//Send data to the computer
function sendToComputer() {
  
  var i = toComputerIndex;
  var method = 'GET';
  var url = "http://82.69.231.230:80/swim_tracker/store.php?" + "uid=" + userID + "&did=" +deviceID + "&len=" + len[i] + "&int=" + int[i] + "&stt=" + stt[i] + "&end=" + end[i] + "&str=" + str[i];
  var request = new XMLHttpRequest();

// Specify the callback for when the request is completed
request.onload = function() {
  // The request was successfully completed!
  console.log('Server responded: ' + this.responseText);
  //if (this.responseText == 'OK') { // just sending blind right now....
    toComputerIndex = toComputerIndex + 1; 
    if (toComputerIndex<len.length) {
       console.log('Trying next length: ' + toComputerIndex);
        sendToComputer();
    }
    else {
    console.log("All lengths sent");
    //localStorage.clear(); Got rid of this - just going to keep 255 lengths in memory and filter server side
    // Pebble.showSimpleNotificationOnPebble("Swim Tracker", "Upload complete"); // Got rid of this as annoying when watch connected to phone
   }
  //}
};

// Send the request
request.open(method, url); 
request.send(); 
}
  
  