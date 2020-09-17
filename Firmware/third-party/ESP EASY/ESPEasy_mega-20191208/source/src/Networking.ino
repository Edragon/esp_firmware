#include "src/Globals/Nodes.h"
#include "src/Globals/ESPEasyWiFiEvent.h"

// Generic Networking routines

// Syslog
// UDP system messaging
// SSDP
//  #if LWIP_VERSION_MAJOR == 2
#define IPADDR2STR(addr) (uint8_t)((uint32_t)addr &  0xFF), (uint8_t)(((uint32_t)addr >> 8) &  0xFF), \
  (uint8_t)(((uint32_t)addr >> 16) &  0xFF), (uint8_t)(((uint32_t)addr >> 24) &  0xFF)

//  #endif

#include <lwip/netif.h>
#ifdef ESP8266
  # if !defined(ARDUINO_ESP8266_RELEASE_2_4_0) && !defined(ARDUINO_ESP8266_RELEASE_2_3_0)
    #  define SUPPORT_ARP
  # endif // if !defined(ARDUINO_ESP8266_RELEASE_2_4_0) && !defined(ARDUINO_ESP8266_RELEASE_2_3_0)
#endif    // ifdef ESP8266

#ifdef ESP32
# define SUPPORT_ARP
#endif // ifdef ESP32

#ifdef SUPPORT_ARP
# include <lwip/etharp.h>

# ifdef ESP32
#  include <lwip/etharp.h>
#  include <lwip/tcpip.h>

void _etharp_gratuitous_func(struct netif *netif) {
  etharp_gratuitous(netif);
}

void etharp_gratuitous_r(struct netif *netif) {
  tcpip_callback_with_block((tcpip_callback_fn)_etharp_gratuitous_func, netif, 0);
}

# endif // ifdef ESP32

#endif  // ifdef SUPPORT_ARP

#ifdef USE_SETTINGS_ARCHIVE
# ifdef ESP8266
#  include <ESP8266HTTPClient.h>
# endif // ifdef ESP8266
# ifdef ESP32
#  include "HTTPClient.h"
# endif // ifdef ESP32
#endif  // USE_SETTINGS_ARCHIVE


#include "src/DataStructs/EventValueSource.h"

/*********************************************************************************************\
   Syslog client
\*********************************************************************************************/
void syslog(byte logLevel, const char *message)
{
  if ((Settings.Syslog_IP[0] != 0) && WiFiConnected())
  {
    IPAddress broadcastIP(Settings.Syslog_IP[0], Settings.Syslog_IP[1], Settings.Syslog_IP[2], Settings.Syslog_IP[3]);
    portUDP.beginPacket(broadcastIP, 514);
    char str[256];
    str[0] = 0;
    byte prio = Settings.SyslogFacility * 8;

    if (logLevel == LOG_LEVEL_ERROR) {
      prio += 3; // syslog error
    }
    else if (logLevel == LOG_LEVEL_INFO) {
      prio += 5; // syslog notice
    }
    else {
      prio += 7;
    }

    // An RFC3164 compliant message must be formated like :  "<PRIO>[TimeStamp ]Hostname TaskName: Message"

    // Using Settings.Name as the Hostname (Hostname must NOT content space)
    snprintf_P(str, sizeof(str), PSTR("<%u>%s EspEasy: %s"), prio, Settings.Name, message);

    // Using Setting.Unit to build a Hostname
    // snprintf_P(str, sizeof(str), PSTR("<7>EspEasy_%u ESP: %s"), Settings.Unit, message);
    #if defined(ESP8266)
    portUDP.write(str);
    #endif // if defined(ESP8266)
    #if defined(ESP32)
    portUDP.write((uint8_t *)str, strlen(str));
    #endif // if defined(ESP32)
    portUDP.endPacket();
  }
}

/*********************************************************************************************\
   Check UDP messages (ESPEasy propiertary protocol)
\*********************************************************************************************/
boolean runningUPDCheck = false;
void checkUDP()
{
  if (Settings.UDPPort == 0) {
    return;
  }

  if (runningUPDCheck) {
    return;
  }

  runningUPDCheck = true;

  // UDP events
  int packetSize = portUDP.parsePacket();

  if (packetSize > 0 /*&& portUDP.remotePort() == Settings.UDPPort*/)
  {
    statusLED(true);

    IPAddress remoteIP = portUDP.remoteIP();

    if (portUDP.remotePort() == 123)
    {
      // unexpected NTP reply, drop for now...
      runningUPDCheck = false;
      return;
    }

    if ((packetSize >= 2) && (packetSize < UDP_PACKETSIZE_MAX)) {
      // Allocate buffer to process packet.
      std::vector<char> packetBuffer;
      packetBuffer.resize(packetSize + 1);
      memset(&packetBuffer[0], 0, packetSize + 1);

      int len = portUDP.read(&packetBuffer[0], packetSize);

      if (len >= 2) {
        if (packetBuffer[0] != 255)
        {
          packetBuffer[len] = 0;
          addLog(LOG_LEVEL_DEBUG, &packetBuffer[0]);
          ExecuteCommand_all(VALUE_SOURCE_SYSTEM, &packetBuffer[0]);
        }
        else
        {
          // binary data!
          switch (packetBuffer[1])
          {
            case 1: // sysinfo message
            {
              if (len < 13) {
                break;
              }
              byte unit = packetBuffer[12];
#ifndef BUILD_NO_DEBUG
              byte mac[6];
              byte ip[4];

              for (byte x = 0; x < 6; x++) {
                mac[x] = packetBuffer[x + 2];
              }

              for (byte x = 0; x < 4; x++) {
                ip[x] = packetBuffer[x + 8];
              }
#endif // ifndef BUILD_NO_DEBUG
              Nodes[unit].age = 0; // Create a new element when not present
              NodesMap::iterator it = Nodes.find(unit);

              if (it != Nodes.end()) {
                for (byte x = 0; x < 4; x++) {
                  it->second.ip[x] = packetBuffer[x + 8];
                }
                it->second.age = 0; // reset 'age counter'

                if (len >= 41)      // extended packet size
                {
                  it->second.build = packetBuffer[13] + 256 * packetBuffer[14];
                  char tmpNodeName[26] = { 0 };
                  memcpy(&tmpNodeName[0], reinterpret_cast<byte *>(&packetBuffer[15]), 25);
                  tmpNodeName[25]     = 0;
                  it->second.nodeName = tmpNodeName;
                  it->second.nodeName.trim();
                  it->second.nodeType = packetBuffer[40];
                }
              }

#ifndef BUILD_NO_DEBUG

              if (loglevelActiveFor(LOG_LEVEL_DEBUG_MORE)) {
                char macaddress[20];
                formatMAC(mac, macaddress);
                char log[80] = { 0 };
                sprintf_P(log, PSTR("UDP  : %s,%s,%u"), macaddress, formatIP(ip).c_str(), unit);
                addLog(LOG_LEVEL_DEBUG_MORE, log);
              }
#endif // ifndef BUILD_NO_DEBUG
              break;
            }

            default:
            {
              struct EventStruct TempEvent;
              TempEvent.Data = reinterpret_cast<byte *>(&packetBuffer[0]);
              TempEvent.Par1 = remoteIP[3];
              TempEvent.Par2 = len;
              String dummy;
              PluginCall(PLUGIN_UDP_IN, &TempEvent, dummy);
              CPluginCall(CPLUGIN_UDP_IN, &TempEvent);
              break;
            }
          }
        }
      }
    }
  }
  #if defined(ESP32) // testing
  portUDP.flush();
  #endif // if defined(ESP32)
  runningUPDCheck = false;
}

/*********************************************************************************************\
   Send event using UDP message
\*********************************************************************************************/
void SendUDPCommand(byte destUnit, const char *data, byte dataLength)
{
  if (!WiFiConnected(10)) {
    return;
  }

  if (destUnit != 0)
  {
    sendUDP(destUnit, (const byte *)data, dataLength);
    delay(10);
  } else {
    for (NodesMap::iterator it = Nodes.begin(); it != Nodes.end(); ++it) {
      if (it->first != Settings.Unit) {
        sendUDP(it->first, (const byte *)data, dataLength);
        delay(10);
      }
    }
  }
  delay(50);
}

/*********************************************************************************************\
   Send UDP message (unit 255=broadcast)
\*********************************************************************************************/
void sendUDP(byte unit, const byte *data, byte size)
{
  if (!WiFiConnected(10)) {
    return;
  }

  IPAddress remoteNodeIP;

  if (unit == 255) {
    remoteNodeIP = { 255, 255, 255, 255 };
  }
  else {
    NodesMap::iterator it = Nodes.find(unit);

    if (it == Nodes.end()) {
      return;
    }

    if (it->second.ip[0] == 0) {
      return;
    }
    remoteNodeIP = it->second.ip;
  }

#ifndef BUILD_NO_DEBUG

  if (loglevelActiveFor(LOG_LEVEL_DEBUG_MORE)) {
    String log = F("UDP  : Send UDP message to ");
    log += unit;
    addLog(LOG_LEVEL_DEBUG_MORE, log);
  }
#endif // ifndef BUILD_NO_DEBUG

  statusLED(true);
  portUDP.beginPacket(remoteNodeIP, Settings.UDPPort);
  portUDP.write(data, size);
  portUDP.endPacket();
}

/*********************************************************************************************\
   Refresh aging for remote units, drop if too old...
\*********************************************************************************************/
void refreshNodeList()
{
  bool mustSendGratuitousARP = false;

  for (NodesMap::iterator it = Nodes.begin(); it != Nodes.end();) {
    bool mustRemove = true;

    if (it->second.ip[0] != 0) {
      if (it->second.age > 8) {
        // Increase frequency sending ARP requests for 2 minutes
        mustSendGratuitousARP = true;
      }

      if (it->second.age < 10) {
        it->second.age++;
        mustRemove = false;
        ++it;
      }
    }

    if (mustRemove) {
      it = Nodes.erase(it);
    }
  }

  if (mustSendGratuitousARP) {
    sendGratuitousARP_now();
  }
}

/*********************************************************************************************\
   Broadcast system info to other nodes. (to update node lists)
\*********************************************************************************************/
void sendSysInfoUDP(byte repeats)
{
  if ((Settings.UDPPort == 0) || !WiFiConnected(10)) {
    return;
  }

  // TODO: make a nice struct of it and clean up
  // 1 byte 'binary token 255'
  // 1 byte id '1'
  // 6 byte mac
  // 4 byte ip
  // 1 byte unit
  // 2 byte build
  // 25 char name
  // 1 byte node type id

  // send my info to the world...
#ifndef BUILD_NO_DEBUG
  addLog(LOG_LEVEL_DEBUG_MORE, F("UDP  : Send Sysinfo message"));
#endif // ifndef BUILD_NO_DEBUG

  for (byte counter = 0; counter < repeats; counter++)
  {
    uint8_t  mac[]   = { 0, 0, 0, 0, 0, 0 };
    uint8_t *macread = WiFi.macAddress(mac);
    byte     data[80];
    data[0] = 255;
    data[1] = 1;

    for (byte x = 0; x < 6; x++) {
      data[x + 2] = macread[x];
    }
    IPAddress ip = WiFi.localIP();

    for (byte x = 0; x < 4; x++) {
      data[x + 8] = ip[x];
    }
    data[12] = Settings.Unit;
    data[13] = Settings.Build & 0xff;
    data[14] = Settings.Build >> 8;
    memcpy((byte *)data + 15, Settings.Name, 25);
    data[40] = NODE_TYPE_ID;
    statusLED(true);

    IPAddress broadcastIP(255, 255, 255, 255);
    portUDP.beginPacket(broadcastIP, Settings.UDPPort);
    portUDP.write(data, 80);
    portUDP.endPacket();

    if (counter < (repeats - 1)) {
      delay(500);
    }
  }

  Nodes[Settings.Unit].age = 0; // Create new node when not already present.
  // store my own info also in the list
  NodesMap::iterator it = Nodes.find(Settings.Unit);

  if (it != Nodes.end())
  {
    IPAddress ip = WiFi.localIP();

    for (byte x = 0; x < 4; x++) {
      it->second.ip[x] = ip[x];
    }
    it->second.age      = 0;
    it->second.build    = Settings.Build;
    it->second.nodeType = NODE_TYPE_ID;
  }
}

#if defined(ESP8266)

# ifdef USES_SSDP

/********************************************************************************************\
   Respond to HTTP XML requests for SSDP information
 \*********************************************************************************************/
void SSDP_schema(WiFiClient& client) {
  if (!WiFiConnected(10)) {
    return;
  }

  const IPAddress ip     = WiFi.localIP();
  const uint32_t  chipId = ESP.getChipId();
  char uuid[64];
  sprintf_P(uuid, PSTR("38323636-4558-4dda-9188-cda0e6%02x%02x%02x"),
            (uint16_t)((chipId >> 16) & 0xff),
            (uint16_t)((chipId >>  8) & 0xff),
            (uint16_t)chipId        & 0xff);

  String ssdp_schema = F(
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/xml\r\n"
    "Connection: close\r\n"
    "Access-Control-Allow-Origin: *\r\n"
    "\r\n"
    "<?xml version=\"1.0\"?>"
    "<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
    "<specVersion>"
    "<major>1</major>"
    "<minor>0</minor>"
    "</specVersion>"
    "<URLBase>http://");
  ssdp_schema += formatIP(ip);
  ssdp_schema += F(":80/</URLBase>"
                   "<device>"
                   "<deviceType>urn:schemas-upnp-org:device:BinaryLight:1</deviceType>"
                   "<friendlyName>");
  ssdp_schema += Settings.Name;
  ssdp_schema += F("</friendlyName>"
                   "<presentationURL>/</presentationURL>"
                   "<serialNumber>");
  ssdp_schema += ESP.getChipId();
  ssdp_schema += F("</serialNumber>"
                   "<modelName>ESP Easy</modelName>"
                   "<modelNumber>");
  ssdp_schema += F(BUILD_GIT);
  ssdp_schema += F("</modelNumber>"
                   "<modelURL>http://www.letscontrolit.com</modelURL>"
                   "<manufacturer>http://www.letscontrolit.com</manufacturer>"
                   "<manufacturerURL>http://www.letscontrolit.com</manufacturerURL>"
                   "<UDN>uuid:");
  ssdp_schema += uuid;
  ssdp_schema += F("</UDN></device>"
                   "</root>\r\n"
                   "\r\n");

  client.printf(ssdp_schema.c_str());
}

/********************************************************************************************\
   Global SSDP stuff
 \*********************************************************************************************/
typedef enum {
  NONE,
  SEARCH,
  NOTIFY
} ssdp_method_t;

UdpContext *_server;

IPAddress _respondToAddr;
uint16_t  _respondToPort;

bool _pending;
unsigned short _delay;
unsigned long  _process_time;
unsigned long  _notify_time;

#  define SSDP_INTERVAL     1200
#  define SSDP_PORT         1900
#  define SSDP_METHOD_SIZE  10
#  define SSDP_URI_SIZE     2
#  define SSDP_BUFFER_SIZE  64
#  define SSDP_MULTICAST_TTL 2

static const IPAddress SSDP_MULTICAST_ADDR(239, 255, 255, 250);


/********************************************************************************************\
   Launch SSDP listener and send initial notify
 \*********************************************************************************************/
bool SSDP_begin() {
  _pending = false;

  if (_server) {
    _server->unref();
    _server = 0;
  }

  _server = new UdpContext;
  _server->ref();

  ip_addr_t ifaddr;
  ifaddr.addr = WiFi.localIP();
  ip_addr_t multicast_addr;
  multicast_addr.addr = (uint32_t)SSDP_MULTICAST_ADDR;

  if (igmp_joingroup(&ifaddr, &multicast_addr) != ERR_OK) {
    return false;
  }

#  ifdef CORE_POST_2_5_0

  // Core 2.5.0 changed the signature of some UdpContext function.
  if (!_server->listen(IP_ADDR_ANY, SSDP_PORT)) {
    return false;
  }

  _server->setMulticastInterface(&ifaddr);
  _server->setMulticastTTL(SSDP_MULTICAST_TTL);
  _server->onRx(&SSDP_update);

  if (!_server->connect(&multicast_addr, SSDP_PORT)) {
    return false;
  }
#  else // ifdef CORE_POST_2_5_0

  if (!_server->listen(*IP_ADDR_ANY, SSDP_PORT)) {
    return false;
  }

  _server->setMulticastInterface(ifaddr);
  _server->setMulticastTTL(SSDP_MULTICAST_TTL);
  _server->onRx(&SSDP_update);

  if (!_server->connect(multicast_addr, SSDP_PORT)) {
    return false;
  }
#  endif // ifdef CORE_POST_2_5_0

  SSDP_update();

  return true;
}

/********************************************************************************************\
   Send SSDP messages (notify & responses)
 \*********************************************************************************************/
void SSDP_send(byte method) {
  uint32_t ip = WiFi.localIP();

  // FIXME TD-er: Why create String objects of these flashstrings?
  String _ssdp_response_template = F(
    "HTTP/1.1 200 OK\r\n"
    "EXT:\r\n"
    "ST: upnp:rootdevice\r\n");

  String _ssdp_notify_template = F(
    "NOTIFY * HTTP/1.1\r\n"
    "HOST: 239.255.255.250:1900\r\n"
    "NT: upnp:rootdevice\r\n"
    "NTS: ssdp:alive\r\n");

  String _ssdp_packet_template = F(
    "%s"                                           // _ssdp_response_template / _ssdp_notify_template
    "CACHE-CONTROL: max-age=%u\r\n"                // SSDP_INTERVAL
    "SERVER: Arduino/1.0 UPNP/1.1 ESPEasy/%u\r\n"  // _modelNumber
    "USN: uuid:%s\r\n"                             // _uuid
    "LOCATION: http://%u.%u.%u.%u:80/ssdp.xml\r\n" // WiFi.localIP(),
    "\r\n");
  {
    char uuid[64]   = { 0 };
    uint32_t chipId = ESP.getChipId();
    sprintf_P(uuid, PSTR("38323636-4558-4dda-9188-cda0e6%02x%02x%02x"),
              (uint16_t)((chipId >> 16) & 0xff),
              (uint16_t)((chipId >>  8) & 0xff),
              (uint16_t)chipId        & 0xff);

    char *buffer = new char[1460]();
    int   len    = snprintf(buffer, 1460,
                            _ssdp_packet_template.c_str(),
                            (method == 0) ? _ssdp_response_template.c_str() : _ssdp_notify_template.c_str(),
                            SSDP_INTERVAL,
                            Settings.Build,
                            uuid,
                            IPADDR2STR(&ip)
                            );

    _server->append(buffer, len);
    delete[] buffer;
  }

  ip_addr_t remoteAddr;
  uint16_t  remotePort;

  if (method == 0) {
    remoteAddr.addr = _respondToAddr;
    remotePort      = _respondToPort;
  } else {
    remoteAddr.addr = SSDP_MULTICAST_ADDR;
    remotePort      = SSDP_PORT;
  }
  _server->send(&remoteAddr, remotePort);
  statusLED(true);
}

/********************************************************************************************\
   SSDP message processing
 \*********************************************************************************************/
void SSDP_update() {
  if (!_pending && _server->next()) {
    ssdp_method_t method = NONE;

    _respondToAddr = _server->getRemoteAddress();
    _respondToPort = _server->getRemotePort();

    typedef enum { METHOD, URI, PROTO, KEY, VALUE, ABORT } states;
    states state = METHOD;

    typedef enum { START, MAN, ST, MX } headers;
    headers header = START;

    uint8_t cursor = 0;
    uint8_t cr     = 0;

    char buffer[SSDP_BUFFER_SIZE] = { 0 };

    while (_server->getSize() > 0) {
      char c = _server->read();

      (c == '\r' || c == '\n') ? cr++ : cr = 0;

      switch (state) {
        case METHOD:

          if (c == ' ') {
            if (strcmp_P(buffer, PSTR("M-SEARCH")) == 0) { method = SEARCH; }
            else if (strcmp_P(buffer, PSTR("NOTIFY")) == 0) { method = NOTIFY; }

            if (method == NONE) { state = ABORT; }
            else { state = URI; }
            cursor = 0;
          } else if (cursor < SSDP_METHOD_SIZE - 1) {
            buffer[cursor++] = c;
            buffer[cursor]   = '\0';
          }
          break;
        case URI:

          if (c == ' ') {
            if (strcmp(buffer, "*")) { state = ABORT; }
            else { state = PROTO; }
            cursor = 0;
          } else if (cursor < SSDP_URI_SIZE - 1) {
            buffer[cursor++] = c;
            buffer[cursor]   = '\0';
          }
          break;
        case PROTO:

          if (cr == 2) {
            state  = KEY;
            cursor = 0;
          }
          break;
        case KEY:

          if (cr == 4) {
            _pending      = true;
            _process_time = millis();
          }
          else if (c == ' ') {
            cursor = 0;
            state  = VALUE;
          }
          else if ((c != '\r') && (c != '\n') && (c != ':') && (cursor < SSDP_BUFFER_SIZE - 1)) {
            buffer[cursor++] = c;
            buffer[cursor]   = '\0';
          }
          break;
        case VALUE:

          if (cr == 2) {
            switch (header) {
              case START:
                break;
              case MAN:
                break;
              case ST:

                if (strcmp_P(buffer, PSTR("ssdp:all"))) {
                  state = ABORT;
                }

                // if the search type matches our type, we should respond instead of ABORT
                if (strcmp_P(buffer, PSTR("urn:schemas-upnp-org:device:BinaryLight:1")) == 0) {
                  _pending      = true;
                  _process_time = millis();
                  state         = KEY;
                }
                break;
              case MX:
                _delay = random(0, atoi(buffer)) * 1000L;
                break;
            }

            if (state != ABORT) {
              state  = KEY;
              header = START;
              cursor = 0;
            }
          } else if ((c != '\r') && (c != '\n')) {
            if (header == START) {
              if (strncmp(buffer, "MA", 2) == 0) { header = MAN; }
              else if (strcmp(buffer, "ST") == 0) { header = ST; }
              else if (strcmp(buffer, "MX") == 0) { header = MX; }
            }

            if (cursor < SSDP_BUFFER_SIZE - 1) {
              buffer[cursor++] = c;
              buffer[cursor]   = '\0';
            }
          }
          break;
        case ABORT:
          _pending = false; _delay = 0;
          break;
      }
    }
  }

  if (_pending && timeOutReached(_process_time + _delay)) {
    _pending = false; _delay = 0;
    SSDP_send(NONE);
  } else if ((_notify_time == 0) || timeOutReached(_notify_time + (SSDP_INTERVAL * 1000L))) {
    _notify_time = millis();
    SSDP_send(NOTIFY);
  }

  if (_pending) {
    while (_server->next()) {
      _server->flush();
    }
  }
}

# endif // ifdef USES_SSDP
#endif // if defined(ESP8266)


// ********************************************************************************
// Return subnet range of WiFi.
// ********************************************************************************
bool getSubnetRange(IPAddress& low, IPAddress& high)
{
  if (wifiStatus < ESPEASY_WIFI_GOT_IP) {
    return false;
  }
  const IPAddress ip     = WiFi.localIP();
  const IPAddress subnet = WiFi.subnetMask();
  low  = ip;
  high = ip;

  // Compute subnet range.
  for (byte i = 0; i < 4; ++i) {
    if (subnet[i] != 255) {
      low[i]  = low[i] & subnet[i];
      high[i] = high[i] | ~subnet[i];
    }
  }
  return true;
}

// ********************************************************************************
// Functions to test and handle network/client connectivity.
// ********************************************************************************

#ifdef CORE_POST_2_5_0
# include <AddrList.h>
#endif // ifdef CORE_POST_2_5_0


bool hasIPaddr() {
#ifdef CORE_POST_2_5_0
  bool configured = false;

  for (auto addr : addrList) {
    if ((configured = (!addr.isLocal() && (addr.ifnumber() == STATION_IF)))) {
      /*
         Serial.printf("STA: IF='%s' hostname='%s' addr= %s\n",
                    addr.ifname().c_str(),
                    addr.ifhostname(),
                    addr.toString().c_str());
       */
      break;
    }
  }
  return configured;
#else // ifdef CORE_POST_2_5_0
  return WiFi.isConnected();
#endif // ifdef CORE_POST_2_5_0
}

// Check WiFi connection. Maximum timeout 500 msec.
bool WiFiConnected(uint32_t timeout_ms) {
  uint32_t timer     = millis() + (timeout_ms > 500 ? 500 : timeout_ms);
  uint32_t min_delay = timeout_ms / 20;

  if (min_delay < 10) {
    delay(0); // Allow at least once time for backgroundtasks
    min_delay = 10;
  }

  // Apparently something needs network, perform check to see if it is ready now.
  while (!WiFiConnected()) {
    if (timeOutReached(timer)) {
      return false;
    }
    delay(min_delay); // Allow the backgroundtasks to continue procesing.
  }
  return true;
}

bool hostReachable(const IPAddress& ip) {
  if (!WiFiConnected()) { return false; }

  return true; // Disabled ping as requested here:
  // https://github.com/letscontrolit/ESPEasy/issues/1494#issuecomment-397872538

  /*
     // Only do 1 ping at a time to return early
     byte retry = 3;
     while (retry > 0) {
   #if defined(ESP8266)
      if (Ping.ping(ip, 1)) return true;
   #endif
   #if defined(ESP32)
     if (ping_start(ip, 4, 0, 0, 5)) return true;
   #endif
      delay(50);
      --retry;
     }
     if (loglevelActiveFor(LOG_LEVEL_ERROR)) {
      String log = F("Host unreachable: ");
      log += formatIP(ip);
      addLog(LOG_LEVEL_ERROR, log);
     }
     if (ip[1] == 0 && ip[2] == 0 && ip[3] == 0) {
      // Work-around to fix connected but not able to communicate.
      addLog(LOG_LEVEL_ERROR, F("Wifi  : Detected strange behavior, reconnect wifi."));
      WifiDisconnect();
     }
     logConnectionStatus();
     return false;
   */
}

bool connectClient(WiFiClient& client, const char *hostname, uint16_t port) {
  IPAddress ip;

  if (resolveHostByName(hostname, ip)) {
    return connectClient(client, ip, port);
  }
  return false;
}

bool connectClient(WiFiClient& client, IPAddress ip, uint16_t port)
{
  START_TIMER;

  if (!WiFiConnected()) {
    return false;
  }
  bool connected = (client.connect(ip, port) == 1);
  yield();

  if (!connected) {
    sendGratuitousARP_now();
  }
  STOP_TIMER(CONNECT_CLIENT_STATS);
#if defined(ESP32) || defined(ARDUINO_ESP8266_RELEASE_2_3_0) || defined(ARDUINO_ESP8266_RELEASE_2_4_0)
#else

  if (connected) {
    client.keepAlive(); // Use default keep alive values
  }
#endif // if defined(ESP32) || defined(ARDUINO_ESP8266_RELEASE_2_3_0) || defined(ARDUINO_ESP8266_RELEASE_2_4_0)
  return connected;
}

bool resolveHostByName(const char *aHostname, IPAddress& aResult) {
  START_TIMER;

  if (!WiFiConnected()) {
    return false;
  }
#if defined(ARDUINO_ESP8266_RELEASE_2_3_0) || defined(ESP32)
  bool resolvedIP = WiFi.hostByName(aHostname, aResult) == 1;
#else // if defined(ARDUINO_ESP8266_RELEASE_2_3_0) || defined(ESP32)
  bool resolvedIP = WiFi.hostByName(aHostname, aResult, CONTROLLER_CLIENTTIMEOUT_DFLT) == 1;
#endif // if defined(ARDUINO_ESP8266_RELEASE_2_3_0) || defined(ESP32)
  yield();

  if (!resolvedIP) {
    sendGratuitousARP_now();
  }
  STOP_TIMER(HOST_BY_NAME_STATS);
  return resolvedIP;
}

bool hostReachable(const String& hostname) {
  IPAddress remote_addr;

  if (resolveHostByName(hostname.c_str(), remote_addr)) {
    return hostReachable(remote_addr);
  }
  String log = F("Hostname cannot be resolved: ");
  log += hostname;
  addLog(LOG_LEVEL_ERROR, log);
  return false;
}

// Create a random port for the UDP connection.
// Return true when successful.
bool beginWiFiUDP_randomPort(WiFiUDP& udp) {
  if (!WiFiConnected()) {
    return false;
  }
  unsigned int attempts = 3;

  while (attempts > 0) {
    --attempts;
    long port = random(1025, 65535);

    if (udp.begin(port) != 0) {
      return true;
    }
  }
  return false;
}

void sendGratuitousARP() {
  if (!WiFiConnected()) {
    return;
  }
#ifdef SUPPORT_ARP

  // See https://github.com/letscontrolit/ESPEasy/issues/2374
  START_TIMER;
  netif *n = netif_list;

  while (n) {
    if ((n->hwaddr_len == ETH_HWADDR_LEN) &&
        (n->flags & NETIF_FLAG_ETHARP) &&
        ((n->flags & NETIF_FLAG_LINK_UP) && (n->flags & NETIF_FLAG_UP))) {
      # ifdef ESP32
      etharp_gratuitous_r(n);
      # else // ifdef ESP32
      etharp_gratuitous(n);
      # endif // ifdef ESP32
    }
    n = n->next;
  }
  STOP_TIMER(GRAT_ARP_STATS);
#endif // ifdef SUPPORT_ARP
}

bool splitHostPortString(const String& hostPortString, String& host, uint16_t& port) {
  port = 80; // Some default
  int index_colon = hostPortString.indexOf(':');

  if (index_colon >= 0) {
    int port_tmp;

    if (!validIntFromString(hostPortString.substring(index_colon + 1), port_tmp)) {
      return false;
    }

    if ((port_tmp < 0) || (port_tmp > 65535)) { return false; }
    port = port_tmp;
    host = hostPortString.substring(0, index_colon);
  } else {
    // No port nr defined.
    host = hostPortString;
  }
  return true;
}

// Split a full URL like "http://hostname:port/path/file.htm"
// Return value is everything after the hostname:port section (including /)
String splitURL(const String& fullURL, String& host, uint16_t& port, String& file) {
  int starthost = fullURL.indexOf(F("//"));

  if (starthost == -1) {
    starthost = 0;
  } else {
    starthost += 2;
  }
  int endhost = fullURL.indexOf('/', starthost);
  splitHostPortString(fullURL.substring(starthost, endhost), host, port);
  int startfile = fullURL.lastIndexOf('/');

  if (startfile >= 0) {
    file = fullURL.substring(startfile);
  }
  return fullURL.substring(endhost);
}

#ifdef USE_SETTINGS_ARCHIVE

// Download a file from a given URL and save to a local file named "file_save"
// If the URL ends with a /, the file part will be assumed the same as file_save.
// If file_save is empty, the file part from the URL will be used as local file name.
// Return true when successful.
bool downloadFile(const String& url, String file_save) {
  String error;

  return downloadFile(url, file_save, "", "", error);
}

bool downloadFile(const String& url, String file_save, const String& user, const String& pass, String& error) {
  String   host, file;
  uint16_t port;
  String   uri = splitURL(url, host, port, file);

  if (file_save.length() == 0) {
    file_save = file;
  } else if ((file.length() == 0) && uri.endsWith("/")) {
    // file = file_save;
    uri += file_save;
  }

  if (loglevelActiveFor(LOG_LEVEL_INFO)) {
    String log = F("downloadFile: URL: ");
    log += url;
    log += F(" decoded: ");
    log += host;
    log += ':';
    log += port;
    log += uri;
    addLog(LOG_LEVEL_ERROR, log);
  }

  if (file_save.length() == 0) {
    error = F("Empty filename");
    addLog(LOG_LEVEL_ERROR, error);
    return false;
  }

  if (fileExists(file_save)) {
    error = F("File exists");
    addLog(LOG_LEVEL_ERROR, error);
    return false;
  }
  unsigned long timeout = millis() + 2000;
  WiFiClient    client;
  HTTPClient    http;
  http.begin(client, host, port, uri);
  {
    if ((user.length() > 0) && (pass.length() > 0)) {
      http.setAuthorization(user.c_str(), pass.c_str());
    }

    /*
       String authHeader = get_auth_header(user, pass);

       if (authHeader.length() > 0) {
       http.setAuthorization(authHeader.c_str());
       }
     */
  }
  int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK) {
    error  = F("HTTP code: ");
    error += httpCode;
    addLog(LOG_LEVEL_ERROR, error);
    return false;
  }

  long len = http.getSize();
  File f   = SPIFFS.open(file_save, "w");

  if (f) {
    uint8_t buff[128];
    size_t  bytesWritten = 0;

    // get tcp stream
    WiFiClient *stream = &client;

    // read all data from server
    while (http.connected() && (len > 0 || len == -1)) {
      // read up to 128 byte
      size_t c = stream->readBytes(buff, std::min((size_t)len, sizeof(buff)));

      if (c > 0) {
        timeout = millis() + 2000;

        if (f.write(buff, c) != c) {
          error  = F("Error saving file, ");
          error += bytesWritten;
          error += F(" Bytes written");
          addLog(LOG_LEVEL_ERROR, error);
          http.end();
          return false;
        }
        bytesWritten += c;

        if (len > 0) { len -= c; }
      }

      if (timeOutReached(timeout)) {
        error = F("Timeout");
        addLog(LOG_LEVEL_ERROR, error);
        delay(0);
        http.end();
        return false;
      }
      delay(0);
    }
    f.close();
    http.end();
    addLog(LOG_LEVEL_INFO, F("downloadFile: Success"));
    return true;
  }
  error = F("Failed to open file for writing");
  addLog(LOG_LEVEL_ERROR, error);
  return false;
}

#endif // USE_SETTINGS_ARCHIVE
