#include "ESPEasyWiFiEvent.h"
#include "src/Globals/ESPEasyWiFiEvent.h"
#include "src/Globals/RTC.h"
#include "ESPEasyTimeTypes.h"

#include "src/DataStructs/RTCStruct.h"

#ifdef ESP32
void WiFi_Access_Static_IP::set_use_static_ip(bool enabled) {
  _useStaticIp = enabled;
}

#endif // ifdef ESP32
#ifdef ESP8266
void WiFi_Access_Static_IP::set_use_static_ip(bool enabled) {
  _useStaticIp = enabled;
}

#endif // ifdef ESP8266


void setUseStaticIP(bool enabled) {
  WiFi_Access_Static_IP tmp_wifi;

  tmp_wifi.set_use_static_ip(enabled);
}

void markGotIP() {
  lastGetIPmoment = millis();
  wifiStatus      |= ESPEASY_WIFI_GOT_IP;
  processedGotIP = false;
}

// ********************************************************************************
// Functions called on events.
// Make sure not to call anything in these functions that result in delay() or yield()
// ********************************************************************************
#ifdef ESP32
#include <WiFi.h>

static bool ignoreDisconnectEvent = false;

void WiFiEvent(system_event_id_t event, system_event_info_t info) {
  switch (event) {
    case SYSTEM_EVENT_STA_CONNECTED:
    {
      RTC.lastWiFiChannel = info.connected.channel;
      for (byte i = 0; i < 6; ++i) {
        if (RTC.lastBSSID[i] != info.connected.bssid[i]) {
          bssid_changed = true;
          RTC.lastBSSID[i]  = info.connected.bssid[i];
        }
      }
      char ssid_copy[33] = { 0 }; // Ensure space for maximum len SSID (32) plus trailing 0
      memcpy(ssid_copy, info.connected.ssid, info.connected.ssid_len);
      ssid_copy[32] = 0; // Potentially add 0-termination if none present earlier
      last_ssid = (const char*) ssid_copy;
      lastConnectMoment = millis();
      processedConnect  = false;
      wifiStatus       |= ESPEASY_WIFI_CONNECTED;
      break;
    }
    case SYSTEM_EVENT_STA_DISCONNECTED:
      if (!ignoreDisconnectEvent) {
        ignoreDisconnectEvent = true;
        lastDisconnectMoment = millis();
        WiFi.persistent(false);
        WiFi.disconnect(true);

        if (timeDiff(lastConnectMoment, last_wifi_connect_attempt_moment) > 0) {
          // There was an unsuccessful connection attempt
          lastConnectedDuration = timeDiff(last_wifi_connect_attempt_moment, lastDisconnectMoment);
        } else {
          lastConnectedDuration = timeDiff(lastConnectMoment, lastDisconnectMoment);
        }
        processedDisconnect  = false;
        lastDisconnectReason = static_cast<WiFiDisconnectReason>(info.disconnected.reason);
        wifiStatus          |= ESPEASY_WIFI_DISCONNECTED;
      }
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      ignoreDisconnectEvent = false;
      markGotIP();
      break;
    case SYSTEM_EVENT_AP_STACONNECTED:

      for (byte i = 0; i < 6; ++i) {
        lastMacConnectedAPmode[i] = info.sta_connected.mac[i];
      }
      processedConnectAPmode = false;
      break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:

      for (byte i = 0; i < 6; ++i) {
        lastMacConnectedAPmode[i] = info.sta_disconnected.mac[i];
      }
      processedDisconnectAPmode = false;
      break;
    case SYSTEM_EVENT_SCAN_DONE:
      processedScanDone = false;
      break;
    default:
      break;
  }
}

#endif // ifdef ESP32

#ifdef ESP8266

void onConnected(const WiFiEventStationModeConnected& event) {
  lastConnectMoment = millis();
  processedConnect  = false;
  wifiStatus       |= ESPEASY_WIFI_CONNECTED;
  channel_changed   = RTC.lastWiFiChannel != event.channel;
  RTC.lastWiFiChannel      = event.channel;
  last_ssid         = event.ssid;
  bssid_changed     = false;

  for (byte i = 0; i < 6; ++i) {
    if (RTC.lastBSSID[i] != event.bssid[i]) {
      bssid_changed = true;
      RTC.lastBSSID[i]  = event.bssid[i];
    }
  }
}

void onDisconnect(const WiFiEventStationModeDisconnected& event) {
  lastDisconnectMoment = millis();

  if (timeDiff(lastConnectMoment, last_wifi_connect_attempt_moment) > 0) {
    // There was an unsuccessful connection attempt
    lastConnectedDuration = timeDiff(last_wifi_connect_attempt_moment, lastDisconnectMoment);
  } else {
    lastConnectedDuration = timeDiff(lastConnectMoment, lastDisconnectMoment);
  }
  lastDisconnectReason = event.reason;
  wifiStatus           = ESPEASY_WIFI_DISCONNECTED;

  if (WiFi.status() == WL_CONNECTED) {
    // See https://github.com/esp8266/Arduino/issues/5912
    WiFi.persistent(false);
    WiFi.disconnect(true);
  }
  processedDisconnect = false;
}

void onGotIP(const WiFiEventStationModeGotIP& event) {
  markGotIP();
}

void ICACHE_RAM_ATTR onDHCPTimeout() {
  processedDHCPTimeout = false;
}

void onConnectedAPmode(const WiFiEventSoftAPModeStationConnected& event) {
  for (byte i = 0; i < 6; ++i) {
    lastMacConnectedAPmode[i] = event.mac[i];
  }
  processedConnectAPmode = false;
}

void onDisonnectedAPmode(const WiFiEventSoftAPModeStationDisconnected& event) {
  for (byte i = 0; i < 6; ++i) {
    lastMacDisconnectedAPmode[i] = event.mac[i];
  }
  processedDisconnectAPmode = false;
}

#endif // ifdef ESP8266
