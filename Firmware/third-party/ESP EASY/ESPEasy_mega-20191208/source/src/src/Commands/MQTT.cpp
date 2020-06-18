#include "../../define_plugin_sets.h"
#include "../Globals/MQTT.h"
#include "../DataStructs/SchedulerTimers.h"
#ifdef USES_MQTT

#include "../Commands/MQTT.h"

#include "../../ESPEasy_common.h"
#include "../Commands/Common.h"
#include "../Globals/Settings.h"

#include "../../ESPEasy_fdwdecl.h"
#include "../../ESPEasy_Log.h"


String Command_MQTT_Retain(struct EventStruct *event, const char *Line)
{
  return Command_GetORSetBool(event, F("MQTT Retain:"),
                              Line,
                              (bool *)&Settings.MQTTRetainFlag,
                              1);
}

String Command_MQTT_UseUnitNameAsClientId(struct EventStruct *event, const char *Line)
{
  return Command_GetORSetBool(event, F("MQTT Use Unit Name as ClientId:"),
                              Line,
                              (bool *)&Settings.MQTTUseUnitNameAsClientId,
                              1);
}

String Command_MQTT_messageDelay(struct EventStruct *event, const char *Line)
{
  if (HasArgv(Line, 2)) {
    Settings.MessageDelay = event->Par1;
  }
  else {
    String result = F("MQTT message delay:");
    result += Settings.MessageDelay;
    serialPrintln();
    serialPrintln(result);
    return result;
  }
  return return_command_success();
}

String Command_MQTT_Publish(struct EventStruct *event, const char *Line)
{
  // ToDo TD-er: Not sure about this function, but at least it sends to an existing MQTTclient
  int enabledMqttController = firstEnabledMQTTController();

  if (enabledMqttController >= 0) {
    // Command structure:  Publish,<topic>,<value>
    String topic = parseStringKeepCase(Line, 2);
    String value = tolerantParseStringKeepCase(Line, 3);
    addLog(LOG_LEVEL_DEBUG, String(F("Publish: ")) + topic + value);

    if ((topic.length() > 0) && (value.length() > 0)) {
      // @giig1967g: if payload starts with '=' then treat it as a Formula and evaluate accordingly
      // The evaluated value is already present in event->Par2
      // FIXME TD-er: Is the evaluated value always present in event->Par2 ?
      // Should it already be evaluated, or should we evaluate it now?

      bool success = false;
      if (value[0] != '=') {
        success = MQTTpublish(enabledMqttController, topic.c_str(), value.c_str(), Settings.MQTTRetainFlag);
      }
      else {
        success = MQTTpublish(enabledMqttController, topic.c_str(), String(event->Par2).c_str(), Settings.MQTTRetainFlag);
      }
      if (success) {
        return return_command_success();
      }
    }
    return return_command_failed();
  }
  return F("No MQTT controller enabled");
}


boolean MQTTsubscribe(int controller_idx, const char* topic, boolean retained)
{
  if (MQTTclient.subscribe(topic)) {
    setIntervalTimerOverride(TIMER_MQTT, 10); // Make sure the MQTT is being processed as soon as possible.
    String log = F("Subscribed to: ");  log += topic;
    addLog(LOG_LEVEL_INFO, log);
    return true;
  }
  addLog(LOG_LEVEL_ERROR, F("MQTT : subscribe failed"));
  return false;
}

String Command_MQTT_Subscribe(struct EventStruct *event, const char* Line)
{
  if (MQTTclient.connected() ) {
    // ToDo TD-er: Not sure about this function, but at least it sends to an existing MQTTclient
    int enabledMqttController = firstEnabledMQTTController();
    if (enabledMqttController >= 0) {
      String eventName = Line;
      String topic = eventName.substring(10);
      if (!MQTTsubscribe(enabledMqttController, topic.c_str(), Settings.MQTTRetainFlag))
         return_command_failed();
      return_command_success();
    }
    return F("No MQTT controller enabled");
  }
  return return_not_connected();
}


#endif // ifdef USES_MQTT
