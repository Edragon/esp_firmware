//********************************************************************************
// Initialize all Controller CPlugins that where defined earlier
// and initialize the function call pointer into the CCPlugin array
//********************************************************************************

static const char ADDCPLUGIN_ERROR[] PROGMEM = "System: Error - Too many C-Plugins";

// Because of compiler-bug (multiline defines gives an error if file ending is CRLF) the define is striped to a single line
/*
#define ADDCPLUGIN(NNN) \
  if (x < CPLUGIN_MAX) \
   { \
    CPlugin_id[x] = CPLUGIN_ID_##NNN; \
    CPlugin_ptr[x++] = &CPlugin_##NNN; \
   } \
  else \
    addLog(LOG_LEVEL_ERROR, FPSTR(ADDCPLUGIN_ERROR));
*/
#define ADDCPLUGIN(NNN) if (x < CPLUGIN_MAX) { CPlugin_id[x] = CPLUGIN_ID_##NNN; CPlugin_ptr[x++] = &CPlugin_##NNN; } else addLog(LOG_LEVEL_ERROR, FPSTR(ADDCPLUGIN_ERROR));

void CPluginInit(void)
{
  byte x;

  // Clear pointer table for all plugins
  for (x = 0; x < CPLUGIN_MAX; x++)
  {
    CPlugin_ptr[x] = 0;
    CPlugin_id[x] = 0;
  }

  x = 0;

#ifdef CPLUGIN_001
  ADDCPLUGIN(001)
#endif

#ifdef CPLUGIN_002
  ADDCPLUGIN(002)
#endif

#ifdef CPLUGIN_003
  ADDCPLUGIN(003)
#endif

#ifdef CPLUGIN_004
  ADDCPLUGIN(004)
#endif

#ifdef CPLUGIN_005
  ADDCPLUGIN(005)
#endif

#ifdef CPLUGIN_006
  ADDCPLUGIN(006)
#endif

#ifdef CPLUGIN_007
  ADDCPLUGIN(007)
#endif

#ifdef CPLUGIN_008
  ADDCPLUGIN(008)
#endif

#ifdef CPLUGIN_009
  ADDCPLUGIN(009)
#endif

#ifdef CPLUGIN_010
  ADDCPLUGIN(010)
#endif

#ifdef CPLUGIN_011
  ADDCPLUGIN(011)
#endif

#ifdef CPLUGIN_012
  ADDCPLUGIN(012)
#endif

#ifdef CPLUGIN_013
  ADDCPLUGIN(013)
#endif

#ifdef CPLUGIN_014
  ADDCPLUGIN(014)
#endif

#ifdef CPLUGIN_015
  ADDCPLUGIN(015)
#endif

#ifdef CPLUGIN_016
  ADDCPLUGIN(016)
#endif

#ifdef CPLUGIN_017
  ADDCPLUGIN(017)
#endif

#ifdef CPLUGIN_018
  ADDCPLUGIN(018)
#endif

#ifdef CPLUGIN_019
  ADDCPLUGIN(019)
#endif

#ifdef CPLUGIN_020
  ADDCPLUGIN(020)
#endif

#ifdef CPLUGIN_021
  ADDCPLUGIN(021)
#endif

#ifdef CPLUGIN_022
  ADDCPLUGIN(022)
#endif

#ifdef CPLUGIN_023
  ADDCPLUGIN(023)
#endif

#ifdef CPLUGIN_024
  ADDCPLUGIN(024)
#endif

#ifdef CPLUGIN_025
  ADDCPLUGIN(025)
#endif

  CPluginCall(CPLUGIN_PROTOCOL_ADD, 0);
  CPluginCall(CPLUGIN_INIT, 0);
}

bool validCPluginID(byte pluginNumber) {
  return CPlugin_id[pluginNumber] != 0 && pluginNumber < CPLUGIN_MAX;
}

bool CPluginCall(byte pluginNumber, byte Function, struct EventStruct *event, String& str) {
  if (validCPluginID(pluginNumber)) {
    START_TIMER;
    bool ret = CPlugin_ptr[pluginNumber](Function, event, str);
    STOP_TIMER_CONTROLLER(pluginNumber, Function);
    return ret;
  }
  return false;
}

bool CPluginCall(byte Function, struct EventStruct *event, String& str)
{
  struct EventStruct TempEvent;

  if (event == 0) {
    event = &TempEvent;
  }

  switch (Function)
  {
    // Unconditional calls to all plugins
    case CPLUGIN_PROTOCOL_ADD:

      for (byte x = 0; x < CPLUGIN_MAX; x++) {
        if (CPlugin_id[x] != 0) {
          const unsigned int next_ProtocolIndex = protocolCount + 2;

          if (next_ProtocolIndex > Protocol.size()) {
            // Increase with 8 to get some compromise between number of resizes and wasted space
            unsigned int newSize = Protocol.size();
            newSize = newSize + 8 - (newSize % 8);
            Protocol.resize(newSize);
          }
          checkRAM(F("CPluginCallADD"), x);
          String dummy;
          CPluginCall(x, Function, event, dummy);
        }
      }
      return true;
      break;


    // calls to active plugins
    case CPLUGIN_INIT:
    case CPLUGIN_UDP_IN:
    case CPLUGIN_INTERVAL:      // calls to send stats information
    case CPLUGIN_GOT_CONNECTED: // calls to send autodetect information
    case CPLUGIN_GOT_INVALID:   // calls to mark unit as invalid
    case CPLUGIN_FLUSH:

      for (byte x = 0; x < CONTROLLER_MAX; x++) {
        if ((Settings.Protocol[x] != 0) && Settings.ControllerEnabled[x]) {
          event->ProtocolIndex = getProtocolIndex(Settings.Protocol[x]);
          String dummy;
          CPluginCall(event->ProtocolIndex, Function, event, dummy);
        }
      }
      return true;
      break;

    case CPLUGIN_ACKNOWLEDGE: // calls to send acknolages back to controller

      for (byte x = 0; x < CONTROLLER_MAX; x++) {
        if ((Settings.Protocol[x] != 0) && Settings.ControllerEnabled[x]) {
          event->ProtocolIndex = getProtocolIndex(Settings.Protocol[x]);
          CPluginCall(event->ProtocolIndex, Function, event, str);
        }
      }
      return true;
      break;
  }

  return false;
}

// Check if there is any controller enabled.
bool anyControllerEnabled() {
  for (byte i = 0; i < CONTROLLER_MAX; i++) {
    if ((Settings.Protocol[i] != 0) && Settings.ControllerEnabled[i]) {
      return true;
    }
  }
  return false;
}

// Find first enabled controller index with this protocol
byte findFirstEnabledControllerWithId(byte cpluginid) {
  for (byte i = 0; i < CONTROLLER_MAX; i++) {
    if ((Settings.Protocol[i] == cpluginid) && Settings.ControllerEnabled[i]) {
      return i;
    }
  }
  return CONTROLLER_MAX;
}

bool CPluginCall(byte Function, struct EventStruct *event) {
  String dummy;

  return CPluginCall(Function, event, dummy);
}

String getCPluginNameFromProtocolIndex(byte ProtocolIndex) {
  String controllerName;

  CPluginCall(ProtocolIndex, CPLUGIN_GET_DEVICENAME, nullptr, controllerName);
  return controllerName;
}

/********************************************************************************************\
   Find protocol index corresponding to protocol setting
 \*********************************************************************************************/
byte getProtocolIndex(byte Number)
{
  for (byte x = 0; x <= protocolCount; x++) {
    if (Protocol[x].Number == Number) {
      return x;
    }
  }
  return 0;
}
