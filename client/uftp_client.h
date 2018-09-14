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
  /// \param argument
  /// \return true if the connection is remaining open, false otherwise.
  ///
  bool SendCommand(const std::string& command, const std::string& argument);

 private:
  bool HandleResponse(const UftpMessage& response);

  bool open_ = false;

  uint32_t current_sequence_num_ = 0;

  uint16_t server_port_ = 0;
  UftpSocketHandle sock_handle_;
  std::string server_addr_str_;
};
