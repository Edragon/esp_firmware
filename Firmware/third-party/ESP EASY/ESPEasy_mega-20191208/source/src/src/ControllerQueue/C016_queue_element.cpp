#include "../ControllerQueue/C016_queue_element.h"

#include "../DataStructs/ESPEasy_EventStruct.h"
#include "../../ESPEasy_fdwdecl.h"

C016_queue_element::C016_queue_element() : timestamp(0), TaskIndex(INVALID_TASK_INDEX), controller_idx(0), sensorType(0) {}

C016_queue_element::C016_queue_element(const struct EventStruct *event, byte value_count, unsigned long unixTime) :
  timestamp(unixTime),
  TaskIndex(event->TaskIndex),
  controller_idx(event->ControllerIndex),
  sensorType(event->sensorType),
  valueCount(value_count)
{
  const byte BaseVarIndex = TaskIndex * VARS_PER_TASK;

  for (byte i = 0; i < VARS_PER_TASK; ++i) {
    if (i < value_count) {
      values[i] = getUserVar(BaseVarIndex + i);
    } else {
      values[i] = 0.0;
    }
  }
}

size_t C016_queue_element::getSize() const {
  return sizeof(*this);
}
