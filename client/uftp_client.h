#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>
#include <string>

#include <uftp_defs.h>

class UftpClient {
 public:
  UftpClient() {}
  UftpClient(const std::string& server_addr, uint16_t server_port)
      : server_addr_str_(server_addr), server_port_(server_port) {}

  void Open();
  void Close();

  ///
  /// \brief SendCommand
  /// \param command
  ///
  void HandleCommand(const std::string& command, const std::string& argument);

 private:
  bool open_ = false;
  uint16_t server_port_ = 0;
  UftpSocketHandle sock_handle_;
  std::string server_addr_str_;
};
