#ifdef WEBSERVER_ADVANCED

// ********************************************************************************
// Web Interface config page
// ********************************************************************************
void handle_advanced() {
  checkRAM(F("handle_advanced"));

  if (!isLoggedIn()) { return; }
  navMenuIndex = MENU_INDEX_TOOLS;
  TXBuffer.startStream();
  sendHeadandTail_stdtemplate();

  int timezone      = getFormItemInt(F("timezone"));
  int dststartweek  = getFormItemInt(F("dststartweek"));
  int dststartdow   = getFormItemInt(F("dststartdow"));
  int dststartmonth = getFormItemInt(F("dststartmonth"));
  int dststarthour  = getFormItemInt(F("dststarthour"));
  int dstendweek    = getFormItemInt(F("dstendweek"));
  int dstenddow     = getFormItemInt(F("dstenddow"));
  int dstendmonth   = getFormItemInt(F("dstendmonth"));
  int dstendhour    = getFormItemInt(F("dstendhour"));
  String edit       = WebServer.arg(F("edit"));


  if (edit.length() != 0)
  {
    Settings.MessageDelay = getFormItemInt(F("messagedelay"));
    Settings.IP_Octet     = WebServer.arg(F("ip")).toInt();
    strncpy_webserver_arg(Settings.NTPHost, F("ntphost"));
    Settings.TimeZone = timezone;
    TimeChangeRule dst_start(dststartweek, dststartdow, dststartmonth, dststarthour, timezone);

    if (dst_start.isValid()) { Settings.DST_Start = dst_start.toFlashStoredValue(); }
    TimeChangeRule dst_end(dstendweek, dstenddow, dstendmonth, dstendhour, timezone);

    if (dst_end.isValid()) { Settings.DST_End = dst_end.toFlashStoredValue(); }
    str2ip(WebServer.arg(F("syslogip")).c_str(), Settings.Syslog_IP);
    Settings.UDPPort = getFormItemInt(F("udpport"));

    Settings.SyslogFacility = getFormItemInt(F("syslogfacility"));
    Settings.UseSerial      = isFormItemChecked(F("useserial"));
    setLogLevelFor(LOG_TO_SYSLOG, getFormItemInt(getInternalLabel(LabelType::SYSLOG_LOG_LEVEL)));
    setLogLevelFor(LOG_TO_SERIAL, getFormItemInt(getInternalLabel(LabelType::SERIAL_LOG_LEVEL)));
    setLogLevelFor(LOG_TO_WEBLOG, getFormItemInt(getInternalLabel(LabelType::WEB_LOG_LEVEL)));
#ifdef FEATURE_SD
    setLogLevelFor(LOG_TO_SDCARD, getFormItemInt(getInternalLabel(LabelType::SD_LOG_LEVEL)));
#endif // ifdef FEATURE_SD
    Settings.UseValueLogger              = isFormItemChecked(F("valuelogger"));
    Settings.BaudRate                    = getFormItemInt(F("baudrate"));
    Settings.UseNTP                      = isFormItemChecked(F("usentp"));
    Settings.DST                         = isFormItemChecked(F("dst"));
    Settings.WDI2CAddress                = getFormItemInt(F("wdi2caddress"));
    #ifdef USES_SSDP
    Settings.UseSSDP                     = isFormItemChecked(F("usessdp"));
    #endif // USES_SSDP
    Settings.WireClockStretchLimit       = getFormItemInt(F("wireclockstretchlimit"));
    Settings.UseRules                    = isFormItemChecked(F("userules"));
    Settings.ConnectionFailuresThreshold = getFormItemInt(F("cft"));
    Settings.MQTTRetainFlag              = isFormItemChecked(F("mqttretainflag"));
    Settings.ArduinoOTAEnable            = isFormItemChecked(F("arduinootaenable"));
    Settings.UseRTOSMultitasking         = isFormItemChecked(F("usertosmultitasking"));
    Settings.MQTTUseUnitNameAsClientId   = isFormItemChecked(F("mqttuseunitnameasclientid"));
    Settings.uniqueMQTTclientIdReconnect(isFormItemChecked(F("uniquemqttclientidreconnect")));
    Settings.Latitude  = getFormItemFloat(F("latitude"));
    Settings.Longitude = getFormItemFloat(F("longitude"));
    Settings.OldRulesEngine(isFormItemChecked(F("oldrulesengine")));
    Settings.TolerantLastArgParse(isFormItemChecked(F("tolerantargparse")));
    Settings.ForceWiFi_bg_mode(isFormItemChecked(getInternalLabel(LabelType::FORCE_WIFI_BG)));
    Settings.WiFiRestart_connection_lost(isFormItemChecked(getInternalLabel(LabelType::RESTART_WIFI_LOST_CONN)));
    Settings.EcoPowerMode(isFormItemChecked(getInternalLabel(LabelType::CPU_ECO_MODE)));
    Settings.WifiNoneSleep(isFormItemChecked(getInternalLabel(LabelType::FORCE_WIFI_NOSLEEP)));
#ifdef SUPPORT_ARP
    Settings.gratuitousARP(isFormItemChecked(getInternalLabel(LabelType::PERIODICAL_GRAT_ARP)));
#endif // ifdef SUPPORT_ARP

    addHtmlError(SaveSettings());

    if (systemTimePresent()) {
      initTime();
    }
  }

  TXBuffer += F("<form  method='post'>");
  html_table_class_normal();

  addFormHeader(F("Advanced Settings"), F("RTDTools/Tools.html#advanced"));

  addFormSubHeader(F("Rules Settings"));

  addFormCheckBox(F("Rules"),      F("userules"),       Settings.UseRules);
  addFormCheckBox(F("Old Engine"), F("oldrulesengine"), Settings.OldRulesEngine());
  addFormCheckBox(F("Tolerant last parameter"), F("tolerantargparse"), Settings.TolerantLastArgParse());
  addFormNote(F("Perform less strict parsing on last argument of some commands (e.g. publish and sendToHttp)"));

  addFormSubHeader(F("Controller Settings"));

  addFormCheckBox(F("MQTT Retain Msg"), F("mqttretainflag"), Settings.MQTTRetainFlag);
  addFormNumericBox(F("Message Interval"), F("messagedelay"), Settings.MessageDelay, 0, INT_MAX);
  addUnit(F("ms"));
  addFormCheckBox(F("MQTT use unit name as ClientId"),    F("mqttuseunitnameasclientid"),   Settings.MQTTUseUnitNameAsClientId);
  addFormCheckBox(F("MQTT change ClientId at reconnect"), F("uniquemqttclientidreconnect"), Settings.uniqueMQTTclientIdReconnect());

  addFormSubHeader(F("NTP Settings"));

  addFormCheckBox(F("Use NTP"), F("usentp"), Settings.UseNTP);
  addFormTextBox(F("NTP Hostname"), F("ntphost"), Settings.NTPHost, 63);

  addFormSubHeader(F("DST Settings"));
  addFormDstSelect(true,  Settings.DST_Start);
  addFormDstSelect(false, Settings.DST_End);
  addFormNumericBox(F("Timezone Offset (UTC +)"), F("timezone"), Settings.TimeZone, -720, 840); // UTC-12H ... UTC+14h
  addUnit(F("minutes"));
  addFormCheckBox(F("DST"), F("dst"), Settings.DST);

  addFormSubHeader(F("Location Settings"));
  addFormFloatNumberBox(F("Latitude"), F("latitude"), Settings.Latitude, -90.0, 90.0);
  addUnit(F("&deg;"));
  addFormFloatNumberBox(F("Longitude"), F("longitude"), Settings.Longitude, -180.0, 180.0);
  addUnit(F("&deg;"));

  addFormSubHeader(F("Log Settings"));

  addFormIPBox(F("Syslog IP"), F("syslogip"), Settings.Syslog_IP);
  addFormLogLevelSelect(getLabel(LabelType::SYSLOG_LOG_LEVEL), getInternalLabel(LabelType::SYSLOG_LOG_LEVEL), Settings.SyslogLevel);
  addFormLogFacilitySelect(F("Syslog Facility"), F("syslogfacility"), Settings.SyslogFacility);
  addFormLogLevelSelect(getLabel(LabelType::SERIAL_LOG_LEVEL), getInternalLabel(LabelType::SERIAL_LOG_LEVEL), Settings.SerialLogLevel);
  addFormLogLevelSelect(getLabel(LabelType::WEB_LOG_LEVEL),    getInternalLabel(LabelType::WEB_LOG_LEVEL),    Settings.WebLogLevel);

#ifdef FEATURE_SD
  addFormLogLevelSelect(getLabel(LabelType::SD_LOG_LEVEL),     getInternalLabel(LabelType::SD_LOG_LEVEL),     Settings.SDLogLevel);

  addFormCheckBox(F("SD Card Value Logger"), F("valuelogger"), Settings.UseValueLogger);
#endif // ifdef FEATURE_SD


  addFormSubHeader(F("Serial Settings"));

  addFormCheckBox(F("Enable Serial port"), F("useserial"), Settings.UseSerial);
  addFormNumericBox(F("Baud Rate"), F("baudrate"), Settings.BaudRate, 0, 1000000);


  addFormSubHeader(F("Inter-ESPEasy Network"));

  addFormNumericBox(F("UDP port"), F("udpport"), Settings.UDPPort, 0, 65535);


  // TODO sort settings in groups or move to other pages/groups
  addFormSubHeader(F("Special and Experimental Settings"));

  addFormNumericBox(F("Fixed IP Octet"),        F("ip"),                    Settings.IP_Octet,     0, 255);

  addFormNumericBox(F("WD I2C Address"),        F("wdi2caddress"),          Settings.WDI2CAddress, 0, 127);
  TXBuffer += F(" (decimal)");

  addFormNumericBox(F("I2C ClockStretchLimit"), F("wireclockstretchlimit"), Settings.WireClockStretchLimit); // TODO define limits
  #if defined(FEATURE_ARDUINO_OTA)
  addFormCheckBox(F("Enable Arduino OTA"), F("arduinootaenable"), Settings.ArduinoOTAEnable);
  #endif // if defined(FEATURE_ARDUINO_OTA)
  #if defined(ESP32)
  addFormCheckBox_disabled(F("Enable RTOS Multitasking"), F("usertosmultitasking"), Settings.UseRTOSMultitasking);
  #endif // if defined(ESP32)

  #ifdef USES_SSDP
  addFormCheckBox_disabled(F("Use SSDP"),                 F("usessdp"),             Settings.UseSSDP);
  #endif

  addFormNumericBox(getLabel(LabelType::CONNECTION_FAIL_THRESH), F("cft"), Settings.ConnectionFailuresThreshold, 0, 100);
#ifdef ESP8266
  addFormCheckBox(LabelType::FORCE_WIFI_BG, Settings.ForceWiFi_bg_mode());
#endif // ifdef ESP8266
#ifdef ESP32

  // Disabled for now, since it is not working properly.
  addFormCheckBox_disabled(LabelType::FORCE_WIFI_BG, Settings.ForceWiFi_bg_mode());
#endif // ifdef ESP32

  addFormCheckBox(LabelType::RESTART_WIFI_LOST_CONN, Settings.WiFiRestart_connection_lost());
#ifdef ESP8266
  addFormCheckBox(LabelType::FORCE_WIFI_NOSLEEP,     Settings.WifiNoneSleep());
#endif // ifdef ESP8266
  addFormNote(F("Change WiFi sleep settings requires reboot to activate"));
#ifdef SUPPORT_ARP
  addFormCheckBox(LabelType::PERIODICAL_GRAT_ARP, Settings.gratuitousARP());
#endif // ifdef SUPPORT_ARP
  addFormCheckBox(LabelType::CPU_ECO_MODE,        Settings.EcoPowerMode());
  addFormNote(F("Node may miss receiving packets with Eco mode enabled"));
  addFormSeparator(2);

  html_TR_TD();
  html_TD();
  addSubmitButton();
  TXBuffer += F("<input type='hidden' name='edit' value='1'>");
  html_end_table();
  html_end_form();
  sendHeadandTail_stdtemplate(true);
  TXBuffer.endStream();
}

void addFormDstSelect(bool isStart, uint16_t choice) {
  String weekid  = isStart ? F("dststartweek")  : F("dstendweek");
  String dowid   = isStart ? F("dststartdow")   : F("dstenddow");
  String monthid = isStart ? F("dststartmonth") : F("dstendmonth");
  String hourid  = isStart ? F("dststarthour")  : F("dstendhour");

  String weeklabel = isStart ? F("Start (week, dow, month)")  : F("End (week, dow, month)");
  String hourlabel = isStart ? F("Start (localtime, e.g. 2h&rarr;3h)")  : F("End (localtime, e.g. 3h&rarr;2h)");

  String week[5]       = { F("Last"), F("1st"), F("2nd"), F("3rd"), F("4th") };
  int    weekValues[5] = { 0, 1, 2, 3, 4 };
  String dow[7]        = { F("Sun"), F("Mon"), F("Tue"), F("Wed"), F("Thu"), F("Fri"), F("Sat") };
  int    dowValues[7]  = { 1, 2, 3, 4, 5, 6, 7 };
  String month[12]     = { F("Jan"), F("Feb"), F("Mar"), F("Apr"), F("May"), F("Jun"), F("Jul"), F("Aug"), F("Sep"), F("Oct"), F("Nov"), F(
                             "Dec") };
  int    monthValues[12] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };

  uint16_t tmpstart(choice);
  uint16_t tmpend(choice);

  if (!TimeChangeRule(choice, 0).isValid()) {
    getDefaultDst_flash_values(tmpstart, tmpend);
  }
  TimeChangeRule rule(isStart ? tmpstart : tmpend, 0);
  addRowLabel(weeklabel);
  addSelector(weekid,  5,  week,   weekValues, NULL, rule.week,  false);
  TXBuffer += F("<BR>");
  addSelector(dowid,   7,  dow,     dowValues, NULL, rule.dow,   false);
  TXBuffer += F("<BR>");
  addSelector(monthid, 12, month, monthValues, NULL, rule.month, false);

  addFormNumericBox(hourlabel, hourid, rule.hour, 0, 23);
  addUnit(isStart ? F("hour &#x21b7;") : F("hour &#x21b6;"));
}

void addFormLogLevelSelect(const String& label, const String& id, int choice)
{
  addRowLabel(label);
  addLogLevelSelect(id, choice);
}

void addLogLevelSelect(const String& name, int choice)
{
  String options[LOG_LEVEL_NRELEMENTS + 1];
  int    optionValues[LOG_LEVEL_NRELEMENTS + 1] = { 0 };

  options[0]      = getLogLevelDisplayString(0);
  optionValues[0] = 0;

  for (int i = 0; i < LOG_LEVEL_NRELEMENTS; ++i) {
    options[i + 1] = getLogLevelDisplayStringFromIndex(i, optionValues[i + 1]);
  }
  addSelector(name, LOG_LEVEL_NRELEMENTS + 1, options, optionValues, NULL, choice, false);
}

void addFormLogFacilitySelect(const String& label, const String& id, int choice)
{
  addRowLabel(label);
  addLogFacilitySelect(id, choice);
}

void addLogFacilitySelect(const String& name, int choice)
{
  String options[12] =
  { F("Kernel"), F("User"),   F("Daemon"), F("Message"), F("Local0"), F("Local1"),
    F("Local2"), F("Local3"), F("Local4"), F("Local5"),  F("Local6"), F("Local7") };
  int optionValues[12] = { 0, 1, 3, 5, 16, 17, 18, 19, 20, 21, 22, 23 };

  addSelector(name, 12, options, optionValues, NULL, choice, false);
}

#endif // ifdef WEBSERVER_ADVANCED
