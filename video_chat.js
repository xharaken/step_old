window.onload = function () {
  var peer = new Peer({key: 'its5por6n2i96bt9'});
  peer.on("open", function (id) {
    document.getElementById("id").innerText = id;
  });

  var active_connection = null;
  document.getElementById("connection_status").innerText = "Not connected";
  peer.on("connection", function(connection) {
    document.getElementById("connection_status").innerText = "Connected with " + connection.peer;
    active_connection = connection;
    active_connection.on("data", function(message) {
      document.getElementById("message_log").innerHTML += "<b>" + connection.peer + "</b>: " + message + "<br>";
    });
  });

  document.getElementById("call_button").addEventListener("click", function (event) {
    document.getElementById("connection_status").innerText = "Connecting...";
    var call_id = document.getElementById("call_id").value;
    connection = peer.connect(call_id);
    connection.on("open", function () {
      document.getElementById("connection_status").innerText = "Connected with " + call_id;
      active_connection = connection;
      active_connection.on("data", function(message) {
        document.getElementById("message_log").innerHTML += "<b>" + connection.peer + "</b>: " + message + "<br>";
      });
    });
  });

  document.getElementById("send_button").addEventListener("click", function (event) {
    var message = document.getElementById("send_message").value;
    document.getElementById("send_message").value = "";
    document.getElementById("message_log").innerHTML += "<b>" + peer.id + "</b>: " + message + "<br>";
    if (active_connection) {
      active_connection.send(message);
    }
  });
}
