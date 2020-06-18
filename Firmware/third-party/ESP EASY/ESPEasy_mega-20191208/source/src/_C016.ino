#ifdef USES_C016
//#######################################################################################################
//########################### Controller Plugin 016: Controller - Cache #################################
//#######################################################################################################

/*
This is a cache layer to collect data while not connected to a network.
The data will first be stored in RTC memory, which will survive a crash/reboot and even an OTA update.
If this RTC buffer is full, it will be flushed to whatever is set here as storage.

Typical sample sets contain:
- UNIX timestamp
- task index delivering the data
- 4 float values

These are the result of any plugin sending data to this controller.

The controller can save the samples from RTC memory to several places on the flash:
- Files on SPIFFS
- Part reserved for OTA update (TODO)
- Unused flash after the partitioned space (TODO)

The controller can deliver the data to:
<TODO>
*/

#define CPLUGIN_016
#define CPLUGIN_ID_016         16
#define CPLUGIN_NAME_016       "Cache Controller [Experimental]"
//#include <ArduinoJson.h>

ControllerCache_struct ControllerCache;

bool CPlugin_016(byte function, struct EventStruct *event, String& string)
{
  bool success = false;

  switch (function)
  {
    case CPLUGIN_PROTOCOL_ADD:
      {
        Protocol[++protocolCount].Number = CPLUGIN_ID_016;
        Protocol[protocolCount].usesMQTT = false;
        Protocol[protocolCount].usesTemplate = true;
        Protocol[protocolCount].usesAccount = false;
        Protocol[protocolCount].usesPassword = false;
        Protocol[protocolCount].defaultPort = 80;
        Protocol[protocolCount].usesID = false;
        Protocol[protocolCount].usesHost = false;
        Protocol[protocolCount].usesPort = false;
        Protocol[protocolCount].usesSampleSets = false;
        break;
      }

    case CPLUGIN_GET_DEVICENAME:
      {
        string = F(CPLUGIN_NAME_016);
        break;
      }

    case CPLUGIN_INIT:
      {
        MakeControllerSettings(ControllerSettings);
        LoadControllerSettings(event->ControllerIndex, ControllerSettings);
        C016_DelayHandler.configureControllerSettings(ControllerSettings);
        ControllerCache.init();
        break;
      }

    case CPLUGIN_WEBFORM_LOAD:
      {

        break;
      }

    case CPLUGIN_WEBFORM_SAVE:
      {

        break;
      }

    case CPLUGIN_PROTOCOL_TEMPLATE:
      {
        event->String1 = "";
        event->String2 = "";
        break;
      }

    case CPLUGIN_PROTOCOL_SEND:
      {
        // Collect the values at the same run, to make sure all are from the same sample
        byte valueCount = getValueCountFromSensorType(event->sensorType);
        C016_queue_element element(event, valueCount, getUnixTime());
        success = ControllerCache.write((uint8_t*)&element, sizeof(element));

/*
        MakeControllerSettings(ControllerSettings);
        LoadControllerSettings(event->ControllerIndex, ControllerSettings);
        success = C016_DelayHandler.addToQueue(element);
        scheduleNextDelayQueue(TIMER_C016_DELAY_QUEUE, C016_DelayHandler.getNextScheduleTime());
*/
        break;
      }

    case CPLUGIN_FLUSH:
      {
        process_c016_delay_queue();
        delay(0);
        break;
      }

  }
  return success;
}



//********************************************************************************
// Process the data from the cache
//********************************************************************************
bool do_process_c016_delay_queue(int controller_number, const C016_queue_element& element, ControllerSettingsStruct& ControllerSettings);

bool do_process_c016_delay_queue(int controller_number, const C016_queue_element& element, ControllerSettingsStruct& ControllerSettings) {
  return true;
  // FIXME TD-er: Hand over data to wherever it needs to be.
  // Ideas:
  // - Upload bin files to some server (HTTP post?)
  // - Provide a sample to any connected controller
  // - Do nothing and let some extern host pull the data from the node.
  // - JavaScript to process the data inside the browser.
  // - Feed it to some plugin (e.g. a display to show a chart)
}

//********************************************************************************
// Helper functions used in the webserver to access the cache data
//********************************************************************************

bool C016_startCSVdump() {
  ControllerCache.resetpeek();
  return ControllerCache.isInitialized();
}

String C016_getCacheFileName(bool& islast) {
  return ControllerCache.getPeekCacheFileName(islast);
}

bool C016_deleteOldestCacheBlock() {
  return ControllerCache.deleteOldestCacheBlock();
}

bool C016_getCSVline(
  unsigned long& timestamp,
  byte& controller_idx,
  byte& TaskIndex,
  byte& sensorType,
  byte& valueCount,
  float& val1,
  float& val2,
  float& val3,
  float& val4)
{
  C016_queue_element element;
  bool result = ControllerCache.peek((uint8_t*)&element, sizeof(element));
  timestamp = element.timestamp;
  controller_idx = element.controller_idx;
  TaskIndex = element.TaskIndex;
  sensorType = element.sensorType;
  valueCount = element.valueCount;
  val1 = element.values[0];
  val2 = element.values[1];
  val3 = element.values[2];
  val4 = element.values[3];
  return result;
}

#endif
