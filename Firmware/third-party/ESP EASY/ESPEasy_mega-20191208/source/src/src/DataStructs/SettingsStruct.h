
#ifndef DATASTRUCTS_SETTINGSSTRUCT_H
#define DATASTRUCTS_SETTINGSSTRUCT_H


#include "../DataStructs/ESPEasyLimits.h"
#include "../Globals/Plugins.h"


/*********************************************************************************************\
 * SettingsStruct
\*********************************************************************************************/
template <unsigned int N_TASKS>
class SettingsStruct_tmpl
{
  public:
  SettingsStruct_tmpl();

  // VariousBits1 defaults to 0, keep in mind when adding bit lookups.
  bool appendUnitToHostname();
  void appendUnitToHostname(bool value);

  bool uniqueMQTTclientIdReconnect();
  void uniqueMQTTclientIdReconnect(bool value);

  bool OldRulesEngine();
  void OldRulesEngine(bool value);

  bool ForceWiFi_bg_mode();
  void ForceWiFi_bg_mode(bool value);

  bool WiFiRestart_connection_lost();
  void WiFiRestart_connection_lost(bool value);

  bool EcoPowerMode();
  void EcoPowerMode(bool value);

  bool WifiNoneSleep();
  void WifiNoneSleep(bool value);

  // Enable send gratuitous ARP by default, so invert the values (default = 0)
  bool gratuitousARP();
  void gratuitousARP(bool value);

  // Be a bit more tolerant when parsing the last argument of a command.
  // See: https://github.com/letscontrolit/ESPEasy/issues/2724
  bool TolerantLastArgParse();
  void TolerantLastArgParse(bool value);

  void validate();

  bool networkSettingsEmpty() const;

  void clearNetworkSettings();

  void clearTimeSettings();

  void clearNotifications();

  void clearControllers();

  void clearTasks();

  void clearLogSettings();

  void clearUnitNameSettings();

  void clearMisc();

  void clearAll();

  void clearTask(taskIndex_t task);

  unsigned long PID;
  int           Version;
  int16_t       Build;
  byte          IP[4];
  byte          Gateway[4];
  byte          Subnet[4];
  byte          DNS[4];
  byte          IP_Octet;
  byte          Unit;
  char          Name[26];
  char          NTPHost[64];
  // FIXME TD-er: Issue #2690
  unsigned long Delay;              // Sleep time in seconds
  int8_t        Pin_i2c_sda;
  int8_t        Pin_i2c_scl;
  int8_t        Pin_status_led;
  int8_t        Pin_sd_cs;
  int8_t        PinBootStates[17];  // FIXME TD-er: this is ESP8266 number of pins. ESP32 has double.
  byte          Syslog_IP[4];
  unsigned int  UDPPort;
  byte          SyslogLevel;
  byte          SerialLogLevel;
  byte          WebLogLevel;
  byte          SDLogLevel;
  unsigned long BaudRate;
  unsigned long MessageDelay;
  byte          deepSleep_wakeTime;   // 0 = Sleep Disabled, else time awake from sleep in seconds
  boolean       CustomCSS;
  boolean       DST;
  byte          WDI2CAddress;
  boolean       UseRules;
  boolean       UseSerial;
  boolean       UseSSDP;
  boolean       UseNTP;
  unsigned long WireClockStretchLimit;
  boolean       GlobalSync;
  unsigned long ConnectionFailuresThreshold;
  int16_t       TimeZone;
  boolean       MQTTRetainFlag;
  boolean       InitSPI;
  byte          Protocol[CONTROLLER_MAX];
  byte          Notification[NOTIFICATION_MAX]; //notifications, point to a NPLUGIN id
  // FIXME TD-er: Must change to pluginID_t, but then also another check must be added since changing the pluginID_t will also render settings incompatible
  byte          TaskDeviceNumber[N_TASKS]; // The "plugin number" set at as task (e.g. 4 for P004_dallas)
  unsigned int  OLD_TaskDeviceID[N_TASKS];  //UNUSED: this can be removed
  union {
    struct {
      int8_t        TaskDevicePin1[N_TASKS];
      int8_t        TaskDevicePin2[N_TASKS];
      int8_t        TaskDevicePin3[N_TASKS];
      byte          TaskDevicePort[N_TASKS];
    };
    int8_t        TaskDevicePin[4][N_TASKS];
  };
  boolean       TaskDevicePin1PullUp[N_TASKS];
  int16_t       TaskDevicePluginConfig[N_TASKS][PLUGIN_CONFIGVAR_MAX];
  boolean       TaskDevicePin1Inversed[N_TASKS];
  float         TaskDevicePluginConfigFloat[N_TASKS][PLUGIN_CONFIGFLOATVAR_MAX];
  long          TaskDevicePluginConfigLong[N_TASKS][PLUGIN_CONFIGLONGVAR_MAX];
  boolean       OLD_TaskDeviceSendData[N_TASKS];
  boolean       TaskDeviceGlobalSync[N_TASKS];
  byte          TaskDeviceDataFeed[N_TASKS];    // When set to 0, only read local connected sensorsfeeds
  unsigned long TaskDeviceTimer[N_TASKS];
  boolean       TaskDeviceEnabled[N_TASKS];
  boolean       ControllerEnabled[CONTROLLER_MAX];
  boolean       NotificationEnabled[NOTIFICATION_MAX];
  unsigned int  TaskDeviceID[CONTROLLER_MAX][N_TASKS];        // IDX number (mainly used by Domoticz)
  boolean       TaskDeviceSendData[CONTROLLER_MAX][N_TASKS];
  boolean       Pin_status_led_Inversed;
  boolean       deepSleepOnFail;
  boolean       UseValueLogger;
  boolean       ArduinoOTAEnable;
  uint16_t      DST_Start;
  uint16_t      DST_End;
  boolean       UseRTOSMultitasking;
  int8_t        Pin_Reset;
  byte          SyslogFacility;
  uint32_t      StructSize;  // Forced to be 32 bit, to make sure alignment is clear.
  boolean       MQTTUseUnitNameAsClientId;

  //its safe to extend this struct, up to several bytes, default values in config are 0
  //look in misc.ino how config.dat is used because also other stuff is stored in it at different offsets.
  //TODO: document config.dat somewhere here
  float         Latitude;
  float         Longitude;
  uint32_t      VariousBits1;
  uint32_t      ResetFactoryDefaultPreference; // Do not clear this one in the clearAll()

  // FIXME @TD-er: As discussed in #1292, the CRC for the settings is now disabled.
  // make sure crc is the last value in the struct
  // Try to extend settings to make the checksum 4-byte aligned.
//  uint8_t       ProgmemMd5[16]; // crc of the binary that last saved the struct to file.
//  uint8_t       md5[16];
};

/*
SettingsStruct* SettingsStruct_ptr = new SettingsStruct;
SettingsStruct& Settings = *SettingsStruct_ptr;
*/



typedef SettingsStruct_tmpl<TASKS_MAX> SettingsStruct;

#endif // DATASTRUCTS_SETTINGSSTRUCT_H
