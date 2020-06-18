#ifndef CONTROLLERQUEUE_CONTROLLER_DELAY_HANDLER_STRUCT_H
#define CONTROLLERQUEUE_CONTROLLER_DELAY_HANDLER_STRUCT_H


#include "../DataStructs/ControllerSettingsStruct.h"
#include "../DataStructs/SchedulerTimers.h"
#include "../DataStructs/TimingStats.h"


/*********************************************************************************************\
* ControllerDelayHandlerStruct
\*********************************************************************************************/
template<class T>
struct ControllerDelayHandlerStruct {
  ControllerDelayHandlerStruct() :
    lastSend(0),
    minTimeBetweenMessages(CONTROLLER_DELAY_QUEUE_DELAY_DFLT),
    max_queue_depth(CONTROLLER_DELAY_QUEUE_DEPTH_DFLT),
    attempt(0),
    max_retries(CONTROLLER_DELAY_QUEUE_RETRY_DFLT),
    delete_oldest(false),
    must_check_reply(false) {}

  void configureControllerSettings(const ControllerSettingsStruct& settings) {
    minTimeBetweenMessages = settings.MinimalTimeBetweenMessages;
    max_queue_depth        = settings.MaxQueueDepth;
    max_retries            = settings.MaxRetry;
    delete_oldest          = settings.DeleteOldest;
    must_check_reply       = settings.MustCheckReply;

    // Set some sound limits when not configured
    if (max_queue_depth == 0) { max_queue_depth = CONTROLLER_DELAY_QUEUE_DEPTH_DFLT; }

    if (max_retries == 0) { max_retries = CONTROLLER_DELAY_QUEUE_RETRY_DFLT; }

    if (minTimeBetweenMessages == 0) { minTimeBetweenMessages = CONTROLLER_DELAY_QUEUE_DELAY_DFLT; }

    // No less than 10 msec between messages.
    if (minTimeBetweenMessages < 10) { minTimeBetweenMessages = 10; }
  }

  bool queueFull(const T& element) const {
    if (sendQueue.size() >= max_queue_depth) { return true; }

    // Number of elements is not exceeding the limit, check memory
    int freeHeap = ESP.getFreeHeap();

    if (freeHeap > 5000) { return false; // Memory is not an issue.
    }
#ifndef BUILD_NO_DEBUG

    if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
      String log = "Controller-";
      log += element.controller_idx + 1;
      log += " : Memory used: ";
      log += getQueueMemorySize();
      log += " bytes ";
      log += sendQueue.size();
      log += " items ";
      log += freeHeap;
      log += " free";
      addLog(LOG_LEVEL_DEBUG, log);
    }
#endif // ifndef BUILD_NO_DEBUG
    return true;
  }

  // Try to add to the queue, if permitted by "delete_oldest"
  // Return false when no item was added.
  bool addToQueue(const T& element) {
    if (delete_oldest) {
      // Force add to the queue.
      // If max buffer is reached, the oldest in the queue (first to be served) will be removed.
      while (queueFull(element)) {
        sendQueue.pop_front();
      }
      sendQueue.emplace_back(element);
      return true;
    }

    if (!queueFull(element)) {
      sendQueue.emplace_back(element);
      return true;
    }
#ifndef BUILD_NO_DEBUG

    if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
      // FIXME TD-er: Must convert it to controller number, but need Settings for that.
      // int controllernummer = Settings.Protocol[element.controller_idx];
      int controllernummer = element.controller_idx + 1;
      String log = get_formatted_Controller_number(controllernummer);
      log += " : queue full";
      addLog(LOG_LEVEL_DEBUG, log);
    }
#endif // ifndef BUILD_NO_DEBUG
    return false;
  }

  // Get the next element.
  // Remove front element when max_retries is reached.
  T* getNext() {
    if (sendQueue.empty()) { return NULL; }

    if (attempt > max_retries) {
      sendQueue.pop_front();
      attempt = 0;

      if (sendQueue.empty()) { return NULL; }
    }
    return &sendQueue.front();
  }

  // Mark as processed and return time to schedule for next process.
  // Return 0 when nothing to process.
  // @param remove_from_queue indicates whether the elements should be removed from the queue.
  unsigned long markProcessed(bool remove_from_queue) {
    if (sendQueue.empty()) { return 0; }

    if (remove_from_queue) {
      sendQueue.pop_front();
      attempt = 0;
      lastSend = millis();
    } else {
      ++attempt;
    }
    return getNextScheduleTime();
  }

  unsigned long getNextScheduleTime() const {
    if (sendQueue.empty()) { return 0; }
    unsigned long nextTime = lastSend + minTimeBetweenMessages;

    if (timePassedSince(nextTime) > 0) {
      nextTime = millis();
    }

    if (nextTime == 0) { nextTime = 1; // Just to make sure it will be executed
    }
    return nextTime;
  }

  size_t getQueueMemorySize() const {
    size_t totalSize = 0;

    for (auto it = sendQueue.begin(); it != sendQueue.end(); ++it) {
      totalSize += it->getSize();
    }
    return totalSize;
  }

  std::list<T>  sendQueue;
  unsigned long lastSend;
  unsigned int  minTimeBetweenMessages;
  byte          max_queue_depth;
  byte          attempt;
  byte          max_retries;
  bool          delete_oldest;
  bool          must_check_reply;
};

// Uncrustify must not be used on macros, so turn it off.
// *INDENT-OFF*



// Define the function wrappers to handle the calling to Cxxx_DelayHandler etc.
// If someone knows how to add leading zeros in macros, please be my guest :)


// This macro defines the code needed to create the 'process_c##NNN####M##_delay_queue()'
// function and all needed objects and forward declarations.
// It is a macro to prevent common typo errors.
// This function will perform the (re)scheduling and mark if it is processed (and can be removed)
// The controller itself must implement the 'do_process_c004_delay_queue' function to actually
// send the data.
// Its return value must state whether it can be marked 'Processed'.
// N.B. some controllers only can send one value per iteration, so a returned "false" can mean it
//      was still successful. The controller should keep track of the last value sent
//      in the element stored in the queue.
#define DEFINE_Cxxx_DELAY_QUEUE_MACRO(NNN, M)                                                                        \
  bool do_process_c##NNN####M##_delay_queue(int controller_number,                                                   \
                                           const C##NNN####M##_queue_element & element,                              \
                                           ControllerSettingsStruct & ControllerSettings);                           \
  ControllerDelayHandlerStruct<C##NNN####M##_queue_element>C##NNN####M##_DelayHandler;                               \
  void process_c##NNN####M##_delay_queue();                                                                          \
  void process_c##NNN####M##_delay_queue() {                                                                         \
    C##NNN####M##_queue_element *element(C##NNN####M##_DelayHandler.getNext());                                      \
    if (element == NULL) return;                                                                                     \
    MakeControllerSettings (ControllerSettings);                                                                     \
    LoadControllerSettings(element->controller_idx, ControllerSettings);                                             \
    C##NNN####M##_DelayHandler.configureControllerSettings(ControllerSettings);                                      \
    if (!WiFiConnected(10)) {                                                                                        \
      scheduleNextDelayQueue(TIMER_C##NNN####M##_DELAY_QUEUE, C##NNN####M##_DelayHandler.getNextScheduleTime());     \
      return;                                                                                                        \
    }                                                                                                                \
    START_TIMER;                                                                                                     \
    C##NNN####M##_DelayHandler.markProcessed(do_process_c##NNN####M##_delay_queue(M, *element, ControllerSettings)); \
    STOP_TIMER(C##NNN####M##_DELAY_QUEUE);                                                                           \
    scheduleNextDelayQueue(TIMER_C##NNN####M##_DELAY_QUEUE, C##NNN####M##_DelayHandler.getNextScheduleTime());       \
  }

// Uncrustify must not be used on macros, but we're now done, so turn Uncrustify on again.
// *INDENT-ON*


#endif // CONTROLLERQUEUE_CONTROLLER_DELAY_HANDLER_STRUCT_H