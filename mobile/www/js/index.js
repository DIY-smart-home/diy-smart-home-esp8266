var app = {
    // Application Constructor
    initialize: function() {
        this.bindEvents();
    },

    // Bind Event Listeners
    //
    // Bind any events that are required on startup. Common events are:
    // 'load', 'deviceready', 'offline', and 'online'.
    bindEvents: function() {
        document.addEventListener('deviceready', this.onDeviceReady, false);
        document.getElementById('button').onclick = this.handleButton;
    },

    // deviceready Event Handler
    //
    // The scope of 'this' is the event. In order to call the 'receivedEvent'
    // function, we must explicitly call 'app.receivedEvent(...);'
    onDeviceReady: function() {

      $.ajax({ url: "http://iot.anavi.org/ping",
        //Success callback
        success: app.handlePingResponse,
        error: app.requestError
      });

      app.receivedEvent('deviceready');
    },

    handlePingResponse : function(data) {
        var result = JSON.parse(data);
        if (result.hasOwnProperty('errorCode') && (0 == result.errorCode) ) {
          app.loadSensorsData();
        }
        else {
          alert('Ping request failed.');
        }
    },

    requestError : function(xhr) {
      alert("Error: " + xhr.status + " " + xhr.statusText);
    },

    loadSensorsData : function() {
      $.ajax({ url: "http://iot.anavi.org/sensors/temperature",
        success: app.loadSensorsTemperature,
        error: app.requestError
      });
    },

    loadSensorsTemperature : function(data) {
        var result = JSON.parse(data);
        var info = "";
        for (var property in result.data) {
          info += property + ": " + result.data[property] + "C <br />\n";
        }
        $("#text").html(info);
    },

    // Update DOM on a Received Event
    receivedEvent: function(id) {
      document.getElementById('loading').setAttribute('style', 'display:none;');
      document.getElementById('info').setAttribute('style', 'display:block;');
      console.log('Received Event: ' + id);
    },

    // Handle button click
    handleButton : function() {
      console.log('clicked :)');
    }
};

app.initialize();
