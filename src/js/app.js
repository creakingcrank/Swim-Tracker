// Store the data we get in these arrays
var len = [];
var int = [];
var stt = [];
var end = [];
var str = [];


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
  var maxNumberOfItems = 255;
  
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
  sendMessage();
});
												
// Called when incoming message from the Pebble is received
Pebble.addEventListener("appmessage", function(e) {
  
  var last;
  
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
  }
  
});