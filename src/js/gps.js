function sendToPebble(dictionary) {
   // Send to Pebble
   Pebble.sendAppMessage(dictionary,
     function(e) {
       // Success
     },
     function(e) {
       console.log('Error sending dictonary to Pebble!');
     }
   );
}

function locationSuccess(pos) {
  var timezone = new Date();
  console.log('timezone = ' + timezone + ' offset = ' + timezone.getTimezoneOffset());
  console.log('longitude = ' + pos.coords.longitude + ' latitude = ' + pos.coords.latitude);
  var offset = timezone.getTimezoneOffset();

  // Assemble dictionary using our keys
  var dictionary = {
    'KEY_LONGITUDE': pos.coords.longitude,
    'KEY_LATITUDE': pos.coords.latitude,
    'KEY_TIMEZONE': offset,
  };

  // Send to Pebble
  sendToPebble(dictionary);
}

function locationError(err) {
  var timezone = new Date();
  console.log('Error requesting location!');
  console.log('timezone = ' + timezone + ' offset = ' + timezone.getTimezoneOffset());
  var offset = timezone.getTimezoneOffset();

  // Assemble dictionary using our keys
  var dictionary = {
    'KEY_LONGITUDE': 0,
    'KEY_LATITUDE': 0,
    'KEY_TIMEZONE': offset,
  };

  // Send to Pebble
  sendToPebble(dictionary);
}

function getPosition() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', function() {
    console.log('PebbleKit JS ready!');
    // Get the position
    getPosition();
});
