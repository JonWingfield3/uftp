#pragma once

#include <arpa/inet.h>
#include <cstdint>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
enum UftpStatusCode {
  NO_ERR = 0,
  ERR_FILE_NOT_FOUND,
  ERR_BAD_PERMISSIONS,
  ERR_BAD_COMMAND,
  ERR_UNKNOWN,
};

///////////////////////////////////////////////////////////////////////////////
struct __attribute__((packed)) UftpHeader {
  uint32_t status_code = NO_ERR;
  uint16_t command_length = 0;
  uint16_t argument_length = 0;
  uint32_t message_length = 0;
  uint32_t sequence_num = 0;
};

///////////////////////////////////////////////////////////////////////////////
struct UftpMessage {
  UftpMessage() {}
  UftpMessage(std::string command_in, std::vector<uint8_t> message_in)
      : command(command_in), message(message_in) {}

  UftpHeader header;
  std::string command;
  std::string argument;
  std::vector<uint8_t> message;
};

///////////////////////////////////////////////////////////////////////////////
struct UftpSocketHandle {
  int sockfd;
  sockaddr_in addr;
};
