#include "src/Globals/Logging.h"
#include "src/Static/WebStaticData.h"

// ********************************************************************************
// Web Interface log page
// ********************************************************************************
void handle_log() {
  if (!isLoggedIn()) { return; }
  navMenuIndex = MENU_INDEX_TOOLS;

  TXBuffer.startStream();
  sendHeadandTail_stdtemplate(_HEAD);
  html_table_class_normal();

  #ifdef WEBSERVER_LOG
  TXBuffer += F("<TR><TH id=\"headline\" align=\"left\">Log");
  addCopyButton(F("copyText"), "", F("Copy log to clipboard"));
  TXBuffer += F(
    "</TR></table><div  id='current_loglevel' style='font-weight: bold;'>Logging: </div><div class='logviewer' id='copyText_1'></div>");
  TXBuffer += F("Autoscroll: ");
  addCheckBox(F("autoscroll"), true);
  TXBuffer += F("<BR></body>");

  html_add_script(true);
  TXBuffer += DATA_FETCH_AND_PARSE_LOG_JS;
  html_add_script_end();

  #else
  TXBuffer += F("Not included in build");
  #endif
  sendHeadandTail_stdtemplate(_TAIL);
  TXBuffer.endStream();
}

// ********************************************************************************
// Web Interface JSON log page
// ********************************************************************************
void handle_log_JSON() {
  if (!isLoggedIn()) { return; }
  #ifdef WEBSERVER_LOG
  TXBuffer.startJsonStream();
  String webrequest = WebServer.arg(F("view"));
  TXBuffer += F("{\"Log\": {");

  if (webrequest == F("legend")) {
    TXBuffer += F("\"Legend\": [");

    for (byte i = 0; i < LOG_LEVEL_NRELEMENTS; ++i) {
      if (i != 0) {
        TXBuffer += ',';
      }
      TXBuffer += '{';
      int loglevel;
      stream_next_json_object_value(F("label"), getLogLevelDisplayStringFromIndex(i, loglevel));
      stream_last_json_object_value(F("loglevel"), String(loglevel));
    }
    TXBuffer += F("],\n");
  }
  TXBuffer += F("\"Entries\": [");
  bool logLinesAvailable       = true;
  int  nrEntries               = 0;
  unsigned long firstTimeStamp = 0;
  unsigned long lastTimeStamp  = 0;

  while (logLinesAvailable) {
    String reply = Logging.get_logjson_formatted(logLinesAvailable, lastTimeStamp);

    if (reply.length() > 0) {
      TXBuffer += reply;

      if (nrEntries == 0) {
        firstTimeStamp = lastTimeStamp;
      }
      ++nrEntries;
    }

    // Do we need to do something here and maybe limit number of lines at once?
  }
  TXBuffer += F("],\n");
  long logTimeSpan       = timeDiff(firstTimeStamp, lastTimeStamp);
  long refreshSuggestion = 1000;
  long newOptimum        = 1000;

  if ((nrEntries > 2) && (logTimeSpan > 1)) {
    // May need to lower the TTL for refresh when time needed
    // to fill half the log is lower than current TTL
    newOptimum = logTimeSpan * (LOG_STRUCT_MESSAGE_LINES / 2);
    newOptimum = newOptimum / (nrEntries - 1);
  }

  if (newOptimum < refreshSuggestion) { refreshSuggestion = newOptimum; }

  if (refreshSuggestion < 100) {
    // Reload times no lower than 100 msec.
    refreshSuggestion = 100;
  }
  stream_next_json_object_value(F("TTL"),                 String(refreshSuggestion));
  stream_next_json_object_value(F("timeHalfBuffer"),      String(newOptimum));
  stream_next_json_object_value(F("nrEntries"),           String(nrEntries));
  stream_next_json_object_value(F("SettingsWebLogLevel"), String(Settings.WebLogLevel));
  stream_last_json_object_value(F("logTimeSpan"), String(logTimeSpan));
  TXBuffer += F("}\n");
  TXBuffer.endStream();
  updateLogLevelCache();

  #else 
  handleNotFound();
  #endif
}
