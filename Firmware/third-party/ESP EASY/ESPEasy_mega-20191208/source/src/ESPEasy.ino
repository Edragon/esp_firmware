
#include <Arduino.h>

#ifdef CONTINUOUS_INTEGRATION
#pragma GCC diagnostic error "-Wall"
#else
#pragma GCC diagnostic warning "-Wall"
#endif

// Needed due to preprocessor issues.
#ifdef PLUGIN_SET_GENERIC_ESP32
  #ifndef ESP32
    #define ESP32
  #endif
#endif


/****************************************************************************************************************************\
 * Arduino project "ESP Easy" © Copyright www.letscontrolit.com
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 * You received a copy of the GNU General Public License along with this program in file 'License.txt'.
 *
 * IDE download    : https://www.arduino.cc/en/Main/Software
 * ESP8266 Package : https://github.com/esp8266/Arduino
 *
 * Source Code     : https://github.com/ESP8266nu/ESPEasy
 * Support         : http://www.letscontrolit.com
 * Discussion      : http://www.letscontrolit.com/forum/
 *
 * Additional information about licensing can be found at : http://www.gnu.org/licenses
\*************************************************************************************************************************/

// This file incorporates work covered by the following copyright and permission notice:

/****************************************************************************************************************************\
* Arduino project "Nodo" © Copyright 2010..2015 Paul Tonkes
*
* This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
* You received a copy of the GNU General Public License along with this program in file 'License.txt'.
*
* Voor toelichting op de licentievoorwaarden zie    : http://www.gnu.org/licenses
* Uitgebreide documentatie is te vinden op          : http://www.nodo-domotica.nl
* Compiler voor deze programmacode te downloaden op : http://arduino.cc
\*************************************************************************************************************************/

//   Simple Arduino sketch for ESP module, supporting:
//   =================================================================================
//   Simple switch inputs and direct GPIO output control to drive relays, mosfets, etc
//   Analog input (ESP-7/12 only)
//   Pulse counters
//   Dallas OneWire DS18b20 temperature sensors
//   DHT11/22/12 humidity sensors
//   BMP085 I2C Barometric Pressure sensor
//   PCF8591 4 port Analog to Digital converter (I2C)
//   RFID Wiegand-26 reader
//   MCP23017 I2C IO Expanders
//   BH1750 I2C Luminosity sensor
//   Arduino Pro Mini with IO extender sketch, connected through I2C
//   LCD I2C display 4x20 chars
//   HC-SR04 Ultrasonic distance sensor
//   SI7021 I2C temperature/humidity sensors
//   TSL2561 I2C Luminosity sensor
//   TSOP4838 IR receiver
//   PN532 RFID reader
//   Sharp GP2Y10 dust sensor
//   PCF8574 I2C IO Expanders
//   PCA9685 I2C 16 channel PWM driver
//   OLED I2C display with SSD1306 driver
//   MLX90614 I2C IR temperature sensor
//   ADS1115 I2C ADC
//   INA219 I2C voltage/current sensor
//   BME280 I2C temp/hum/baro sensor
//   MSP5611 I2C temp/baro sensor
//   BMP280 I2C Barometric Pressure sensor
//   SHT1X temperature/humidity sensors
//   Ser2Net server


// Define globals before plugin sets to allow a personal override of the selected plugins
#include "ESPEasy-Globals.h"
// Must be included after all the defines, since it is using TASKS_MAX
#include "_Plugin_Helper.h"
// Plugin helper needs the defined controller sets, thus include after 'define_plugin_sets.h'
#include "_CPlugin_Helper.h"
#include "src/ControllerQueue/DelayQueueElements.h"

#include "src/DataStructs/ControllerSettingsStruct.h"
#include "src/DataStructs/DeviceModel.h"
#include "src/DataStructs/ESPEasy_EventStruct.h"
#include "src/DataStructs/PortStatusStruct.h"
#include "src/DataStructs/ProtocolStruct.h"
#include "src/DataStructs/RTCStruct.h"
#include "src/DataStructs/SchedulerTimers.h"
#include "src/DataStructs/SettingsType.h"
#include "src/DataStructs/SystemTimerStruct.h"
#include "src/DataStructs/TimingStats.h"

#include "src/Globals/Device.h"
#include "src/Globals/ESPEasyWiFiEvent.h"
#include "src/Globals/ExtraTaskSettings.h"
#include "src/Globals/GlobalMapPortStatus.h"
#include "src/Globals/MQTT.h"
#include "src/Globals/Plugins.h"
#include "src/Globals/RTC.h"
#include "src/Globals/SecuritySettings.h"
#include "src/Globals/Services.h"
#include "src/Globals/Settings.h"
#include "src/Globals/Statistics.h"

#if FEATURE_ADC_VCC
ADC_MODE(ADC_VCC);
#endif


// FIXME TD-er: This must be moves to src/Globals/Services
// But right now, it seems hard to define WevServer in a .h/.cpp file
// error: 'WebServer' does not name a type
#ifdef ESP32
  #include <WiFi.h>
  #include <WebServer.h>
  WebServer WebServer(80);
#endif

// Get functions to give access to global defined variables.
// These are needed to get direct access to global defined variables, since they cannot be defined in .h files and included more than once.

float& getUserVar(unsigned int varIndex) {return UserVar[varIndex]; }


#ifdef USES_BLYNK
// Blynk_get prototype
boolean Blynk_get(const String& command, byte controllerIndex,float *data = NULL );

int firstEnabledBlynkController();
#endif

//void checkRAM( const __FlashStringHelper* flashString);

#ifdef CORE_POST_2_5_0
void preinit();
#endif

#ifdef CORE_POST_2_5_0
/*********************************************************************************************\
 * Pre-init
\*********************************************************************************************/
void preinit() {
  // Global WiFi constructors are not called yet
  // (global class instances like WiFi, Serial... are not yet initialized)..
  // No global object methods or C++ exceptions can be called in here!
  //The below is a static class method, which is similar to a function, so it's ok.
  ESP8266WiFiClass::preinitWiFiOff();
}
#endif

/*********************************************************************************************\
 * ISR call back function for handling the watchdog.
\*********************************************************************************************/
void sw_watchdog_callback(void *arg) 
{
  yield(); // feed the WD
  ++sw_watchdog_callback_count;
}




/*********************************************************************************************\
 * SETUP
\*********************************************************************************************/
void setup()
{
#ifdef ESP8266_DISABLE_EXTRA4K
  disable_extra4k_at_link_time();
#endif
  WiFi.persistent(false); // Do not use SDK storage of SSID/WPA parameters
  WiFi.setAutoReconnect(false);
  // The WiFi.disconnect() ensures that the WiFi is working correctly. If this is not done before receiving WiFi connections,
  // those WiFi connections will take a long time to make or sometimes will not work at all.
  WiFi.disconnect();
  setWifiMode(WIFI_OFF);
  
  run_compiletime_checks();
  lowestFreeStack = getFreeStackWatermark();
  lowestRAM = FreeMem();
#ifndef ESP32
//  ets_isr_attach(8, sw_watchdog_callback, NULL);  // Set a callback for feeding the watchdog.
#endif

  resetPluginTaskData();

  checkRAM(F("setup"));
  #if defined(ESP32)
    for(byte x = 0; x < 16; x++)
      ledChannelPin[x] = -1;
  #endif

  Serial.begin(115200);
  // serialPrint("\n\n\nBOOOTTT\n\n\n");

  initLog();

#if defined(ESP32)
  WiFi.onEvent(WiFiEvent);
#else
  // WiFi event handlers
  stationConnectedHandler = WiFi.onStationModeConnected(onConnected);
	stationDisconnectedHandler = WiFi.onStationModeDisconnected(onDisconnect);
	stationGotIpHandler = WiFi.onStationModeGotIP(onGotIP);
  stationModeDHCPTimeoutHandler = WiFi.onStationModeDHCPTimeout(onDHCPTimeout);
  APModeStationConnectedHandler = WiFi.onSoftAPModeStationConnected(onConnectedAPmode);
  APModeStationDisconnectedHandler = WiFi.onSoftAPModeStationDisconnected(onDisonnectedAPmode);
#endif

  if (SpiffsSectors() < 32)
  {
    serialPrintln(F("\nNo (or too small) SPIFFS area..\nSystem Halted\nPlease reflash with 128k SPIFFS minimum!"));
    while (true)
      delay(1);
  }

  emergencyReset();

  String log = F("\n\n\rINIT : Booting version: ");
  log += F(BUILD_GIT);
  log += " (";
  log += getSystemLibraryString();
  log += ')';
  addLog(LOG_LEVEL_INFO, log);
  log = F("INIT : Free RAM:");
  log += FreeMem();
  addLog(LOG_LEVEL_INFO, log);


  //warm boot
  if (readFromRTC())
  {
    RTC.bootFailedCount++;
    RTC.bootCounter++;
    lastMixedSchedulerId_beforereboot = RTC.lastMixedSchedulerId;
    readUserVarFromRTC();

    if (RTC.deepSleepState == 1)
    {
      log = F("INIT : Rebooted from deepsleep #");
      lastBootCause=BOOT_CAUSE_DEEP_SLEEP;
    }
    else
      log = F("INIT : Warm boot #");

    log += RTC.bootCounter;
    log += F(" Last Task: ");
    log += decodeSchedulerId(lastMixedSchedulerId_beforereboot);
  }
  //cold boot (RTC memory empty)
  else
  {
    initRTC();

    // cold boot situation
    if (lastBootCause == BOOT_CAUSE_MANUAL_REBOOT) // only set this if not set earlier during boot stage.
      lastBootCause = BOOT_CAUSE_COLD_BOOT;
    log = F("INIT : Cold Boot");
  }
  log += F(" - Restart Reason: ");
  log += getResetReasonString();

  RTC.deepSleepState=0;
  saveToRTC();

  addLog(LOG_LEVEL_INFO, log);

  fileSystemCheck();
  progMemMD5check();
  LoadSettings();
  Settings.UseRTOSMultitasking = false; // For now, disable it, we experience heap corruption.
  if (RTC.bootFailedCount > 10 && RTC.bootCounter > 10) {
    byte toDisable = RTC.bootFailedCount - 10;
    toDisable = disablePlugin(toDisable);
    if (toDisable != 0) {
      toDisable = disableController(toDisable);
    }
    if (toDisable != 0) {
      toDisable = disableNotification(toDisable);
    }
  }
  if (!selectValidWiFiSettings()) {
    wifiSetup = true;
    RTC.lastWiFiChannel = 0; // Must scan all channels
    // Wait until scan has finished to make sure as many as possible are found
    // We're still in the setup phase, so nothing else is taking resources of the ESP.
    WifiScan(false); 
  }

//  setWifiMode(WIFI_STA);
  checkRuleSets();

  // if different version, eeprom settings structure has changed. Full Reset needed
  // on a fresh ESP module eeprom values are set to 255. Version results into -1 (signed int)
  if (Settings.Version != VERSION || Settings.PID != ESP_PROJECT_PID)
  {
    // Direct Serial is allowed here, since this is only an emergency task.
    serialPrint(F("\nPID:"));
    serialPrintln(String(Settings.PID));
    serialPrint(F("Version:"));
    serialPrintln(String(Settings.Version));
    serialPrintln(F("INIT : Incorrect PID or version!"));
    delay(1000);
    ResetFactory();
  }

  if (Settings.UseSerial)
  {
    //make sure previous serial buffers are flushed before resetting baudrate
    Serial.flush();
    Serial.begin(Settings.BaudRate);
//    Serial.setDebugOutput(true);
  }

  if (Settings.Build != BUILD)
    BuildFixes();


  log = F("INIT : Free RAM:");
  log += FreeMem();
  addLog(LOG_LEVEL_INFO, log);

  if (Settings.UseSerial && Settings.SerialLogLevel >= LOG_LEVEL_DEBUG_MORE)
    Serial.setDebugOutput(true);

  checkRAM(F("hardwareInit"));
  hardwareInit();

  timermqtt_interval = 250; // Interval for checking MQTT
  timerAwakeFromDeepSleep = millis();
  CPluginInit();
  NPluginInit();
  PluginInit();
  log = F("INFO : Plugins: ");
  log += deviceCount + 1;
  log += getPluginDescriptionString();
  log += " (";
  log += getSystemLibraryString();
  log += ')';
  addLog(LOG_LEVEL_INFO, log);

  if (deviceCount + 1 >= PLUGIN_MAX) {
    addLog(LOG_LEVEL_ERROR, F("Programming error! - Increase PLUGIN_MAX"));
  }

  if (Settings.UseRules && isDeepSleepEnabled())
  {
    String event = F("System#NoSleep=");
    event += Settings.deepSleep_wakeTime;
    rulesProcessing(event); // TD-er: Process events in the setup() now.
  }

  if (Settings.UseRules)
  {
    String event = F("System#Wake");
    rulesProcessing(event); // TD-er: Process events in the setup() now.
  }

  WiFiConnectRelaxed();

  #ifdef FEATURE_REPORTING
  ReportStatus();
  #endif

  #ifdef FEATURE_ARDUINO_OTA
  ArduinoOTAInit();
  #endif

  // setup UDP
  if (Settings.UDPPort != 0)
    portUDP.begin(Settings.UDPPort);

  if (systemTimePresent())
    initTime();

#if FEATURE_ADC_VCC
  if (!wifiConnectInProgress) {
    vcc = ESP.getVcc() / 1000.0;
  }
#endif

  if (Settings.UseRules)
  {
    String event = F("System#Boot");
    rulesProcessing(event); // TD-er: Process events in the setup() now.
  }

  writeDefaultCSS();

  UseRTOSMultitasking = Settings.UseRTOSMultitasking;
  #ifdef USE_RTOS_MULTITASKING
    if(UseRTOSMultitasking){
      log = F("RTOS : Launching tasks");
      addLog(LOG_LEVEL_INFO, log);
      xTaskCreatePinnedToCore(RTOS_TaskServers, "RTOS_TaskServers", 16384, NULL, 1, NULL, 1);
      xTaskCreatePinnedToCore(RTOS_TaskSerial, "RTOS_TaskSerial", 8192, NULL, 1, NULL, 1);
      xTaskCreatePinnedToCore(RTOS_Task10ps, "RTOS_Task10ps", 8192, NULL, 1, NULL, 1);
      xTaskCreatePinnedToCore(
                    RTOS_HandleSchedule,   /* Function to implement the task */
                    "RTOS_HandleSchedule", /* Name of the task */
                    16384,      /* Stack size in words */
                    NULL,       /* Task input parameter */
                    1,          /* Priority of the task */
                    NULL,       /* Task handle. */
                    1);         /* Core where the task should run */
    }
  #endif

  // Start the interval timers at N msec from now.
  // Make sure to start them at some time after eachother,
  // since they will keep running at the same interval.
  setIntervalTimerOverride(TIMER_20MSEC,  5); // timer for periodic actions 50 x per/sec
  setIntervalTimerOverride(TIMER_100MSEC, 66); // timer for periodic actions 10 x per/sec
  setIntervalTimerOverride(TIMER_1SEC,    777); // timer for periodic actions once per/sec
  setIntervalTimerOverride(TIMER_30SEC,   1333); // timer for watchdog once per 30 sec
  setIntervalTimerOverride(TIMER_MQTT,    88); // timer for interaction with MQTT
  setIntervalTimerOverride(TIMER_STATISTICS, 2222);
}

#ifdef USE_RTOS_MULTITASKING
void RTOS_TaskServers( void * parameter )
{
 while (true){
  delay(100);
  WebServer.handleClient();
  checkUDP();
 }
}

void RTOS_TaskSerial( void * parameter )
{
 while (true){
    delay(100);
    if (Settings.UseSerial)
    if (Serial.available())
      if (!PluginCall(PLUGIN_SERIAL_IN, 0, dummyString))
        serial();
 }
}

void RTOS_Task10ps( void * parameter )
{
 while (true){
    delay(100);
    run10TimesPerSecond();
 }
}

void RTOS_HandleSchedule( void * parameter )
{
 while (true){
    handle_schedule();
 }
}

#endif


void updateLoopStats() {
  ++loopCounter;
  ++loopCounter_full;
  if (lastLoopStart == 0) {
    lastLoopStart = micros();
    return;
  }
  const long usecSince = usecPassedSince(lastLoopStart);
  #ifdef USES_TIMING_STATS
  miscStats[LOOP_STATS].add(usecSince);
  #endif

  loop_usec_duration_total += usecSince;
  lastLoopStart = micros();
  if (usecSince <= 0 || usecSince > 10000000)
    return; // No loop should take > 10 sec.
  if (shortestLoop > static_cast<unsigned long>(usecSince)) {
    shortestLoop = usecSince;
    loopCounterMax = 30 * 1000000 / usecSince;
  }
  if (longestLoop < static_cast<unsigned long>(usecSince))
    longestLoop = usecSince;
}

void updateLoopStats_30sec(byte loglevel) {
  loopCounterLast = loopCounter;
  loopCounter = 0;
  if (loopCounterLast > loopCounterMax)
    loopCounterMax = loopCounterLast;

  msecTimerHandler.updateIdleTimeStats();

#ifndef BUILD_NO_DEBUG
  if (loglevelActiveFor(loglevel)) {
    String log = F("LoopStats: shortestLoop: ");
    log += shortestLoop;
    log += F(" longestLoop: ");
    log += longestLoop;
    log += F(" avgLoopDuration: ");
    log += loop_usec_duration_total / loopCounter_full;
    log += F(" loopCounterMax: ");
    log += loopCounterMax;
    log += F(" loopCounterLast: ");
    log += loopCounterLast;
    addLog(loglevel, log);
  }
#endif
  loop_usec_duration_total = 0;
  loopCounter_full = 1;
}

float getCPUload() {
  return 100.0 - msecTimerHandler.getIdleTimePct();
}

int getLoopCountPerSec() {
  return loopCounterLast / 30;
}




/*********************************************************************************************\
 * MAIN LOOP
\*********************************************************************************************/
void loop()
{
  /*
  //FIXME TD-er: No idea what this does.
  if(MainLoopCall_ptr)
      MainLoopCall_ptr();
  */
  dummyString = String(); // Fixme TD-er  Make sure this global variable doesn't keep memory allocated.

  updateLoopStats();

  handle_unprocessedWiFiEvents();

  bool firstLoopConnectionsEstablished = WiFiConnected() && firstLoop;
  if (firstLoopConnectionsEstablished) {
     addLog(LOG_LEVEL_INFO, F("firstLoopConnectionsEstablished"));
     firstLoop = false;
     timerAwakeFromDeepSleep = millis(); // Allow to run for "awake" number of seconds, now we have wifi.
     // schedule_all_task_device_timers(); Disabled for now, since we are now using queues for controllers.
     if (Settings.UseRules && isDeepSleepEnabled())
     {
        String event = F("System#NoSleep=");
        event += Settings.deepSleep_wakeTime;
        eventQueue.add(event);
     }


     RTC.bootFailedCount = 0;
     saveToRTC();
     sendSysInfoUDP(1);
  }
  // Work around for nodes that do not have WiFi connection for a long time and may reboot after N unsuccessful connect attempts
  if ((wdcounter / 2) > 2) {
    // Apparently the uptime is already a few minutes. Let's consider it a successful boot.
     RTC.bootFailedCount = 0;
     saveToRTC();
  }

  // Deep sleep mode, just run all tasks one (more) time and go back to sleep as fast as possible
  if ((firstLoopConnectionsEstablished || readyForSleep()) && isDeepSleepEnabled())
  {
#ifdef USES_MQTT
      runPeriodicalMQTT();
#endif //USES_MQTT
      // Now run all frequent tasks
      run50TimesPerSecond();
      run10TimesPerSecond();
      runEach30Seconds();
      runOncePerSecond();
  }
  //normal mode, run each task when its time
  else
  {
    if (!UseRTOSMultitasking) {
      // On ESP32 the schedule is executed on the 2nd core.
      handle_schedule();
    }
  }

  backgroundtasks();

  if (readyForSleep()){
    prepare_deepSleep(Settings.Delay);
    //deepsleep will never return, its a special kind of reboot
  }
}

void flushAndDisconnectAllClients() {
  if (anyControllerEnabled()) {
#ifdef USES_MQTT
    bool mqttControllerEnabled = firstEnabledMQTTController() >= 0;
#endif //USES_MQTT
    unsigned long timer = millis() + 1000;
    while (!timeOutReached(timer)) {
      // call to all controllers (delay queue) to flush all data.
      CPluginCall(CPLUGIN_FLUSH, 0);
#ifdef USES_MQTT      
      if (mqttControllerEnabled && MQTTclient.connected()) {
        MQTTclient.loop();
      }
#endif //USES_MQTT      
    }
#ifdef USES_MQTT    
    if (mqttControllerEnabled && MQTTclient.connected()) {
      MQTTclient.disconnect();
      updateMQTTclient_connected();
    }
#endif //USES_MQTT      
    saveToRTC();
    delay(100); // Flush anything in the network buffers.
  }
  process_serialWriteBuffer();
}


#ifdef USES_MQTT

void updateMQTTclient_connected() {
  if (MQTTclient_connected != MQTTclient.connected()) {
    MQTTclient_connected = !MQTTclient_connected;
    if (!MQTTclient_connected) {
      if (loglevelActiveFor(LOG_LEVEL_ERROR)) {
        String connectionError = F("MQTT : Connection lost, state: ");
        connectionError += getMQTT_state();
        addLog(LOG_LEVEL_ERROR, connectionError);
      }
    } else {
      schedule_all_tasks_using_MQTT_controller();
    }
    if (Settings.UseRules) {
      if (MQTTclient_connected) {
        eventQueue.add(F("MQTT#Connected"));
      } else {
        eventQueue.add(F("MQTT#Disconnected"));
      }
    }
  }
  if (!MQTTclient_connected) {
    // As suggested here: https://github.com/letscontrolit/ESPEasy/issues/1356
    if (timermqtt_interval < 30000) {
      timermqtt_interval += 5000;
    }
  } else {
    timermqtt_interval = 250;
  }
  setIntervalTimer(TIMER_MQTT);
}

void runPeriodicalMQTT() {
  // MQTT_KEEPALIVE = 15 seconds.
  if (!WiFiConnected(10)) {
    updateMQTTclient_connected();
    return;
  }
  //dont do this in backgroundtasks(), otherwise causes crashes. (https://github.com/letscontrolit/ESPEasy/issues/683)
  int enabledMqttController = firstEnabledMQTTController();
  if (enabledMqttController >= 0) {
    if (!MQTTclient.loop()) {
      updateMQTTclient_connected();
      if (MQTTCheck(enabledMqttController)) {
        updateMQTTclient_connected();
      }
    }
  } else {
    if (MQTTclient.connected()) {
      MQTTclient.disconnect();
      updateMQTTclient_connected();
    }
  }
}

int firstEnabledMQTTController() {
  for (byte i = 0; i < CONTROLLER_MAX; ++i) {
    byte ProtocolIndex = getProtocolIndex(Settings.Protocol[i]);
    if (Protocol[ProtocolIndex].usesMQTT && Settings.ControllerEnabled[i]) {
      return i;
    }
  }
  return -1;
}

#endif //USES_MQTT

#ifdef USES_BLYNK
// Blynk_get prototype
//boolean Blynk_get(const String& command, byte controllerIndex,float *data = NULL );

int firstEnabledBlynkController() {
  for (byte i = 0; i < CONTROLLER_MAX; ++i) {
    byte ProtocolIndex = getProtocolIndex(Settings.Protocol[i]);
    if (Protocol[ProtocolIndex].Number == 12 && Settings.ControllerEnabled[i]) {
      return i;
    }
  }
  return -1;
}
#endif


/*********************************************************************************************\
 * Tasks that run 50 times per second
\*********************************************************************************************/

void run50TimesPerSecond() {
  START_TIMER;
  String dummy;
  PluginCall(PLUGIN_FIFTY_PER_SECOND, 0, dummy);
  STOP_TIMER(PLUGIN_CALL_50PS);
}

/*********************************************************************************************\
 * Tasks that run 10 times per second
\*********************************************************************************************/
void run10TimesPerSecond() {
  String dummy;
  {
    START_TIMER;
    PluginCall(PLUGIN_TEN_PER_SECOND, 0, dummy);
    STOP_TIMER(PLUGIN_CALL_10PS);
  }
  {
    START_TIMER;
//    PluginCall(PLUGIN_UNCONDITIONAL_POLL, 0, dummyString);
    PluginCall(PLUGIN_MONITOR, 0, dummy);
    STOP_TIMER(PLUGIN_CALL_10PSU);
  }
  if (Settings.UseRules)
  {
    processNextEvent();
  }
  #ifdef USES_C015
  if (WiFiConnected())
      Blynk_Run_c015();
  #endif
  #ifndef USE_RTOS_MULTITASKING
    WebServer.handleClient();
  #endif
}


/*********************************************************************************************\
 * Tasks each second
\*********************************************************************************************/
void runOncePerSecond()
{
  START_TIMER;
  updateLogLevelCache();
  dailyResetCounter++;
  if (dailyResetCounter > 86400) // 1 day elapsed... //86400
  {
    RTC.flashDayCounter=0;
    saveToRTC();
    dailyResetCounter=0;
    addLog(LOG_LEVEL_INFO, F("SYS  : Reset 24h counters"));
  }

  if (Settings.ConnectionFailuresThreshold)
    if (connectionFailures > Settings.ConnectionFailuresThreshold)
      delayedReboot(60);

  if (cmd_within_mainloop != 0)
  {
    switch (cmd_within_mainloop)
    {
      case CMD_WIFI_DISCONNECT:
        {
          WifiDisconnect();
          break;
        }
      case CMD_REBOOT:
        {
          reboot();
          break;
        }
    }
    cmd_within_mainloop = 0;
  }
  // clock events
  if (systemTimePresent())
    checkTime();

//  unsigned long start = micros();
  String dummy;
  PluginCall(PLUGIN_ONCE_A_SECOND, 0, dummy);
//  unsigned long elapsed = micros() - start;

  if (Settings.UseRules)
    rulesTimers();


  if (SecuritySettings.Password[0] != 0)
  {
    if (WebLoggedIn)
      WebLoggedInTimer++;
    if (WebLoggedInTimer > 300)
      WebLoggedIn = false;
  }

  // I2C Watchdog feed
  if (Settings.WDI2CAddress != 0)
  {
    Wire.beginTransmission(Settings.WDI2CAddress);
    Wire.write(0xA5);
    Wire.endTransmission();
  }

  checkResetFactoryPin();
  STOP_TIMER(PLUGIN_CALL_1PS);
}

void logTimerStatistics() {
  byte loglevel = LOG_LEVEL_DEBUG;
  updateLoopStats_30sec(loglevel);
#ifndef BUILD_NO_DEBUG
//  logStatistics(loglevel, true);
  if (loglevelActiveFor(loglevel)) {
    String queueLog = F("Scheduler stats: (called/tasks/max_length/idle%) ");
    queueLog += msecTimerHandler.getQueueStats();
    addLog(loglevel, queueLog);
  }
#endif
}

/*********************************************************************************************\
 * Tasks each 30 seconds
\*********************************************************************************************/
void runEach30Seconds()
{
   extern void checkRAMtoLog();
  checkRAMtoLog();
  wdcounter++;
  if (loglevelActiveFor(LOG_LEVEL_INFO)) {
    String log;
    log.reserve(80);
    log = F("WD   : Uptime ");
    log += wdcounter / 2;
    log += F(" ConnectFailures ");
    log += connectionFailures;
    log += F(" FreeMem ");
    log += FreeMem();
    log += F(" WiFiStatus ");
    log += WiFi.status();
//    log += F(" ListenInterval ");
//    log += WiFi.getListenInterval();
    addLog(LOG_LEVEL_INFO, log);
  }
  sendSysInfoUDP(1);
  refreshNodeList();

  // sending $stats to homie controller
  CPluginCall(CPLUGIN_INTERVAL, 0);

  #if defined(ESP8266)
  #ifdef USES_SSDP
  if (Settings.UseSSDP)
    SSDP_update();

  #endif // USES_SSDP
  #endif
#if FEATURE_ADC_VCC
  if (!wifiConnectInProgress) {
    vcc = ESP.getVcc() / 1000.0;
  }
#endif

  #ifdef FEATURE_REPORTING
  ReportStatus();
  #endif

}




/*********************************************************************************************\
 * run background tasks
\*********************************************************************************************/
bool runningBackgroundTasks=false;
void backgroundtasks()
{
  //checkRAM(F("backgroundtasks"));
  //always start with a yield
  delay(0);
/*
  // Remove this watchdog feed for now.
  // See https://github.com/letscontrolit/ESPEasy/issues/1722#issuecomment-419659193

  #ifdef ESP32
  // Have to find a similar function to call ESP32's esp_task_wdt_feed();
  #else
  ESP.wdtFeed();
  #endif
*/

  //prevent recursion!
  if (runningBackgroundTasks)
  {
    return;
  }
  START_TIMER
  const bool wifiConnected = WiFiConnected();
  runningBackgroundTasks=true;

  #if defined(ESP8266)
  if (wifiConnected) {
    tcpCleanup();
  }
  #endif
  process_serialWriteBuffer();
  if(!UseRTOSMultitasking){
    if (Settings.UseSerial && Serial.available()) {
      String dummy;
      if (!PluginCall(PLUGIN_SERIAL_IN, 0, dummy)) {
        serial();
      }
    }
    if (webserverRunning) {
      WebServer.handleClient();
    }
    if (WiFi.getMode() != WIFI_OFF) {
      checkUDP();
    }
  }

  // process DNS, only used if the ESP has no valid WiFi config
  if (dnsServerActive)
    dnsServer.processNextRequest();

  #ifdef FEATURE_ARDUINO_OTA
  if(Settings.ArduinoOTAEnable && wifiConnected)
    ArduinoOTA.handle();

  //once OTA is triggered, only handle that and dont do other stuff. (otherwise it fails)
  while (ArduinoOTAtriggered)
  {
    delay(0);
    if (WiFiConnected()) {
      ArduinoOTA.handle();
    }
  }

  #endif

  #ifdef FEATURE_MDNS
  // Allow MDNS processing
  if (wifiConnected) {
    MDNS.update();
  }
  #endif

  delay(0);

  statusLED(false);

  runningBackgroundTasks=false;
  STOP_TIMER(BACKGROUND_TASKS);
}
