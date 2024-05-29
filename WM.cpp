#include "WM.h"

AsyncWebServer server(WEBsrv_PORT);
DynamicJsonDocument Config(2048);
JsonObject obj = Config.as<JsonObject>();

void WM::begin_server(){
     server.begin();
}
void WM::listDir(const char * dirname) {
      Serial.printf("DEBUG: Listing directory: %s\n", dirname);

      Dir root = LittleFS.openDir(dirname);

      while (root.next()) {
        File file = root.openFile("r");
        #if DEBUG_PROC
        Serial.print("DEBUG:  FILE: ");
        Serial.print(root.fileName());
        Serial.print("DEBUG:  SIZE: ");
        Serial.print(file.size());
        #endif
        file.close();
      }
}

void WM::create_server() {
      server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
        // scan for networks and get ssid
        String network_html = "";

        // link to code https://github.com/me-no-dev/ESPAsyncWebServer/issues/85#issuecomment-258603281
        // there is a problem with scanning whil using espasync that is why the code below is not just WiFi.scan()
        int n = WiFi.scanComplete();
        if (n == -2)
            WiFi.scanNetworks(true);
        else if (n) {
            for (int i = 0; i < n; ++i) {
                String router = WiFi.SSID(i);
                Serial.println(router);
                network_html += "<input type=\"radio\" id=\"#radiobuttonex\" name=\"ssid\" value=" + router + " required ><label for=\"html\">" + router + "</label><<br>";
            }
            WiFi.scanDelete();
            if (WiFi.scanComplete() == -2)
                WiFi.scanNetworks(true);
        }

        String html = "<!DOCTYPE html><html>";
        html+= "<head>";
        html+=" <link rel=\"stylesheet\" href=\"styles.css\">";
        html+= "</head>";
        html += "<body>";
        html+= "<div>";
        html += "<h1>WiFi Extender Config page</h1>";
        html += "<button onclick=\"window.location.href='/';\">Scan </button>";
        html += "<p>networks found </p>";
        html += "<form action=\"/credentials\">";
        html += "<p>Please select a WiFi network:</p>" + network_html;
        html += "<input type=\"password\" id=\"pass\" name=\"pass\" value=\"\" required ><label for=\"pass\">password</label><br><br>";
        html += "<input type=\"text\" id=\"ap\" name=\"ap\" value=\"\" required ><label for=\"ap\">A.P name:</label><br>";
        html += "<input type=\"submit\" value=\"Submit\">";
        html += "</form></body></html>";
        html+= "</div>";

        request->send(200, "text/html", html);
      });

// css style from grepper
      server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest * request) {
        String style = "#radiobuttonex{\n"
"  border: 2px solid rgb(255,1,199);\n"
"  text-align: center;\n"
"  font-family: sans serif;\n"
"  width: 305px;\n"
"  background: rgb(50,50,100);\n"
"  border-radius: 40px;\n"
"}";
        
       request->send(200, "text/css", style);
        });

      // Send a GET request to <IP>/get?message=<message> 
      server.on("/credentials", HTTP_GET, [] (AsyncWebServerRequest * request) {
        String param = "ssid";

        if (request->hasParam(param)) {
          String ssid = request->getParam(param)->value();
          Config["ssid"] = ssid;
          Serial.println(ssid);
        } else
            Serial.println("WARNING: No " + param + " sent");

        param = "pass";
        if (request->hasParam(param)) {
          String pass = request->getParam(param)->value();
          Config["pass"] = pass;
          Serial.println(pass);
        } else
            Serial.println("WARNING: No " + param + " sent");

        param = "ap";
        if (request->hasParam(param)) {
          String ap = request->getParam(param)->value();
          Config["ap"] = ap;
          Serial.println(ap);
        } else
            Serial.println("WARNING: No " + param + " sent");

        String output;
        serializeJson(Config, output);
        Serial.print(output);

        String path = CONFIG_FILE;

        #if DEBUG_PROC
        Serial.printf("DEBUG: Writing file: %s\n", path);
        #endif

        File file = LittleFS.open(path, "w");
        if (!file) {
            #if DEBUG_PROC
            Serial.println("DEBUG: Failed to open file for writing");
            #endif
            return "undefined";
        }
        #if DEBUG_PROC
        if (file.print(output))
            Serial.println("DEBUG: File written");
        else
            Serial.println("DEBUG: Write failed");
        #else
            file.print(output);
        #endif

        file.close();
      });
    }

String WM::get_credentials(int a) {
      // a: 0=ssid, 1=pass; 2=ap name
      String path = CONFIG_FILE;
      String credentials = "";

      #if DEBUG_PROC
      Serial.print("DEBUG: reading file ");
      Serial.println(path);
      #endif

      File file = LittleFS.open(path, "r");
      if (!file) {
        Serial.println("ERROR: Failed to open file for reading");
        Serial.println("NOTICE: this is probally first usage, so the file does not exist");
        return "undefined";
      }

      #if DEBUG_PROC
      Serial.print("DEBUG: Read from file: ");
      #endif
      while (file.available())
          credentials += file.readString();

      Serial.println(credentials);
      auto error = deserializeJson(Config, credentials);
      if (error) {
         Serial.println("ERROR: Failed to parse JSON config file!");
        return "undefined";
      }
      file.close();
      String credential_array [3] = {Config["ssid"], Config["pass"], Config["ap"]};
      return credential_array[a];
}
