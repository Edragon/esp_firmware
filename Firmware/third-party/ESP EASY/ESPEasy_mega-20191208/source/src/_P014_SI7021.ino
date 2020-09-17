#ifdef USES_P014
//#######################################################################################################
//######################## Plugin 014 SI7021 I2C Temperature Humidity Sensor  ###########################
//#######################################################################################################
// 12-10-2015 Charles-Henri Hallard, see my projects and blog at https://hallard.me

#define PLUGIN_014
#define PLUGIN_ID_014        14
#define PLUGIN_NAME_014       "Environment - SI7021/HTU21D"
#define PLUGIN_VALUENAME1_014 "Temperature"
#define PLUGIN_VALUENAME2_014 "Humidity"

// ======================================
// SI7021 sensor
// ======================================
#define SI7021_I2C_ADDRESS      0x40 // I2C address for the sensor
#define SI7021_MEASURE_TEMP_HUM 0xE0 // Measure Temp only after a RH conversion done
#define SI7021_MEASURE_TEMP_HM  0xE3 // Default hold Master
#define SI7021_MEASURE_HUM_HM   0xE5 // Default hold Master
#define SI7021_MEASURE_TEMP     0xF3 // No hold
#define SI7021_MEASURE_HUM      0xF5 // No hold
#define SI7021_WRITE_REG        0xE6
#define SI7021_READ_REG         0xE7
#define SI7021_SOFT_RESET       0xFE

// SI7021 Sensor resolution
// default at power up is SI7021_RESOLUTION_14T_12RH
#define SI7021_RESOLUTION_14T_12RH 0x00 // 12 bits RH / 14 bits Temp
#define SI7021_RESOLUTION_13T_10RH 0x80 // 10 bits RH / 13 bits Temp
#define SI7021_RESOLUTION_12T_08RH 0x01 //  8 bits RH / 12 bits Temp
#define SI7021_RESOLUTION_11T_11RH 0x81 // 11 bits RH / 11 bits Temp
#define SI7021_RESOLUTION_MASK 0B01111110

enum SI7021_state {
  SI7021_Uninitialized = 0,
  SI7021_Initialized,
  SI7021_Wait_for_temperature_samples,
  SI7021_Wait_for_humidity_samples,
  SI7021_New_values,
  SI7021_Values_read
};

SI7021_state state = SI7021_Uninitialized;
uint16_t si7021_humidity;    // latest humidity value read
int16_t  si7021_temperature; // latest temperature value read (*100)

boolean Plugin_014(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_014;
        Device[deviceCount].Type = DEVICE_TYPE_I2C;
        Device[deviceCount].VType = SENSOR_TYPE_TEMP_HUM;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 2;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_014);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_014));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_014));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        #define SI7021_RESOLUTION_OPTION 4

        byte choice = PCONFIG(0);
        String options[SI7021_RESOLUTION_OPTION];
        int optionValues[SI7021_RESOLUTION_OPTION];
        optionValues[0] = SI7021_RESOLUTION_14T_12RH;
        options[0] = F("Temp 14 bits / RH 12 bits");
        optionValues[1] = SI7021_RESOLUTION_13T_10RH;
        options[1] = F("Temp 13 bits / RH 10 bits");
        optionValues[2] = SI7021_RESOLUTION_12T_08RH;
        options[2] = F("Temp 12 bits / RH  8 bits");
        optionValues[3] = SI7021_RESOLUTION_11T_11RH;
        options[3] = F("Temp 11 bits / RH 11 bits");
        addFormSelector(F("Resolution"), F("p014_res"), SI7021_RESOLUTION_OPTION, options, optionValues, choice);
        //addUnit(F("bits"));

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        PCONFIG(0) = getFormItemInt(F("p014_res"));
        state = SI7021_Uninitialized; // Force device setup next time
        success = true;
        break;
      }

      
    case PLUGIN_INIT:
      {
        // Get sensor resolution configuration
        uint8_t res = PCONFIG(0);

        // Init sensor
        if (state == SI7021_Uninitialized) {
          success = Plugin_014_si7021_begin(res);
          if (success) {
            state = SI7021_Initialized;
          }
        }
        break;
      }

    case PLUGIN_TEN_PER_SECOND:
      {
        // Get sensor resolution configuration
        uint8_t res = PCONFIG(0);

        switch (state)
        {
          case SI7021_Initialized:
            {
              // Start conversion for humidity
              Plugin_014_si7021_startConv(SI7021_MEASURE_HUM);
              // change state of sensor
              state = SI7021_Wait_for_temperature_samples;
              success = true;
            }
          break;

          case SI7021_Wait_for_temperature_samples:
            {
              // Check if conversion is finished
              if (Plugin_014_si7021_readValues(SI7021_MEASURE_HUM, res) == 0) {
                // Start conversion for temperature
                Plugin_014_si7021_startConv(SI7021_MEASURE_TEMP);
                // change state of sensor
                state = SI7021_Wait_for_humidity_samples;
                success = true;
              }
            }
          break;

          case SI7021_Wait_for_humidity_samples:
            {
              // Check if conversion is finished
              if (Plugin_014_si7021_readValues(SI7021_MEASURE_TEMP, res) == 0) {
                // Update was succesfull, schedule a read.
                schedule_task_device_timer(event->TaskIndex, millis() + 10);
                // change state of sensor
                state = SI7021_New_values;
                success = true;
              }
            }
          break;
        
          default:
          break;
        }

        break;
      }

    case PLUGIN_READ:
      {
        // Change state of sensor for non bloking reading 
        if (state == SI7021_Values_read) {
          state = SI7021_Initialized;
        }
        // New value
        else if (state == SI7021_New_values) {
          UserVar[event->BaseVarIndex] = si7021_temperature / 100.0;
          UserVar[event->BaseVarIndex + 1] = si7021_humidity / 10.0;
          state = SI7021_Values_read;
          success = true;
          if (loglevelActiveFor(LOG_LEVEL_INFO)) {
            String log = F("SI7021 : Temperature: ");
            log += UserVar[event->BaseVarIndex];
            addLog(LOG_LEVEL_INFO,log);
            log = F("SI7021 : Humidity: ");
            log += UserVar[event->BaseVarIndex + 1];
            addLog(LOG_LEVEL_INFO,log);
          }
        } else if (state == SI7021_Uninitialized) {
          addLog(LOG_LEVEL_INFO,F("SI7021 : sensor not initialized !"));
        }
        else {
          addLog(LOG_LEVEL_INFO,F("SI7021 : Read Error !"));
        }

        break;
      }

  }
  return success;
}

/* ======================================================================
Function: Plugin_014_si7021_begin
Purpose : read the user register from the sensor
Input   : user register value filled by function
Output  : true if okay
Comments: -
====================================================================== */
boolean Plugin_014_si7021_begin(uint8_t resolution)
{
  uint8_t ret;

  // Set the resolution we want
  ret = Plugin_014_si7021_setResolution(resolution);
  if ( ret == 0 ) {
    ret = true;
  } else {
    String log = F("SI7021 : Res=0x");
    log += String(resolution,HEX);
    log += F(" => Error 0x");
    log += String(ret,HEX);
    addLog(LOG_LEVEL_INFO,log);
    ret = false;
  }

  return ret;
}

/* ======================================================================
Function: Plugin_014_si7021_checkCRC
Purpose : check the CRC of received data
Input   : value read from sensor
Output  : CRC read from sensor
Comments: 0 if okay
====================================================================== */
uint8_t Plugin_014_si7021_checkCRC(uint16_t data, uint8_t check)
{
  uint32_t remainder, divisor;

  //Pad with 8 bits because we have to add in the check value
  remainder = (uint32_t)data << 8;

  // From: http://www.nongnu.org/avr-libc/user-manual/group__util__crc.html
  // POLYNOMIAL = 0x0131 = x^8 + x^5 + x^4 + 1 : http://en.wikipedia.org/wiki/Computation_of_cyclic_redundancy_checks
  // 0x988000 is the 0x0131 polynomial shifted to farthest left of three bytes
  divisor = (uint32_t) 0x988000;

  // Add the check value
  remainder |= check;

  // Operate on only 16 positions of max 24.
  // The remaining 8 are our remainder and should be zero when we're done.
  for (uint8_t i = 0 ; i < 16 ; i++) {
    //Check if there is a one in the left position
    if( remainder & (uint32_t)1<<(23 - i) )
      remainder ^= divisor;

    //Rotate the divisor max 16 times so that we have 8 bits left of a remainder
    divisor >>= 1;
  }
  return ((uint8_t) remainder);
}

/* ======================================================================
Function: si7021_readRegister
Purpose : read the user register from the sensor
Input   : user register value filled by function
Output  : 0 if okay
Comments: -
====================================================================== */
int8_t Plugin_014_si7021_readRegister(uint8_t * value)
{

  // Request user register
  Wire.beginTransmission(SI7021_I2C_ADDRESS);
  Wire.write(SI7021_READ_REG);
  Wire.endTransmission();

  // request 1 byte result
  Wire.requestFrom(SI7021_I2C_ADDRESS, 1);
  if (Wire.available()>=1) {
      *value = Wire.read();
      return 0;
  }

  return 1;
}

/* ======================================================================
Function: Plugin_014_si7021_startConv
Purpose : return temperature or humidity measured
Input   : data type SI7021_READ_HUM or SI7021_READ_TEMP
Output  : 0 if okay
Comments: -
====================================================================== */
uint8_t Plugin_014_si7021_startConv(uint8_t datatype)
{
  //Request a reading
  Wire.beginTransmission(SI7021_I2C_ADDRESS);
  Wire.write(datatype);
  Wire.endTransmission();

  return 0;
}


/* ======================================================================
Function: Plugin_014_si7021_readValues
Purpose : read temperature and humidity from SI7021 sensor
Input   : current config resolution
Output  : 0 if okay
Comments: -
====================================================================== */
int8_t Plugin_014_si7021_readValues(uint8_t datatype, uint8_t resolution)
{
  long data;
  uint16_t raw ;
  uint8_t checksum;

  if (Wire.requestFrom(SI7021_I2C_ADDRESS, 3) != 3) {
    return -1;
  }

  // Comes back in three bytes, data(MSB) / data(LSB) / Checksum
  raw  = ((uint16_t) Wire.read()) << 8;
  raw |= Wire.read();
  checksum = Wire.read();

  // Check CRC of data received
  if(Plugin_014_si7021_checkCRC(raw, checksum) != 0) {
    addLog(LOG_LEVEL_INFO,F("SI7021 : checksum error!"));
    return -1;
  }

  // Humidity
  if (datatype == SI7021_MEASURE_HUM || datatype == SI7021_MEASURE_HUM_HM) {
    // Convert value to Himidity percent
    // pm-cz: it is possible to enable decimal places for humidity as well by multiplying the value in formula by 100
    data = ((1250 * (long)raw) >> 16) - 60;

    // Datasheet says doing this check
    if (data>1000) data = 1000;
    if (data<0)   data = 0;

    //pm-cz: Let us make sure we have enough precision due to ADC bits
    if (resolution == SI7021_RESOLUTION_12T_08RH) {
      data = (data + 5) / 10;
      data *= 10;
    }
    // save value
    si7021_humidity = (uint16_t) data;

  // Temperature
  } else  if (datatype == SI7021_MEASURE_TEMP ||datatype == SI7021_MEASURE_TEMP_HM || datatype == SI7021_MEASURE_TEMP_HUM) {
    // Convert value to Temperature (*100)
    // for 23.45C value will be 2345
    data =  ((17572 * (long)raw) >> 16) - 4685;

    /*
    // pm-cz: We should probably check for precision here as well
    if (resolution != SI7021_RESOLUTION_14T_12RH) {
      if (data > 0) {
        data = (data + 5) / 10;
      } else {
        data = (data - 5) / 10;
      }
      data *= 10;
    }
    */

    // save value
    si7021_temperature = (int16_t) data;
  }

  return 0;
}

/* ======================================================================
Function: Plugin_014_si7021_setResolution
Purpose : Sets the sensor resolution to one of four levels
Input   : see #define default is SI7021_RESOLUTION_14T_12RH
Output  : 0 if okay
Comments: -
====================================================================== */
int8_t Plugin_014_si7021_setResolution(uint8_t res)
{
  uint8_t reg;
  uint8_t error;

  // Get the current register value
  error = Plugin_014_si7021_readRegister(&reg);
  if ( error == 0) {
    // remove resolution bits
    reg &= SI7021_RESOLUTION_MASK ;

    // Prepare to write to the register value
    Wire.beginTransmission(SI7021_I2C_ADDRESS);
    Wire.write(SI7021_WRITE_REG);

    // Write the new resolution bits but clear unused before
    Wire.write(reg | ( res &= ~SI7021_RESOLUTION_MASK) );
    return (int8_t) Wire.endTransmission();
  }

  return error;
}
#endif // USES_P014
