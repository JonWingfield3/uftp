#pragma once

#include <arpa/inet.h>
#include <cstdint>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
enum UftpStatusCode {
  NO_ERR,
  CONN_CLOSING,
  ERR_FILE_NOT_FOUND,
  ERR_BAD_PERMISSIONS,
  ERR_FILE_ALREADY_EXISTS,
  ERR_BAD_COMMAND,
  ERR_BAD_CRC,
  ERR_UNKNOWN,
};

///////////////////////////////////////////////////////////////////////////////
struct __attribute__((packed)) UftpHeader {
  uint8_t status_code = 0;
  uint8_t command_length = 0;
  uint8_t argument_length = 0;
  uint8_t crc = 0;
  uint32_t message_length = 0;
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

// CLIENT SAMPLE CODE:
//  sendto(sockfd_, command.c_str(), command.size(), 0,
//         (struct sockaddr*)&server_addr_, sizeof(server_addr_));
//  DEBUG_LOG("Sent:", command);
//
//  std::vector<uint8_t> buffer(1);
//  socklen_t len = sizeof(server_addr_);
//  recvfrom(sockfd_, buffer.data(), buffer.size(), 0,
//           (struct sockaddr*)&server_addr_, &len);
//  DEBUG_LOG("Received:", buffer[0]);
//  UftpMessage send_message, response_message;
//  send_message.command = command + argument;
//  UftpUtils::SendMessage(sock_handle_, send_message);
//  UftpUtils::ReceiveMessage(sock_handle_, response_message);

// SERVER SAMPLE CODE:
//  std::vector<uint8_t> buffer(1);
//  sockaddr_in clientaddr;
//  socklen_t clientlen = sizeof(clientaddr);
//  recvfrom(sockfd_, buffer.data(), buffer.size(), 0,
//           (struct sockaddr*)&clientaddr, &clientlen);
//  DEBUG_LOG("Received:", buffer[0]);
//  sendto(sockfd_, buffer.data(), buffer.size(), 0,
//         (struct sockaddr*)&clientaddr, clientlen)
