#include "../DataStructs/ControllerSettingsStruct.h"

#include "../../ESPEasy_common.h"
#include "../../ESPEasy_fdwdecl.h"
#include "../DataStructs/ESPEasyLimits.h"


#include <IPAddress.h>
#include <WString.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>

ControllerSettingsStruct::ControllerSettingsStruct()
{
  reset();
}

void ControllerSettingsStruct::reset() {
  UseDNS                     = false;
  Port                       = 0;
  MinimalTimeBetweenMessages = CONTROLLER_DELAY_QUEUE_DELAY_DFLT;
  MaxQueueDepth              = CONTROLLER_DELAY_QUEUE_DEPTH_DFLT;
  MaxRetry                   = CONTROLLER_DELAY_QUEUE_RETRY_DFLT;
  DeleteOldest               = false;
  ClientTimeout              = CONTROLLER_CLIENTTIMEOUT_DFLT;
  MustCheckReply             = false;
  SampleSetInitiator         = INVALID_TASK_INDEX;

  for (byte i = 0; i < 4; ++i) {
    IP[i] = 0;
  }
  ZERO_FILL(HostName);
  ZERO_FILL(Publish);
  ZERO_FILL(Subscribe);
  ZERO_FILL(MQTTLwtTopic);
  ZERO_FILL(LWTMessageConnect);
  ZERO_FILL(LWTMessageDisconnect);
}

void ControllerSettingsStruct::validate() {
  if (Port > 65535) { Port = 0; }

  if ((MinimalTimeBetweenMessages < 1) ||  (MinimalTimeBetweenMessages > CONTROLLER_DELAY_QUEUE_DELAY_MAX)) {
    MinimalTimeBetweenMessages = CONTROLLER_DELAY_QUEUE_DELAY_DFLT;
  }

  if (MaxQueueDepth > CONTROLLER_DELAY_QUEUE_DEPTH_MAX) { MaxQueueDepth = CONTROLLER_DELAY_QUEUE_DEPTH_DFLT; }

  if (MaxRetry > CONTROLLER_DELAY_QUEUE_RETRY_MAX) { MaxRetry = CONTROLLER_DELAY_QUEUE_RETRY_MAX; }

  if (MaxQueueDepth == 0) { MaxQueueDepth = CONTROLLER_DELAY_QUEUE_DEPTH_DFLT; }

  if (MaxRetry == 0) { MaxRetry = CONTROLLER_DELAY_QUEUE_RETRY_DFLT; }

  if ((ClientTimeout < 10) || (ClientTimeout > CONTROLLER_CLIENTTIMEOUT_MAX)) {
    ClientTimeout = CONTROLLER_CLIENTTIMEOUT_DFLT;
  }
  ZERO_TERMINATE(HostName);
  ZERO_TERMINATE(Publish);
  ZERO_TERMINATE(Subscribe);
  ZERO_TERMINATE(MQTTLwtTopic);
  ZERO_TERMINATE(LWTMessageConnect);
  ZERO_TERMINATE(LWTMessageDisconnect);
}

IPAddress ControllerSettingsStruct::getIP() const {
  IPAddress host(IP[0], IP[1], IP[2], IP[3]);

  return host;
}

String ControllerSettingsStruct::getHost() const {
  if (UseDNS) {
    return HostName;
  }
  return getIP().toString();
}

void ControllerSettingsStruct::setHostname(const String& controllerhostname) {
  safe_strncpy(HostName, controllerhostname.c_str(), sizeof(HostName));
  updateIPcache();
}

boolean ControllerSettingsStruct::checkHostReachable(bool quick) {
  if (!WiFiConnected(10)) {
    return false; // Not connected, so no use in wasting time to connect to a host.
  }
  delay(1);       // Make sure the Watchdog will not trigger a reset.

  if (quick && ipSet()) { return true; }

  if (UseDNS) {
    if (!updateIPcache()) {
      return false;
    }
  }
  return hostReachable(getIP());
}

boolean ControllerSettingsStruct::connectToHost(WiFiClient& client) {
  if (!checkHostReachable(true)) {
    return false; // Host not reachable
  }
  byte retry     = 2;
  bool connected = false;

  while (retry > 0 && !connected) {
    --retry;

    // In case of domain name resolution error result can be negative.
    // https://github.com/esp8266/Arduino/blob/18f643c7e2d6a0da9d26ff2b14c94e6536ab78c1/libraries/Ethernet/src/Dns.cpp#L44
    // Thus must match the result with 1.
    connected = connectClient(client, getIP(), Port);

    if (connected) { return true; }

    if (!checkHostReachable(false)) {
      return false;
    }
  }
  return false;
}

// Returns 1 if successful, 0 if there was a problem resolving the hostname or port
int ControllerSettingsStruct::beginPacket(WiFiUDP& client) {
  if (!checkHostReachable(true)) {
    return 0; // Host not reachable
  }
  byte retry     = 2;
  int  connected = 0;

  while (retry > 0 && connected == 0) {
    --retry;
    connected = client.beginPacket(getIP(), Port);

    if (connected != 0) { return connected; }

    if (!checkHostReachable(false)) {
      return 0;
    }
    delay(10);
  }
  return 0;
}

String ControllerSettingsStruct::getHostPortString() const {
  String result = getHost();

  result += ":";
  result += Port;
  return result;
}

bool ControllerSettingsStruct::ipSet() {
  for (byte i = 0; i < 4; ++i) {
    if (IP[i] != 0) { return true; }
  }
  return false;
}

bool ControllerSettingsStruct::updateIPcache() {
  if (!UseDNS) {
    return true;
  }

  if (!WiFiConnected()) { return false; }
  IPAddress tmpIP;

  if (resolveHostByName(HostName, tmpIP)) {
    for (byte x = 0; x < 4; x++) {
      IP[x] = tmpIP[x];
    }
    return true;
  }
  return false;
}
