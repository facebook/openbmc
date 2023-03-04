/*
 *  Copyright 2022 AMI
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0   Unless required by applicable law or agreed to in writing, software
 * 
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
*/
/** @file AmiSmbusInterfaceSrcLib.c
    Smbus interface common functions

**/
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <openbmc/kv.h>
#include <openbmc/obmc-i2c.h>
#include "AmiSmbusInterfaceSrcLib.h"

#define DEBUG(fmt, args...) \
  do {                      \
    if (debug_enable)       \
      printf(fmt, ##args);  \
  } while (0)

#define ERROR(fmt, args...) \
  do {                      \
    fprintf(stderr, fmt, ##args);  \
  } while (0)

static bool debug_enable = false;

void EnableAmiSmbusInterfaceDebug(
  bool enable)
{
  debug_enable = enable;
}

int SmbusWriteBlockExecute(
  int fd,
  unsigned char command,
  unsigned int length,
  unsigned char* data)
{
  int ret = -1;
  int retry_count = 5;
  do {
    ret = i2c_smbus_write_block_data(fd, command, length, data);
    retry_count--;
    MicroSecondDelay(100000);
  } while (ret && retry_count);

  if (ret) {
    DEBUG("SmbusWriteBlockExecute still fail after retry 5 times");
  }

  return ret;
}

int SendDataPacketThroughLinkLayer(
  unsigned char slot_id,
  int fd,
  unsigned char* DataPacket,
  unsigned int data_payload_length,
  unsigned int linklayer_ack_delay)
{
  unsigned char* Source = NULL;
  unsigned char LinkLayerAckBuffer[64] = {0};
  // prevent  *** stack smashing detected ***: <unknown> terminated
  unsigned char AckNeeded = 0;
  int package_size = 0;
  int totalPackageCount = 0;
  int PackageIndex = 0;
  int ReSendNeeded = 1;
  int RemainLength = 0;
  int ThisPackageLength = 0;
  int package_retry_count = 0;
  int ack_retry_count = 0;
  int ret = 0;
  //This is datalayer session id will increment for every data layer transaction
  unsigned char session_id = 0;
  char key[128] = {0};
  char value[128] = {0};

  if (data_payload_length > 255) {
    DATA_LAYER_PACKET_EXTEND* data_packet = (DATA_LAYER_PACKET_EXTEND*)DataPacket;
    AckNeeded = (data_packet->Flag & ACK_FLAG) >> 2;
    package_size = data_payload_length + 5;
  } else {
    DATA_LAYER_PACKET* data_packet = (DATA_LAYER_PACKET*)DataPacket;
    AckNeeded = (data_packet->Flag & ACK_FLAG) >> 2;
    package_size = data_payload_length + 4;
  }
  snprintf(key, sizeof(key), PROT_SESSION_KEY, slot_id);
  //the session id range (6 bits), the range from 0x10 to 0x3F
  if (kv_get(key, value, NULL, 0)) {
    session_id = 0x10;
  } else {
    session_id = (unsigned char)strtoul(value, NULL, 0);
    session_id++;
    if (session_id > 0x3F) {
      session_id = 0x10;
    }
  }
  DEBUG("session_id = %d \n", session_id);
  snprintf(value, 128, "%d", session_id);
  kv_set(key, value, 0, 0);

  Source = DataPacket;
  totalPackageCount = (package_size / LINK_LAYER_MAX_LOAD);
  RemainLength = package_size;

  if ((package_size % LINK_LAYER_MAX_LOAD) != 0) {
    totalPackageCount++;
  }

  for (PackageIndex = 0; PackageIndex <= totalPackageCount; PackageIndex++) {
    package_retry_count = 0;

    if (RemainLength == 0)
      break; // to break the for loop

    if (RemainLength >= LINK_LAYER_MAX_LOAD) {
      ThisPackageLength = LINK_LAYER_MAX_LOAD;
      RemainLength = RemainLength - LINK_LAYER_MAX_LOAD;
    } else {
      ThisPackageLength = RemainLength;
      RemainLength = 0;
    }

    do {
      DEBUG("\n");
      DEBUG(
          "PackageIndex:%d, totalPackageCount:%d, SendPacketThroughLinkLayer %s \n",
          PackageIndex,
          totalPackageCount,
          (package_retry_count ? "retry..." : ""));

      ReSendNeeded = 1;//ReSendNeeded need to set, if when AckFlag send success ReSendNeeded become 0.  

      if (package_retry_count) {
        MicroSecondDelay(200*1000); // provide delay of 200ms on retrying the same packet.
      }

      ret = SendPacketThroughLinkLayer(fd, session_id, Source, ThisPackageLength,
               AckNeeded, PackageIndex, totalPackageCount);
      DEBUG("SendPacketThroughLinkLayer ret : %d\n", ret);

      if (ret == 0) {
        ack_retry_count = 0;
        if (AckNeeded == 1) {
          do {
            MicroSecondDelay(linklayer_ack_delay * 1000);
            DEBUG("Get command LinkLayer Ack %s \n", (ack_retry_count ? "retry..." : ""));
            ret = LinkLayerReceiveAcknowledgment(fd, session_id, LinkLayerAckBuffer);
            if (ret) {
              DEBUG("LinkLayerReceiveAcknowledgment fail \n");
              ack_retry_count++;
              continue;
            }

            if (((LINK_LAYER_PACKET_ACK_MASTER*)LinkLayerAckBuffer)->Length && // Length should be valid
                ((LINK_LAYER_PACKET_ACK_MASTER*)LinkLayerAckBuffer)->AckFlag ==1) { // ACK=0x1, NoACK=0x2
              ReSendNeeded = 0;
              break;
            } else {
              DEBUG("LinkLayer NAck, or content check fail \n");
            }

            ack_retry_count++;
          } while (ack_retry_count < 3);
          package_retry_count++;

        } else {
          // AckNeeded = 0
          ReSendNeeded = 0;
        }

      } else {
        package_retry_count++;
      }
    } while ((package_retry_count < 5) && ReSendNeeded);

    if (ReSendNeeded) {
      ERROR("platfire ack keep busy after retry 5 times, please try later \n");
      return -1;
    }

    Source += ThisPackageLength;
  }

  return ret;
}

int SendPacketThroughLinkLayer(
  int fd,
  unsigned char Command,
  unsigned char* SendBuffer,
  unsigned int BufferLength,
  unsigned char AckNeeded,
  unsigned char SubPackageIndex,
  unsigned char LastSubPackageIndex)
{
  unsigned char TempBuffer[64] = {0};
  LINK_LAYER_PACKET_MASTER* LinkLayerPackage = (LINK_LAYER_PACKET_MASTER*)TempBuffer;
  unsigned int Index = 0;
  int ret = 0;

  LinkLayerPackage->Command = Command; //session ID
  LinkLayerPackage->AckNeeded = AckNeeded;
  LinkLayerPackage->PackageType = 0;  //data
  LinkLayerPackage->Length = (unsigned char)BufferLength + 2; // add checksum 1Byte & PackageIndex 1Byte
  LinkLayerPackage->LastPackageIndex = LastSubPackageIndex;
  LinkLayerPackage->SubPackageIndex = SubPackageIndex;

  DEBUG("LinkLayerPackage->Command = 0x%02X \n", LinkLayerPackage->Command);
  DEBUG("LinkLayerPackage->AckNeeded = 0x%02X \n", LinkLayerPackage->AckNeeded);
  DEBUG("LinkLayerPackage->PackageType = 0x%02X \n", LinkLayerPackage->PackageType);
  DEBUG("LinkLayerPackage->Length = 0x%02X \n", LinkLayerPackage->Length);
  DEBUG("LinkLayerPackage->LastPackageIndex = 0x%02X \n", LinkLayerPackage->LastPackageIndex);
  DEBUG("LinkLayerPackage->SubPackageIndex = 0x%02X \n", LinkLayerPackage->SubPackageIndex);
  DEBUG("\n");

  for (Index = 0; Index < BufferLength; Index++) {
    LinkLayerPackage->PayLoad[Index] = SendBuffer[Index];
  }
  LinkLayerPackage->PayLoad[Index] = CalculateCheckSum(TempBuffer, sizeof(LINK_LAYER_PACKET_MASTER) - 1);
  if (LinkLayerPackage->Length != 0) {
    DEBUG("Send LinkLayer SubPackage : %x\n", LinkLayerPackage->SubPackageIndex);
    PrintBuffer(TempBuffer, LinkLayerPackage->Length + 2);
  }

  ret = SmbusWriteBlockExecute(fd, TempBuffer[0], LinkLayerPackage->Length, &TempBuffer[2]);

  return ret;
}

static int get_linklayer_ack_delay(unsigned char command)
{
  //unit ms
  switch (command) {
    case CMD_DECOMMSION_REQUST:
      return 2000; //ami request 2s
    default:
      return 100; //ami default 100ms
  }
}

static unsigned char command_mapping_flag(unsigned char command)
{
  switch (command) {
    case CMD_LOG_DATA_LAYER:
      return 0x00; //no need Ack
    default:
      return 0x04; //need Ack
  }
}

int datalayer_send_receive(
  unsigned char slot_id,
  int fd,
  unsigned char command,
  unsigned char *data_payload,
  unsigned int data_payload_length,
  unsigned char *DataLayerAckBuffer)
{
  int ret = -1;
  int retry = 3;
  int delay = 0;
  
  do {
    ret = sent_data_packet(slot_id, fd, command, data_payload, data_payload_length);
    if (ret) {
      ERROR("sent_data_packet fail \n");
      retry--;
      sleep(1);
      continue;
    }
    
    switch (command) {
      case CMD_DECOMMSION_REQUST:
        delay = 70; //ami request
        break;
        
      case CMD_RECOMMISSION_REQUST:
        delay = 15; //ami request 2022-01-11
        break;
        
      default:
        delay = 0;
    }
    
    if (delay > 0) {
      sleep(delay);
      DEBUG("sleep %ds \n", delay);
    }
    
    ret = DataLayerReceiveFromPlatFire(slot_id, fd, DataLayerAckBuffer);
    if (ret) {
      ERROR("DataLayerReceiveFromPlatFire fail \n");
      retry--;
      sleep(1);
      continue;
    }
    
  } while (ret && retry);
  
  if (ret) {
    return -1;
  }
  
  return 0;
}

int sent_data_packet(
  unsigned char slot_id,
  int fd,
  unsigned char command,
  unsigned char *data_payload,
  unsigned int data_payload_length)
{
  unsigned char data[DATA_LAYER_EXTEND_MAX_PACKET] = {0};
  unsigned int index = 0;
  unsigned int linklayer_ack_delay = get_linklayer_ack_delay(command);

  if (data_payload_length >= DATA_LAYER_EXTEND_MAX_PAYLOAD) {
    ERROR("the data length >= DATA_LAYER_EXTEND_MAX_PAYLOAD(%d) \n", DATA_LAYER_EXTEND_MAX_PAYLOAD);
    return -1;
  } else if (data_payload_length >= DATA_LAYER_MAX_PAYLOAD) {
    DEBUG("data_payload_length > DATA_LAYER_MAX_PAYLOAD, use 2 bytes DataPacket.Length \n");
    DATA_LAYER_PACKET_EXTEND* packet_ext = (DATA_LAYER_PACKET_EXTEND*)data;
    packet_ext->Command = command;
    //  ++ command & length 2 bytes & Flag & CheckSum
    packet_ext->Length = data_payload_length + 5;
    // TBD: according to command, and fill in this layer ? need spec definition.
    packet_ext->Flag = command_mapping_flag(command);

    if (data_payload != NULL) {
      memcpy(packet_ext->PayLoad, data_payload, data_payload_length);
    }
    packet_ext->PayLoad[data_payload_length] = CalculateCheckSum(
        (unsigned char*)packet_ext,
        packet_ext->Length - 1); // command + length + Flag

    DEBUG("DataPacket.Command = 0x%02X \n", packet_ext->Command);
    DEBUG("DataPacket.Length = %d \n", packet_ext->Length);
    DEBUG("DataPacket.Flag = 0x%02X \n", packet_ext->Flag);
    for (index = 0; index < data_payload_length; index++) {
      DEBUG("DataPacket.PayLoad[%u] = 0x%02X \n", index, packet_ext->PayLoad[index]);
    }
    DEBUG("DataPacket.PayLoad[%u] = 0x%02X = CheckSum \n", index, packet_ext->PayLoad[index]);
    DEBUG("\n");
  } else {
    DATA_LAYER_PACKET* packet_old =  (DATA_LAYER_PACKET*)data;
    packet_old->Command = command;
    packet_old->Length = data_payload_length + 4; //  ++ command & length & Flag & CheckSum
    // TBD: according to command, and fill in this layer ? need spec definition.
    packet_old->Flag = command_mapping_flag(command);

    if (data_payload_length && (data_payload != NULL)) {
      memcpy(packet_old->PayLoad, data_payload, data_payload_length);
    }
    packet_old->PayLoad[data_payload_length] = CalculateCheckSum((unsigned char*)packet_old, packet_old->Length - 1); // command + length + Flag

    DEBUG("DataPacket.Command = 0x%02X \n", packet_old->Command);
    DEBUG("DataPacket.Length = %d \n", packet_old->Length);
    DEBUG("DataPacket.Flag = 0x%02X \n", packet_old->Flag);
    for (index = 0; index < data_payload_length; index++) {
      DEBUG("DataPacket.PayLoad[%u] = 0x%02X \n", index, packet_old->PayLoad[index]);
    }
    DEBUG("DataPacket.PayLoad[%u] = 0x%02X = CheckSum \n", index, packet_old->PayLoad[index]);
    DEBUG("\n");
  }

  return SendDataPacketThroughLinkLayer(slot_id, fd, data, data_payload_length, linklayer_ack_delay);
}

int DataLayerReceiveFromPlatFire(
  unsigned char slot_id,
  int fd,
  unsigned char* DataLayerAckBuffer)
{
  DEBUG("\n--------------------------DataLayerReceive--------------------------\n");
  unsigned char RecieveDataBuffer[512] = {0};
  unsigned char RecieveBuffer[sizeof(LINK_LAYER_PACKET_ACK_MASTER)] = {0};
  LINK_LAYER_PACKET_ACK_MASTER* LinkLayerPkgBuffer = (LINK_LAYER_PACKET_ACK_MASTER*)RecieveBuffer;
  int ret = -1;
  int Start = 0;
  char key[128] = {0};
  char value[128] = {0};
  unsigned char session_id = 0;
  unsigned char checksum = 0;
  int retry = 0;

  snprintf(key, sizeof(key), PROT_SESSION_KEY, slot_id);
  if (kv_get(key, value, NULL, 0)) {
    session_id = 0;
    DEBUG("session_id = %d \n", session_id);
    kv_set(key, "0", 0, 0);
  } else {
    session_id = (unsigned char)strtoul(value, NULL, 0);
    DEBUG("session_id = %d \n", session_id);
    snprintf(value, 128, "%d", session_id);
    kv_set(key, value, 0, 0);
  }

  do {
    MicroSecondDelay(200 * 1000); 
    ret = LinkLayerReceiveAcknowledgment(fd, session_id, RecieveBuffer);
    if (ret) {
      DEBUG("LinkLayerReceiveAcknowledgment fail \n");
      if (retry < 5) {
        retry++;
        continue;
      } else {
        ERROR("get response fail after 5 retry \n");
        return -1;
      }
    }
    DEBUG("LinkLayerPkgBuffer->AckNeeded = %u...", LinkLayerPkgBuffer->AckNeeded);
    if (LinkLayerPkgBuffer->AckNeeded) { // If AckNeeded bit set, need to send acknowldgement
      DEBUG("sent ACK to platfire \n");
      LinkLayerSendAcknowledgment(fd, session_id, LinkLayerPkgBuffer->SubPackageIndex);
    } else {
      DEBUG("no need to sent ACK to platfire \n");
    }
    DEBUG("\n");

    Start = LinkLayerPkgBuffer->SubPackageIndex * LINK_LAYER_MAX_RECEIVE_LOAD;
    memcpy(RecieveDataBuffer + Start, LinkLayerPkgBuffer->PayLoad, LINK_LAYER_MAX_RECEIVE_LOAD);

    if (LinkLayerPkgBuffer->SubPackageIndex == (LinkLayerPkgBuffer->AckFlag - 1)) {
      break;
    }
  } while (1);

  DEBUG("\nRECEIVED DATA BUFFER\n");
  PrintBuffer(RecieveDataBuffer, RecieveDataBuffer[0] /* data layer length*/);

  DEBUG("verify datalayer checksum...");
  checksum = checksum_verify(RecieveDataBuffer, RecieveDataBuffer[0]);
  if (checksum) {
    DEBUG(" fail, checksum = %u\n\n", checksum);
    return -1;
  } else {
    DEBUG(" successful\n\n");
  }

  if ((((DATA_LAYER_PACKET_ACK*)RecieveDataBuffer)->Flag == ACK_CMD_UNSUPPORTED) || 
      (((DATA_LAYER_PACKET_ACK*)RecieveDataBuffer)->Flag == ACK_BUSY_FLAG) ||
      (((DATA_LAYER_PACKET_ACK*)RecieveDataBuffer)->Flag == CHECKSUM_FAILURE) ||
      (((DATA_LAYER_PACKET_ACK*)RecieveDataBuffer)->Flag == DECOMMISSIONED)) {
    ERROR("unexpected FLAG : %d \n\n", ((DATA_LAYER_PACKET_ACK*)RecieveDataBuffer)->Flag);
    return -1;
  }
  
  memset(DataLayerAckBuffer, 0, sizeof(DATA_LAYER_PACKET_ACK));
  memcpy(DataLayerAckBuffer, RecieveDataBuffer, sizeof(DATA_LAYER_PACKET_ACK));

  return 0;
}

int LinkLayerSendAcknowledgment(
  int fd,
  unsigned int SessionId,
  unsigned char SubPackageId)
{
  int ret = -1;
  LINK_LAYER_PACKET_MASTER LinkLayerAckBuffer = {0};
  unsigned char* TempBuffer = (unsigned char*)&LinkLayerAckBuffer;
  unsigned int Length = 0;

  LinkLayerAckBuffer.Command = SessionId;
  LinkLayerAckBuffer.AckNeeded = 0;
  LinkLayerAckBuffer.PackageType = 1;
  LinkLayerAckBuffer.SubPackageIndex = SubPackageId;
  LinkLayerAckBuffer.LastPackageIndex = 1;
  LinkLayerAckBuffer.Length = sizeof(LINK_LAYER_PACKET_MASTER) - 2;
  LinkLayerAckBuffer.PayLoad[0] = CalculateCheckSum(TempBuffer, sizeof(LINK_LAYER_PACKET_MASTER) - 1);

  Length = LinkLayerAckBuffer.Length;
  DEBUG("LinkLayerAckBuffer: \n");
  PrintBuffer((unsigned char*)&LinkLayerAckBuffer, 34);
  DEBUG("\n");

  ret = SmbusWriteBlockExecute(fd, TempBuffer[0], Length, (TempBuffer + 2));

  return ret;
}

int LinkLayerReceiveAcknowledgment(
  int fd,
  unsigned char session_id,
  unsigned char* ReceiveBuffer)
{
  int ret = 0;
  unsigned char checksum = 0xFF;
  unsigned char tbuf[1] = {SMBUS_READ_COMMAND};

  memset(ReceiveBuffer, 0, sizeof(LINK_LAYER_PACKET_ACK_MASTER));

  ret = i2c_rdwr_msg_transfer(
      fd,
      (PLATFIRE_SLAVE_ADDRESS << 1),
      tbuf,
      1,
      ReceiveBuffer,
      sizeof(LINK_LAYER_PACKET_ACK_MASTER));

  if (ret) {
    DEBUG("i2c_rdwr_msg_transfer(), ret = %d\n", ret);
    return -1;
  } else {
    DEBUG("print link layer data:\n");
    DEBUG(
        "LinkLayerPkgBuffer->SubPackageIndex = %d\n",
        ((LINK_LAYER_PACKET_ACK_MASTER*)ReceiveBuffer)->SubPackageIndex);
    PrintBuffer(ReceiveBuffer, 32);

    unsigned char r_session_id =
        ((LINK_LAYER_PACKET_ACK_MASTER*)ReceiveBuffer)->Command;
    if (r_session_id != session_id) {
      ERROR(
          "session_id mismatch, expect: 0x%02X actual: 0x%02X\n",
          session_id,
          r_session_id);
      return -1;
    } else {
      DEBUG("verify session_id... successful\n");
      DEBUG("verify checksum...");
      checksum = checksum_verify(ReceiveBuffer, 32);
      if (checksum) {
        DEBUG(" fail, checksum = %u\n", checksum);
        return -1;
      } else {
        DEBUG(" successful\n");
        return 0;
      }
    }
  }

  return 0;
}

unsigned char checksum_verify(
  unsigned char* buffer,
  int length)
{
  unsigned char checksum = 0;
  for (int index = 0; index < length; index++) {
    checksum = checksum + buffer[index];
  }

  return checksum;
}

unsigned char CalculateCheckSum(
  unsigned char* buffer,
  int length)
{
  unsigned char checksum = 0;

  for (int index = 0; index < length; index++) {
    checksum += buffer[index];
  }

  return 0x100 - checksum;
}

void MicroSecondDelay(
  int delay)
{
  DEBUG("sleep %d ms \n", delay / 1000);
  usleep(delay);
}

void PrintBuffer(
  unsigned char* Buffer,
  unsigned int BufferLength)
{
  unsigned int index = 0;
  for (index = 0; index < BufferLength; index++) {
    if ((index % 16 == 0) && (index != 0))
      DEBUG("\n");
    DEBUG("0x%02X ", Buffer[index]);
  }
  DEBUG("\n");
}
