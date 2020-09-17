#ifdef WEBSERVER_DEVICES

#include "src/Globals/Nodes.h"
#include "src/Globals/Device.h"
#include "src/Globals/Plugins.h"
#include "src/Static/WebStaticData.h"


void handle_devices() {
  checkRAM(F("handle_devices"));

  if (!isLoggedIn()) { return; }
  navMenuIndex = MENU_INDEX_DEVICES;
  TXBuffer.startStream();
  sendHeadandTail_stdtemplate(_HEAD);


  // char tmpString[41];


  // String taskindex = WebServer.arg(F("index"));

  pluginID_t taskdevicenumber;

  if (WebServer.hasArg(F("del"))) {
    taskdevicenumber = 0;
  }
  else {
    taskdevicenumber = getFormItemInt(F("TDNUM"), 0);
  }


  // String taskdeviceid[CONTROLLER_MAX];
  // String taskdevicepin1 = WebServer.arg(F("taskdevicepin1"));   // "taskdevicepin*" should not be changed because it is uses by plugins
  // and expected to be saved by this code
  // String taskdevicepin2 = WebServer.arg(F("taskdevicepin2"));
  // String taskdevicepin3 = WebServer.arg(F("taskdevicepin3"));
  // String taskdevicepin1pullup = WebServer.arg(F("TDPPU"));
  // String taskdevicepin1inversed = WebServer.arg(F("TDPI"));
  // String taskdevicename = WebServer.arg(F("TDN"));
  // String taskdeviceport = WebServer.arg(F("TDP"));
  // String taskdeviceformula[VARS_PER_TASK];
  // String taskdevicevaluename[VARS_PER_TASK];
  // String taskdevicevaluedecimals[VARS_PER_TASK];
  // String taskdevicesenddata[CONTROLLER_MAX];
  // String taskdeviceglobalsync = WebServer.arg(F("TDGS"));
  // String taskdeviceenabled = WebServer.arg(F("TDE"));

  // for (byte varNr = 0; varNr < VARS_PER_TASK; varNr++)
  // {
  //   char argc[25];
  //   String arg = F("TDF");
  //   arg += varNr + 1;
  //   arg.toCharArray(argc, 25);
  //   taskdeviceformula[varNr] = WebServer.arg(argc);
  //
  //   arg = F("TDVN");
  //   arg += varNr + 1;
  //   arg.toCharArray(argc, 25);
  //   taskdevicevaluename[varNr] = WebServer.arg(argc);
  //
  //   arg = F("TDVD");
  //   arg += varNr + 1;
  //   arg.toCharArray(argc, 25);
  //   taskdevicevaluedecimals[varNr] = WebServer.arg(argc);
  // }

  // for (byte controllerNr = 0; controllerNr < CONTROLLER_MAX; controllerNr++)
  // {
  //   char argc[25];
  //   String arg = F("TDID");
  //   arg += controllerNr + 1;
  //   arg.toCharArray(argc, 25);
  //   taskdeviceid[controllerNr] = WebServer.arg(argc);
  //
  //   arg = F("TDSD");
  //   arg += controllerNr + 1;
  //   arg.toCharArray(argc, 25);
  //   taskdevicesenddata[controllerNr] = WebServer.arg(argc);
  // }

  byte page = getFormItemInt(F("page"), 0);

  if (page == 0) {
    page = 1;
  }
  byte setpage = getFormItemInt(F("setpage"), 0);

  if (setpage > 0)
  {
    if (setpage <= (TASKS_MAX / TASKS_PER_PAGE)) {
      page = setpage;
    }
    else {
      page = TASKS_MAX / TASKS_PER_PAGE;
    }
  }
  const int edit = getFormItemInt(F("edit"), 0);

  // taskIndex in the URL is 1 ... TASKS_MAX
  // For use in other functions, set it to 0 ... (TASKS_MAX - 1)
  taskIndex_t taskIndex       = getFormItemInt(F("index"), 0);
  boolean     taskIndexNotSet = taskIndex == 0;

  if (!taskIndexNotSet) {
    --taskIndex;
    LoadTaskSettings(taskIndex);       // Make sure ExtraTaskSettings are up-to-date
  }

  // FIXME TD-er: Might have to clear any caches here.
  if ((edit != 0) && !taskIndexNotSet) // when form submitted
  {
    if (Settings.TaskDeviceNumber[taskIndex] != taskdevicenumber)
    {
      // change of device: cleanup old device and reset default settings
      setTaskDevice_to_TaskIndex(taskdevicenumber, taskIndex);
    }
    else if (taskdevicenumber != 0) // save settings
    {
      handle_devices_CopySubmittedSettings(taskIndex, taskdevicenumber);
    }

    if (taskdevicenumber != 0) {
      // Task index has a task device number, so it makes sense to save.
      // N.B. When calling delete, the settings were already saved.
      addHtmlError(SaveTaskSettings(taskIndex));
      addHtmlError(SaveSettings());

      if (Settings.TaskDeviceEnabled[taskIndex]) {
        struct EventStruct TempEvent;
        TempEvent.TaskIndex = taskIndex;
        String dummy;
        PluginCall(PLUGIN_INIT, &TempEvent, dummy);
      }
    }
  }

  // show all tasks as table
  if (taskIndexNotSet)
  {
    handle_devicess_ShowAllTasksTable(page);
  }

  // Show edit form if a specific entry is chosen with the edit button
  else
  {
    handle_devices_TaskSettingsPage(taskIndex, page);
  }

  checkRAM(F("handle_devices"));
#ifndef BUILD_NO_DEBUG

  if (loglevelActiveFor(LOG_LEVEL_DEBUG_DEV)) {
    String log = F("DEBUG: String size:");
    log += String(TXBuffer.sentBytes);
    addLog(LOG_LEVEL_DEBUG_DEV, log);
  }
#endif // ifndef BUILD_NO_DEBUG
  sendHeadandTail_stdtemplate(_TAIL);
  TXBuffer.endStream();
}

// ********************************************************************************
// Add a device select dropdown list
// TODO TD-er: Add JavaScript filter:
//             https://www.w3schools.com/howto/howto_js_filter_dropdown.asp
// ********************************************************************************
void addDeviceSelect(const String& name,  int choice)
{
  String deviceName;

  addSelector_Head(name, true);
  addSelector_Item(F("- None -"), 0, false, false, "");

  for (byte x = 0; x <= deviceCount; x++)
  {
    const deviceIndex_t deviceIndex = DeviceIndex_sorted[x];
    if (validDeviceIndex(deviceIndex)) {
      const pluginID_t    pluginID    = DeviceIndex_to_Plugin_id[deviceIndex];

      if (validPluginID(pluginID)) {
        deviceName = getPluginNameFromDeviceIndex(deviceIndex);


  #ifdef PLUGIN_BUILD_DEV
        pluginID_t num    = DeviceIndex_to_Plugin_id[deviceIndex];
        String     plugin = "P";

        if (num < 10) { plugin += '0'; }

        if (num < 100) { plugin += '0'; }
        plugin    += num;
        plugin    += F(" - ");
        deviceName = plugin + deviceName;
  #endif // ifdef PLUGIN_BUILD_DEV

        addSelector_Item(deviceName,
                        Device[deviceIndex].Number,
                        choice == Device[deviceIndex].Number,
                        false,
                        "");
      }
    }
  }
  addSelector_Foot();
}


// ********************************************************************************
// Collect all submitted form data and store the task settings
// ********************************************************************************
void handle_devices_CopySubmittedSettings(taskIndex_t taskIndex, pluginID_t taskdevicenumber)
{
  if (!validTaskIndex(taskIndex)) { return; }
  const deviceIndex_t DeviceIndex = getDeviceIndex(taskdevicenumber);

  if (!validDeviceIndex(DeviceIndex)) { return; }

  unsigned long taskdevicetimer = getFormItemInt(F("TDT"), 0);
  Settings.TaskDeviceNumber[taskIndex] = taskdevicenumber;


  int pin1 = -1;
  int pin2 = -1;
  int pin3 = -1;
  update_whenset_FormItemInt(F("taskdevicepin1"), pin1);
  update_whenset_FormItemInt(F("taskdevicepin2"), pin2);
  update_whenset_FormItemInt(F("taskdevicepin3"), pin3);
  setBasicTaskValues(taskIndex, taskdevicetimer,
                     isFormItemChecked(F("TDE")), WebServer.arg(F("TDN")),
                     pin1, pin2, pin3);
  Settings.TaskDevicePort[taskIndex] = getFormItemInt(F("TDP"), 0);
  update_whenset_FormItemInt(F("remoteFeed"), Settings.TaskDeviceDataFeed[taskIndex]);

  for (byte controllerNr = 0; controllerNr < CONTROLLER_MAX; controllerNr++)
  {
    Settings.TaskDeviceID[controllerNr][taskIndex]       = getFormItemInt(String(F("TDID")) + (controllerNr + 1));
    Settings.TaskDeviceSendData[controllerNr][taskIndex] = isFormItemChecked(String(F("TDSD")) + (controllerNr + 1));
  }

  if (Device[DeviceIndex].PullUpOption) {
    Settings.TaskDevicePin1PullUp[taskIndex] = isFormItemChecked(F("TDPPU"));
  }

  if (Device[DeviceIndex].InverseLogicOption) {
    Settings.TaskDevicePin1Inversed[taskIndex] = isFormItemChecked(F("TDPI"));
  }

  for (byte varNr = 0; varNr < Device[DeviceIndex].ValueCount; varNr++)
  {
    strncpy_webserver_arg(ExtraTaskSettings.TaskDeviceFormula[varNr],    String(F("TDF")) + (varNr + 1));
    ExtraTaskSettings.TaskDeviceValueDecimals[varNr] = getFormItemInt(String(F("TDVD")) + (varNr + 1));
    strncpy_webserver_arg(ExtraTaskSettings.TaskDeviceValueNames[varNr], String(F("TDVN")) + (varNr + 1));

    // taskdeviceformula[varNr].toCharArray(tmpString, 41);
    // strcpy(ExtraTaskSettings.TaskDeviceFormula[varNr], tmpString);
    // ExtraTaskSettings.TaskDeviceValueDecimals[varNr] = taskdevicevaluedecimals[varNr].toInt();
    // taskdevicevaluename[varNr].toCharArray(tmpString, 41);
  }

  // // task value names handling.
  // for (byte varNr = 0; varNr < Device[DeviceIndex].ValueCount; varNr++)
  // {
  //   taskdevicevaluename[varNr].toCharArray(tmpString, 41);
  //   strcpy(ExtraTaskSettings.TaskDeviceValueNames[varNr], tmpString);
  // }

  struct EventStruct TempEvent;
  TempEvent.TaskIndex = taskIndex;

  if (ExtraTaskSettings.TaskIndex != TempEvent.TaskIndex) { // if field set empty, reload defaults
    String dummy;
    PluginCall(PLUGIN_GET_DEVICEVALUENAMES, &TempEvent, dummy);
  }

  // allow the plugin to save plugin-specific form settings.
  {
    String dummy;
    PluginCall(PLUGIN_WEBFORM_SAVE, &TempEvent, dummy);
  }

  // notify controllers: CPLUGIN_TASK_CHANGE_NOTIFICATION
  for (byte x = 0; x < CONTROLLER_MAX; x++)
  {
    TempEvent.ControllerIndex = x;

    if (Settings.TaskDeviceSendData[TempEvent.ControllerIndex][TempEvent.TaskIndex] &&
        Settings.ControllerEnabled[TempEvent.ControllerIndex] && Settings.Protocol[TempEvent.ControllerIndex])
    {
      TempEvent.ProtocolIndex = getProtocolIndex(Settings.Protocol[TempEvent.ControllerIndex]);
      String dummy;
      CPluginCall(TempEvent.ProtocolIndex, CPLUGIN_TASK_CHANGE_NOTIFICATION, &TempEvent, dummy);
    }
  }
}

// ********************************************************************************
// Show table with all selected Tasks/Devices
// ********************************************************************************
void handle_devicess_ShowAllTasksTable(byte page)
{
  html_add_script(true);
  TXBuffer += DATA_UPDATE_SENSOR_VALUES_DEVICE_PAGE_JS;
  html_add_script_end();
  html_table_class_multirow();
  html_TR();
  html_table_header("", 70);

  if (TASKS_MAX != TASKS_PER_PAGE)
  {
    html_add_button_prefix();
    TXBuffer += F("devices?setpage=");

    if (page > 1) {
      TXBuffer += page - 1;
    }
    else {
      TXBuffer += page;
    }
    TXBuffer += F("'>&lt;</a>");
    html_add_button_prefix();
    TXBuffer += F("devices?setpage=");

    if (page < (TASKS_MAX / TASKS_PER_PAGE)) {
      TXBuffer += page + 1;
    }
    else {
      TXBuffer += page;
    }
    TXBuffer += F("'>&gt;</a>");
  }

  html_table_header("Task",       50);
  html_table_header(F("Enabled"), 100);
  html_table_header(F("Device"));
  html_table_header("Name");
  html_table_header("Port");
  html_table_header(F("Ctr (IDX)"), 100);
  html_table_header("GPIO",         70);
  html_table_header(F("Values"));

  String deviceName;

  for (byte x = (page - 1) * TASKS_PER_PAGE; x < ((page) * TASKS_PER_PAGE) && validTaskIndex(x); x++)
  {
    const deviceIndex_t DeviceIndex      = getDeviceIndex_from_TaskIndex(x);
    const bool pluginID_set              = validPluginID(Settings.TaskDeviceNumber[x]);
    const bool pluginID_set_notsupported = pluginID_set && !validDeviceIndex(DeviceIndex);

    html_TR_TD();

    if (pluginID_set_notsupported) {
      html_add_button_prefix(F("red"), true);
    } else {
      html_add_button_prefix();
    }
    TXBuffer += F("devices?index=");
    TXBuffer += x + 1;
    TXBuffer += F("&page=");
    TXBuffer += page;
    TXBuffer += F("'>");

    if (pluginID_set) {
      TXBuffer += F("Edit");
    } else {
      TXBuffer += F("Add");
    }
    TXBuffer += F("</a>");
    html_TD();
    TXBuffer += x + 1;
    html_TD();

    // Show table of all configured tasks
    // A task may also refer to a non supported plugin.
    // This will be shown as not supported.
    // Editing a task which has a non supported plugin will present the same as when assigning a new plugin to a task.
    if (pluginID_set)
    {
      LoadTaskSettings(x);
      struct EventStruct TempEvent;
      TempEvent.TaskIndex = x;
      addEnabled(Settings.TaskDeviceEnabled[x]  && validDeviceIndex(DeviceIndex));

      html_TD();
      TXBuffer += getPluginNameFromPluginID(Settings.TaskDeviceNumber[x]);
      html_TD();
      TXBuffer += ExtraTaskSettings.TaskDeviceName;
      html_TD();

      if (validDeviceIndex(DeviceIndex)) {
        if (Settings.TaskDeviceDataFeed[x] != 0) {
          // Show originating node number
          byte remoteUnit = Settings.TaskDeviceDataFeed[x];
          TXBuffer += F("Unit ");
          TXBuffer += remoteUnit;

          if (remoteUnit != 255) {
            NodesMap::iterator it = Nodes.find(remoteUnit);

            if (it != Nodes.end()) {
              TXBuffer += F(" - ");
              TXBuffer += it->second.nodeName;
            } else {
              TXBuffer += F(" - Not Seen recently");
            }
          }
        } else {
          String portDescr;

          if (PluginCall(PLUGIN_WEBFORM_SHOW_CONFIG, &TempEvent, portDescr)) {
            TXBuffer += portDescr;
          } else {
            // Plugin has no custom port formatting, show default one.
            if (Device[DeviceIndex].Ports != 0)
            {
              TXBuffer += formatToHex_decimal(Settings.TaskDevicePort[x]);
            }
          }
        }
      }

      html_TD();

      if (validDeviceIndex(DeviceIndex)) {
        if (Device[DeviceIndex].SendDataOption)
        {
          boolean doBR = false;

          for (byte controllerNr = 0; controllerNr < CONTROLLER_MAX; controllerNr++)
          {
            byte ProtocolIndex = getProtocolIndex(Settings.Protocol[controllerNr]);

            if (Settings.TaskDeviceSendData[controllerNr][x])
            {
              if (doBR) {
                TXBuffer += F("<BR>");
              }
              TXBuffer += getControllerSymbol(controllerNr);

              if (Protocol[ProtocolIndex].usesID && (Settings.Protocol[controllerNr] != 0))
              {
                TXBuffer += " (";
                TXBuffer += Settings.TaskDeviceID[controllerNr][x];
                TXBuffer += ')';

                if (Settings.TaskDeviceID[controllerNr][x] == 0) {
                  TXBuffer += F(" " HTML_SYMBOL_WARNING);
                }
              }
              doBR = true;
            }
          }
        }
      }

      html_TD();

      if (validDeviceIndex(DeviceIndex)) {
        if (Settings.TaskDeviceDataFeed[x] == 0)
        {
          if (Device[DeviceIndex].Type == DEVICE_TYPE_I2C)
          {
            TXBuffer += F("GPIO-");
            TXBuffer += Settings.Pin_i2c_sda;
            TXBuffer += F("<BR>GPIO-");
            TXBuffer += Settings.Pin_i2c_scl;
          }

          if (Device[DeviceIndex].Type == DEVICE_TYPE_ANALOG) {
            TXBuffer += F("ADC (TOUT)");
          }

          if (Settings.TaskDevicePin1[x] != -1)
          {
            TXBuffer += F("GPIO-");
            TXBuffer += Settings.TaskDevicePin1[x];
          }

          if (Settings.TaskDevicePin2[x] != -1)
          {
            TXBuffer += F("<BR>GPIO-");
            TXBuffer += Settings.TaskDevicePin2[x];
          }

          if (Settings.TaskDevicePin3[x] != -1)
          {
            TXBuffer += F("<BR>GPIO-");
            TXBuffer += Settings.TaskDevicePin3[x];
          }
        }
      }

      html_TD();

      if (validDeviceIndex(DeviceIndex)) {
        byte   customValues = false;
        String customValuesString;
        customValues = PluginCall(PLUGIN_WEBFORM_SHOW_VALUES, &TempEvent, customValuesString);

        if (!customValues)
        {
          for (byte varNr = 0; varNr < Device[DeviceIndex].ValueCount; varNr++)
          {
            if (validPluginID(Settings.TaskDeviceNumber[x]))
            {
              TXBuffer += pluginWebformShowValue(x, varNr, ExtraTaskSettings.TaskDeviceValueNames[varNr], formatUserVarNoCheck(x, varNr));
            }
          }
        }
      }
    }
    else {
      html_TD(6);
    }
  } // next
  html_end_table();
  html_end_form();
}

// ********************************************************************************
// Show the task settings page
// ********************************************************************************
void handle_devices_TaskSettingsPage(taskIndex_t taskIndex, byte page)
{
  if (!validTaskIndex(taskIndex)) { return; }
  const deviceIndex_t DeviceIndex = getDeviceIndex_from_TaskIndex(taskIndex);

  LoadTaskSettings(taskIndex);
  struct EventStruct TempEvent;
  TempEvent.TaskIndex = taskIndex;

  html_add_form();
  html_table_class_normal();
  addFormHeader(F("Task Settings"));


  TXBuffer += F("<TR><TD style='width:150px;' align='left'>Device:<TD>");

  // no (supported) device selected, this effectively checks for validDeviceIndex
  if (!supportedPluginID(Settings.TaskDeviceNumber[taskIndex]))
  {
    // takes lots of memory/time so call this only when needed.
    addDeviceSelect("TDNUM", Settings.TaskDeviceNumber[taskIndex]); // ="taskdevicenumber"
  }

  // device selected
  else
  {
    // remember selected device number
    TXBuffer += F("<input type='hidden' name='TDNUM' value='");
    TXBuffer += Settings.TaskDeviceNumber[taskIndex];
    TXBuffer += "'>";

    // show selected device name and delete button
    TXBuffer += getPluginNameFromDeviceIndex(DeviceIndex);

    addHelpButton(String(F("Plugin")) + Settings.TaskDeviceNumber[taskIndex]);
    addRTDPluginButton(Settings.TaskDeviceNumber[taskIndex]);


    if ((Device[DeviceIndex].Number == 3) && (taskIndex >= 4)) // Number == 3 = PulseCounter Plugin
    {
      addFormNote(F("This plugin is only supported on task 1-4 for now"));
    }

    addFormTextBox(F("Name"), F("TDN"), ExtraTaskSettings.TaskDeviceName, NAME_FORMULA_LENGTH_MAX); // ="taskdevicename"

    addFormCheckBox(F("Enabled"), F("TDE"), Settings.TaskDeviceEnabled[taskIndex]);                 // ="taskdeviceenabled"

    // section: Sensor / Actuator
    if (!Device[DeviceIndex].Custom && (Settings.TaskDeviceDataFeed[taskIndex] == 0) &&
        ((Device[DeviceIndex].Ports != 0) ||
         (Device[DeviceIndex].PullUpOption) ||
         (Device[DeviceIndex].InverseLogicOption) ||
         (Device[DeviceIndex].connectedToGPIOpins())))
    {
      addFormSubHeader((Device[DeviceIndex].SendDataOption) ? F("Sensor") : F("Actuator"));

      if (Device[DeviceIndex].Ports != 0) {
        addFormNumericBox(F("Port"), F("TDP"), Settings.TaskDevicePort[taskIndex]); // ="taskdeviceport"
      }

      if (Device[DeviceIndex].PullUpOption)
      {
        addFormCheckBox(F("Internal PullUp"), F("TDPPU"), Settings.TaskDevicePin1PullUp[taskIndex]); // ="taskdevicepin1pullup"
          #if defined(ESP8266)

        if ((Settings.TaskDevicePin1[taskIndex] == 16) || (Settings.TaskDevicePin2[taskIndex] == 16) ||
            (Settings.TaskDevicePin3[taskIndex] == 16)) {
          addFormNote(F("PullDown for GPIO-16 (D0)"));
        }
          #endif // if defined(ESP8266)
      }

      if (Device[DeviceIndex].InverseLogicOption)
      {
        addFormCheckBox(F("Inversed Logic"), F("TDPI"), Settings.TaskDevicePin1Inversed[taskIndex]); // ="taskdevicepin1inversed"
        addFormNote(F("Will go into effect on next input change."));
      }

      // get descriptive GPIO-names from plugin
      TempEvent.String1 = F("1st GPIO");
      TempEvent.String2 = F("2nd GPIO");
      TempEvent.String3 = F("3rd GPIO");
      String dummy;
      PluginCall(PLUGIN_GET_DEVICEGPIONAMES, &TempEvent, dummy);

      if (Device[DeviceIndex].connectedToGPIOpins()) {
        if (Device[DeviceIndex].Type >= DEVICE_TYPE_SINGLE) {
          addFormPinSelect(TempEvent.String1, F("taskdevicepin1"), Settings.TaskDevicePin1[taskIndex]);
        }

        if (Device[DeviceIndex].Type >= DEVICE_TYPE_DUAL) {
          addFormPinSelect(TempEvent.String2, F("taskdevicepin2"), Settings.TaskDevicePin2[taskIndex]);
        }

        if (Device[DeviceIndex].Type == DEVICE_TYPE_TRIPLE) {
          addFormPinSelect(TempEvent.String3, F("taskdevicepin3"), Settings.TaskDevicePin3[taskIndex]);
        }
      }
    }

    // add plugins content
    if (Settings.TaskDeviceDataFeed[taskIndex] == 0) { // only show additional config for local connected sensors
      String webformLoadString;
      PluginCall(PLUGIN_WEBFORM_LOAD, &TempEvent, webformLoadString);

      if (webformLoadString.length() > 0) {
        String errorMessage;
        PluginCall(PLUGIN_GET_DEVICENAME, &TempEvent, errorMessage);
        errorMessage += F(": Bug in PLUGIN_WEBFORM_LOAD, should not append to string, use addHtml() instead");
        addHtmlError(errorMessage);
      }
    }
    else {
      // Show remote feed information.
      addFormSubHeader(F("Data Source"));
      byte remoteUnit = Settings.TaskDeviceDataFeed[taskIndex];
      addFormNumericBox(F("Remote Unit"), F("RemoteUnit"), remoteUnit, 0, 255);

      if (remoteUnit != 255) {
        NodesMap::iterator it = Nodes.find(remoteUnit);

        if (it != Nodes.end()) {
          addUnit(it->second.nodeName);
        } else {
          addUnit(F("Unknown Unit Name"));
        }
      }
      addFormNote(F("0 = disable remote feed, 255 = broadcast")); // FIXME TD-er: Must verify if broadcast can be set.
    }


    // section: Data Acquisition
    if (Device[DeviceIndex].SendDataOption)
    {
      addFormSubHeader(F("Data Acquisition"));

      for (byte controllerNr = 0; controllerNr < CONTROLLER_MAX; controllerNr++)
      {
        if (Settings.Protocol[controllerNr] != 0)
        {
          String id = F("TDSD"); // ="taskdevicesenddata"
          id += controllerNr + 1;

          html_TR_TD(); TXBuffer += F("Send to Controller ");
          TXBuffer               += getControllerSymbol(controllerNr);
          html_TD();
          addCheckBox(id, Settings.TaskDeviceSendData[controllerNr][taskIndex]);

          byte ProtocolIndex = getProtocolIndex(Settings.Protocol[controllerNr]);

          if (Protocol[ProtocolIndex].usesID && (Settings.Protocol[controllerNr] != 0))
          {
            addRowLabel(F("IDX"));
            id  = F("TDID"); // ="taskdeviceid"
            id += controllerNr + 1;
            addNumericBox(id, Settings.TaskDeviceID[controllerNr][taskIndex], 0, DOMOTICZ_MAX_IDX);
          }
        }
      }
    }

    addFormSeparator(2);

    if (Device[DeviceIndex].TimerOption)
    {
      // FIXME: shoudn't the max be ULONG_MAX because Settings.TaskDeviceTimer is an unsigned long? addFormNumericBox only supports ints
      // for min and max specification
      addFormNumericBox(F("Interval"), F("TDT"), Settings.TaskDeviceTimer[taskIndex], 0, 65535); // ="taskdevicetimer"
      addUnit(F("sec"));

      if (Device[DeviceIndex].TimerOptional) {
        TXBuffer += F(" (Optional for this Device)");
      }
    }

    // section: Values
    if (!Device[DeviceIndex].Custom && (Device[DeviceIndex].ValueCount > 0))
    {
      addFormSubHeader(F("Values"));
      html_end_table();
      html_table_class_normal();

      // table header
      TXBuffer += F("<TR><TH style='width:30px;' align='center'>#");
      html_table_header("Name");

      if (Device[DeviceIndex].FormulaOption)
      {
        html_table_header(F("Formula"), F("EasyFormula"), 0);
      }

      if (Device[DeviceIndex].FormulaOption || Device[DeviceIndex].DecimalsOnly)
      {
        html_table_header(F("Decimals"), 30);
      }

      // table body
      for (byte varNr = 0; varNr < Device[DeviceIndex].ValueCount; varNr++)
      {
        html_TR_TD();
        TXBuffer += varNr + 1;
        html_TD();
        String id = F("TDVN"); // ="taskdevicevaluename"
        id += (varNr + 1);
        addTextBox(id, ExtraTaskSettings.TaskDeviceValueNames[varNr], NAME_FORMULA_LENGTH_MAX);

        if (Device[DeviceIndex].FormulaOption)
        {
          html_TD();
          String id = F("TDF"); // ="taskdeviceformula"
          id += (varNr + 1);
          addTextBox(id, ExtraTaskSettings.TaskDeviceFormula[varNr], NAME_FORMULA_LENGTH_MAX);
        }

        if (Device[DeviceIndex].FormulaOption || Device[DeviceIndex].DecimalsOnly)
        {
          html_TD();
          String id = F("TDVD"); // ="taskdevicevaluedecimals"
          id += (varNr + 1);
          addNumericBox(id, ExtraTaskSettings.TaskDeviceValueDecimals[varNr], 0, 6);
        }
      }
    }
  }

  addFormSeparator(4);

  html_TR_TD();
  TXBuffer += F("<TD colspan='3'>");
  html_add_button_prefix();
  TXBuffer += F("devices?setpage=");
  TXBuffer += page;
  TXBuffer += F("'>Close</a>");
  addSubmitButton();
  TXBuffer += F("<input type='hidden' name='edit' value='1'>");
  TXBuffer += F("<input type='hidden' name='page' value='1'>");

  // if user selected a device, add the delete button
  if (validPluginID(Settings.TaskDeviceNumber[taskIndex])) {
    addSubmitButton(F("Delete"), F("del"));
  }

  html_end_table();
  html_end_form();
}

#endif // ifdef WEBSERVER_DEVICES



// ********************************************************************************
// change of device: cleanup old device and reset default settings
// ********************************************************************************
void setTaskDevice_to_TaskIndex(pluginID_t taskdevicenumber, taskIndex_t taskIndex) {
  struct EventStruct TempEvent;

  TempEvent.TaskIndex = taskIndex;
  String dummy;

  // let the plugin do its cleanup by calling PLUGIN_EXIT with this TaskIndex
  PluginCall(PLUGIN_EXIT, &TempEvent, dummy);
  taskClear(taskIndex, false); // clear settings, but do not save
  ClearCustomTaskSettings(taskIndex);

  Settings.TaskDeviceNumber[taskIndex] = taskdevicenumber;

  if (validPluginID(taskdevicenumber)) // set default values if a new device has been selected
  {
    // NOTE: do not enable task by default. allow user to enter sensible valus first and let him enable it when ready.
    PluginCall(PLUGIN_SET_DEFAULTS,         &TempEvent, dummy);
    PluginCall(PLUGIN_GET_DEVICEVALUENAMES, &TempEvent, dummy); // the plugin should populate ExtraTaskSettings with its default values.
  } else {
    // New task is empty task, thus save config now.
    taskClear(taskIndex, true);                                 // clear settings, and save
  }
}

// ********************************************************************************
// Initialize task with some default values applicable for almost all tasks
// ********************************************************************************
void setBasicTaskValues(taskIndex_t taskIndex, unsigned long taskdevicetimer,
                        bool enabled, const String& name, int pin1, int pin2, int pin3) {
  if (!validTaskIndex(taskIndex)) { return; }
  const deviceIndex_t DeviceIndex = getDeviceIndex_from_TaskIndex(taskIndex);

  if (!validDeviceIndex(DeviceIndex)) { return; }

  LoadTaskSettings(taskIndex); // Make sure ExtraTaskSettings are up-to-date

  if (taskdevicetimer > 0) {
    Settings.TaskDeviceTimer[taskIndex] = taskdevicetimer;
  } else {
    if (!Device[DeviceIndex].TimerOptional) { // Set default delay, unless it's optional...
      Settings.TaskDeviceTimer[taskIndex] = Settings.Delay;
    }
    else {
      Settings.TaskDeviceTimer[taskIndex] = 0;
    }
  }
  Settings.TaskDeviceEnabled[taskIndex] = enabled;
  safe_strncpy(ExtraTaskSettings.TaskDeviceName, name.c_str(), sizeof(ExtraTaskSettings.TaskDeviceName));

  if (pin1 >= 0) { Settings.TaskDevicePin1[taskIndex] = pin1; }

  if (pin2 >= 0) { Settings.TaskDevicePin2[taskIndex] = pin2; }

  if (pin3 >= 0) { Settings.TaskDevicePin3[taskIndex] = pin3; }
  SaveTaskSettings(taskIndex);
}
