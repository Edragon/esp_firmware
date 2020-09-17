#ifdef USES_C010
//#######################################################################################################
//########################### Controller Plugin 010: Generic UDP ########################################
//#######################################################################################################

#define CPLUGIN_010
#define CPLUGIN_ID_010         10
#define CPLUGIN_NAME_010       "Generic UDP"

bool CPlugin_010(byte function, struct EventStruct *event, String& string)
{
  bool success = false;

  switch (function)
  {
    case CPLUGIN_PROTOCOL_ADD:
      {
        Protocol[++protocolCount].Number = CPLUGIN_ID_010;
        Protocol[protocolCount].usesMQTT = false;
        Protocol[protocolCount].usesTemplate = true;
        Protocol[protocolCount].usesAccount = false;
        Protocol[protocolCount].usesPassword = false;
        Protocol[protocolCount].defaultPort = 514;
        Protocol[protocolCount].usesID = false;
        break;
      }

    case CPLUGIN_GET_DEVICENAME:
      {
        string = F(CPLUGIN_NAME_010);
        break;
      }

    case CPLUGIN_PROTOCOL_TEMPLATE:
      {
        event->String1 = "";
        event->String2 = F("%sysname%_%tskname%_%valname%=%value%");
        break;
      }

    case CPLUGIN_PROTOCOL_SEND:
      {
        byte valueCount = getValueCountFromSensorType(event->sensorType);
        C010_queue_element element(event, valueCount);
        if (ExtraTaskSettings.TaskIndex != event->TaskIndex) {
          String dummy;
          PluginCall(PLUGIN_GET_DEVICEVALUENAMES, event, dummy);
        }

        MakeControllerSettings(ControllerSettings);
        LoadControllerSettings(event->ControllerIndex, ControllerSettings);

        for (byte x = 0; x < valueCount; x++)
        {
          bool isvalid;
          String formattedValue = formatUserVar(event, x, isvalid);
          if (isvalid) {
            element.txt[x] = "";
            element.txt[x] += ControllerSettings.Publish;
            parseControllerVariables(element.txt[x], event, false);
            element.txt[x].replace(F("%valname%"), ExtraTaskSettings.TaskDeviceValueNames[x]);
            element.txt[x].replace(F("%value%"), formattedValue);
            addLog(LOG_LEVEL_DEBUG_MORE, element.txt[x]);
          }
        }
        success = C010_DelayHandler.addToQueue(element);
        scheduleNextDelayQueue(TIMER_C010_DELAY_QUEUE, C010_DelayHandler.getNextScheduleTime());
        break;
      }

    case CPLUGIN_FLUSH:
      {
        process_c010_delay_queue();
        delay(0);
        break;
      }


  }
  return success;
}


//********************************************************************************
// Generic UDP message
//********************************************************************************
bool do_process_c010_delay_queue(int controller_number, const C010_queue_element& element, ControllerSettingsStruct& ControllerSettings);

bool do_process_c010_delay_queue(int controller_number, const C010_queue_element& element, ControllerSettingsStruct& ControllerSettings) {
  while (element.txt[element.valuesSent] == "") {
    // A non valid value, which we are not going to send.
    // Increase sent counter until a valid value is found.
    if (element.checkDone(true))
      return true;
  }
  WiFiUDP C010_portUDP;
  if (!beginWiFiUDP_randomPort(C010_portUDP)) return false;
  if (!try_connect_host(controller_number, C010_portUDP, ControllerSettings))
    return false;

  C010_portUDP.write(
    (uint8_t*)element.txt[element.valuesSent].c_str(),
              element.txt[element.valuesSent].length());
  bool reply = C010_portUDP.endPacket();
  C010_portUDP.stop();
  if (ControllerSettings.MustCheckReply)
    return element.checkDone(reply);
  return element.checkDone(true);
}
#endif
