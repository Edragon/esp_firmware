
// ********************************************************************************
// Web Interface server web file from SPIFFS
// ********************************************************************************
bool loadFromFS(boolean spiffs, String path) {
  // path is a deepcopy, since it will be changed here.
  checkRAM(F("loadFromFS"));

  if (!isLoggedIn()) { return false; }

  statusLED(true);

  String dataType = F("text/plain");

  if (path.endsWith("/")) { path += F("index.htm"); }

  if (path.endsWith(F(".src"))) { path = path.substring(0, path.lastIndexOf(".")); }
  else if (path.endsWith(F(".htm")) || path.endsWith(F(".htm.gz"))) { dataType = F("text/html"); }
  else if (path.endsWith(F(".css")) || path.endsWith(F(".css.gz"))) { dataType = F("text/css"); }
  else if (path.endsWith(F(".js")) || path.endsWith(F(".js.gz"))) { dataType = F("application/javascript"); }
  else if (path.endsWith(F(".png")) || path.endsWith(F(".png.gz"))) { dataType = F("image/png"); }
  else if (path.endsWith(F(".gif")) || path.endsWith(F(".gif.gz"))) { dataType = F("image/gif"); }
  else if (path.endsWith(F(".jpg")) || path.endsWith(F(".jpg.gz"))) { dataType = F("image/jpeg"); }
  else if (path.endsWith(F(".ico"))) { dataType = F("image/x-icon"); }
  else if (path.endsWith(F(".txt")) ||
           path.endsWith(F(".dat"))) { dataType = F("application/octet-stream"); }
  else if (path.endsWith(F(".esp"))) { return handle_custom(path); }

#ifndef BUILD_NO_DEBUG

  if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
    String log = F("HTML : Request file ");
    log += path;
    addLog(LOG_LEVEL_DEBUG, log);
  }
#endif // ifndef BUILD_NO_DEBUG

#if !defined(ESP32)
  path = path.substring(1);
#endif // if !defined(ESP32)

  if (spiffs)
  {
    fs::File dataFile = tryOpenFile(path.c_str(), "r");

    if (!dataFile) {
      return false;
    }

    // prevent reloading stuff on every click
    WebServer.sendHeader(F("Cache-Control"), F("max-age=3600, public"));
    WebServer.sendHeader(F("Vary"),          "*");
    WebServer.sendHeader(F("ETag"),          F("\"2.0.0\""));

    if (path.endsWith(F(".dat"))) {
      WebServer.sendHeader(F("Content-Disposition"), F("attachment;"));
    }

    WebServer.streamFile(dataFile, dataType);
    dataFile.close();
  }
  else
  {
#ifdef FEATURE_SD
    File dataFile = SD.open(path.c_str());

    if (!dataFile) {
      return false;
    }

    if (path.endsWith(F(".DAT"))) {
      WebServer.sendHeader(F("Content-Disposition"), F("attachment;"));
    }
    WebServer.streamFile(dataFile, dataType);
    dataFile.close();
#else // ifdef FEATURE_SD

    // File from SD requested, but no SD support.
    return false;
#endif // ifdef FEATURE_SD
  }
  statusLED(true);
  return true;
}
