#ifdef USES_P022

#include "src/DataStructs/PinMode.h"

// #######################################################################################################
// #################################### Plugin 022: PCA9685 ##############################################
// #######################################################################################################


#define PLUGIN_022
#define PLUGIN_ID_022         22
#define PLUGIN_NAME_022       "Extra IO - PCA9685"
#define PLUGIN_VALUENAME1_022 "PWM"

#define PLUGIN_022_PCA9685_MODE1   0x00 // location for Mode1 register address
#define PCA9685_MODE2              0x01 // location for Mode2 register address
#define PCA9685_MODE2_VALUES       0x20
#define PCA9685_LED0               0x06 // location for start of LED0 registers
#define PCA9685_ADDRESS            0x40 // I2C address
#define PCA9685_MAX_ADDRESS        0x7F
#define PCA9685_NUMS_ADDRESS       (PCA9685_MAX_ADDRESS - PCA9685_ADDRESS)
#define PCA9685_MAX_PINS           15
#define PCA9685_MAX_PWM            4095
#define PCA9685_MIN_FREQUENCY      23.0   // Min possible PWM cycle frequency
#define PCA9685_MAX_FREQUENCY      1500.0 // Max possible PWM cycle frequency
#define PCA9685_ALLLED_REG         (byte)0xFA


// Bit mask to keep track of addresses initialized.
static uint32_t initializeState_lo = 0;
static uint32_t initializeState_hi = 0;

bool p022_is_init(uint8_t address) {
  if ((address < PCA9685_ADDRESS) || (address > PCA9685_MAX_ADDRESS)) { return false; }
  uint32_t address_offset = address - PCA9685_ADDRESS;

  if (address_offset < 32) {
    return initializeState_lo & (1 << address_offset);
  } else {
    return initializeState_hi & (1 << (address_offset - 32));
  }
}

bool p022_set_init(uint8_t address) {
  if ((address < PCA9685_ADDRESS) || (address > PCA9685_MAX_ADDRESS)) { return false; }
  uint32_t address_offset = address - PCA9685_ADDRESS;

  if (address_offset < 32) {
    initializeState_lo |= (1 << address_offset);
  } else {
    initializeState_hi |= (1 << (address_offset - 32));
  }
  return true;
}

bool p022_clear_init(uint8_t address) {
  if ((address < PCA9685_ADDRESS) || (address > PCA9685_MAX_ADDRESS)) { return false; }
  uint32_t address_offset = address - PCA9685_ADDRESS;

  if (address_offset < 32) {
    initializeState_lo &= ~(1 << address_offset);
  } else {
    initializeState_hi &= ~(1 << (address_offset - 32));
  }
  return true;
}


boolean Plugin_022(byte function, struct EventStruct *event, String& string)
{
  boolean  success = false;
  int      address = 0;
  int      mode2   = 0x10;
  uint16_t freq    = PCA9685_MAX_FREQUENCY;
  uint16_t range   = PCA9685_MAX_PWM;

  if ((event != NULL) && (event->TaskIndex >= 0))
  {
    address = CONFIG_PORT;
    mode2   = PCONFIG(0);
    freq    = PCONFIG(1);
    range   = PCONFIG(2);
  }

  if ((address < PCA9685_ADDRESS) || (address > PCA9685_MAX_ADDRESS)) {
    address = PCA9685_ADDRESS;
  }

  if (freq == 0) {
    freq = PCA9685_MAX_FREQUENCY;
  }

  if (range == 0) {
    range = PCA9685_MAX_PWM;
  }

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
    {
      Device[++deviceCount].Number           = PLUGIN_ID_022;
      Device[deviceCount].Type               = DEVICE_TYPE_I2C;
      Device[deviceCount].VType              = SENSOR_TYPE_NONE;
      Device[deviceCount].Ports              = 1;
      Device[deviceCount].PullUpOption       = false;
      Device[deviceCount].InverseLogicOption = false;
      Device[deviceCount].FormulaOption      = false;
      Device[deviceCount].ValueCount         = 0;
      Device[deviceCount].Custom             = true;
      Device[deviceCount].TimerOption        = false;
      break;
    }

    case PLUGIN_GET_DEVICENAME:
    {
      string = F(PLUGIN_NAME_022);
      break;
    }

    case PLUGIN_GET_DEVICEVALUENAMES:
    {
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_022));
      break;
    }

    case PLUGIN_WEBFORM_LOAD:
    {
      // The options lists are quite long.
      // To prevent stack overflow issues, each selection has its own scope.
      {
        int optionValues[PCA9685_NUMS_ADDRESS];

        for (int i = 0; i < PCA9685_NUMS_ADDRESS; i++)
        {
          optionValues[i] = PCA9685_ADDRESS + i;
        }
        addFormSelectorI2C(F("i2c_addr"), PCA9685_NUMS_ADDRESS, optionValues, address);
      }
      {
        String m2Options[PCA9685_MODE2_VALUES];
        int    m2Values[PCA9685_MODE2_VALUES];

        for (int i = 0; i < PCA9685_MODE2_VALUES; i++)
        {
          m2Values[i]  = i;
          m2Options[i] = formatToHex_decimal(i);

          if (i == 0x10) {
            m2Options[i] += F(" - (default)");
          }
        }
        addFormSelector(F("MODE2"), F("p022_mode2"), PCA9685_MODE2_VALUES, m2Options, m2Values, mode2);
      }
      {
        String freqString = F("Frequency (");
        freqString += PCA9685_MIN_FREQUENCY;
        freqString += '-';
        freqString += PCA9685_MAX_FREQUENCY;
        freqString += ')';
        addFormNumericBox(freqString, F("p022_freq"), freq, PCA9685_MIN_FREQUENCY, PCA9685_MAX_FREQUENCY);
      }
      {
        String funitString = F("default ");
        funitString += PCA9685_MAX_FREQUENCY;
        addUnit(funitString);
      }
      {
        addFormNumericBox(F("Range (1-10000)"), F("p022_range"), range, 1, 10000);
        String runitString = F("default ");
        runitString += PCA9685_MAX_PWM;
        addUnit(runitString);
      }
      success = true;
      break;
    }

    case PLUGIN_WEBFORM_SAVE:
    {
      p022_clear_init(CONFIG_PORT);
      CONFIG_PORT = getFormItemInt(F("i2c_addr"));
      PCONFIG(0)  = getFormItemInt(F("p022_mode2"));
      PCONFIG(1)  = getFormItemInt(F("p022_freq"));
      PCONFIG(2)  = getFormItemInt(F("p022_range"));

      if (!p022_is_init(CONFIG_PORT))
      {
        Plugin_022_initialize(address);
        if (PCONFIG(0) != mode2) {
          Plugin_022_writeRegister(address, PCA9685_MODE2, PCONFIG(0));
        }

        if (PCONFIG(1) != freq) {
          Plugin_022_Frequency(address, PCONFIG(1));
        }
      }
      success = true;
      break;
    }

    case PLUGIN_WRITE:
    {
      String log            = "";
      String line           = String(string);
      String command        = "";
      int    dotPos         = line.indexOf('.');
      bool   istanceCommand = false;

      if (dotPos > -1)
      {
        LoadTaskSettings(event->TaskIndex);
        String name = line.substring(0, dotPos);
        name.replace("[", "");
        name.replace("]", "");

        if (name.equalsIgnoreCase(getTaskDeviceName(event->TaskIndex)) == true)
        {
          line           = line.substring(dotPos + 1);
          istanceCommand = true;
        }
        else
        {
          break;
        }
      }
      command = parseString(line, 1);

      if ((command == F("pcapwm")) || (istanceCommand && (command == F("pwm"))))
      {
        success = true;
        log     = String(F("PCA 0x")) + String(address, HEX) + String(F(": PWM ")) + String(event->Par1);

        if ((event->Par1 >= 0) && (event->Par1 <= PCA9685_MAX_PINS))
        {
          if ((event->Par2 >= 0) && (event->Par2 <= range))
          {
            if (!p022_is_init(address))
            {
              Plugin_022_initialize(address);
              Plugin_022_writeRegister(address, PCA9685_MODE2, mode2);
              Plugin_022_Frequency(address, freq);
            }
            Plugin_022_Write(address, event->Par1, map(event->Par2, 0, range, 0, PCA9685_MAX_PWM));

            // setPinState(PLUGIN_ID_022, event->Par1, PIN_MODE_PWM, event->Par2);
            portStatusStruct newStatus;
            const uint32_t   key = createKey(PLUGIN_ID_022, event->Par1);

            // WARNING: operator [] creates an entry in the map if key does not exist
            newStatus         = globalMapPortStatus[key];
            newStatus.command = 1;
            newStatus.mode    = PIN_MODE_PWM;
            newStatus.state   = event->Par2;
            savePortStatus(key, newStatus);

            addLog(LOG_LEVEL_INFO, log);

            // SendStatus(event->Source, getPinStateJSON(SEARCH_PIN_STATE, PLUGIN_ID_022, event->Par1, log, 0));
            SendStatusOnlyIfNeeded(event->Source, SEARCH_PIN_STATE, key, log, 0);
          }
          else {
            addLog(LOG_LEVEL_ERROR, log + String(F(" the pwm value ")) + String(event->Par2) + String(F(" is invalid value.")));
          }
        }
        else {
          addLog(LOG_LEVEL_ERROR, log + String(F(" is invalid value.")));
        }
      }

      if ((command == F("pcafrq")) || (istanceCommand && (command == F("frq"))))
      {
        success = true;

        if ((event->Par1 >= PCA9685_MIN_FREQUENCY) && (event->Par1 <= PCA9685_MAX_FREQUENCY))
        {
          if (!p022_is_init(address))
          {
            Plugin_022_initialize(address);
            Plugin_022_writeRegister(address, PCA9685_MODE2, mode2);
          }
          Plugin_022_Frequency(address, event->Par1);

          // setPinState(PLUGIN_ID_022, 99, PIN_MODE_UNDEFINED, event->Par1);
          portStatusStruct newStatus;
          const uint32_t   key = createKey(PLUGIN_ID_022, 99);

          // WARNING: operator [] creates an entry in the map if key does not exist
          newStatus         = globalMapPortStatus[key];
          newStatus.command = 1;
          newStatus.mode    = PIN_MODE_UNDEFINED;
          newStatus.state   = event->Par1;
          savePortStatus(key, newStatus);

          log = String(F("PCA 0x")) + String(address, HEX) + String(F(": FREQ ")) + String(event->Par1);
          addLog(LOG_LEVEL_INFO, log);

          // SendStatus(event->Source, getPinStateJSON(SEARCH_PIN_STATE, PLUGIN_ID_022, 99, log, 0));
          SendStatusOnlyIfNeeded(event->Source, SEARCH_PIN_STATE, key, log, 0);
        }
        else {
          addLog(LOG_LEVEL_ERROR,
                 String(F("PCA 0x")) +
                 String(address, HEX) + String(F(" The frequency ")) + String(event->Par1) + String(F(" is out of range.")));
        }
      }

      if (istanceCommand && (command == F("mode2")))
      {
        success = true;

        if ((event->Par1 >= 0) && (event->Par1 < PCA9685_MODE2_VALUES))
        {
          if (!p022_is_init(address))
          {
            Plugin_022_initialize(address);
            Plugin_022_Frequency(address, freq);
          }
          Plugin_022_writeRegister(address, PCA9685_MODE2, event->Par1);
          log = String(F("PCA 0x")) + String(address, HEX) + String(F(": MODE2 0x")) + String(event->Par1, HEX);
          addLog(LOG_LEVEL_INFO, log);
        }
        else {
          addLog(LOG_LEVEL_ERROR,
                 String(F("PCA 0x")) +
                 String(address, HEX) + String(F(" MODE2 0x")) + String(event->Par1, HEX) + String(F(" is out of range.")));
        }
      }

      if (command == F("status"))
      {
        if (parseString(line, 2) == F("pca"))
        {
          if (!p022_is_init(address))
          {
            Plugin_022_initialize(address);
            Plugin_022_writeRegister(address, PCA9685_MODE2, mode2);
            Plugin_022_Frequency(address, freq);
          }
          success = true;

          // SendStatus(event->Source, getPinStateJSON(SEARCH_PIN_STATE, PLUGIN_ID_022, event->Par2, dummyString, 0));
          SendStatusOnlyIfNeeded(event->Source, SEARCH_PIN_STATE, createKey(PLUGIN_ID_022, event->Par2), dummyString, 0);
        }
      }

      if (istanceCommand && (command == F("gpio")))
      {
        success = true;
        log     = String(F("PCA 0x")) + String(address, HEX) + String(F(": GPIO "));

        if ((event->Par1 >= 0) && (event->Par1 <= PCA9685_MAX_PINS))
        {
          if (!p022_is_init(address))
          {
            Plugin_022_initialize(address);
            Plugin_022_writeRegister(address, PCA9685_MODE2, mode2);
            Plugin_022_Frequency(address, freq);
          }
          int pin = event->Par1;

          if (parseString(line, 2) == "all")
          {
            pin  = -1;
            log += String(F("all"));
          }
          else
          {
            log += String(pin);
          }

          if (event->Par2 == 0)
          {
            log += F(" off");
            Plugin_022_Off(address, pin);
          }
          else
          {
            log += F(" on");
            Plugin_022_On(address, pin);
          }
          addLog(LOG_LEVEL_INFO, log);

          // setPinState(PLUGIN_ID_022, pin, PIN_MODE_OUTPUT, event->Par2);
          portStatusStruct newStatus;
          const uint32_t   key = createKey(PLUGIN_ID_022, pin);

          // WARNING: operator [] creates an entry in the map if key does not exist
          newStatus         = globalMapPortStatus[key];
          newStatus.command = 1;
          newStatus.mode    = PIN_MODE_OUTPUT;
          newStatus.state   = event->Par2;
          savePortStatus(key, newStatus);

          // SendStatus(event->Source, getPinStateJSON(SEARCH_PIN_STATE, PLUGIN_ID_022, pin, log, 0));
          SendStatusOnlyIfNeeded(event->Source, SEARCH_PIN_STATE, key, log, 0);
        }
        else {
          addLog(LOG_LEVEL_ERROR, log + String(F(" is invalid value.")));
        }
      }

      if (istanceCommand && (command == F("pulse")))
      {
        success = true;
        log     = String(F("PCA 0x")) + String(address, HEX) + String(F(": GPIO ")) + String(event->Par1);

        if ((event->Par1 >= 0) && (event->Par1 <= PCA9685_MAX_PINS))
        {
          if (!p022_is_init(address))
          {
            Plugin_022_initialize(address);
            Plugin_022_writeRegister(address, PCA9685_MODE2, mode2);
            Plugin_022_Frequency(address, freq);
          }

          if (event->Par2 == 0)
          {
            log += F(" off");
            Plugin_022_Off(address, event->Par1);
          }
          else
          {
            log += F(" on");
            Plugin_022_On(address, event->Par1);
          }
          log += String(F(" Pulse set for ")) + event->Par3;
          log += String(F("ms"));
          int autoreset = 0;

          if (event->Par3 > 0)
          {
            if (parseString(line, 5) == F("auto"))
            {
              autoreset = -1;
              log      += String(F(" with autoreset infinity"));
            }
            else
            {
              autoreset = event->Par4;

              if (autoreset > 0)
              {
                log += String(F(" for "));
                log += String(autoreset);
              }
            }
          }
          setPluginTaskTimer(event->Par3
                             , event->TaskIndex
                             , event->Par1
                             , !event->Par2
                             , event->Par3
                             , autoreset);

          // setPinState(PLUGIN_ID_022, event->Par1, PIN_MODE_OUTPUT, event->Par2);
          portStatusStruct newStatus;
          const uint32_t   key = createKey(PLUGIN_ID_022, event->Par1);

          // WARNING: operator [] creates an entry in the map if key does not exist
          newStatus         = globalMapPortStatus[key];
          newStatus.command = 1;
          newStatus.mode    = PIN_MODE_OUTPUT;
          newStatus.state   = event->Par2;
          savePortStatus(key, newStatus);

          addLog(LOG_LEVEL_INFO, log);

          // SendStatus(event->Source, getPinStateJSON(SEARCH_PIN_STATE, PLUGIN_ID_022, event->Par1, log, 0));
          SendStatusOnlyIfNeeded(event->Source, SEARCH_PIN_STATE, key, log, 0);
        }
        else {
          addLog(LOG_LEVEL_ERROR, log + String(F(" is invalid value.")));
        }
      }

      break;
    }
    case PLUGIN_TIMER_IN:
    {
      String log       = String(F("PCA 0x")) + String(address, HEX) + String(F(": GPIO ")) + String(event->Par1);
      int    autoreset = event->Par4;

      if (event->Par2 == 0)
      {
        log += F(" off");
        Plugin_022_Off(address, event->Par1);
      }
      else
      {
        log += F(" on");
        Plugin_022_On(address, event->Par1);
      }

      if ((autoreset > 0) || (autoreset == -1))
      {
        if (autoreset > -1)
        {
          log += String(F(" Pulse auto restart for "));
          log += String(autoreset);
          autoreset--;
        }
        setPluginTaskTimer(event->Par3
                           , event->TaskIndex
                           , event->Par1
                           , !event->Par2
                           , event->Par3
                           , autoreset);
      }

      // setPinState(PLUGIN_ID_022, event->Par1, PIN_MODE_OUTPUT, event->Par2);
      portStatusStruct newStatus;
      const uint32_t   key = createKey(PLUGIN_ID_022, event->Par1);

      // WARNING: operator [] creates an entry in the map if key does not exist
      newStatus         = globalMapPortStatus[key];
      newStatus.command = 1;
      newStatus.mode    = PIN_MODE_OUTPUT;
      newStatus.state   = event->Par2;
      savePortStatus(key, newStatus);

      // SendStatus(event->Source, getPinStateJSON(SEARCH_PIN_STATE, PLUGIN_ID_022, event->Par1, log, 0));
      SendStatusOnlyIfNeeded(event->Source, SEARCH_PIN_STATE, key, log, 0);
      break;
    }
  }
  return success;
}

// ********************************************************************************
// PCA9685 config
// ********************************************************************************
void Plugin_022_writeRegister(int i2cAddress, int regAddress, byte data) {
  Wire.beginTransmission(i2cAddress);
  Wire.write(regAddress);
  Wire.write(data);
  Wire.endTransmission();
}

uint8_t Plugin_022_readRegister(int i2cAddress, int regAddress) {
  uint8_t res = 0;

  Wire.requestFrom(i2cAddress, 1, 1);

  while (Wire.available()) {
    res = Wire.read();
  }
  return res;
}

// ********************************************************************************
// PCA9685 write
// ********************************************************************************
void Plugin_022_Off(int address, int pin)
{
  Plugin_022_Write(address, pin, 0);
}

void Plugin_022_On(int address, int pin)
{
  Plugin_022_Write(address, pin, PCA9685_MAX_PWM);
}

void Plugin_022_Write(int address, int Par1, int Par2)
{
  int i2cAddress = address;

  // boolean success = false;
  int regAddress = Par1 == -1
                   ? PCA9685_ALLLED_REG
                   : PCA9685_LED0 + 4 * Par1;
  uint16_t LED_ON  = 0;
  uint16_t LED_OFF = Par2;

  Wire.beginTransmission(i2cAddress);
  Wire.write(regAddress);
  Wire.write(lowByte(LED_ON));
  Wire.write(highByte(LED_ON));
  Wire.write(lowByte(LED_OFF));
  Wire.write(highByte(LED_OFF));
  Wire.endTransmission();
}

void Plugin_022_Frequency(int address, uint16_t freq)
{
  int i2cAddress = address;

  Plugin_022_writeRegister(i2cAddress, PLUGIN_022_PCA9685_MODE1, (byte)0x0);
  freq *= 0.9;

  //  prescale = 25000000 / 4096;
  uint16_t prescale = 6103;
  prescale /=  freq;
  prescale -= 1;
  uint8_t oldmode = Plugin_022_readRegister(i2cAddress, 0);
  uint8_t newmode = (oldmode & 0x7f) | 0x10;
  Plugin_022_writeRegister(i2cAddress, PLUGIN_022_PCA9685_MODE1, (byte)newmode);
  Plugin_022_writeRegister(i2cAddress, 0xfe,                     (byte)prescale); // prescale register
  Plugin_022_writeRegister(i2cAddress, PLUGIN_022_PCA9685_MODE1, (byte)oldmode);
  delayMicroseconds(5000);
  Plugin_022_writeRegister(i2cAddress, PLUGIN_022_PCA9685_MODE1, (byte)oldmode | 0xa1);
}

void Plugin_022_initialize(int address)
{
  int i2cAddress = address;

  // default mode is open drain output, drive leds connected to VCC
  Plugin_022_writeRegister(i2cAddress, PLUGIN_022_PCA9685_MODE1, (byte)0x01);      // reset the device
  delay(1);
  Plugin_022_writeRegister(i2cAddress, PLUGIN_022_PCA9685_MODE1, (byte)B10100000); // set up for auto increment
  // Plugin_022_writeRegister(i2cAddress, PCA9685_MODE2, (byte)0x10); // set to output
  p022_set_init(address);
}

#endif // USES_P022
