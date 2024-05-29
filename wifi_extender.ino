
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

/* Set these to your desired credentials. */

#if LWIP_FEATURES && !LWIP_IPV6

#define HAVE_NETDUMP 0
#include <lwip/napt.h>
#include <lwip/dns.h>

#define LWIP_DHCP_AP 0
#if LWIP_DHCP_AP
#include <LwipDhcpServer.h>
#endif

#define NAPT 1000
#define NAPT_PORT 10

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
        Serial.printf("\nNOTICE: Detected baudrate is %lu, switching to that baudrate now...\n", detectedBaudrate);

        // Wait for printf to finish
        while (Serial.availableForWrite() != UART_TX_FIFO_SIZE)
            yield();

        // Clear Tx buffer to avoid extra characters being printed
        Serial.flush();

        // After this, any writing to Serial will print gibberish on the serial monitor if the baudrate doesn't match
        Serial.begin(detectedBaudrate);
    }
    Serial.setDebugOutput(DEBUG_PROC);
}

void TriggerBuzzer(int _delay, bool _infinite, int _count);
void TriggerBuzzer(int _delay, bool _infinite = false, int _count = 3) {
    #if DEBUG_PROC
    Serial.println("DEBUG: Buzzer is triggered!");
    #endif
    int __count = 0;
    while (true) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(_delay);
        digitalWrite(BUZZER_PIN, LOW);
        __count += 1;
        if(_infinite != true && __count >= _count)
            break;
    }
}

void setup() {
    delay(1000);
    pinMode(0,INPUT_PULLUP);
    pinMode(LED_BUILTIN,OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH); //active low
    InitSerial();
    
     if (WiFi.status() == WL_NO_SHIELD) {
        Serial.println("ERROR: WiFi shield not present");
        TriggerBuzzer(1000, true);
        delay_time = 4000;
        return;
     }      
     
     // ??
    Serial.println();

    if (!LittleFS.begin()) {
        Serial.println("ERROR: LittleFS mount failed");
        TriggerBuzzer(1000, true);
        delay_time = 4000;
        return;
    }

    Serial.printf("\n\nNAPT Range extender\n");
    #if DEBUG_PROC
    Serial.printf("DEBUG: Heap on start: %d\n", ESP.getFreeHeap());
    #endif

#if HAVE_NETDUMP
    phy_capture = dump;
#endif

    // first, connect to STA so we can get a proper local DNS server
    
   String ssid = my_wifi.get_credentials(0); // if the file does not exist the function will always return null
   String pass = my_wifi.get_credentials(1);
   String ap = my_wifi.get_credentials(2);

    if (ssid == "undefined") {
        // if the file does not exist, ssid will be undefined
    start_webserver:
        #if DEBUG_PROC
        uint8_t macAddress[6];
        WiFi.macAddress(macAddress);
        Serial.print("\nDEBUG: MAC Address: ");
        for (int i = 0; i < 6; ++i) {
            Serial.printf("%02X", macAddress[i]);
            if (i < 5)
                Serial.print(":");
        }
        #endif
        Serial.println();
        WiFi.softAP(DefaultWiFi);
        #if DEBUG_PROC
        IPAddress AccessPointIP = WiFi.softAPIP();
        Serial.printf("DEBUG: AP IP address: %s\n", AccessPointIP.toString().c_str());
        #endif
        my_wifi.create_server();
        //server.begin();
        my_wifi.begin_server();
        #if DEBUG_PROC
        Serial.printf("DEBUG: HTTP server started on port %d\n", WEBsrv_PORT);
        #endif
        delay_time=1000; // blink every sec if webserver is active
  }
  else if (ssid == "null") {
        // if the JSON parser failed, ssid will be null
        Serial.println("NOTICE: temporary reversing configurations to defaults...");
        goto start_webserver;
        delay_time=500; // blink every half second
  }
  else
  {
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, pass); // check function to understand
      #if DEBUG_PROC
      IPAddress StationIP = WiFi.localIP();
     Serial.printf("DEBUG: Station IP Address: %s", StationIP.toString().c_str());
      #endif
      int timeout_counter=0;
      Serial.print("\nINFO: Connecting");
      while (WiFi.status() != WL_CONNECTED) {
          if(timeout_counter>=WAIT_TIMEOUT)
              goto start_webserver; // if it fails to connect start_webserver

      Serial.print('.');
      timeout_counter++;
      digitalWrite(LED_BUILTIN, LOW);// leave led on when trying to connect
      delay(500);
    }

     IPAddress __DNS1(8,8,8,8);
     IPAddress __DNS2(8,8,4,4);
     //WiFi.setDNS(__DNS1, __DNS2);
    #if LWIP_DHCP_AP
    // give DNS servers to AP side
    dhcpSoftAP.dhcps_set_dns(0, WiFi.dnsIP(0));
    dhcpSoftAP.dhcps_set_dns(1, WiFi.dnsIP(1));
    #endif

    Serial.printf("\nStation DNS: %s & %s\n",
                  WiFi.dnsIP(0).toString().c_str(),
                  WiFi.dnsIP(1).toString().c_str());
                
    IPAddress __localIP(192,168,1,50);
    IPAddress __Gateway(192,168,1,1);
    IPAddress __Subnet(255,255,255,0);

    Serial.printf("INFO: Setting Access point configuration ... %s", (WiFi.softAPConfig(__localIP, __Gateway, __Subnet) ? "Ready" : "Failed"));
    Serial.printf("INFO: Setting Access point ... %s", (WiFi.softAP(ap, pass, WiFiChannel, WiFiHidden, MaxWiFiConnections) ? "Ready" : "Failed"));

       #if DEBUG_PROC
       Serial.printf("DEBUG: Heap before: %d\n", ESP.getFreeHeap());
       #endif
      err_t ret = ip_napt_init(NAPT, NAPT_PORT);
      #if DEBUG_PROC
      Serial.printf("DEBUG: ip_napt_init(%d,%d): ret=%d (OK=%d)\n", NAPT, NAPT_PORT, (int)ret, (int)ERR_OK);
      #endif
      if (ret == ERR_OK) {
          ret = ip_napt_enable_no(SOFTAP_IF, 1);
          #if DEBUG_PROC
          Serial.printf("DEBUG: ip_napt_enable_no(SOFTAP_IF): ret=%d (OK=%d)\n", (int)ret, (int)ERR_OK);
          #endif
          if (ret == ERR_OK) {
                Serial.printf("INFO: Successfully NATed to WiFi Network '%s' with the same password", ssid.c_str());
                TriggerBuzzer(1000, false, 2);
           }
      }
      #if DEBUG_PROC
      Serial.printf("DEBUG: Heap after napt init: %d\n", ESP.getFreeHeap());
      #endif
      if (ret != ERR_OK) {
          Serial.print("ERROR: NAPT initialization failed\n");
          TriggerBuzzer(1000, true);
          delay_time = 4000;
          return;
      }
      delay_time=500; // blink every half second if connection was succesfull
  }
  RepeaterIsWorking=true;
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
