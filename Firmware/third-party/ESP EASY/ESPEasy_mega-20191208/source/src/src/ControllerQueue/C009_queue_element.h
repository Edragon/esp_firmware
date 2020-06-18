#ifndef CONTROLLERQUEUE_C009_QUEUE_ELEMENT_H
#define CONTROLLERQUEUE_C009_QUEUE_ELEMENT_H

#include "../../ESPEasy_common.h"
#include "../DataStructs/ESPEasyLimits.h"
#include "../Globals/Plugins.h"

struct EventStruct;


// #ifdef USES_C009

/*********************************************************************************************\
* C009_queue_element for queueing requests for C009: FHEM HTTP.
\*********************************************************************************************/
class C009_queue_element {
public:

  C009_queue_element();

  C009_queue_element(const struct EventStruct *event);

  size_t getSize() const;

  String txt[VARS_PER_TASK];
  int controller_idx    = 0;
  int idx               = 0;
  taskIndex_t TaskIndex = INVALID_TASK_INDEX;
  byte sensorType       = 0;
};

// #endif //USES_C009


#endif // CONTROLLERQUEUE_C009_QUEUE_ELEMENT_H
