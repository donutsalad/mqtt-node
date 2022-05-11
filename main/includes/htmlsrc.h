#define WIFI_CONFIG_HTML_WIFI_DETAILS "<html><head><title>ESP32 MQTT Node Setup</title><link rel=\"stylesheet\" href=\"style.css\"></head><body><div class=\"card\"><div class=\"header\">Set WLAN Details (1/3)</div><div class=\"body\"><form action=\"/wifi\" method=\"POST\"><div class=\"form-group\"><label for=\"a\">Network SSID (Name)</label><input name=\"a\" id=\"a\" placeholder=\"Your Network Here\"></div><div class=\"form-group\"><label for=\"b\">Network Password</label><input name=\"b\" id=\"b\" placeholder=\"Password\"></div><div class=\"form-group\"><button>Set WLAN Details</button></div></form></div><div class=\"footer\">The WLAN that a MQTT Server is available on.</div></div></body></html>"
#define WIFI_CONFIG_HTML_WIFI_CONFIRM "<html><head><title>ESP32 MQTT Node Setup</title><link rel=\"stylesheet\" href=\"style.css\"><meta http-equiv=\"refresh\" content=\"5;url=/mqtt\" /></head><body><div class=\"card\"><div class=\"header\">WiFi Config Set!</div><div class=\"body\"><div class=\"message\"><p>The WiFi settings have been parsed!</p><p class=\"new\">You will be automatically directed to</p><p>the MQTT settings in 5 seconds!</p></div></div><div class=\"footer\"><a href=\"/mqtt\">Go there now!</a></div></div></body></html>"
#define WIFI_CONFIG_HTML_MQTT_DETAILS "<html><head><title>ESP32 MQTT Node Setup</title><link rel=\"stylesheet\" href=\"style.css\"></head><body><div class=\"card\"><div class=\"header\">Set MQTT Details (2/3)</div><div class=\"body\"><form action=\"/mqtt\" method=\"POST\"><div class=\"form-group\"><label for=\"a\">IP/DNS Address</label><input name=\"a\" id=\"a\" placeholder=\"192.168.0.2\"></div><div class=\"form-group\"><label for=\"b\">MQTT Username</label><input name=\"b\" id=\"b\" placeholder=\"mymqttnode1\"></div><div class=\"form-group\"><button>Set MQTT Details</button></div></form></div><div class=\"footer\">The custom MQTT server to connect to.</div></div></body></html>"
#define WIFI_CONFIG_HTML_MQTT_CONFIRM "<html><head><title>ESP32 MQTT Node Setup</title><link rel=\"stylesheet\" href=\"style.css\"><meta http-equiv=\"refresh\" content=\"5;url=/boot\" /></head><body><div class=\"card\"><div class=\"header\">MQTT Config Set!</div><div class=\"body\"><div class=\"message\"><p>The MQTT settings have been parsed!</p><p class=\"new\">You will be automatically directed to</p><p>the Boot Mode settings in 5 seconds!</p></div></div><div class=\"footer\"><a href=\"/boot\">Go there now!</a></div></div></body></html>"
#define WIFI_CONFIG_HTML_DENY_BOOTNOW "<html><head><title>ESP32 MQTT Node Setup</title><link rel=\"stylesheet\" href=\"style.css\"><meta http-equiv=\"refresh\" content=\"5;url=/\" /></head><body><div class=\"card\"><div class=\"header\">Can't boot yet!</div><div class=\"body\"><div class=\"message\"><p>Not all configuration is complete!</p><p class=\"new\">You will be automatically directed to</p><p>the root address in 5 seconds!</p></div></div><div class=\"footer\"><a href=\"/\">Go there now!</a></div></div></body></html>"
#define WIFI_CONFIG_HTML_MODE_DETAILS "<html><head><title>ESP32 MQTT Node Setup</title><link rel=\"stylesheet\" href=\"style.css\"></head><body><div class=\"card\"><div class=\"header\">Set Boot Mode (3/3)</div><div class=\"body\"><form action=\"/boot\" method=\"POST\"><div class=\"form-group\"><label for=\"a\">State loop mode</label><select name=\"a\" id=\"a\"><option value=\"f\">Full State Poll</option><option value=\"c\">Changed States Poll</option><option value=\"l\">Logging Output Only</option><option value=\"n\">No Poll</option></select></div><div class=\"form-group\"><label for=\"b\">State loop sleep period (in ms)</label><input name=\"b\" id=\"b\" placeholder=\"100\"></div><div class=\"form-group\"><button>Boot With Above Settings</button></div></form></div><div class=\"footer\">The boot mode to launch in.</div></div></body></html>"
#define WIFI_CONFIG_HTML_DONE_CONFIRM "<html><head><title>ESP32 MQTT Node Setup</title><link rel=\"stylesheet\" href=\"style.css\"></head><body><div class=\"card\"><div class=\"header\">Configuration Complete!</div><div class=\"body\"><div class=\"message\"><p>Awesome! Startup in progress, here's a quick synopsis</p><p>of the information we're booting up with:</p><hr /><p>WiFi SSID: %s</p><p>WiFi Password: %s</p><p>MQTT Broker: %s</p><p>MQTT Username: %s</p><p>Boot Mode: %s</p><p>Poll Delay: %d(ms)</p><hr /><p>If any of this information is incorrect hard reset and</p><p>follow the configuration steps again.</p><p class=\"new\">If the details are repetitively incorrect it's likely an</p><p>issue with the firmware, please open a GitHub issue!</p></div></div><div class=\"footer\"><a href=\"https://github.com/donutsalad/mqtt-node\">Open GitHub Repo</a></div></div></body></html>"
#define WIFI_CONFIG_HTML_E400_ERRPAGE "<html><head><title>ESP32 MQTT Node Setup</title><link rel=\"stylesheet\" href=\"style.css\"></head><body><div class=\"card\"><div class=\"header\">Formatting Error</div><div class=\"body\"><div class=\"message\"><p>Whilst parsing and setting configs with the provided options</p><p>something wasn't formatted correctly.</p><hr /><p>If you are formatting your data correctly, it could be an</p><p>issue with either the firmware or html forms.</p><p class=\"new\">Please open a GitHub issue if the issue persists!</p></div></div><div class=\"footer\"><a href=\"/\">Return to root address router.</a></div></div></body></html>"
#define WIFI_CONFIG_HTML_E404_ERRPAGE "<html><head><title>ESP32 MQTT Node Setup</title><link rel=\"stylesheet\" href=\"style.css\"><meta http-equiv=\"refresh\" content=\"15;url=/\" /></head><body><div class=\"card\"><div class=\"header\">Invalid URL</div><div class=\"body\"><div class=\"message\"><p>The URL you attempted to GET does not exist!</p><p>You will automatically be redirected to root in 15 seconds.</p><hr /><p>If you are getting this message from a valid URL, it could be</p><p>an issue with either the firmware or webserver code.</p><p class=\"new\">Please open a GitHub issue if the issue persists!</p></div></div><div class=\"footer\"><a href=\"/\">Return to root address router.</a></div></div></body></html>"
#define WIFI_CONFIG_HTML_E500_ERRPAGE "<html><head><title>ESP32 MQTT Node Setup</title><link rel=\"stylesheet\" href=\"style.css\"><meta http-equiv=\"refresh\" content=\"30\" /></head><body><div class=\"card\"><div class=\"header\">Internal Error (500)</div><div class=\"body\"><div class=\"message\"><p>Something unknown or unhandled has gone wrong whilst</p><p>processing your request. Refreshing in 30 seconds...</p><hr /><p>If this is a repetitive issue, please connect the ESP</p><p>to a serial monitor and read the UART output, it's likely</p><p>an issue with either the firmware or webserver code.</p><p class=\"new\">Please open a GitHub issue if the issue persists!</p></div></div><div class=\"footer\"><a href=\"/\">Return to root address router.</a></div></div></body></html>"
#define WIFI_CONFIG_HTML_ONLY_CSSFILE "body {display: grid;grid-template-columns: 1fr auto 1fr;grid-template-rows: 1fr auto 1fr;grid-template-areas:\". . .\"\". card .\"\". . .\";width: 100%;background: #2a2438;color: white;overflow: clip;}div.card {grid-area: card;box-shadow: 0 0 10px 10px rgba(0, 0, 0, 0.2);border-radius: 15px;}div.card > div {padding: 10px 10px 0 15px;}div.card > div.header {height: 35px;font-size: 20px;background: #5c5470;border-radius: 15px 15px 0 0;}div.card > div.footer {background: #5c5470;border-radius: 0 0 15px 15px;height: 30px;}div.card > div.body {background: #352f44;}div.body > form {margin-block-end: 0;}form > div.form-group {display: grid;grid-template-rows: 1fr 1fr;gap: 4px;padding-bottom: 1em;}div.form-group > button {grid-row-start: span 2;font-size: 15px;padding: 4px;border-radius: 5px;border: 1px white;background: #dbd8e3;color: black;}div.body > div.message {padding: 0px 10px 10px 0;}div.body > div.message p {margin-block-end: 0;margin-block-start: 0;}p.new {margin-block-start: 4px !important;}a {color: white;}br {font-size: 3px;}label {font-size: 18px;font-weight: bold;}input {width: 100%;border-radius: 5px;border: 1px solid #ccc;background: #dbd8e3;font-size: 15px;padding: 3px 6px;}select {width: 100%;border-radius: 5px;border: 1px solid #ccc;background: #dbd8e3;font-size: 15px;padding: 3px 6px;}"
