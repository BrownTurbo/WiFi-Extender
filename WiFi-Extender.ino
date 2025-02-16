/* 
 *  Author: Pius Onyema Ndukwu
 *  Improved by Zorono
 *  License: AGPL 3.0
 *  GitHub:https://github.com/Pius171/esp8266-wifi-extender
 *  
 */

#include "WM.h"
#include "config.h"
#include "utils.h"

// variables
bool RepeaterIsWorking= true;
int ledState = LOW; 
unsigned long previousMillis = 0;
unsigned long delay_time=0; // interval between blinks
// blink every 200ms if connected to router
// blink every 1sec if web server is active
// led is off is there is an error with the repeater
//led is on when trying to connect to router.
int connectedStations = 0;

unsigned long lastConnectTry = 0;
unsigned long lastNetCheck = 0;
#if DEBUG_PROC
uint8_t lastStatus = 0;
#endif

#include <ESP8266WiFi.h>

#if LWIP_FEATURES && !LWIP_IPV6

#include <lwip/napt.h>
#include <lwip/dns.h>

#if HAVE_NETDUMP

#include <NetDump.h>
void dump(int netif_idx, const char* data, size_t len, int out, int success) {
  (void)success;
  Serial.print(out ? F("out ") : F(" in "));
  Serial.printf("%d ", netif_idx);

  // optional filter example: if (netDump_is_ARP(data))
  {
    netDump(Serial, data, len);
    //netDumpHex(Serial, data, len);
  }
}

#endif

WM WiFiM;

void InitSerial();
void InitSerial() {
    Serial.begin(BAUD_RATE);

    // There must be activity on the serial port for the baudrate to be detected
    unsigned long detectedBaudrate = Serial.detectBaudrate(SERIAL_TIMEOUT);

    if (detectedBaudrate) {
        Serial.printf("\nNOTICE: Detected baudrate is %lu, switching to that baudrate now...", detectedBaudrate);

        // Wait for printf to finish
        while (Serial.availableForWrite() != UART_TX_FIFO_SIZE)
              yield();

        // Clear Tx buffer to avoid extra characters being printed
        Serial.flush();

        // After this, any writing to Serial will print gibberish on the serial monitor if the baudrate doesn't match
        Serial.begin(detectedBaudrate);
        #if DEBUG_PROC
        Serial.printf("\nDEBUG: Serial is %lu bps", detectedBaudrate);
        #endif
    }
    Serial.setDebugOutput(DEBUG_PROC);

    Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
}


enum EventType {
  EVENT_NO_SSID,
  EVENT_CONNECT_FAILED,
  EVENT_CONNECTION_LOST,
  EVENT_TIMEOUT,
  EVENT_DEFAULT
};
#if BUZZER_ENABLED
#include "buzzer.h"
void triggerBuzzerForEvent(EventType event);
void triggerBuzzerForEvent(EventType event = EVENT_DEFAULT) {
  switch(event) {
    case EVENT_NO_SSID:
      InitBuzzer(1000, false, 5);
      break;
    case EVENT_CONNECT_FAILED:
      InitBuzzer(500, false, 3);
      break;
    case EVENT_CONNECTION_LOST:
      InitBuzzer(750, false, 4);
      break;
    case EVENT_TIMEOUT:
      InitBuzzer(2000, false, 5);
      break;
    default:
      InitBuzzer(1000, false, 6);
      break;
  }
  TriggerBuzzer();
}
#endif

unsigned char clientCount = 0; 
void  clientStatus();
void clientStatus() {
    struct station_info *stat_info = wifi_softap_get_station_info();
    int clientID = 0;

    while (stat_info != NULL) {
        clientID++;
        Serial.printf("\nINFO: (client ID = %d)IP Address = %s with MAC Address is = ", clientID, IPAddress((&stat_info->ip)->addr).toString().c_str());
        for (int i = 0; i < 6; ++i) {
            Serial.printf("%02X", stat_info->bssid[i]);
            if (i < 5)
                Serial.print(":");
        }

        stat_info = STAILQ_NEXT(stat_info, next);
        Serial.println();
    }
    wifi_softap_free_station_info();
    SafeDelay(500);
}

bool wServerStarted = false;
void StartWebserver();
void StartWebserver() {
        if (wServerStarted) {
          #if DEBUG_PROC
          Serial.println("\nWeb Server is already running.");
          #endif
          return;
        }
        WiFiClient client;
        client.setTimeout(2000);
        if(client.connect(WiFi.localIP(), WEBsrv_PORT)) {
          client.stop();
          #if DEBUG_PROC
          Serial.println("\nWeb Server is already listening on port.");
          #endif
          wServerStarted = true;
          return;
        }
        if(WiFi.getMode() != WIFI_AP || WiFi.softAPIP() == IPAddress(0,0,0,0)) {
          WiFi.softAP(DefaultWiFi);
        }
        #if DEBUG_PROC
        IPAddress AccessPointIP = WiFi.softAPIP();
        Serial.printf("\nDEBUG: AP IP address: %s", AccessPointIP.toString().c_str());
        
        Serial.printf("\nFree heap: %d\n", ESP.getFreeHeap());
        #endif
        WiFiM.create_server();
        WiFiM.begin_server();
        #if DEBUG_PROC
        Serial.printf("\nFree heap: %d\n", ESP.getFreeHeap());
        
        Serial.printf("\nDEBUG: HTTP server started on port %d", WEBsrv_PORT);
        #endif
        //delay_time = 1000; // blink every sec if webserver is active

        wServerStarted = true;
}

bool WaitWiFiConnection();
bool WaitWiFiConnection() {
    int timeout_counter=0;
      Serial.print("\nINFO: Waiting for connection to WiFi");
      while (WiFi.status() != WL_CONNECTED) {
          int ConnectionStatus = WiFi.waitForConnectResult();
          switch (ConnectionStatus) {
              /*case WL_CONNECT_WRONG_PASSWORD: {
                  Serial.print("\nERROR: Wrong WiFi Password!");
                  timeout_counter = WAIT_TIMEOUT;
                  break;
             }*/
             case WL_NO_SSID_AVAIL: {
                 Serial.print("\nERROR: SSID can't be reached!");
                 timeout_counter = WAIT_TIMEOUT;
                #if BUZZER_ENABLED
                InitBuzzer(1000, false, 5);
                TriggerBuzzer();
                #endif
                 break;
             }
             case WL_CONNECT_FAILED: {
                 Serial.print("\nERROR: Failed to connect to WiFi!");
                 timeout_counter = WAIT_TIMEOUT;
                 #if BUZZER_ENABLED
                 InitBuzzer(1000, false, 5);
                 TriggerBuzzer();
                 #endif
                 break;
             }
             case WL_CONNECTION_LOST: {
                 Serial.print("\nERROR: Lost Connection to WiFi!");
                 timeout_counter = WAIT_TIMEOUT;
                 #if BUZZER_ENABLED
                 InitBuzzer(1000, false, 5);
                 TriggerBuzzer();
                 #endif
                 break;
             }
             case -1: {
                 Serial.print("\nERROR: Timeout on connecting to WiFi!");
                 timeout_counter = WAIT_TIMEOUT;
                 #if BUZZER_ENABLED
                 InitBuzzer(1000, false, 5);
                 TriggerBuzzer();
                 #endif
                 break;
              }
              default: {
                 Serial.print("\nERROR: Something went wrong!");
                 timeout_counter = WAIT_TIMEOUT;
                 #if BUZZER_ENABLED
                 InitBuzzer(1000, false, 6);
                 TriggerBuzzer();
                 #endif
                 break;
              }
          }
          if(timeout_counter>=WAIT_TIMEOUT) {
              Serial.print("\nERROR: Timeout on connecting to WiFi! Starting fallback web server.");
              #if BUZZER_ENABLED
              InitBuzzer(2000, false, 5);
              TriggerBuzzer();
              #endif
              StartWebserver();
          }

          Serial.print('.');
          timeout_counter++;
          digitalWrite(LED_BUILTIN, LOW);
          SafeDelay(WIFI_RECONNECT_TIMER);
    }
    return (WiFi.isConnected());
}

void PrintMacAddresses();
void PrintMacAddresses() {
       #if DEBUG_PROC
        uint8_t macAddress[6];
        WiFi.macAddress(macAddress);
        Serial.print("\nDEBUG: Station MAC Address: ");
        for (int i = 0; i < 6; ++i) {
            Serial.printf("%02X", macAddress[i]);
            if (i < 5)
                Serial.print(":");
        }
        WiFi.softAPmacAddress(macAddress);
        Serial.print("\nDEBUG: Access point MAC Address: ");
        for (int i = 0; i < 6; ++i) {
            Serial.printf("%02X", macAddress[i]);
            if (i < 5)
                Serial.print(":");
        }
        #endif
}

bool CheckAvailability(const String url);
bool CheckAvailability(const String url) {
  WiFiClient client;
  if (!client.connect(url, 80))
      return false;
  client.stop();
  return true;
}

void parseBytes(const char* str, char sep, byte* bytes, int maxBytes, int base);
void parseBytes(const char* str, char sep, byte* bytes, int maxBytes, int base) {
    for (int i = 0; i < maxBytes; i++) {
        bytes[i] = strtoul(str, NULL, base);
        str = strchr(str, sep);
        if (str == NULL || *str == '\0')
            break;                            
        str++;
    }
}

void setup() {
    #if STARTUP_DELAY >= 500
    SafeDelay(STARTUP_DELAY);
    #endif
    pinMode(0,INPUT_PULLUP);
    pinMode(LED_BUILTIN,OUTPUT);
    #if BUZZER_ENABLED
    pinMode(BUZZER_PIN, OUTPUT);
    #endif
    digitalWrite(LED_BUILTIN, HIGH);
    InitSerial();
    
     if (WiFi.status() == WL_NO_SHIELD) {
        Serial.println("ERROR: WiFi shield not present");
        #if BUZZER_ENABLED
        InitBuzzer(1000, false, 5);
        TriggerBuzzer();
        #endif
        delay_time = 4000;
        return;
     }      
     
     // ??
    Serial.println();

    if (!LittleFS.begin()) {
        Serial.println("ERROR: LittleFS mount failed");
        #if BUZZER_ENABLED
        InitBuzzer(1000, false, 5);
        TriggerBuzzer();
        #endif
        delay_time = 4000;
        return;
    }

    Serial.printf("\n\nNAPT Range extender");
    #if DEBUG_PROC
    Serial.printf("\nDEBUG: Heap on start: %d", ESP.getFreeHeap());
    #endif

#if HAVE_NETDUMP
    phy_capture = dump;
#endif
    
   String ssid = WiFiM.get_credentials(0);
   String pass =WiFiM.get_credentials(1);
   String ap = WiFiM.get_credentials(2);

    if (ssid == "undefined") {
        // if the file does not exist, ssid will be undefined
        StartWebserver();
        PrintMacAddresses();
        delay_time = 1500;
  }
  else if (ssid == "null" || strlen(ssid.c_str()) < 1 || strlen(ap.c_str()) < 1) {
        // if the JSON parser failed, ssid will be null
        Serial.print("\nNOTICE: temporary reversing configurations to defaults...");
        StartWebserver();
        PrintMacAddresses();
        delay_time=750;
  }
  else
  {
      WiFi.mode(WIFI_STA);
      PrintMacAddresses();
      #if DEBUG_PROC
      Serial.printf("\nDEBUG: Default hostname: %s\n", WiFi.hostname().c_str());
      #endif
      Serial.printf("\nINFO: Setting Station Hostname ... %s", (WiFi.hostname(WIFI_STA_HOSTNAME) ? "OK" : "Failed"));
      WiFi.begin(ssid, pass, WiFiChannel);
      WiFi.setAutoConnect(WIFI_AUTOCONNECT);
      WiFi.setAutoReconnect(WIFI_AUTORECONNECT);
      #if DEBUG_PROC
      Serial.printf("\nDEBUG: Local IP Address: %s", IPAddress(WiFi.localIP()).toString().c_str());
      Serial.printf("\nDEBUG: Subnet IP Address: %s", IPAddress(WiFi.subnetMask()).toString().c_str());
      Serial.printf("\nDEBUG: Gataway IP Address: %s", IPAddress(WiFi.gatewayIP()).toString().c_str());
      Serial.printf("\nDEBUG: Station RSSI: %d dBm", WiFi.RSSI());
      Serial.printf("\nDEBUG: Station SSID: %s", WiFi.SSID().c_str());
      #endif
      WaitWiFiConnection();

    #if STATIC_DNS == Google
        IPAddress _DNS1(8,8,8,8);
        IPAddress _DNS2(8,8,4,4);
        #if DEBUG_PROC
        Serial.printf("\nDEBUG: Setting DNS Configuration to 'Google'");
        #endif
    #elif STATIC_DNS == Cloudflare-Filter
        IPAddress _DNS1(1,1,1,2);
        IPAddress _DNS2(1,0,0,2);
        #if DEBUG_PROC
        Serial.printf("\nDEBUG: Setting DNS Configuration to 'Cloudflare with filters'");
        #endif
    #elif STATIC_DNS == Cloudflare-FilterAdult
        IPAddress _DNS1(1,1,1,3);
        IPAddress _DNS2(1,0,0,3);
        #if DEBUG_PROC
        Serial.printf("\nDEBUG: Setting DNS Configuration to 'Cloudflare with extra filters'");
        #endif
    #elif STATIC_DNS == Cloudflare
        IPAddress _DNS1(1,1,1,1);
        IPAddress _DNS2(1,0,0,1);
        #if DEBUG_PROC
        Serial.printf("\nDEBUG: Setting DNS Configuration to 'Cloudflare'");
        #endif
    #else
        //
        
    #endif

    /*
    dhcps_lease_t lease;
    lease.enable = true;
    lease.start_ip.addr = static_cast<uint32_t>(local_ip) + (1 << 24);
    lease.end_ip.addr = static_cast<uint32_t>(local_ip) + (n << 24);
    */
    auto& _DHCPserver = WiFi.softAPDhcpServer();
    ip_addr_t primaryDNS, secondaryDNS;
    primaryDNS = WiFi.dnsIP(0);
    secondaryDNS = WiFi.dnsIP(1);
     _DHCPserver.setDns(primaryDNS);
     dns_setserver(0, &primaryDNS);
     _DHCPserver.setDns(secondaryDNS);
     dns_setserver(1, &secondaryDNS);

    Serial.printf("\nStation DNS: %s & %s",
                  WiFi.dnsIP(0).toString().c_str(),
                  WiFi.dnsIP(1).toString().c_str());
                
    #if STATIC_DHCP_AP
    byte ip[4];
    parseBytes(localIPAddr.c_str(),'.', ip, 4, 10);
    IPAddress __localIP((uint8_t)ip[0], (uint8_t)ip[1], (uint8_t)ip[2], (uint8_t)ip[3]);
    parseBytes(GatewayAddr.c_str(),'.', ip, 4, 10);
    IPAddress __Gateway((uint8_t)ip[0], (uint8_t)ip[1], (uint8_t)ip[2], (uint8_t)ip[3]);
    parseBytes(SubnetAddr.c_str(),'.', ip, 4, 10);
    IPAddress __Subnet((uint8_t)ip[0], (uint8_t)ip[1], (uint8_t)ip[2], (uint8_t)ip[3]);

    Serial.printf("\nINFO: Setting Access point DHCP configuration ... %s", (WiFi.softAPConfig(__localIP, __Gateway, __Subnet) ? "Ready" : "Failed"));
    #endif
    Serial.printf("\nINFO: Setting Access point ... %s", (WiFi.softAP(ap, pass, WiFiChannel, WiFiHidden, MaxWiFiConnections) ? "Ready" : "Failed"));
        
    #if DEBUG_PROC
    Serial.printf("\nDEBUG: Heap before: %d", ESP.getFreeHeap());
    #endif
    err_t ret = ip_napt_init(NAPT, NAPT_PORT);
    #if DEBUG_PROC
    Serial.printf("\nDEBUG: ip_napt_init(%d,%d): ret=%d (OK=%d)", NAPT, NAPT_PORT, (int)ret, (int)ERR_OK);
    #endif
    if (ret == ERR_OK) {
        ret = ip_napt_enable_no(SOFTAP_IF, 1);
        #if DEBUG_PROC
        Serial.printf("\nDEBUG: ip_napt_enable_no(SOFTAP_IF): ret=%d (OK=%d)", (int)ret, (int)ERR_OK);
        #endif
        if (ret == ERR_OK) {
              Serial.printf("\nINFO: Successfully NATed to WiFi Network '%s' with the same password", ssid.c_str());
              #if BUZZER_ENABLED
              InitBuzzer(1000,false, 2);
              TriggerBuzzer();
              #endif
         }
    }
    else {
        Serial.print("\nERROR: NAPT initialization failed");
        #if BUZZER_ENABLED
        InitBuzzer(1000, false, 5);
        TriggerBuzzer();
        #endif
        delay_time = 2500;
        RepeaterIsWorking = true;
        return;
    }
    #if DEBUG_PROC
    Serial.printf("\nDEBUG: Heap after napt init: %d", ESP.getFreeHeap());
    #endif
    delay_time = 500; // blink every half second if connection was succesfull
  }
  RepeaterIsWorking = true;
}

#else

void setup() {
    ESP.wdtFeed();
     InitSerial();
    
    Serial.printf("\n\nNAPT not supported in this configuration\n");
    RepeaterIsWorking= false;
    digitalWrite(LED_BUILTIN, HIGH);
}
        
#endif

void loop() {
    #if DEBUG_PROC
    if (lastStatus != WiFi.status() && lastStatus != 0) {
        Serial.printf("\nDEBUG: OLD WiFi status: %d", lastStatus);
        lastStatus = WiFi.status();
        Serial.printf("\nDEBUG: NEW WiFi status: %d", lastStatus);
    }
    #endif
    if (WiFi.isConnected()) {
        if(clientCount != wifi_softap_get_station_num())
        {
            clientCount = wifi_softap_get_station_num();
            Serial.printf(" \nTotal connected clients are %d ", clientCount);
            clientStatus();
        }
        if (millis() > (lastNetCheck + WiFi_CONNECTION_WAIT)) {
            #if DEBUG_PROC
            Serial.print("\nDEBUG: Checking for Internet Connection...");
            #endif
            if (CheckAvailability("google.com")) {
                lastNetCheck = millis();
                #if DEBUG_PROC
                Serial.print(" OK");
                #endif
            }
            else {
                #if BUZZER_ENABLED
                InitBuzzer(1000, false, 5);
                TriggerBuzzer();
                #endif
                Serial.print("\nERROR: Failed to check for Internet connection.");
                lastNetCheck = (millis() - (WiFi_CONNECTION_WAIT + WiFi_CONNECTION_DELAY));
             }
        }
    }
    #if WIFI_AUTORECONNECT
    else {
        #if BUZZER_ENABLED
        InitBuzzer(1000, false, 3);
        TriggerBuzzer();
        #endif
        if (WiFi.status() != WL_IDLE_STATUS) {
            if (millis() > (lastConnectTry + WIFI_RECONNECT_WAIT)) {
                Serial.println("Reconnecting to WiFi...");
                WiFi.reconnect();
                lastConnectTry = millis();
            }
            WaitWiFiConnection();
        }
    }
    #endif
    if(digitalRead(0)==LOW){
        LittleFS.format();
        ESP.restart();
    }

    while(RepeaterIsWorking){
        unsigned long currentMillis = millis();

        if (currentMillis - previousMillis >= delay_time) {
            // save the last time you blinked the LED
            previousMillis = currentMillis;

            // if the LED is off turn it on and vice-versa:
            if (ledState == LOW)
                ledState = HIGH;
            else
                ledState = LOW;

            // set the LED with the ledState of the variable:
            digitalWrite(LED_BUILTIN, ledState);
      }
      break;
    }
}
