#ifdef USES_C017
//#######################################################################################################
//###########################   Controller Plugin 017: ZABBIX  ##########################################
//#######################################################################################################
// Based on https://www.zabbix.com/documentation/current/manual/appendix/items/trapper
// and https://www.zabbix.com/documentation/4.2/manual/appendix/protocols/header_datalen

// USAGE: at Zabbix server you go at Configuration -> Hosts -> Create host
// The "Host name" should match exactly the EspEasy name (Config -> Unit Name)
// Add a group (mandatory) and hit add. No need to set up IP address or agent.
// Go to the newly created host ->Items ->Create Item
// Nane the item something descriptive
// For Key add the EspEasy task Value name (case sensitive)
// Type of information select "Numeric (float)" and press add. Thats it.

#define CPLUGIN_017
#define CPLUGIN_ID_017 17
#define CPLUGIN_NAME_017 "Zabbix"
#include <ArduinoJson.h>

bool CPlugin_017(byte function, struct EventStruct *event, String &string)
{
  bool success = false;

  switch (function)
  {
  case CPLUGIN_PROTOCOL_ADD:
  {
    Protocol[++protocolCount].Number = CPLUGIN_ID_017;
    Protocol[protocolCount].usesMQTT = false;
    Protocol[protocolCount].usesTemplate = false;
    Protocol[protocolCount].usesAccount = false;
    Protocol[protocolCount].usesPassword = false;
    Protocol[protocolCount].usesID = false;
    Protocol[protocolCount].defaultPort = 10051;
    break;
  }

  case CPLUGIN_GET_DEVICENAME:
  {
    string = F(CPLUGIN_NAME_017);
    break;
  }

  case CPLUGIN_PROTOCOL_SEND:
  {
    byte valueCount = getValueCountFromSensorType(event->sensorType);
    C017_queue_element element(event);

    MakeControllerSettings(ControllerSettings);
    LoadControllerSettings(event->ControllerIndex, ControllerSettings);

    for (byte x = 0; x < valueCount; x++)
    {
      element.txt[x] = formatUserVarNoCheck(event, x);
    }
    success = C017_DelayHandler.addToQueue(element);
    scheduleNextDelayQueue(TIMER_C017_DELAY_QUEUE, C017_DelayHandler.getNextScheduleTime());
    break;
  }

  case CPLUGIN_FLUSH:
  {
    process_c017_delay_queue();
    delay(0);
    break;
  }
  }
  return success;
}

bool do_process_c017_delay_queue(int controller_number, const C017_queue_element &element, ControllerSettingsStruct &ControllerSettings);

bool do_process_c017_delay_queue(int controller_number, const C017_queue_element &element, ControllerSettingsStruct &ControllerSettings)
{
  byte valueCount = getValueCountFromSensorType(element.sensorType);
  if (valueCount == 0)
    return true; //exit if we don't have anything to send.

  if (!WiFiConnected(10))
  {
    return false;
  }

  WiFiClient client;
  if (!ControllerSettings.connectToHost(client))
  {
    connectionFailures++;
    addLog(LOG_LEVEL_ERROR, String(F("ZBX: Cannot connect")));
    return false;
  }
  statusLED(true);
  if (connectionFailures)
    connectionFailures--;

  LoadTaskSettings(element.TaskIndex);

  const size_t capacity = JSON_ARRAY_SIZE(VARS_PER_TASK) + JSON_OBJECT_SIZE(2) + VARS_PER_TASK * JSON_OBJECT_SIZE(3) + VARS_PER_TASK * 50; //Size for esp8266 with 4 variables per task: 288+200
  DynamicJsonDocument root(capacity);

  // Create the schafolding
  root[F("request")] = F("sender data");
  JsonArray data = root.createNestedArray(F("data"));
  // Populate JSON with the data
  for (uint8_t i = 0; i < valueCount; i++)
  {
    if (ExtraTaskSettings.TaskDeviceValueNames[i][0] == 0)
      continue; //Zabbix will ignore an empty key anyway
    JsonObject block = data.createNestedObject();
    block[F("host")] = Settings.Name;                            // Zabbix hostname, Unit Name for the ESP easy
    block[F("key")] = ExtraTaskSettings.TaskDeviceValueNames[i]; // Zabbix item key // Value Name for the ESP easy
    block[F("value")] = atof(element.txt[i].c_str());            // ESPeasy supports only floats
  }

  // Assemble packet
  char packet_header[] = "ZBXD\1";
  String JSON_packet_content="";

  serializeJson(root, JSON_packet_content);
  uint64_t payload_len = JSON_packet_content.length();
  
  // addLog(LOG_LEVEL_INFO, String(F("ZBX: ")) + JSON_packet_content);
  // Send the packet
  client.write(packet_header, sizeof(packet_header) - 1);
  client.write((char *)&payload_len, sizeof(payload_len));
  client.write(JSON_packet_content.c_str(), payload_len);

  client.stop();
  return true;
}
#endif