#include <Arduino.h>

#include "ESPEasy-Globals.h"


NotificationStruct Notification[NPLUGIN_MAX];

#if defined(ESP32)
  int8_t ledChannelPin[16];
#endif


I2Cdev i2cdev;




// Setup DNS, only used if the ESP has no valid WiFi config
const byte DNS_PORT = 53;
IPAddress apIP(DEFAULT_AP_IP);
DNSServer dnsServer;
bool dnsServerActive = false;

//NTP status
bool statusNTPInitialized = false;

// udp protocol stuff (syslog, global sync, node info list, ntp time)
WiFiUDP portUDP;





int protocolCount = -1;
int notificationCount = -1;

boolean printToWeb = false;
String printWebString;
boolean printToWebJSON = false;

rulesTimerStatus RulesTimer[RULES_TIMER_MAX];

msecTimerHandlerStruct msecTimerHandler;

unsigned long timer_gratuitous_arp_interval = 5000;
unsigned long timermqtt_interval = 250;
unsigned long lastSend = 0;
unsigned long lastWeb = 0;
byte cmd_within_mainloop = 0;
unsigned long wdcounter = 0;
unsigned long timerAPoff = 0;    // Timer to check whether the AP mode should be disabled (0 = disabled)
unsigned long timerAPstart = 0;  // Timer to start AP mode, started when no valid network is detected.
unsigned long timerAwakeFromDeepSleep = 0;
unsigned long last_system_event_run = 0;

#if FEATURE_ADC_VCC
float vcc = -1.0;
#endif

boolean WebLoggedIn = false;
int WebLoggedInTimer = 300;


bool (*CPlugin_ptr[CPLUGIN_MAX])(byte, struct EventStruct*, String&);
byte CPlugin_id[CPLUGIN_MAX];

boolean (*NPlugin_ptr[NPLUGIN_MAX])(byte, struct EventStruct*, String&);
byte NPlugin_id[NPLUGIN_MAX];

String dummyString = "";  // FIXME @TD-er  This may take a lot of memory over time, since long-lived Strings only tend to grow.





bool webserverRunning(false);
bool webserver_init(false);


EventQueueStruct eventQueue;




bool shouldReboot(false);
bool firstLoop(true);

boolean activeRuleSets[RULESETS_MAX];

boolean UseRTOSMultitasking(false);
