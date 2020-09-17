#ifndef PLUGIN_HELPER_H
#define PLUGIN_HELPER_H

#include "ESPEasy_common.h"
#include "src/DataStructs/ESPEasyLimits.h"
#include "src/Globals/Plugins.h"

// Defines to make plugins more readable.

#ifndef PCONFIG
  # define PCONFIG(n) (Settings.TaskDevicePluginConfig[event->TaskIndex][(n)])
#endif // ifndef PCONFIG
#ifndef PCONFIG_FLOAT
  # define PCONFIG_FLOAT(n) (Settings.TaskDevicePluginConfigFloat[event->TaskIndex][(n)])
#endif // ifndef PCONFIG_FLOAT
#ifndef PCONFIG_LONG
  # define PCONFIG_LONG(n) (Settings.TaskDevicePluginConfigLong[event->TaskIndex][(n)])
#endif // ifndef PCONFIG_LONG
#ifndef PIN

// Please note the 'offset' of N compared to normal pin numbering.
  # define PIN(n) (Settings.TaskDevicePin[n][event->TaskIndex])
#endif // ifndef PIN
#ifndef CONFIG_PIN1
  # define CONFIG_PIN1 (Settings.TaskDevicePin1[event->TaskIndex])
#endif // ifndef CONFIG_PIN1
#ifndef CONFIG_PIN2
  # define CONFIG_PIN2 (Settings.TaskDevicePin2[event->TaskIndex])
#endif // ifndef CONFIG_PIN2
#ifndef CONFIG_PIN3
  # define CONFIG_PIN3 (Settings.TaskDevicePin3[event->TaskIndex])
#endif // ifndef CONFIG_PIN3
#ifndef CONFIG_PORT
  # define CONFIG_PORT (Settings.TaskDevicePort[event->TaskIndex])
#endif // ifndef CONFIG_PORT

String PCONFIG_LABEL(int n);

// ==============================================
// Data used by instances of plugins.
// =============================================

// base class to be able to delete a data object from the array.
// N.B. in order to use this, a data object must inherit from this base class.
//      This is a compile time check.
struct PluginTaskData_base {
  virtual ~PluginTaskData_base() {}

  // We cannot use dynamic_cast, so we must keep track of the plugin ID to
  // perform checks on the casting.
  // This is also a check to only use these functions and not to insert pointers
  // at random in the Plugin_task_data array.
  deviceIndex_t _taskdata_deviceIndex = INVALID_DEVICE_INDEX;
};



void resetPluginTaskData();

void clearPluginTaskData(taskIndex_t taskIndex);

void initPluginTaskData(taskIndex_t taskIndex, PluginTaskData_base *data);

PluginTaskData_base* getPluginTaskData(taskIndex_t taskIndex);

bool pluginTaskData_initialized(taskIndex_t taskIndex);

String getPluginCustomArgName(int varNr);

// Helper function to create formatted custom values for display in the devices overview page.
// When called from PLUGIN_WEBFORM_SHOW_VALUES, the last item should add a traling div_br class
// if the regular values should also be displayed.
// The call to PLUGIN_WEBFORM_SHOW_VALUES should only return success = true when no regular values should be displayed
// Note that the varNr of the custom values should not conflict with the existing variable numbers (e.g. start at VARS_PER_TASK)
String pluginWebformShowValue(taskIndex_t taskIndex, byte varNr, const String& label, const String& value, bool addTrailingBreak = false);




#endif // PLUGIN_HELPER_H
