#ifndef CONFIG_H
#define CONFIG_H

#define DEBUG_PROC true

#define WEBsrv_PORT 4500

#define STARTUP_DELAY 1000
#define WAIT_TIMEOUT 120
#define BAUD_RATE 115200
#define SERIAL_TIMEOUT (10000UL)  // Maximum time to wait for serial activity to start

#define DefaultWiFi "The Box @ Beast"
#define WiFiChannel 10
#define MaxWiFiConnections 7
#define WiFiHidden true
#define WIFI_AUTOCONNECT true
#define WIFI_AUTORECONNECT true
#define WIFI_RECONNECT_TIMER 500
#define WIFI_STA_HOSTNAME "Beast's AP"
#define WiFi_CONNECTION_WAIT 10000
#define WiFi_CONNECTION_DELAY 1500

#define STATIC_DHCP_AP false
#define STATIC_DNS Google

#define BUZZER_PIN 13 //GPIO 13 = D7
#define BUZZER_ENABLED true

#define CONFIG_FILE "/config.json"

#define HAVE_NETDUMP 0
#define LWIP_DHCP_AP 0

#define NAPT 1000
#define NAPT_PORT 10

#endif
