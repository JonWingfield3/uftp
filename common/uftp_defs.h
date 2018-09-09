#pragma once

#include <cstdint>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
enum UftpStatusCode {
  NO_ERR,
  ERR_FILE_NOT_FOUND,
  ERR_FILE_BAD_PERMISSIONS,
  ERR_FILE_ALREADY_EXISTS,
  ERR_BAD_COMMAND,
  ERR_BAD_CRC,
};

///////////////////////////////////////////////////////////////////////////////
struct __attribute__((packed)) UftpHeader {
  UftpHeader() {}
  UftpHeader(uint8_t status_code_in, uint8_t command_length_in,
             uint32_t message_length_in, uint8_t crc_in)
      : status_code(status_code_in),
        command_length(command_length_in),
        message_length(message_length_in),
        crc(crc_in) {}

  uint8_t status_code = 0;
  uint8_t command_length = 0;
  uint32_t message_length = 0;
  uint8_t crc;
};

///////////////////////////////////////////////////////////////////////////////
struct UftpMessage {
  UftpMessage() {}
  UftpMessage(std::string command_in, std::vector<uint8_t> message_in)
      : command(command_in), message(message_in) {}

  UftpHeader header;
  std::string command;
  std::vector<uint8_t> message;
};

///////////////////////////////////////////////////////////////////////////////
struct UftpSocketHandle {
  int sockfd;
  sockaddr_in addr;
};
