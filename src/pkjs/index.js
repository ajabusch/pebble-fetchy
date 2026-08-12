var CONFIG = 'https://ajabusch.github.io/pebble-fetchy/';

var METHODS_WITH_BODY = ['POST', 'PUT', 'PATCH'];

// 6 request slots: 0=Up, 1=Select, 2=Down (short press)
//                  3=Up-Hold, 4=Select-Hold, 5=Down-Hold (long press)
var NUM_SLOTS = 6;

function getRequests() {
  var raw = localStorage.getItem('requests');
  if (raw) {
    try {
      return JSON.parse(raw);
    } catch (e) { }
  }
  var empty = [];
  for (var i = 0; i < NUM_SLOTS; i++) empty.push(null);
  return empty;
}

function hasStoredRequests() {
  var requests = getRequests();
  for (var i = 0; i < requests.length; i++) {
    if (requests[i] !== null) return true;
  }
  return false;
}

function sendRequestNames() {
  var requests = getRequests();
  var msg = {};
  for (var i = 0; i < NUM_SLOTS; i++) {
    var key = 'RequestName' + i;
    msg[key] = requests[i] && requests[i].name ? requests[i].name : '';
  }
  Pebble.sendAppMessage(msg);
}

function sendConfigToWatch(requests) {
  var json = JSON.stringify({ requests: requests });
  Pebble.sendAppMessage({ ConfigData: json });
}

function executeRequest(index) {
  if (index < 0 || index >= NUM_SLOTS) {
    Pebble.sendAppMessage({
      ResponseSuccess: 0,
      ResponseStatus: 0
    });
    return;
  }

  var requests = getRequests();
  var config = requests[index];

  if (!config || !config.url) {
    Pebble.sendAppMessage({
      ResponseSuccess: 0,
      ResponseStatus: 0
    });
    return;
  }

  var options = {
    method: config.method || 'GET',
    headers: {}
  };

  if (config.headers) {
    for (var i = 0; i < config.headers.length; i++) {
      var h = config.headers[i];
      if (h.key && h.value) {
        options.headers[h.key] = h.value;
      }
    }
  }

  if (METHODS_WITH_BODY.indexOf(config.method) !== -1 && config.body) {
    options.body = config.body;
  }

  var xhr = new XMLHttpRequest();
  xhr.open(options.method, config.url, true);

  var headerKeys = Object.keys(options.headers);
  for (var j = 0; j < headerKeys.length; j++) {
    xhr.setRequestHeader(headerKeys[j], options.headers[headerKeys[j]]);
  }

  xhr.onload = function () {
    Pebble.sendAppMessage({
      ResponseSuccess: xhr.status >= 200 && xhr.status < 300 ? 1 : 0,
      ResponseStatus: xhr.status
    });
  };

  xhr.onerror = function () {
    Pebble.sendAppMessage({
      ResponseSuccess: 0,
      ResponseStatus: 0
    });
  };

  xhr.timeout = 15000;
  xhr.ontimeout = function () {
    Pebble.sendAppMessage({
      ResponseSuccess: 0,
      ResponseStatus: 408
    });
  };

  xhr.send(options.body || null);
}

Pebble.addEventListener('ready', function () {
  if (hasStoredRequests()) {
    sendRequestNames();
  } else {
    // localStorage is empty — request backup from watch
    Pebble.sendAppMessage({ RequestBackup: 1 });
  }
});

Pebble.addEventListener('appmessage', function (e) {
  var buttonIndex = e.payload.ButtonIndex;
  if (buttonIndex !== undefined) {
    executeRequest(buttonIndex);
  }

  // Handle config restore from watch backup
  var configData = e.payload.ConfigData;
  if (configData) {
    try {
      var data = JSON.parse(configData);
      if (data.requests) {
        localStorage.setItem('requests', JSON.stringify(data.requests));
        sendRequestNames();
      }
    } catch (err) {
      console.log('Failed to restore config backup: ' + err);
    }
  }
});

Pebble.addEventListener('showConfiguration', function () {
  var currentSettings = { requests: getRequests() };
  var url = CONFIG + '#' + encodeURIComponent(JSON.stringify(currentSettings));
  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function (e) {
  if (e && e.response && e.response.length > 0) {
    try {
      var data = JSON.parse(decodeURIComponent(e.response));
      if (data.requests) {
        localStorage.setItem('requests', JSON.stringify(data.requests));
        sendRequestNames();
        // Back up config to watch persistent storage
        sendConfigToWatch(data.requests);
      }
    } catch (err) {
      console.log('Failed to parse config: ' + err);
    }
  }
});
