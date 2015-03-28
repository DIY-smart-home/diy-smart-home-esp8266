var app = {

    sensorsData : null,

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
        document.addEventListener("resume", this.ping, false);
    },

    // deviceready Event Handler
    //
    // The scope of 'this' is the event. In order to call the 'receivedEvent'
    // function, we must explicitly call 'app.receivedEvent(...);'
    onDeviceReady: function() {

      app.ping();

      //retrieve data from the sensors each 10 seconds
      setInterval( app.loadSensorsData, 10000);

      $( "#buttonSaveThreshold" ).bind( "click", app.handleButton);
      $( "#buttonLogin" ).bind( "click", app.login);
      app.receivedEvent('deviceready');
    },

    login: function() {
      $.mobile.pageContainer.pagecontainer("change", "#page-temperature");
    },

    ping: function() {
      $.ajax({ url: "http://iot.anavi.org/ping",
        //Success callback
        success: app.handlePingResponse,
        error: app.requestError
      });
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
        if (result === app.sensorsData) {
          return;
        }
        app.sensorsData = result;
        //Update text
        var info = "";
        for (var property in result.data) {
          info += property + ": " + result.data[property] + "C <br />\n";
        }
        $("#text").html(info);
    },

    // Update DOM on a Received Event
    receivedEvent: function(id) {
      //document.getElementById('loading').setAttribute('style', 'display:none;');
      //document.getElementById('info').setAttribute('style', 'display:block;');
      console.log('Received Event: ' + id);
    },

    // Handle button click
    handleButton : function() {
      var thresholdTemperature = parseInt($('#threshold').val(), 10);
      if (isNaN(thresholdTemperature)) {
        thresholdTemperature = 0;
      }

      $.ajax({ url: "http://iot.anavi.org/settings/temperature",
        type: "GET",

        data: { temperature: thresholdTemperature },

        success: function(data) {
          var result = JSON.parse(data);
          if (result.hasOwnProperty('errorCode') && (0 == result.errorCode) ) {
            alert('Threshold temperature saved.');
          }
          else {
            alert('Unable to save threshold temperature. Please try again later.');
          }
        },
        error: app.requestError
      });

    }
};

app.initialize();
