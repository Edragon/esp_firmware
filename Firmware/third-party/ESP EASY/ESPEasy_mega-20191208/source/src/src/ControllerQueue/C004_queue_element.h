#ifndef CONTROLLERQUEUE_C004_QUEUE_ELEMENT_H
#define CONTROLLERQUEUE_C004_QUEUE_ELEMENT_H

#include "../../ESPEasy_common.h"
#include "../Globals/Plugins.h"

struct EventStruct;

// #ifdef USES_C004

/*********************************************************************************************\
* C004_queue_element for queueing requests for C004 ThingSpeak.
*   Typical use case for Thingspeak is to only send values every N seconds/minutes.
*   So we just store everything needed to recreate the event when the time is ready.
\*********************************************************************************************/
class C004_queue_element {
public:

  C004_queue_element();

  C004_queue_element(const struct EventStruct *event);

  size_t getSize() const;

  String txt;
  int controller_idx    = 0;
  int idx               = 0;
  taskIndex_t TaskIndex = INVALID_TASK_INDEX;
  byte sensorType       = 0;
};

// #endif //USES_C004


#endif // CONTROLLERQUEUE_C004_QUEUE_ELEMENT_H
