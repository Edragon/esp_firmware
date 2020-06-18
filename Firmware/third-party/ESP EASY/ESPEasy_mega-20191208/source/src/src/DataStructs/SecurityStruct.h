#ifndef DATASTRUCTS_SECURITYSTRUCT_H
#define DATASTRUCTS_SECURITYSTRUCT_H

#include "../../ESPEasy_common.h"
#include "../DataStructs/ESPEasyLimits.h"

/*********************************************************************************************\
 * SecurityStruct
\*********************************************************************************************/
struct SecurityStruct
{
  SecurityStruct();

  void validate();

  char          WifiSSID[32];
  char          WifiKey[64];
  char          WifiSSID2[32];
  char          WifiKey2[64];
  char          WifiAPKey[64];
  char          ControllerUser[CONTROLLER_MAX][26];
  char          ControllerPassword[CONTROLLER_MAX][64];
  char          Password[26];
  byte          AllowedIPrangeLow[4]; // TD-er: Use these
  byte          AllowedIPrangeHigh[4];
  byte          IPblockLevel;

  //its safe to extend this struct, up to 4096 bytes, default values in config are 0. Make sure crc is last
  uint8_t       ProgmemMd5[16] = {0}; // crc of the binary that last saved the struct to file.
  uint8_t       md5[16] = {0};
};


#endif // DATASTRUCTS_SECURITYSTRUCT_H
