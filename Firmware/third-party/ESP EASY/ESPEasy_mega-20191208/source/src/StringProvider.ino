#include "StringProviderTypes.h"

String getInternalLabel(LabelType::Enum label, char replaceSpace) {
  return to_internal_string(getLabel(label), replaceSpace);
}

String getLabel(LabelType::Enum label) {
  switch (label)
  {
    case LabelType::UNIT_NR:                return F("Unit Number");
    case LabelType::UNIT_NAME:              return F("Unit Name");
    case LabelType::HOST_NAME:              return F("Hostname");

    case LabelType::LOCAL_TIME:             return F("Local Time");
    case LabelType::UPTIME:                 return F("Uptime");
    case LabelType::LOAD_PCT:               return F("Load");
    case LabelType::LOOP_COUNT:             return F("Load LC");
    case LabelType::CPU_ECO_MODE:           return F("CPU Eco Mode");

    case LabelType::FREE_MEM:               return F("Free RAM");
    case LabelType::FREE_STACK:             return F("Free Stack");
#ifdef CORE_POST_2_5_0
    case LabelType::HEAP_MAX_FREE_BLOCK:    return F("Heap Max Free Block");
    case LabelType::HEAP_FRAGMENTATION:     return F("Heap Fragmentation");
#endif

    case LabelType::BOOT_TYPE:              return F("Last Boot Cause");
    case LabelType::BOOT_COUNT:             return F("Boot Count");
    case LabelType::RESET_REASON:           return F("Reset Reason");
    case LabelType::LAST_TASK_BEFORE_REBOOT: return F("Last Task");
    case LabelType::SW_WD_COUNT:            return F("SW WD count");


    case LabelType::WIFI_CONNECTION:        return F("WiFi Connection");
    case LabelType::WIFI_RSSI:              return F("RSSI");
    case LabelType::IP_CONFIG:              return F("IP Config");
    case LabelType::IP_CONFIG_STATIC:       return F("Static");
    case LabelType::IP_CONFIG_DYNAMIC:      return F("DHCP");
    case LabelType::IP_ADDRESS:             return F("IP Address");
    case LabelType::IP_SUBNET:              return F("IP Subnet");
    case LabelType::IP_ADDRESS_SUBNET:      return F("IP / Subnet");
    case LabelType::GATEWAY:                return F("Gateway");
    case LabelType::CLIENT_IP:              return F("Client IP");
    #ifdef FEATURE_MDNS
    case LabelType::M_DNS:                  return F("mDNS");
    #endif
    case LabelType::DNS:                    return F("DNS");
    case LabelType::DNS_1:                  return F("DNS 1");
    case LabelType::DNS_2:                  return F("DNS 2");
    case LabelType::ALLOWED_IP_RANGE:       return F("Allowed IP Range");
    case LabelType::STA_MAC:                return F("STA MAC");
    case LabelType::AP_MAC:                 return F("AP MAC");
    case LabelType::SSID:                   return F("SSID");
    case LabelType::BSSID:                  return F("BSSID");
    case LabelType::CHANNEL:                return F("Channel");
    case LabelType::CONNECTED:              return F("Connected");
    case LabelType::CONNECTED_MSEC:         return F("Connected msec");
    case LabelType::LAST_DISCONNECT_REASON: return F("Last Disconnect Reason");
    case LabelType::LAST_DISC_REASON_STR:   return F("Last Disconnect Reason str");
    case LabelType::NUMBER_RECONNECTS:      return F("Number Reconnects");

    case LabelType::FORCE_WIFI_BG:          return F("Force WiFi B/G");
    case LabelType::RESTART_WIFI_LOST_CONN: return F("Restart WiFi Lost Conn");
    case LabelType::FORCE_WIFI_NOSLEEP:     return F("Force WiFi No Sleep");
    case LabelType::PERIODICAL_GRAT_ARP:    return F("Periodical send Gratuitous ARP");
    case LabelType::CONNECTION_FAIL_THRESH: return F("Connection Failure Threshold");

    case LabelType::BUILD_DESC:             return F("Build");
    case LabelType::GIT_BUILD:              return F("Git Build");
    case LabelType::SYSTEM_LIBRARIES:       return F("System Libraries");
    case LabelType::PLUGINS:                return F("Plugins");
    case LabelType::PLUGIN_DESCRIPTION:     return F("Plugin description");
    case LabelType::BUILD_TIME:             return F("Build Time");
    case LabelType::BINARY_FILENAME:        return F("Binary Filename");

    case LabelType::SYSLOG_LOG_LEVEL:       return F("Syslog Log Level");
    case LabelType::SERIAL_LOG_LEVEL:       return F("Serial Log Level");
    case LabelType::WEB_LOG_LEVEL:          return F("Web Log Level");
  #ifdef FEATURE_SD
    case LabelType::SD_LOG_LEVEL:           return F("SD Log Level");
  #endif

    case LabelType::ESP_CHIP_ID:            return F("ESP Chip ID");
    case LabelType::ESP_CHIP_FREQ:          return F("ESP Chip Frequency");
    case LabelType::ESP_BOARD_NAME:         return F("ESP Board Name");

    case LabelType::FLASH_CHIP_ID:          return F("Flash Chip ID");
    case LabelType::FLASH_CHIP_REAL_SIZE:   return F("Flash Chip Real Size");
    case LabelType::FLASH_IDE_SIZE:         return F("Flash IDE Size");
    case LabelType::FLASH_IDE_SPEED:        return F("Flash IDE Speed");
    case LabelType::FLASH_IDE_MODE:         return F("Flash IDE Mode");
    case LabelType::FLASH_WRITE_COUNT:      return F("Flash Writes");
    case LabelType::SKETCH_SIZE:            return F("Sketch Size");
    case LabelType::SKETCH_FREE:            return F("Sketch Free");
    case LabelType::SPIFFS_SIZE:            return F("SPIFFS Size");
    case LabelType::SPIFFS_FREE:            return F("SPIFFS Free");
    case LabelType::MAX_OTA_SKETCH_SIZE:    return F("Max. OTA Sketch Size");
    case LabelType::OTA_2STEP:              return F("OTA 2-step Needed");
    case LabelType::OTA_POSSIBLE:           return F("OTA possible");

  }
  return F("MissingString");
}



String getValue(LabelType::Enum label) {
  switch (label)
  {
    case LabelType::UNIT_NR:                return String(Settings.Unit);
    case LabelType::UNIT_NAME:              return String(Settings.Name);
    case LabelType::HOST_NAME:
    #ifdef ESP32
      return WiFi.getHostname();
    #else
      return WiFi.hostname();
    #endif

    case LabelType::LOCAL_TIME:             return getDateTimeString('-',':',' ');
    case LabelType::UPTIME:                 return String(wdcounter / 2);
    case LabelType::LOAD_PCT:               return String(getCPUload());
    case LabelType::LOOP_COUNT:             return String(getLoopCountPerSec());
    case LabelType::CPU_ECO_MODE:           return jsonBool(Settings.EcoPowerMode());

    case LabelType::FREE_MEM:               return String(ESP.getFreeHeap());
    case LabelType::FREE_STACK:             break;
#ifdef CORE_POST_2_5_0
    case LabelType::HEAP_MAX_FREE_BLOCK:    return String(ESP.getMaxFreeBlockSize());
    case LabelType::HEAP_FRAGMENTATION:     return String(ESP.getHeapFragmentation());
#endif

    case LabelType::BOOT_TYPE:              return getLastBootCauseString();
    case LabelType::BOOT_COUNT:             break;
    case LabelType::RESET_REASON:           return getResetReasonString();
    case LabelType::LAST_TASK_BEFORE_REBOOT: return decodeSchedulerId(lastMixedSchedulerId_beforereboot);
    case LabelType::SW_WD_COUNT:            return String(sw_watchdog_callback_count);

    case LabelType::WIFI_CONNECTION:        break;
    case LabelType::WIFI_RSSI:              return String(WiFi.RSSI());
    case LabelType::IP_CONFIG:              return useStaticIP() ? getLabel(LabelType::IP_CONFIG_STATIC) : getLabel(LabelType::IP_CONFIG_DYNAMIC);
    case LabelType::IP_CONFIG_STATIC:       break;
    case LabelType::IP_CONFIG_DYNAMIC:      break;
    case LabelType::IP_ADDRESS:             return WiFi.localIP().toString();
    case LabelType::IP_SUBNET:              return WiFi.subnetMask().toString();
    case LabelType::IP_ADDRESS_SUBNET:      return String(getValue(LabelType::IP_ADDRESS) + F(" / ") + getValue(LabelType::IP_SUBNET));
    case LabelType::GATEWAY:                return WiFi.gatewayIP().toString();
    case LabelType::CLIENT_IP:              return formatIP(WebServer.client().remoteIP());
    #ifdef FEATURE_MDNS
    case LabelType::M_DNS:                  return String(WifiGetHostname()) + F(".local");
    #endif
    case LabelType::DNS:                    return String(getValue(LabelType::DNS_1) + F(" / ") + getValue(LabelType::DNS_2));
    case LabelType::DNS_1:                  return WiFi.dnsIP(0).toString();
    case LabelType::DNS_2:                  return WiFi.dnsIP(1).toString();
    case LabelType::ALLOWED_IP_RANGE:       return describeAllowedIPrange();
    case LabelType::STA_MAC:                return WiFi.macAddress();
    case LabelType::AP_MAC:                 break;
    case LabelType::SSID:                   return WiFi.SSID();
    case LabelType::BSSID:                  return WiFi.BSSIDstr();
    case LabelType::CHANNEL:                return String(WiFi.channel());
    case LabelType::CONNECTED:              return format_msec_duration(timeDiff(lastConnectMoment, millis()));
    case LabelType::CONNECTED_MSEC:         return String(timeDiff(lastConnectMoment, millis()));
    case LabelType::LAST_DISCONNECT_REASON: return String(lastDisconnectReason);
    case LabelType::LAST_DISC_REASON_STR:   return getLastDisconnectReason();
    case LabelType::NUMBER_RECONNECTS:      return String(wifi_reconnects);

    case LabelType::FORCE_WIFI_BG:          return jsonBool(Settings.ForceWiFi_bg_mode());
    case LabelType::RESTART_WIFI_LOST_CONN: return jsonBool(Settings.WiFiRestart_connection_lost());
    case LabelType::FORCE_WIFI_NOSLEEP:     return jsonBool(Settings.WifiNoneSleep());
    case LabelType::PERIODICAL_GRAT_ARP:    return jsonBool(Settings.gratuitousARP());
    case LabelType::CONNECTION_FAIL_THRESH: return String(Settings.ConnectionFailuresThreshold);

    case LabelType::BUILD_DESC:             return String(BUILD);
    case LabelType::GIT_BUILD:              return String(F(BUILD_GIT));
    case LabelType::SYSTEM_LIBRARIES:       return getSystemLibraryString();
    case LabelType::PLUGINS:                return String(deviceCount + 1);
    case LabelType::PLUGIN_DESCRIPTION:     return getPluginDescriptionString();
    case LabelType::BUILD_TIME:             break;
    case LabelType::BINARY_FILENAME:        break;

    case LabelType::SYSLOG_LOG_LEVEL:       return getLogLevelDisplayString(Settings.SyslogLevel);
    case LabelType::SERIAL_LOG_LEVEL:       return getLogLevelDisplayString(getSerialLogLevel());
    case LabelType::WEB_LOG_LEVEL:          return getLogLevelDisplayString(getWebLogLevel());
  #ifdef FEATURE_SD
    case LabelType::SD_LOG_LEVEL:           return getLogLevelDisplayString(Settings.SDLogLevel);
  #endif

    case LabelType::ESP_CHIP_ID:            break;
    case LabelType::ESP_CHIP_FREQ:          break;
    case LabelType::ESP_BOARD_NAME:         break;

    case LabelType::FLASH_CHIP_ID:          break;
    case LabelType::FLASH_CHIP_REAL_SIZE:   break;
    case LabelType::FLASH_IDE_SIZE:         break;
    case LabelType::FLASH_IDE_SPEED:        break;
    case LabelType::FLASH_IDE_MODE:         break;
    case LabelType::FLASH_WRITE_COUNT:      break;
    case LabelType::SKETCH_SIZE:            break;
    case LabelType::SKETCH_FREE:            break;
    case LabelType::SPIFFS_SIZE:            break;
    case LabelType::SPIFFS_FREE:            break;
    case LabelType::MAX_OTA_SKETCH_SIZE:    break;
    case LabelType::OTA_2STEP:              break;
    case LabelType::OTA_POSSIBLE:           break;

  }
  return F("MissingString");
}

String getExtendedValue(LabelType::Enum label) {
  switch (label)
  {
    case LabelType::UPTIME:
    {
      String result;
      result.reserve(40);
      int minutes = wdcounter / 2;
      int days = minutes / 1440;
      minutes = minutes % 1440;
      int hrs = minutes / 60;
      minutes = minutes % 60;

      result += days;
      result += F(" days ");
      result += hrs;
      result += F(" hours ");
      result += minutes;
      result += F(" minutes");
      return result;
    }

    default:
    break;
  }
  return "";
}


String getFileName(FileType::Enum filetype) {
  String result;
  switch (filetype) 
  {
    case FileType::CONFIG_DAT: 
      result += F("config.dat");
      break;
    case FileType::NOTIFICATION_DAT:
      result += F("notification.dat");
      break;
    case FileType::SECURITY_DAT:
      result += F("security.dat");
      break;
    case FileType::RULES_TXT:
      // Use getRulesFileName     
      break;
  }
  return result;
}

String getFileName(FileType::Enum filetype, unsigned int filenr) {
  if (filetype == FileType::RULES_TXT) {
    return getRulesFileName(filenr);
  }
  return getFileName(filetype);
}

// filenr = 0...3 for files rules1.txt ... rules4.txt
String getRulesFileName(unsigned int filenr) {
  String result;
  if (filenr < 4) {
    result += F("rules");
    result += filenr + 1;
    result += F(".txt");
  }
  return result;
}