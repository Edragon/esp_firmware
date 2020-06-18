// #######################################################################################################
// #  Helper functions to encode data for use on LoRa/TTN network.
// #######################################################################################################

#if defined(USES_PACKED_RAW_DATA)

#include "ESPEasy_packed_raw_data.h"


String getPackedFromPlugin(struct EventStruct *event, uint8_t sampleSetCount)
{
  byte   value_count = getValueCountFromSensorType(event->sensorType);
  String raw_packed;

  if (PluginCall(PLUGIN_GET_PACKED_RAW_DATA, event, raw_packed)) {
    value_count = event->Par1;
  }
  String packed;
  packed.reserve(32);
  packed += LoRa_addInt(Settings.TaskDeviceNumber[event->TaskIndex], PackedData_uint8);
  packed += LoRa_addInt(event->idx, PackedData_uint16);
  packed += LoRa_addInt(sampleSetCount, PackedData_uint8);
  packed += LoRa_addInt(value_count, PackedData_uint8);

  if (raw_packed.length() > 0) {
    packed += raw_packed;
  } else {
    const byte BaseVarIndex = event->TaskIndex * VARS_PER_TASK;

    switch (event->sensorType)
    {
      case SENSOR_TYPE_LONG:
      {
        unsigned long longval = (unsigned long)UserVar[BaseVarIndex] + ((unsigned long)UserVar[BaseVarIndex + 1] << 16);
        packed += LoRa_addInt(longval, PackedData_uint32);
        break;
      }

      default:

        for (byte i = 0; i < value_count && i < VARS_PER_TASK; ++i) {
          // For now, just store the floats as an int32 by multiplying the value with 10000.
          packed += LoRa_addFloat(value_count, PackedData_int32_1e4);
        }
        break;
    }
  }
  return packed;
}

// Compute the air time for a packet in msec.
// Formula used from https://www.loratools.nl/#/airtime
// @param pl   Payload length in bytes
// @param sf   Spreading factor 7 - 12
// @param bw   Bandwidth 125 kHz default for LoRaWAN. 250 kHz also supported.
// @param cr   Code Rate 4 / (CR + 4) = 4/5.  4/5 default for LoRaWAN
// @param n_preamble Preamble length Default for frame = 8, beacon = 10
// @param header    Explicit header Default on for LoRaWAN
// @param crc       CRC Default on for LoRaWAN
float getLoRaAirTime(uint8_t  pl,
                     uint8_t  sf,
                     uint16_t bw         = 125,
                     uint8_t  cr         = 1,
                     uint8_t  n_preamble = 8,
                     bool     header     = true,
                     bool     crc        = true);

float getLoRaAirTime(uint8_t pl, uint8_t sf, uint16_t bw, uint8_t cr, uint8_t n_preamble, bool header, bool crc)
{
  if (sf > 12) {
    sf = 12;
  } else if (sf < 7) {
    sf = 7;
  }

  if (cr > 4) {
    cr = 4;
  } else if (cr < 1) {
    cr = 1;
  }

  // Symbols in frame
  int payload_length = 8;
  {
    int beta_offset = 28;

    if (crc) { beta_offset += 16; }

    if (!header) { beta_offset -= 20; }
    float beta_f                  = 8 * pl - 4 * sf + beta_offset;
    bool  lowDataRateOptimization = (bw == 125 && sf >= 11);

    if (lowDataRateOptimization) {
      beta_f = beta_f / (4 * (sf - 2));
    } else {
      beta_f = beta_f / (4 * sf);
    }
    int beta = static_cast<int>(beta_f + 1.0); // ceil

    if (beta > 0) {
      payload_length += (beta * (cr + 4));
    }
  }

  // t_symbol and t_air in msec
  float t_symbol = (1 << sf) / bw;
  float t_air    = ((n_preamble + 4.25) + payload_length) * t_symbol;
  return t_air;
}

#endif // USES_PACKED_RAW_DATA
