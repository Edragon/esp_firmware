
// ********************************************************************************
// Web Interface notifcations page
// ********************************************************************************
#ifndef NOTIFIER_SET_NONE
void handle_notifications() {
  checkRAM(F("handle_notifications"));

  if (!isLoggedIn()) { return; }
  navMenuIndex = MENU_INDEX_NOTIFICATIONS;
  TXBuffer.startStream();
  sendHeadandTail_stdtemplate(_HEAD);

  struct EventStruct TempEvent;

  // char tmpString[64];


  byte notificationindex          = getFormItemInt(F("index"), 0);
  boolean notificationindexNotSet = notificationindex == 0;
  --notificationindex;

  const int notification = getFormItemInt(F("notification"), -1);

  if ((notification != -1) && !notificationindexNotSet)
  {
    MakeNotificationSettings(NotificationSettings);

    if (Settings.Notification[notificationindex] != notification)
    {
      Settings.Notification[notificationindex] = notification;
    }
    else
    {
      if (Settings.Notification != 0)
      {
        byte NotificationProtocolIndex = getNotificationProtocolIndex(Settings.Notification[notificationindex]);

        if (NotificationProtocolIndex != NPLUGIN_NOT_FOUND) {
          NPlugin_ptr[NotificationProtocolIndex](NPLUGIN_WEBFORM_SAVE, 0, dummyString);
        }
        NotificationSettings.Port                       = getFormItemInt(F("port"), 0);
        NotificationSettings.Pin1                       = getFormItemInt(F("pin1"), 0);
        NotificationSettings.Pin2                       = getFormItemInt(F("pin2"), 0);
        Settings.NotificationEnabled[notificationindex] = isFormItemChecked(F("notificationenabled"));
        strncpy_webserver_arg(NotificationSettings.Domain,   F("domain"));
        strncpy_webserver_arg(NotificationSettings.Server,   F("server"));
        strncpy_webserver_arg(NotificationSettings.Sender,   F("sender"));
        strncpy_webserver_arg(NotificationSettings.Receiver, F("receiver"));
        strncpy_webserver_arg(NotificationSettings.Subject,  F("subject"));
        strncpy_webserver_arg(NotificationSettings.User,     F("user"));
        strncpy_webserver_arg(NotificationSettings.Pass,     F("pass"));
        strncpy_webserver_arg(NotificationSettings.Body,     F("body"));
      }
    }

    // Save the settings.
    addHtmlError(SaveNotificationSettings(notificationindex, (byte *)&NotificationSettings, sizeof(NotificationSettingsStruct)));
    addHtmlError(SaveSettings());

    if (WebServer.hasArg(F("test"))) {
      // Perform tests with the settings in the form.
      byte NotificationProtocolIndex = getNotificationProtocolIndex(Settings.Notification[notificationindex]);

      if (NotificationProtocolIndex != NPLUGIN_NOT_FOUND)
      {
        // TempEvent.NotificationProtocolIndex = NotificationProtocolIndex;
        TempEvent.NotificationIndex = notificationindex;
        schedule_notification_event_timer(NotificationProtocolIndex, NPLUGIN_NOTIFY, &TempEvent);
      }
    }
  }

  html_add_form();

  if (notificationindexNotSet)
  {
    html_table_class_multirow();
    html_TR();
    html_table_header("",           70);
    html_table_header("Nr",         50);
    html_table_header(F("Enabled"), 100);
    html_table_header(F("Service"));
    html_table_header(F("Server"));
    html_table_header("Port");

    MakeNotificationSettings(NotificationSettings);

    for (byte x = 0; x < NOTIFICATION_MAX; x++)
    {
      LoadNotificationSettings(x, (byte *)&NotificationSettings, sizeof(NotificationSettingsStruct));
      NotificationSettings.validate();
      html_TR_TD();
      html_add_button_prefix();
      TXBuffer += F("notifications?index=");
      TXBuffer += x + 1;
      TXBuffer += F("'>Edit</a>");
      html_TD();
      TXBuffer += x + 1;
      html_TD();

      if (Settings.Notification[x] != 0)
      {
        addEnabled(Settings.NotificationEnabled[x]);

        html_TD();
        byte   NotificationProtocolIndex = getNotificationProtocolIndex(Settings.Notification[x]);
        String NotificationName          = F("(plugin not found?)");

        if (NotificationProtocolIndex != NPLUGIN_NOT_FOUND)
        {
          NPlugin_ptr[NotificationProtocolIndex](NPLUGIN_GET_DEVICENAME, 0, NotificationName);
        }
        TXBuffer += NotificationName;
        html_TD();
        TXBuffer += NotificationSettings.Server;
        html_TD();
        TXBuffer += NotificationSettings.Port;
      }
      else {
        html_TD(3);
      }
    }
    html_end_table();
    html_end_form();
  }
  else
  {
    html_table_class_normal();
    addFormHeader(F("Notification Settings"));
    addRowLabel(F("Notification"));
    byte choice = Settings.Notification[notificationindex];
    addSelector_Head(F("notification"), true);
    addSelector_Item(F("- None -"), 0, false, false, "");

    for (byte x = 0; x <= notificationCount; x++)
    {
      String NotificationName = "";
      NPlugin_ptr[x](NPLUGIN_GET_DEVICENAME, 0, NotificationName);
      addSelector_Item(NotificationName,
                       Notification[x].Number,
                       choice == Notification[x].Number,
                       false,
                       "");
    }
    addSelector_Foot();

    addHelpButton(F("EasyNotifications"));

    if (Settings.Notification[notificationindex])
    {
      MakeNotificationSettings(NotificationSettings);
      LoadNotificationSettings(notificationindex, (byte *)&NotificationSettings, sizeof(NotificationSettingsStruct));
      NotificationSettings.validate();

      byte NotificationProtocolIndex = getNotificationProtocolIndex(Settings.Notification[notificationindex]);

      if (NotificationProtocolIndex != NPLUGIN_NOT_FOUND)
      {
        if (Notification[NotificationProtocolIndex].usesMessaging)
        {
          addFormTextBox(F("Domain"), F("domain"), NotificationSettings.Domain, sizeof(NotificationSettings.Domain) - 1);
          addFormTextBox(F("Server"), F("server"), NotificationSettings.Server, sizeof(NotificationSettings.Server) - 1);
          addFormNumericBox(F("Port"), F("port"), NotificationSettings.Port, 1, 65535);

          addFormTextBox(F("Sender"),   F("sender"),   NotificationSettings.Sender,   sizeof(NotificationSettings.Sender) - 1);
          addFormTextBox(F("Receiver"), F("receiver"), NotificationSettings.Receiver, sizeof(NotificationSettings.Receiver) - 1);
          addFormTextBox(F("Subject"),  F("subject"),  NotificationSettings.Subject,  sizeof(NotificationSettings.Subject) - 1);

          addFormTextBox(F("User"),     F("user"),     NotificationSettings.User,     sizeof(NotificationSettings.User) - 1);
          addFormTextBox(F("Pass"),     F("pass"),     NotificationSettings.Pass,     sizeof(NotificationSettings.Pass) - 1);

          addRowLabel(F("Body"));
          TXBuffer += F("<textarea name='body' rows='20' size=512 wrap='off'>");
          TXBuffer += NotificationSettings.Body;
          TXBuffer += F("</textarea>");
        }

        if (Notification[NotificationProtocolIndex].usesGPIO > 0)
        {
          addRowLabel(F("1st GPIO"));
          addPinSelect(false, "pin1", NotificationSettings.Pin1);
        }

        addRowLabel(F("Enabled"));
        addCheckBox(F("notificationenabled"), Settings.NotificationEnabled[notificationindex]);

        TempEvent.NotificationIndex = notificationindex;
        String webformLoadString;
        NPlugin_ptr[NotificationProtocolIndex](NPLUGIN_WEBFORM_LOAD, &TempEvent, webformLoadString);

        if (webformLoadString.length() > 0) {
          addHtmlError(F("Bug in NPLUGIN_WEBFORM_LOAD, should not append to string, use addHtml() instead"));
        }
      }
    }

    addFormSeparator(2);

    html_TR_TD();
    html_TD();
    addButton(F("notifications"), F("Close"));
    addSubmitButton();
    addSubmitButton(F("Test"), F("test"));
    html_end_table();
    html_end_form();
  }
  sendHeadandTail_stdtemplate(_TAIL);
  TXBuffer.endStream();
}

#endif // NOTIFIER_SET_NONE
