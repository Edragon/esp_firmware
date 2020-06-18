#ifdef USES_C002

// #######################################################################################################
// ########################### Controller Plugin 002: Domoticz MQTT ######################################
// #######################################################################################################

#define CPLUGIN_002
#define CPLUGIN_ID_002         2
#define CPLUGIN_NAME_002       "Domoticz MQTT"

#include <ArduinoJson.h>

bool CPlugin_002(byte function, struct EventStruct *event, String& string)
{
  bool success = false;

  switch (function)
  {
    case CPLUGIN_PROTOCOL_ADD:
    {
      Protocol[++protocolCount].Number     = CPLUGIN_ID_002;
      Protocol[protocolCount].usesMQTT     = true;
      Protocol[protocolCount].usesTemplate = true;
      Protocol[protocolCount].usesAccount  = true;
      Protocol[protocolCount].usesPassword = true;
      Protocol[protocolCount].defaultPort  = 1883;
      Protocol[protocolCount].usesID       = true;
      break;
    }

    case CPLUGIN_GET_DEVICENAME:
    {
      string = F(CPLUGIN_NAME_002);
      break;
    }

    case CPLUGIN_INIT:
    {
      MakeControllerSettings(ControllerSettings);
      LoadControllerSettings(event->ControllerIndex, ControllerSettings);
      MQTTDelayHandler.configureControllerSettings(ControllerSettings);
      break;
    }

    case CPLUGIN_PROTOCOL_TEMPLATE:
    {
      event->String1 = F("domoticz/out");
      event->String2 = F("domoticz/in");
      break;
    }

    case CPLUGIN_PROTOCOL_RECV:
    {
      // char json[512];
      // json[0] = 0;
      // event->String2.toCharArray(json, 512);
      // Find first enabled controller index with this protocol
      byte ControllerID = findFirstEnabledControllerWithId(CPLUGIN_ID_002);

      if (ControllerID < CONTROLLER_MAX) {
        DynamicJsonDocument root(512);
        deserializeJson(root, event->String2.c_str());

        if (!root.isNull())
        {
          unsigned int idx = root[F("idx")];
          float nvalue     = root[F("nvalue")];
          long  nvaluealt  = root[F("nvalue")];

          // const char* name = root["name"]; // Not used
          // const char* svalue = root["svalue"]; // Not used
          const char *svalue1 = root[F("svalue1")];

          // const char* svalue2 = root["svalue2"]; // Not used
          // const char* svalue3 = root["svalue3"]; // Not used
          const char *switchtype = root[F("switchType")]; // Expect "On/Off" or "dimmer"

          if (nvalue == 0) {
            nvalue = nvaluealt;
          }

          if ((int)switchtype == 0) {
            switchtype = "?";
          }

          for (taskIndex_t x = 0; x < TASKS_MAX; x++) {
            // We need the index of the controller we are: 0...CONTROLLER_MAX
            if (Settings.TaskDeviceEnabled[x] && (Settings.TaskDeviceID[ControllerID][x] == idx)) // get idx for our controller index
            {
              String action = "";

              switch (Settings.TaskDeviceNumber[x]) {
                case 1: // temp solution, if input switch, update state
                {
                  action  = F("inputSwitchState,");
                  action += x;
                  action += ',';
                  action += nvalue;
                  break;
                }
                case 29: // temp solution, if plugin 029, set gpio
                {
                  action = "";
                  int baseVar = x * VARS_PER_TASK;

                  if (strcasecmp_P(switchtype, PSTR("dimmer")) == 0)
                  {
                    int pwmValue = UserVar[baseVar];
                    action  = F("pwm,");
                    action += Settings.TaskDevicePin1[x];
                    action += ',';

                    switch ((int)nvalue)
                    {
                      case 0:
                        pwmValue         = 0;
                        UserVar[baseVar] = pwmValue;
                        break;
                      case 1:
                        pwmValue = UserVar[baseVar];
                        break;
                      case 2:
                        pwmValue         = 10 * atol(svalue1);
                        UserVar[baseVar] = pwmValue;
                        break;
                    }
                    action += pwmValue;
                  } else {
                    UserVar[baseVar] = nvalue;
                    action           = F("gpio,");
                    action          += Settings.TaskDevicePin1[x];
                    action          += ',';
                    action          += nvalue;
                  }
                  break;
                }
#if defined(USES_P088) || defined(USES_P115)
                case 88: // Send heatpump IR (P088) if IDX matches
                case 115: // Send heatpump IR (P115) if IDX matches
                {
                  action = F("heatpumpir,");
                  action += svalue1; // svalue1 is like 'gree,1,1,0,22,0,0'
                  break;
                }
#endif // USES_P088 || USES_P115
                default:
                  break;
              }

              if (action.length() > 0) {
                ExecuteCommand_plugin(x, VALUE_SOURCE_MQTT, action.c_str());

                // trigger rulesprocessing
                if (Settings.UseRules) {
                  struct EventStruct TempEvent;
                  TempEvent.TaskIndex = x;
                  parseCommandString(&TempEvent, action);
                  createRuleEvents(&TempEvent);
                }
              }
            }
          }
          LoadTaskSettings(event->TaskIndex);
        }
      }
      break;
    }

    case CPLUGIN_PROTOCOL_SEND:
    {
      if (event->idx != 0)
      {
        MakeControllerSettings(ControllerSettings);
        LoadControllerSettings(event->ControllerIndex, ControllerSettings);

        /*
                  if (!ControllerSettings.checkHostReachable(true)) {
                    success = false;
                    break;
                  }
         */

        DynamicJsonDocument root(200);
        root[F("idx")]  = event->idx;
        root[F("RSSI")] = mapRSSItoDomoticz();
          #if FEATURE_ADC_VCC
        root[F("Battery")] = mapVccToDomoticz();
          #endif // if FEATURE_ADC_VCC

        switch (event->sensorType)
        {
          case SENSOR_TYPE_SWITCH:
            root[F("command")] = String(F("switchlight"));

            if (UserVar[event->BaseVarIndex] == 0) {
              root[F("switchcmd")] = String(F("Off"));
            }
            else {
              root[F("switchcmd")] = String(F("On"));
            }
            break;
          case SENSOR_TYPE_DIMMER:
            root[F("command")] = String(F("switchlight"));

            if (UserVar[event->BaseVarIndex] == 0) {
              root[F("switchcmd")] = String(F("Off"));
            }
            else {
              root[F("Set%20Level")] = UserVar[event->BaseVarIndex];
            }
            break;

          case SENSOR_TYPE_SINGLE:
          case SENSOR_TYPE_LONG:
          case SENSOR_TYPE_DUAL:
          case SENSOR_TYPE_TRIPLE:
          case SENSOR_TYPE_QUAD:
          case SENSOR_TYPE_TEMP_HUM:
          case SENSOR_TYPE_TEMP_BARO:
          case SENSOR_TYPE_TEMP_EMPTY_BARO:
          case SENSOR_TYPE_TEMP_HUM_BARO:
          case SENSOR_TYPE_WIND:
          case SENSOR_TYPE_STRING:
          default:
            root[F("nvalue")] = 0;
            root[F("svalue")] = formatDomoticzSensorType(event);
            break;
        }

        String json;
        serializeJson(root, json);
#ifndef BUILD_NO_DEBUG
        String log = F("MQTT : ");
        log += json;
        addLog(LOG_LEVEL_DEBUG, log);
#endif // ifndef BUILD_NO_DEBUG

        String pubname = ControllerSettings.Publish;
        parseControllerVariables(pubname, event, false);

        if (!MQTTpublish(event->ControllerIndex, pubname.c_str(), json.c_str(), Settings.MQTTRetainFlag))
        {
          connectionFailures++;
        }
        else if (connectionFailures) {
          connectionFailures--;
        }
      } // if ixd !=0
      else
      {
        String log = F("MQTT : IDX cannot be zero!");
        addLog(LOG_LEVEL_ERROR, log);
      }
      break;
    }

    case CPLUGIN_FLUSH:
    {
      processMQTTdelayQueue();
      delay(0);
      break;
    }
  }
  return success;
}

#endif // ifdef USES_C002
