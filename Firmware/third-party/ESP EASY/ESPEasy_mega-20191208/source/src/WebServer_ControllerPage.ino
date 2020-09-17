#ifdef WEBSERVER_CONTROLLERS

// ********************************************************************************
// Web Interface controller page
// ********************************************************************************
void handle_controllers() {
  checkRAM(F("handle_controllers"));

  if (!isLoggedIn()) { return; }
  navMenuIndex = MENU_INDEX_CONTROLLERS;
  TXBuffer.startStream();
  sendHeadandTail_stdtemplate(_HEAD);

  byte controllerindex     = getFormItemInt(F("index"), 0);
  boolean controllerNotSet = controllerindex == 0;
  --controllerindex; // Index in URL is starting from 1, but starting from 0 in the array.

  const int protocol = getFormItemInt(F("protocol"), -1);

  // submitted data
  if ((protocol != -1) && !controllerNotSet)
  {
    MakeControllerSettings(ControllerSettings);
    bool mustInit = false;

    if (Settings.Protocol[controllerindex] != protocol)
    {
      // Protocol has changed.
      Settings.Protocol[controllerindex] = protocol;

      // there is a protocol selected?
      if (protocol != 0)
      {
        mustInit = true;
        handle_controllers_clearLoadDefaults(controllerindex, ControllerSettings);
      }
    }

    // subitted same protocol
    else
    {
      // there is a protocol selected
      if (protocol != 0)
      {
        mustInit = true;
        handle_controllers_CopySubmittedSettings(controllerindex, ControllerSettings);
      }
    }
    addHtmlError(SaveControllerSettings(controllerindex, ControllerSettings));
    addHtmlError(SaveSettings());

    if (mustInit) {
      // Init controller plugin using the new settings.
      byte ProtocolIndex = getProtocolIndex(Settings.Protocol[controllerindex]);
      struct EventStruct TempEvent;
      TempEvent.ControllerIndex = controllerindex;
      TempEvent.ProtocolIndex   = ProtocolIndex;
      String dummy;
      CPluginCall(ProtocolIndex, CPLUGIN_INIT, &TempEvent, dummy);
    }
  }

  html_add_form();

  if (controllerNotSet)
  {
    handle_controllers_ShowAllControllersTable();
  }
  else
  {
    handle_controllers_ControllerSettingsPage(controllerindex);
  }

  sendHeadandTail_stdtemplate(_TAIL);
  TXBuffer.endStream();
}

// ********************************************************************************
// Selected controller has changed.
// Clear all Controller settings and load some defaults
// ********************************************************************************
void handle_controllers_clearLoadDefaults(byte controllerindex, ControllerSettingsStruct& ControllerSettings)
{
  // Protocol has changed and it was not an empty one.
  // reset (some) default-settings
  byte ProtocolIndex = getProtocolIndex(Settings.Protocol[controllerindex]);

  ControllerSettings.reset();
  ControllerSettings.Port = Protocol[ProtocolIndex].defaultPort;

  // Load some templates from the controller.
  struct EventStruct TempEvent;

  if (Protocol[ProtocolIndex].usesTemplate) {
    String dummy;
    CPluginCall(ProtocolIndex, CPLUGIN_PROTOCOL_TEMPLATE, &TempEvent, dummy);
  }
  safe_strncpy(ControllerSettings.Subscribe,            TempEvent.String1.c_str(), sizeof(ControllerSettings.Subscribe));
  safe_strncpy(ControllerSettings.Publish,              TempEvent.String2.c_str(), sizeof(ControllerSettings.Publish));
  safe_strncpy(ControllerSettings.MQTTLwtTopic,         TempEvent.String3.c_str(), sizeof(ControllerSettings.MQTTLwtTopic));
  safe_strncpy(ControllerSettings.LWTMessageConnect,    TempEvent.String4.c_str(), sizeof(ControllerSettings.LWTMessageConnect));
  safe_strncpy(ControllerSettings.LWTMessageDisconnect, TempEvent.String5.c_str(), sizeof(ControllerSettings.LWTMessageDisconnect));

  // NOTE: do not enable controller by default, give user a change to enter sensible values first
  Settings.ControllerEnabled[controllerindex] = false;

  // not resetted to default (for convenience)
  // SecuritySettings.ControllerUser[controllerindex]
  // SecuritySettings.ControllerPassword[controllerindex]

  ClearCustomControllerSettings(controllerindex);
}

// ********************************************************************************
// Collect all submitted form data and store in the ControllerSettings
// ********************************************************************************
void handle_controllers_CopySubmittedSettings(byte controllerindex, ControllerSettingsStruct& ControllerSettings)
{
  // copy all settings to controller settings struct
  for (int parameterIdx = 1; parameterIdx <= CONTROLLER_ENABLED; ++parameterIdx) {
    saveControllerParameterForm(ControllerSettings, controllerindex, parameterIdx);
  }

  byte ProtocolIndex = getProtocolIndex(Settings.Protocol[controllerindex]);
  struct EventStruct TempEvent;
  TempEvent.ControllerIndex = controllerindex;
  TempEvent.ProtocolIndex   = ProtocolIndex;

  // Call controller plugin to save CustomControllerSettings
  String dummy;
  CPluginCall(ProtocolIndex, CPLUGIN_WEBFORM_SAVE, &TempEvent, dummy);
}

// ********************************************************************************
// Show table with all selected controllers
// ********************************************************************************
void handle_controllers_ShowAllControllersTable()
{
  html_table_class_multirow();
  html_TR();
  html_table_header("",           70);
  html_table_header("Nr",         50);
  html_table_header(F("Enabled"), 100);
  html_table_header(F("Protocol"));
  html_table_header("Host");
  html_table_header("Port");

  MakeControllerSettings(ControllerSettings);

  for (byte x = 0; x < CONTROLLER_MAX; x++)
  {
    LoadControllerSettings(x, ControllerSettings);
    html_TR_TD();
    html_add_button_prefix();
    TXBuffer += F("controllers?index=");
    TXBuffer += x + 1;
    TXBuffer += F("'>Edit</a>");
    html_TD();
    TXBuffer += getControllerSymbol(x);
    html_TD();

    if (Settings.Protocol[x] != 0)
    {
      addEnabled(Settings.ControllerEnabled[x]);

      html_TD();
      byte ProtocolIndex = getProtocolIndex(Settings.Protocol[x]);
      TXBuffer += getCPluginNameFromProtocolIndex(ProtocolIndex);
      html_TD();
      {
        String hostDescription;
        CPluginCall(ProtocolIndex, CPLUGIN_WEBFORM_SHOW_HOST_CONFIG, 0, hostDescription);

        if (hostDescription.length() != 0) {
          TXBuffer += hostDescription;
        } else {
          TXBuffer += ControllerSettings.getHost();
        }
      }

      html_TD();
      TXBuffer += ControllerSettings.Port;
    }
    else {
      html_TD(3);
    }
  }
  html_end_table();
  html_end_form();
}

// ********************************************************************************
// Show the controller settings page
// ********************************************************************************
void handle_controllers_ControllerSettingsPage(byte controllerindex)
{
  // Show controller settings page
  html_table_class_normal();
  addFormHeader(F("Controller Settings"));
  addRowLabel(F("Protocol"));
  byte choice = Settings.Protocol[controllerindex];
  addSelector_Head(F("protocol"), true);
  addSelector_Item(F("- Standalone -"), 0, false, false, "");

  for (byte x = 0; x <= protocolCount; x++)
  {
    boolean disabled = false; // !((controllerindex == 0) || !Protocol[x].usesMQTT);
    addSelector_Item(getCPluginNameFromProtocolIndex(x),
                     Protocol[x].Number,
                     choice == Protocol[x].Number,
                     disabled,
                     "");
  }
  addSelector_Foot();

  addHelpButton(F("EasyProtocols"));

  if (Settings.Protocol[controllerindex])
  {
    MakeControllerSettings(ControllerSettings);
    LoadControllerSettings(controllerindex, ControllerSettings);

    byte ProtocolIndex = getProtocolIndex(Settings.Protocol[controllerindex]);

    if (!Protocol[ProtocolIndex].Custom)
    {
      if (Protocol[ProtocolIndex].usesHost) {
        addControllerParameterForm(ControllerSettings, controllerindex, CONTROLLER_USE_DNS);

        if (ControllerSettings.UseDNS)
        {
          addControllerParameterForm(ControllerSettings, controllerindex, CONTROLLER_HOSTNAME);
        }
        else
        {
          addControllerParameterForm(ControllerSettings, controllerindex, CONTROLLER_IP);
        }
      }
      addControllerParameterForm(ControllerSettings, controllerindex, CONTROLLER_PORT);

      if (Protocol[ProtocolIndex].usesQueue) {
        addTableSeparator(F("Controller Queue"), 2, 3);
        addControllerParameterForm(ControllerSettings, controllerindex, CONTROLLER_MIN_SEND_INTERVAL);
        addControllerParameterForm(ControllerSettings, controllerindex, CONTROLLER_MAX_QUEUE_DEPTH);
        addControllerParameterForm(ControllerSettings, controllerindex, CONTROLLER_MAX_RETRIES);
        addControllerParameterForm(ControllerSettings, controllerindex, CONTROLLER_FULL_QUEUE_ACTION);
      }
      addControllerParameterForm(ControllerSettings, controllerindex, CONTROLLER_CHECK_REPLY);
      addControllerParameterForm(ControllerSettings, controllerindex, CONTROLLER_TIMEOUT);

      if (Protocol[ProtocolIndex].usesSampleSets) {
        addControllerParameterForm(ControllerSettings, controllerindex, CONTROLLER_SAMPLE_SET_INITIATOR);
      }

      if (Protocol[ProtocolIndex].usesAccount || Protocol[ProtocolIndex].usesPassword) {
        addTableSeparator(F("Credentials"), 2, 3);
      }

      if (Protocol[ProtocolIndex].usesAccount)
      {
        addControllerParameterForm(ControllerSettings, controllerindex, CONTROLLER_USER);
      }

      if (Protocol[ProtocolIndex].usesPassword)
      {
        addControllerParameterForm(ControllerSettings, controllerindex, CONTROLLER_PASS);
      }

      if (Protocol[ProtocolIndex].usesMQTT) {
        addTableSeparator(F("MQTT"), 2, 3);
      }

      if (Protocol[ProtocolIndex].usesTemplate || Protocol[ProtocolIndex].usesMQTT)
      {
        addControllerParameterForm(ControllerSettings, controllerindex, CONTROLLER_SUBSCRIBE);
        addControllerParameterForm(ControllerSettings, controllerindex, CONTROLLER_PUBLISH);
      }

      if (Protocol[ProtocolIndex].usesMQTT)
      {
        addControllerParameterForm(ControllerSettings, controllerindex, CONTROLLER_LWT_TOPIC);
        addControllerParameterForm(ControllerSettings, controllerindex, CONTROLLER_LWT_CONNECT_MESSAGE);
        addControllerParameterForm(ControllerSettings, controllerindex, CONTROLLER_LWT_DISCONNECT_MESSAGE);
      }
    }
    {
      // Load controller specific settings
      struct EventStruct TempEvent;
      TempEvent.ControllerIndex = controllerindex;
      TempEvent.ProtocolIndex   = ProtocolIndex;

      String webformLoadString;
      CPluginCall(ProtocolIndex, CPLUGIN_WEBFORM_LOAD, &TempEvent, webformLoadString);

      if (webformLoadString.length() > 0) {
        addHtmlError(F("Bug in CPLUGIN_WEBFORM_LOAD, should not append to string, use addHtml() instead"));
      }
    }
    addControllerParameterForm(ControllerSettings, controllerindex, CONTROLLER_ENABLED);
  }

  addFormSeparator(2);
  html_TR_TD();
  html_TD();
  addButton(F("controllers"), F("Close"));
  addSubmitButton();
  html_end_table();
  html_end_form();
}

#endif // ifdef WEBSERVER_CONTROLLERS
