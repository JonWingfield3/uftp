#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <array>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <vector>

#include <uftp_defs.h>

class UftpServer {
 public:
  UftpServer();
  UftpServer(uint16_t port);

  void Open();
  void Close();

  bool ReceiveCommand();

 private:
  void HandleRequest(const UftpMessage& request);
  UftpStatusCode HandleLsRequest(std::vector<uint8_t>& message);
  UftpStatusCode HandleDeleteRequest(const std::string& filename);

  bool open_ = false;

  UftpMessage response_;

  uint16_t server_port_ = 0;
  UftpSocketHandle sock_handle_;
};
