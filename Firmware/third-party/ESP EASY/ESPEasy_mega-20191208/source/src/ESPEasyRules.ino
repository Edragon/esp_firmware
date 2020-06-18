#define RULE_FILE_SEPARAROR '/'
#define RULE_MAX_FILENAME_LENGTH 24

#include "src/DataStructs/EventValueSource.h"
#include "src/Globals/Device.h"
#include "src/Globals/Plugins.h"

String EventToFileName(const String& eventName) {
  int size  = eventName.length();
  int index = eventName.indexOf('=');

  if (index > -1) {
    size = index;
  }
#if defined(ESP8266)
  String fileName = F("rules/");
#endif // if defined(ESP8266)
#if defined(ESP32)
  String fileName = F("/rules/");
#endif // if defined(ESP32)
  fileName += eventName.substring(0, size);
  fileName.replace('#', RULE_FILE_SEPARAROR);
  fileName.toLowerCase();
  return fileName;
}

String FileNameToEvent(const String& fileName) {
#if defined(ESP8266)
  String eventName = fileName.substring(6);
#endif // if defined(ESP8266)
#if defined(ESP32)
  String eventName = fileName.substring(7);
#endif // if defined(ESP32)
  eventName.replace(RULE_FILE_SEPARAROR, '#');
  return eventName;
}

void checkRuleSets() {
  for (byte x = 0; x < RULESETS_MAX; x++) {
#if defined(ESP8266)
    String fileName = F("rules");
#endif // if defined(ESP8266)
#if defined(ESP32)
    String fileName = F("/rules");
#endif // if defined(ESP32)
    fileName += x + 1;
    fileName += F(".txt");

    if (SPIFFS.exists(fileName)) {
      activeRuleSets[x] = true;
    }
    else {
      activeRuleSets[x] = false;
    }

#ifndef BUILD_NO_DEBUG

    if (Settings.SerialLogLevel == LOG_LEVEL_DEBUG_DEV) {
      serialPrint(fileName);
      serialPrint(" ");
      serialPrintln(String(activeRuleSets[x]));
    }
#endif // ifndef BUILD_NO_DEBUG
  }
}

/********************************************************************************************\
   Process next event from event queue
 \*********************************************************************************************/
bool processNextEvent() {
  if (Settings.UseRules)
  {
    String nextEvent;
    if (eventQueue.getNext(nextEvent)) {
      rulesProcessing(nextEvent);
      return true;
    }
  }
  return false;
}


/********************************************************************************************\
   Rules processing
 \*********************************************************************************************/
void rulesProcessing(String& event) {
  if (!Settings.UseRules) {
    return;
  }
  START_TIMER
    checkRAM(F("rulesProcessing"));
#ifndef BUILD_NO_DEBUG
  unsigned long timer = millis();
#endif // ifndef BUILD_NO_DEBUG

  if (loglevelActiveFor(LOG_LEVEL_INFO)) {
    String log = F("EVENT: ");
    log += event;
    addLog(LOG_LEVEL_INFO, log);
  }

  if (Settings.OldRulesEngine()) {
    for (byte x = 0; x < RULESETS_MAX; x++) {
#if defined(ESP8266)
      String fileName = F("rules");
#endif // if defined(ESP8266)
#if defined(ESP32)
      String fileName = F("/rules");
#endif // if defined(ESP32)
      fileName += x + 1;
      fileName += F(".txt");

      if (activeRuleSets[x]) {
        rulesProcessingFile(fileName, event);
      }
    }
  } else {
    String fileName = EventToFileName(event);

    // if exists processed the rule file
    if (SPIFFS.exists(fileName)) {
      rulesProcessingFile(fileName, event);
    }
#ifndef BUILD_NO_DEBUG
    else {
      addLog(LOG_LEVEL_DEBUG, String(F("EVENT: ")) + event +
             String(F(" is ingnored. File ")) + fileName +
             String(F(" not found.")));
    }
#endif // ifndef BUILD_NO_DEBUG
  }

#ifndef BUILD_NO_DEBUG

  if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
    String log = F("EVENT: ");
    log += event;
    log += F(" Processing time:");
    log += timePassedSince(timer);
    log += F(" milliSeconds");
    addLog(LOG_LEVEL_DEBUG, log);
  }
#endif // ifndef BUILD_NO_DEBUG
  STOP_TIMER(RULES_PROCESSING);
  backgroundtasks();
}

/********************************************************************************************\
   Rules processing
 \*********************************************************************************************/
String rulesProcessingFile(const String& fileName, String& event) {
  if (!Settings.UseRules || !fileExists(fileName)) {
    return "";
  }
  checkRAM(F("rulesProcessingFile"));
#ifndef BUILD_NO_DEBUG

  if (Settings.SerialLogLevel == LOG_LEVEL_DEBUG_DEV) {
    serialPrint(F("RuleDebug Processing:"));
    serialPrintln(fileName);
    serialPrintln(F("     flags CMI  parse output:"));
  }
#endif // ifndef BUILD_NO_DEBUG

  static byte nestingLevel = 0;
  String log               = "";

  nestingLevel++;

  if (nestingLevel > RULES_MAX_NESTING_LEVEL) {
    addLog(LOG_LEVEL_ERROR, F("EVENT: Error: Nesting level exceeded!"));
    nestingLevel--;
    return log;
  }

  fs::File f = tryOpenFile(fileName, "r+");
  SPIFFS_CHECK(f, fileName.c_str());

  // Try to get the best possible estimate on line length based on earlier parsing of the rules.
  static size_t longestLineSize = RULES_BUFFER_SIZE;
  String line;
  line.reserve(longestLineSize);
  bool match     = false;
  bool codeBlock = false;
  bool isCommand = false;
  bool condition[RULES_IF_MAX_NESTING_LEVEL];
  bool ifBranche[RULES_IF_MAX_NESTING_LEVEL];
  byte ifBlock     = 0;
  byte fakeIfBlock = 0;

  std::vector<byte> buf;
  buf.resize(RULES_BUFFER_SIZE);

  bool firstNonSpaceRead = false;
  bool commentFound      = false;

  while (f.available()) {
    int len = f.read(&buf[0], RULES_BUFFER_SIZE);

    for (int x = 0; x < len; x++) {
      int data = buf[x];

      SPIFFS_CHECK(data >= 0, fileName.c_str());

      switch (static_cast<char>(data))
      {
        case '\n':
        {
          // Line end, parse rule
          line.trim();
          const size_t lineLength = line.length();

          if (lineLength > longestLineSize) {
            longestLineSize = lineLength;
          }

          if ((lineLength > 0) && !line.startsWith(F("//"))) {
            // Parse the line and extract the action (if there is any)
            String action;
            parseCompleteNonCommentLine(line, event, log, action, match, codeBlock,
                                        isCommand, condition, ifBranche, ifBlock,
                                        fakeIfBlock);

            if (match) // rule matched for one action or a block of actions
            {
              processMatchedRule(action, event, log, match, codeBlock,
                                 isCommand, condition, ifBranche, ifBlock, fakeIfBlock);
            }

            backgroundtasks();
          }

          // Prepare for new line
          line = "";
          line.reserve(longestLineSize);
          firstNonSpaceRead = false;
          commentFound      = false;
          break;
        }
        case '\r': // Just skip this character
          break;
        case '\t': // tab
        case ' ':  // space
        {
          // Strip leading spaces.
          if (firstNonSpaceRead) {
            line += ' ';
          }
          break;
        }
        case '/':
        {
          if (!commentFound) {
            line += '/';

            if (line.endsWith("//")) {
              // consider the rest of the line a comment
              commentFound = true;
            }
          }
          break;
        }
        default: // Any other character
        {
          firstNonSpaceRead = true;

          if (!commentFound) {
            line += char(data);
          }
          break;
        }
      }
    }
  }

  if (f) {
    f.close();
  }

  nestingLevel--;
  checkRAM(F("rulesProcessingFile2"));
  return "";
}

void replace_EventValueN_Argv(String& line, const String& argString, unsigned int argc)
{
  String eventvalue;

  eventvalue.reserve(16);
  eventvalue = F("%eventvalue");

  if (argc == 0) {
    // Used for compatibility reasons
    // it still needs to call the "1st" argument
    argc = 1;
  } else {
    eventvalue += argc;
  }
  eventvalue += '%';
  String tmpParam;

  if (GetArgv(argString.c_str(), tmpParam, argc)) {
    line.replace(eventvalue, tmpParam);
  }
}

void substitute_eventvalue(String& line, const String& event) {
  if (line.indexOf(F("%eventvalue")) == -1) {
    return; // Nothing to replace.
  }

  if (event.charAt(0) == '!') {
    line.replace(F("%eventvalue%"), event); // substitute %eventvalue% with
                                            // literal event string if
                                            // starting with '!'
  } else {
    int equalsPos = event.indexOf("=");

    if (equalsPos > 0) {
      // Replace %eventvalueX% with the actual value of the event.
      // For compatibility reasons also replace %eventvalue%  (argc = 0)
      String argString = event.substring(equalsPos + 1);

      for (unsigned int argc = 0; argc <= 4; ++argc) {
        replace_EventValueN_Argv(line, argString, argc);
      }
    }
  }
}

void parseCompleteNonCommentLine(String& line, String& event, String& log,
                                 String& action, bool& match,
                                 bool& codeBlock, bool& isCommand,
                                 bool condition[], bool ifBranche[],
                                 byte& ifBlock, byte& fakeIfBlock) {
  const bool lineStartsWith_on = line.substring(0, 3).equalsIgnoreCase(F("on "));

  if (!codeBlock && !match) {
    // We're looking for a new code block.
    // Continue with next line if none was found on current line.
    if (!lineStartsWith_on) {
      return;
    }
  }

  isCommand = true;

  // Strip trailing comments
  int comment = line.indexOf(F("//"));

  if (comment >= 0) {
    line = line.substring(0, comment);
  }
  line.trim();

  if (match || !codeBlock) {
    // only parse [xxx#yyy] if we have a matching ruleblock or need to eval the
    // "on" (no codeBlock)
    // This to avoid wasting CPU time...
    if (match && !fakeIfBlock) {
      // substitution of %eventvalue% is made here so it can be used on if
      // statement too
      substitute_eventvalue(line, event);
    }

    if (match || lineStartsWith_on) {
      // Only parseTemplate when we are actually doing something with the line.
      // When still looking for the "on ... do" part, do not change it before we found the block.
      line = parseTemplate(line, line.length());
    }
  }


  String lineOrg = line; // store original line for future use
  line.toLowerCase();    // convert all to lower case to make checks easier

  String eventTrigger = "";
  action = "";

  if (!codeBlock) // do not check "on" rules if a block of actions is to be
                  // processed
  {
    if (lineStartsWith_on) {
      ifBlock     = 0;
      fakeIfBlock = 0;
      line        = line.substring(3);
      int split = line.indexOf(F(" do"));

      if (split != -1) {
        eventTrigger = line.substring(0, split);
        action       = lineOrg.substring(split + 7);

        // Remove trailing and leadin spaces on the eventTrigger and action.
        eventTrigger.trim();
        action.trim();
      }

      if (eventTrigger == "*") { // wildcard, always process
        match = true;
      }
      else {
        match = ruleMatch(event, eventTrigger);
      }

      if (action.length() > 0) // single on/do/action line, no block
      {
        isCommand = true;
        codeBlock = false;
      } else {
        isCommand = false;
        codeBlock = true;
      }
    }
  } else {
    action = lineOrg;
  }

  if (action.equalsIgnoreCase(F("endon"))) // Check if action block has ended, then we will
                                           // wait for a new "on" rule
  {
    isCommand   = false;
    codeBlock   = false;
    match       = false;
    ifBlock     = 0;
    fakeIfBlock = 0;
  }

#ifndef BUILD_NO_DEBUG

  if (loglevelActiveFor(LOG_LEVEL_DEBUG_DEV)) {
    String log = F("RuleDebug: ");
    log += codeBlock;
    log += match;
    log += isCommand;
    log += ": ";
    log += line;
    addLog(LOG_LEVEL_DEBUG_DEV, log);
  }
#endif // ifndef BUILD_NO_DEBUG
}

void processMatchedRule(String& action, String& event,
                        String& log, bool& match, bool& codeBlock,
                        bool& isCommand, bool condition[], bool ifBranche[],
                        byte& ifBlock, byte& fakeIfBlock) {
  String lcAction = action;

  lcAction.toLowerCase();

  if (fakeIfBlock) {
    isCommand = false;
  }
  else if (ifBlock) {
    if (condition[ifBlock - 1] != ifBranche[ifBlock - 1]) {
      isCommand = false;
    }
  }
  int split =
    lcAction.indexOf(F("elseif ")); // check for optional "elseif" condition

  if (split != -1) {
    // Found "elseif" condition
    isCommand = false;

    if (ifBlock && !fakeIfBlock) {
      if (ifBranche[ifBlock - 1]) {
        if (condition[ifBlock - 1]) {
          ifBranche[ifBlock - 1] = false;
        }
        else {
          String check = lcAction.substring(split + 7);
          check.trim();
          condition[ifBlock - 1] = conditionMatchExtended(check);
#ifndef BUILD_NO_DEBUG

          if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
            log  = F("Lev.");
            log += String(ifBlock);
            log += F(": [elseif ");
            log += check;
            log += F("]=");
            log += boolToString(condition[ifBlock - 1]);
            addLog(LOG_LEVEL_DEBUG, log);
          }
#endif // ifndef BUILD_NO_DEBUG
        }
      }
    }
  } else {
    split = lcAction.indexOf(F("if ")); // check for optional "if" condition

    if (split != -1) {
      if (ifBlock < RULES_IF_MAX_NESTING_LEVEL) {
        if (isCommand) {
          ifBlock++;
          String check = lcAction.substring(split + 3);
          check.trim();
          condition[ifBlock - 1] = conditionMatchExtended(check);
          ifBranche[ifBlock - 1] = true;
#ifndef BUILD_NO_DEBUG

          if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
            log  = F("Lev.");
            log += String(ifBlock);
            log += F(": [if ");
            log += check;
            log += F("]=");
            log += boolToString(condition[ifBlock - 1]);
            addLog(LOG_LEVEL_DEBUG, log);
          }
#endif // ifndef BUILD_NO_DEBUG
        } else {
          fakeIfBlock++;
        }
      } else {
        fakeIfBlock++;

        if (loglevelActiveFor(LOG_LEVEL_ERROR)) {
          log  = F("Lev.");
          log += String(ifBlock);
          log  = F(": Error: IF Nesting level exceeded!");
          addLog(LOG_LEVEL_ERROR, log);
        }
      }
      isCommand = false;
    }
  }

  if ((lcAction == F("else")) && !fakeIfBlock) // in case of an "else" block of
                                               // actions, set ifBranche to
                                               // false
  {
    ifBranche[ifBlock - 1] = false;
    isCommand              = false;
#ifndef BUILD_NO_DEBUG

    if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
      log  = F("Lev.");
      log += String(ifBlock);
      log += F(": [else]=");
      log += boolToString(condition[ifBlock - 1] == ifBranche[ifBlock - 1]);
      addLog(LOG_LEVEL_DEBUG, log);
    }
#endif // ifndef BUILD_NO_DEBUG
  }

  if (lcAction == F("endif")) // conditional block ends here
  {
    if (fakeIfBlock) {
      fakeIfBlock--;
    }
    else if (ifBlock) {
      ifBlock--;
    }
    isCommand = false;
  }

  // process the action if it's a command and unconditional, or conditional and
  // the condition matches the if or else block.
  if (isCommand) {
    substitute_eventvalue(action, event);

    if (loglevelActiveFor(LOG_LEVEL_INFO)) {
      String log = F("ACT  : ");
      log += action;
      addLog(LOG_LEVEL_INFO, log);
    }

    ExecuteCommand_all(VALUE_SOURCE_RULES, action.c_str());
    delay(0);
  }
}

/********************************************************************************************\
   Check if an event matches to a given rule
 \*********************************************************************************************/
bool ruleMatch(const String& event, const String& rule) {
  checkRAM(F("ruleMatch"));

  String tmpEvent = event;
  String tmpRule  = rule;
  tmpEvent.trim();
  tmpRule.trim();

  // Ignore escape char
  tmpRule.replace("[", "");
  tmpRule.replace("]", "");

  if (tmpEvent.equalsIgnoreCase(tmpRule)) {
    return true;
  }


  // Special handling of literal string events, they should start with '!'
  if (event.charAt(0) == '!') {
    const int pos = rule.indexOf('*');

    if (pos != -1) // a * sign in rule, so use a'wildcard' match on message
    {
      return event.substring(0, pos - 1).equalsIgnoreCase(rule.substring(0, pos - 1));
    } else {
      const bool pound_char_found = rule.indexOf('#') != -1;

      if (!pound_char_found)
      {
        // no # sign in rule, use 'wildcard' match on event 'source'
        return event.substring(0, rule.length()).equalsIgnoreCase(rule);
      }
    }
    return tmpEvent.equalsIgnoreCase(tmpRule);
  }

  if (event.startsWith(
        F("Clock#Time"))) // clock events need different handling...
  {
    int pos1 = event.indexOf("=");
    int pos2 = rule.indexOf("=");

    if ((pos1 > 0) && (pos2 > 0)) {
      if (event.substring(0, pos1).equalsIgnoreCase(rule.substring(0, pos2))) // if this is a clock rule
      {
        unsigned long clockEvent = string2TimeLong(event.substring(pos1 + 1));
        unsigned long clockSet   = string2TimeLong(rule.substring(pos2 + 1));

        return matchClockEvent(clockEvent, clockSet);
      }
    } else {
      // Not supported yet, see: https://github.com/letscontrolit/ESPEasy/issues/2640
      return false;
    }
  }

  // parse event into verb and value
  float value = 0;
  int   pos   = event.indexOf("=");

  if (pos >= 0) {
    if (!validFloatFromString(event.substring(pos + 1), value)) {
      return false;

      // FIXME TD-er: What to do when trying to match NaN values?
    }
    tmpEvent = event.substring(0, pos);
  }

  // parse rule
  int  posStart, posEnd;
  char compare;

  if (!findCompareCondition(rule, compare, posStart, posEnd)) {
    // No compare condition found, so just check if the event- and rule string match.
    return tmpEvent.equalsIgnoreCase(rule);
  }

  const bool stringMatch = tmpEvent.equalsIgnoreCase(rule.substring(0, posStart));
  float ruleValue        = 0;

  if (!validFloatFromString(rule.substring(posEnd), ruleValue)) {
    return false;

    // FIXME TD-er: What to do when trying to match NaN values?
  }

  bool match = false;

  if (stringMatch) {
    match = compareValues(compare, value, ruleValue);
  }
  checkRAM(F("ruleMatch2"));
  return match;
}

/********************************************************************************************\
   Check expression
 \*********************************************************************************************/
bool conditionMatchExtended(String& check) {
  int    condAnd   = -1;
  int    condOr    = -1;
  bool   rightcond = false;
  bool   leftcond  = conditionMatch(check); // initial check
  #ifndef BUILD_NO_DEBUG
  String debugstr;
  if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
    debugstr += boolToString(leftcond);
  }
  #endif

  do {
    #ifndef BUILD_NO_DEBUG
    if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
      String log = F("conditionMatchExtended: ");
      log += debugstr;
      log += '_';
      log += check;
      addLog(LOG_LEVEL_DEBUG, log);
    }
    #endif
    condAnd = check.indexOf(F(" and "));
    condOr  = check.indexOf(F(" or "));

    if ((condAnd > 0) || (condOr > 0)) {                             // we got AND/OR
      if ((condAnd > 0) && (((condOr < 0) && (condOr < condAnd)) ||
                            ((condOr > 0) && (condOr > condAnd)))) { // AND is first
        check     = check.substring(condAnd + 5);
        rightcond = conditionMatch(check);
        leftcond  = (leftcond && rightcond);

        #ifndef BUILD_NO_DEBUG
        if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
          debugstr += F(" && ");
        }
        #endif
      } else { // OR is first
        check     = check.substring(condOr + 4);
        rightcond = conditionMatch(check);
        leftcond  = (leftcond || rightcond);

        #ifndef BUILD_NO_DEBUG
        if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
          debugstr += F(" || ");
        }
        #endif
      }
      
      #ifndef BUILD_NO_DEBUG
      if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
        debugstr += boolToString(rightcond);
      }
      #endif
    }
  } while (condAnd > 0 || condOr > 0);

  #ifndef BUILD_NO_DEBUG
  if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
    check = debugstr;
  }
  #endif
  return leftcond;
}

// Find the compare condition.
// @param posStart = first position of the compare condition in the string
// @param posEnd   = first position rest of the string, right after the compare condition.
bool findCompareCondition(const String& check, char& compare, int& posStart, int& posEnd)
{
  posStart = check.length();
  posEnd   = posStart;
  int comparePos = 0;
  bool found = false;

  if (((comparePos = check.indexOf("!=")) > 0) && (comparePos < posStart)) {
    posStart = comparePos;
    posEnd   = posStart + 2;
    compare  = '!' + '=';
    found = true;
  }

  if (((comparePos = check.indexOf("<>")) > 0) && (comparePos < posStart)) {
    posStart = comparePos;
    posEnd   = posStart + 2;
    compare  = '!' + '=';
    found = true;
  }

  if (((comparePos = check.indexOf(">=")) > 0) && (comparePos < posStart)) {
    posStart = comparePos;
    posEnd   = posStart + 2;
    compare  = '>' + '=';
    found = true;
  }

  if (((comparePos = check.indexOf("<=")) > 0) && (comparePos < posStart)) {
    posStart = comparePos;
    posEnd   = posStart + 2;
    compare  = '<' + '=';
    found = true;
  }

  if (((comparePos = check.indexOf("<")) > 0) && (comparePos < posStart)) {
    posStart = comparePos;
    posEnd   = posStart + 1;
    compare  = '<';
    found = true;
  }

  if (((comparePos = check.indexOf(">")) > 0) && (comparePos < posStart)) {
    posStart = comparePos;
    posEnd   = posStart + 1;
    compare  = '>';
    found = true;
  }

  if (((comparePos = check.indexOf("=")) > 0) && (comparePos < posStart)) {
    posStart = comparePos;
    posEnd   = posStart + 1;
    compare  = '=';
    found = true;
  }
  return found;
}

bool compareValues(char compare, float Value1, float Value2)
{
  switch (compare) {
    case '>' + '=': return Value1 >= Value2;
    case '<' + '=': return Value1 <= Value2;
    case '!' + '=': return Value1 != Value2;
    case '>':       return Value1 > Value2;
    case '<':       return Value1 < Value2;
    case '=':       return Value1 == Value2;
  }
  return false;
}

bool conditionMatch(const String& check) {
  int  posStart, posEnd;
  char compare;

  if (!findCompareCondition(check, compare, posStart, posEnd)) {
    return false;
  }

  String tmpCheck1 = check.substring(0, posStart);
  String tmpCheck2 = check.substring(posEnd);
  float  Value1    = 0;
  float  Value2    = 0;

  int  timeInSec1 = 0;
  int  timeInSec2 = 0;
  bool validTime1 = timeStringToSeconds(tmpCheck1, timeInSec1);
  bool validTime2 = timeStringToSeconds(tmpCheck2, timeInSec2);

  if ((validTime1 || validTime2) && (timeInSec1 != -1) && (timeInSec2 != -1))
  {
    // At least one is a time containing ':' separator
    // AND both can be considered a time, so use it as a time and compare seconds.
    Value1 = timeInSec1;
    Value2 = timeInSec2;
  } else {
    if (!validFloatFromString(tmpCheck1, Value1) ||
        !validFloatFromString(tmpCheck2, Value2))
    {
      return false;
    }
  }

  bool result = compareValues(compare, Value1, Value2);
  #ifndef BUILD_NO_DEBUG
  if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
    String log = F("compareValues: _");
    log += check;
    log += F("_ val1: ");
    log += Value1;
    log += F(" val2: ");
    log += Value2;
    log += F(" = ");
    log += boolToString(result);
    addLog(LOG_LEVEL_DEBUG, log);
  }
  #endif
  return result;
}

/********************************************************************************************\
   Check rule timers
 \*********************************************************************************************/
void rulesTimers() {
  if (!Settings.UseRules) {
    return;
  }
  // FIXME TD-er:  Maybe not use the timer struct, but add the timers to the scheduler?

  for (byte x = 0; x < RULES_TIMER_MAX; x++) {
    if (!RulesTimer[x].paused && (RulesTimer[x].timestamp != 0L)) // timer active?
    {
      if (timeOutReached(RulesTimer[x].timestamp))                // timer finished?
      {
        RulesTimer[x].timestamp = 0L;                             // turn off this timer
        String event = F("Rules#Timer=");
        event += x + 1;
        rulesProcessing(event); // TD-er: Do not add to the eventQueue, but execute right now.
      }
    }
  }
}

/********************************************************************************************\
   Generate rule events based on task refresh
 \*********************************************************************************************/
void createRuleEvents(struct EventStruct *event) {
  if (!Settings.UseRules) {
    return;
  }
  const deviceIndex_t DeviceIndex = getDeviceIndex_from_TaskIndex(event->TaskIndex);

  if (!validDeviceIndex(DeviceIndex)) { return; }

  LoadTaskSettings(event->TaskIndex);
  byte BaseVarIndex = event->TaskIndex * VARS_PER_TASK;
  byte sensorType   = Device[DeviceIndex].VType;

  for (byte varNr = 0; varNr < Device[DeviceIndex].ValueCount; varNr++) {
    String eventString;
    eventString.reserve(32); // Enough for most use cases, prevent lots of memory allocations.
    eventString  = getTaskDeviceName(event->TaskIndex);
    eventString += F("#");
    eventString += ExtraTaskSettings.TaskDeviceValueNames[varNr];
    eventString += F("=");

    switch (sensorType) {
      case SENSOR_TYPE_LONG:
        eventString += (unsigned long)UserVar[BaseVarIndex] +
                       ((unsigned long)UserVar[BaseVarIndex + 1] << 16);
        break;
      case SENSOR_TYPE_STRING:

        // FIXME TD-er: What to add here? length of string?
        break;
      default:

        // FIXME TD-er: Do we need to call formatUserVarNoCheck here? (or with check)
        eventString += UserVar[BaseVarIndex + varNr];
        break;
    }
    eventQueue.add(eventString);
  }
}
