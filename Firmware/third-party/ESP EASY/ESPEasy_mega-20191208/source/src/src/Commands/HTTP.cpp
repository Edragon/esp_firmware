#include "../Commands/HTTP.h"

#include "../../_CPlugin_Helper.h"
#include "../Commands/Common.h"
#include "../../ESPEasy_Log.h"
#include "../../src/DataStructs/ControllerSettingsStruct.h"

#include "../../ESPEasy_fdwdecl.h"
#include "../../ESPEasy_common.h"



String Command_HTTP_SendToHTTP(struct EventStruct *event, const char* Line)
{
	if (WiFiConnected()) {
		String host = parseString(Line, 2);
		const int port = parseCommandArgumentInt(Line, 2);
		if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
			String log = F("SendToHTTP: Host: ");
			log += host;
			log += F(" port: ");
			log += port;
			addLog(LOG_LEVEL_DEBUG, log);
		}
		if (!port < 0 || port > 65535) return return_command_failed();
		// FIXME TD-er: This is not using the tolerant settings option.
    // String path = tolerantParseStringKeepCase(Line, 4);
		String path = parseStringToEndKeepCase(Line, 4);
#ifndef BUILD_NO_DEBUG
		if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
			String log = F("SendToHTTP: Path: ");
			log += path;
			addLog(LOG_LEVEL_DEBUG, log);
		}
#endif
		WiFiClient client;
		client.setTimeout(CONTROLLER_CLIENTTIMEOUT_DFLT);
		const bool connected = connectClient(client, host.c_str(), port);
		if (connected) {
			String hostportString = host;
			if (port != 0 && port != 80) {
				hostportString += ':';
				hostportString += port;
			}
			String request = do_create_http_request(hostportString, F("GET"), path);
#ifndef BUILD_NO_DEBUG
			addLog(LOG_LEVEL_DEBUG, request);
#endif
			send_via_http(F("Command_HTTP_SendToHTTP"), client, request, false);
		}
	}
	return return_command_success();
}
