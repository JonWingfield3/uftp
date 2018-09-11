#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <array>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <uftp_defs.h>

class UftpServer {
 public:
  UftpServer() {}
  UftpServer(uint16_t port) : server_port_(port) {}

  void Open();
  void Close();

  bool ReceiveCommand();

 private:
  void HandleRequest(const UftpMessage& request, UftpMessage& response);
  UftpStatusCode HandleLsRequest(std::vector<uint8_t>& message);
  UftpStatusCode HandleDeleteRequest(const std::string& filename);

  bool open_ = false;
  uint16_t server_port_ = 0;
  UftpSocketHandle sock_handle_;
};
