module.exports = function(RED) {
    "use strict";
    // require any external libraries we may need....
    //var foo = require("foo-library");
		
    // The main node definition - most things happen in here
    function AgentNode(n) {
        // Create a RED node
        RED.nodes.createNode(this,n);

        // Store local copies of the node configuration (as defined in the .html)
        this.topic = n.topic;
		this.port = n.port;
		this.host = n.host;
		this.guid = n.guid;
		this.passkey = n.passkey;

        // copy "this" object in case we need it in context of callbacks of other functions.
        var node = this;

        // Do whatever you need to do in here - declare callbacks etc
        
        var msg = {};
        msg.topic = this.topic;
		msg.payload = n.port + " " + n.host + " " + n.guid + " " + n.passkey;
		
		
		
		var XMLHttpRequest = require('xhr2');
		var crypto = require('crypto');
		
        // respond to inputs....
        this.on('input', function (msg) {
			var i = 1;
			var connected = 0;
			
			var callback = function (json, status) {
                    if (status) {
                        node.warn("Please check info is correct. " + status);
						connected = 0;
                    }

                    if (json['success'] == true) {
						node.warn("Broker found"+i);
						connected = 1;
                        //brokerFound(json);
                    }
                    else if (json['success'] == false) {
                        //node.warn("Broker server is being uncooperative. Check with the owner.");
                        node.warn("Error: " + json['error']);
						connected = 0;
                    }
					
					msg.payload = connected;
					if (connected == 1) {
						node.status({fill:"green",shape:"dot",text:"connected"});					
						node.send(msg);
					}
					else {
						node.status({fill:"red",shape:"ring",text:"disconnected"});
						return null;
					}
				};
				
            // in this example just send it straight on... should process it here really
			node.warn("xhr start");
			var xhr = new XMLHttpRequest();
			xhr.open('post', "http://" + node.host + ":" + node.port + "/wrapper-setup", true);
            xhr.timeout = 5000;			
			xhr.setRequestHeader("Content-Type", "application/json");
			node.warn("before ontimeout");
            xhr.ontimeout = function () {
                callback('', "Server is not responding. Request timed out.");
            }
            xhr.onerror = function (e) {
                callback('', "Server is not responding. " + xhr.statusText);
            };
			node.warn("before onload");
			xhr.onload = function () {
                var xmlDoc = xhr.responseText;
                var jsonResponse = JSON.parse(xmlDoc);
                callback(jsonResponse, '');
            }
			
            var json_data = JSON.stringify({action: "ping"});
            xhr.send(json_data);
			node.warn("xhr end");
			i+=1;
			var data_dictionary = [];
			
			data_dictionary['action'] = "registerWrapper";
			data_dictionary['wrapper_host'] = node.host;
			data_dictionary['wrapper_port'] = node.port;
			data_dictionary['time-stamp'] = new Date().getTime();
			data_dictionary['uid'] = node.guid;
			data_dictionary['token'] = '';
			node.warn("data dictionary filled mostly");
			var dictionaryAttribute = JSON.stringify(data_dictionary);
			node.warn("stringify");
			var computedSignature = crypto.createHmac('sha1', node.passkey).update(node.guid + dictionaryAttribute).digest('base64');
			
			data_dictionary['token'] = computedSignature;
			node.warn("xhr2 begin");
			var xhr2 = new XMLHttpRequest();
			xhr2.timeout = 5000;
			xhr2.open('post', "http://" + node.host + ":" + node.port + "/" + "wrapper-json", true);
			xhr2.setRequestHeader("Content-Type", "application/json");
			node.warn("xhr2 onerror");
			xhr2.onerror = function (e) {
				callback('', xhr2.statusText);
			};
			xhr2.ontimeout = function (e) {
				callback('', "Server is not responding. Request timed out.");
			};
			xhr2.onload = function () {
				var xmlDoc2 = xhr2.responseText;
				var jsonResponse2 = JSON.parse(xmlDoc2);
				callback(jsonResponse2, '');
			}

			node.warn("xhr2 send");
			var json_data2 = JSON.stringify(data_dictionary);
			xhr2.send(json_data2);

		});

        this.on("close", function() {
            // Called when the node is shutdown - eg on redeploy.
            // Allows ports to be closed, connections dropped etc.
            // eg: node.client.disconnect();
        });
    }

    // Register the node by name. This must be called before overriding any of the
    // Node functions.
    RED.nodes.registerType("agent",AgentNode);

}
