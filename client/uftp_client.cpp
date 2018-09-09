#include "uftp_client.h"

#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <ios>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <uftp_defs.h>
#include <uftp_utils.h>

///////////////////////////////////////////////////////////////////////////////
void UftpClient::Open() {
  sock_handle_ = UftpUtils::GetSocketHandle(server_addr_str_, server_port_);
  DEBUG_LOG("Opened socket to host:", server_addr_str_, ", port:",
            server_port_);
  open_ = true;
}

///////////////////////////////////////////////////////////////////////////////
void UftpClient::Close() {
  if (!open_) {
    return;
  }
  UftpUtils::CheckErr(close(sock_handle_.sockfd), "Error closing udp socket");
  DEBUG_LOG("Closed socket to host:", server_addr_str_, ", port:",
            server_port_);
  open_ = false;
}

///////////////////////////////////////////////////////////////////////////////
void UftpClient::HandleCommand(const std::string& command,
                               const std::string& argument) {
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
  //
  //  return;

  UftpMessage request, response;
  request.command = command;

  if (command == "put") {  // need to check argument and try to read in file.
    UftpUtils::ReadFile(argument, request.message);
  }
  request.command += argument;

  UftpUtils::SendMessage(sock_handle_, request);
  UftpUtils::ReceiveMessage(sock_handle_, response);
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
  if (argc != 3) {
    std::cout << "uftp_client: missing argument\n\tUsage: uftp_client "
                 "<ip_address> <port_number>";
    std::exit(1);
  }

  const std::string server_address = argv[1];
  const uint16_t server_port_number = atoi(argv[2]);
  DEBUG_LOG("Server IP address:", server_address, ", Server port number:",
            server_port_number);

  UftpClient uftp_client(server_address, server_port_number);
  uftp_client.Open();

  while (true) {
    std::string next_command;
    std::string next_argument;  // may be empty string
    std::cin >> next_command;
    DEBUG_LOG("Received command:", next_command);
    //    std::getline(std::cin, next_argument);
    //    DEBUG_LOG("Received argument:", next_argument);
    uftp_client.HandleCommand(next_command, next_argument);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
  }

  uftp_client.Close();
  std::exit(0);
}
