/* 
 *  Author: Pius Onyema Ndukwu
 *  License: MIT License
 *  GitHub:https://github.com/Pius171/esp8266-wifi-extender
 *  
 */

#include "WM.h"
#include "config.h"
// variables
bool RepeaterIsWorking= true;
int ledState = LOW; 
unsigned long previousMillis = 0;
long delay_time=0; // interval between blinks
// blink every 200ms if connected to router
// blink every 1sec if web server is active
// led is off is there is an error with the repeater
//led is on when trying to connect to router.
int connectedStations = 0;

unsigned long lastConnectTry = 0;
#if DEBUG_PROC
uint8_t lastStatus = NULL;
#endif

/* Set these to your desired credentials. */

#if LWIP_FEATURES && !LWIP_IPV6

#include <lwip/napt.h>
#include <lwip/dns.h>

#if LWIP_DHCP_AP
#include <LwipDhcpServer.h>
#else
#include <dhcpserver.h>
#endif

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

WM my_wifi;

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
        Serial.printf("\nDEBUG: Serial is %d bps", detectedBaudrate);
        #endif
    }
    Serial.setDebugOutput(DEBUG_PROC);
}

#if BUZZER_ENABLED
void TriggerBuzzer(int _delay, bool _infinite, int _count);
void TriggerBuzzer(int _delay, bool _infinite = false, int _count = 3) {
    #if DEBUG_PROC
    Serial.println("\nDEBUG: Buzzer is triggered!");
    #endif
    int __count = 0;
    while (true) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(_delay);
        digitalWrite(BUZZER_PIN, LOW);
        delay(_delay);
        __count += 1;
        if(!_infinite && _count > 0 && __count >= _count)
            break;
    }
}
#endif

unsigned char clientCount = 0; 
void  clientStatus();
void clientStatus() {
    struct station_info *stat_info = wifi_softap_get_station_info();
    int clientID = 0;

    while (stat_info != NULL) {
        clientID++;
        Serial.printf("\nINFO: (client = %d)IP Address = %s with MAC Address is = ", clientID, IPAddress((&stat_info->ip)->addr).toString().c_str());
        for (int i = 0; i < 6; ++i) {
            Serial.printf("%02X", stat_info->bssid[i]);
            if (i < 5)
                Serial.print(":");
        }

        stat_info = STAILQ_NEXT(stat_info, next);
        Serial.println();
    }
    wifi_softap_free_station_info();
    delay(500);
}

void StartWebserver();
void StartWebserver() {
        WiFi.softAP(DefaultWiFi);
        #if DEBUG_PROC
        IPAddress AccessPointIP = WiFi.softAPIP();
        Serial.printf("\nDEBUG: AP IP address: %s", AccessPointIP.toString().c_str());
        #endif
        my_wifi.create_server();
        my_wifi.begin_server();
        #if DEBUG_PROC
        Serial.printf("\nDEBUG: HTTP server started on port %d", WEBsrv_PORT);
        #endif
        //delay_time = 1000; // blink every sec if webserver is active
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
                TriggerBuzzer(1000, false, 5);
                #endif
                 break;
            }
            case WL_CONNECT_FAILED: {
                 Serial.print("\nERROR: Failed to connect to WiFi!");
                 timeout_counter = WAIT_TIMEOUT;
                 #if BUZZER_ENABLED
                 TriggerBuzzer(1000, false, 5);
                 #endif
                 break;
            }
            case WL_CONNECTION_LOST: {
                 Serial.print("\nERROR: Lost Connection to WiFi!");
                 timeout_counter = WAIT_TIMEOUT;
                 #if BUZZER_ENABLED
                 TriggerBuzzer(1000, false, 5);
                 #endif
                 break;
            }
            case -1: {
                 Serial.print("\nERROR: Timeout on connecting to WiFi!");
                 timeout_counter = WAIT_TIMEOUT;
                 #if BUZZER_ENABLED
                 TriggerBuzzer(1000, false, 5);
                 #endif
                 break;
             }
          }
          if(timeout_counter>=WAIT_TIMEOUT) {
              Serial.print("\nERROR: Timeout on connecting to WiFi!");
              #if BUZZER_ENABLED
              TriggerBuzzer(2000, false, 5);
              #endif
              StartWebserver();
          }

          Serial.print('.');
          timeout_counter++;
          digitalWrite(LED_BUILTIN, LOW);
          delay(WIFI_RECONNECT_TIMER);
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

void setup() {
    #if STARTUP_DELAY >= 500
    delay(STARTUP_DELAY);
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
        TriggerBuzzer(1000, false, 5);
        #endif
        delay_time = 4000;
        return;
     }      
     
     // ??
    Serial.println();

    if (!LittleFS.begin()) {
        Serial.println("ERROR: LittleFS mount failed");
        #if BUZZER_ENABLED
        TriggerBuzzer(1000, false, 5);
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
    
   String ssid = my_wifi.get_credentials(0);
   String pass =my_wifi.get_credentials(1);
   String ap = my_wifi.get_credentials(2);

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
        delay_time=3000; // blink every half second
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

    #if LWIP_DHCP_AP
    // give DNS servers to AP side
    dhcpSoftAP.dhcps_set_dns(0, WiFi.dnsIP(0));
    dhcpSoftAP.dhcps_set_dns(1, WiFi.dnsIP(1));
    #else
    dhcps_set_dns(0, WiFi.dnsIP(0));
    dhcps_set_dns(1, WiFi.dnsIP(1));
    #endif
    // auto& _DHCPserver = WiFi.softAPDhcpServer();
     //_DHCPserver.setDns(WiFi.dnsIP(0), WiFi.dnsIP(1));

    Serial.printf("\nStation DNS: %s & %s",
                  WiFi.dnsIP(0).toString().c_str(),
                  WiFi.dnsIP(1).toString().c_str());
                
    #if STATIC_DHCP_AP
    IPAddress __localIP(192,168,4,2); // 172, 217, 28, 254
    IPAddress __Gateway(192,168,4,1); // 172, 217, 28, 254
    IPAddress __Subnet(255,255,255,0);

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
              TriggerBuzzer(1000,false, 2);
              #endif
         }
    }
    else {
        Serial.print("\nERROR: NAPT initialization failed");
        #if BUZZER_ENABLED
        TriggerBuzzer(1000, false, 5);
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
     InitSerial();
    
    Serial.printf("\n\nNAPT not supported in this configuration\n");
    RepeaterIsWorking= false;
    digitalWrite(LED_BUILTIN, HIGH); // led stays off
}
        
#endif
            
void loop() {
    #if DEBUG_PROC
    if (lastStatus != WiFi.status()) {
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
    }
    #if WIFI_AUTORECONNECT
    else {
        #if BUZZER_ENABLED
        TriggerBuzzer(1000, false, 3);
        #endif
        if (WiFi.status() != WL_IDLE_STATUS) {
            if (millis() > (lastConnectTry + 3000)) {
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
