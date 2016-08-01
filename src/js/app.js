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
  sendMessage();
});
												
// Called when incoming message from the Pebble is received
Pebble.addEventListener("appmessage", function(e) {
  console.log("Received Length: " + e.payload.Length);
  console.log("Received Interval: " + e.payload.Interval);
  console.log("Received Start: " + e.payload.Start);
  console.log("Received End: " + e.payload.End);
});