#include "../ControllerQueue/C018_queue_element.h"

#include "../DataStructs/ESPEasy_EventStruct.h"


#ifdef USES_PACKED_RAW_DATA
String getPackedFromPlugin(struct EventStruct *event,
                           uint8_t             sampleSetCount);
#endif // USES_PACKED_RAW_DATA

C018_queue_element::C018_queue_element() {}

C018_queue_element::C018_queue_element(struct EventStruct *event, uint8_t sampleSetCount) :
  controller_idx(event->ControllerIndex)
{
    #ifdef USES_PACKED_RAW_DATA
  packed = getPackedFromPlugin(event, sampleSetCount);
    #endif // USES_PACKED_RAW_DATA
}

size_t C018_queue_element::getSize() const {
  return sizeof(*this) + packed.length();
}
