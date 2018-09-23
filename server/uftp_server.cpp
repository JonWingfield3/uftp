#include "uftp_server.h"

#include <dirent.h>
#include <errno.h>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <uftp_defs.h>
#include <uftp_utils.h>

/////////////////////////////////////////////////////////////////////////////////
UftpServer::UftpServer() {
  response_.header.sequence_num = std::numeric_limits<uint32_t>::max();
}

/////////////////////////////////////////////////////////////////////////////////
UftpServer::UftpServer(uint16_t port) : server_port_(port) {
  response_.header.sequence_num = std::numeric_limits<uint32_t>::max();
}

/////////////////////////////////////////////////////////////////////////////////
UftpStatusCode UftpServer::HandleLsRequest(std::vector<uint8_t>& message) {
  DIR* dir = opendir(".");
  if (dir == nullptr) {
    return UftpStatusCode::ERR_BAD_PERMISSIONS;
  }

  std::ostringstream osstream;

  dirent* dir_entry = nullptr;
  while ((dir_entry = readdir(dir)) != nullptr) {
    osstream << dir_entry->d_name << "\n";
  }

  const std::string& str = osstream.str();
  message.insert(message.end(), str.begin(), str.end());

  return UftpStatusCode::NO_ERR;
}

//////////////////////////////////////////////////////////////////////////////
UftpStatusCode UftpServer::HandleDeleteRequest(const std::string& filename) {
  const int ret = std::remove(filename.c_str());
  if (ret != 0) {
    return UftpUtils::ErrnoToStatusCode(errno);
  } else {
    return UftpStatusCode::NO_ERR;
  }
}

///////////////////////////////////////////////////////////////////////////////
void UftpServer::Open() {
  std::string ip_addr_any;
  sock_handle_ = UftpUtils::GetSocketHandle(ip_addr_any, server_port_);

  const int optval = 1;
  setsockopt(sock_handle_.sockfd, SOL_SOCKET, SO_REUSEADDR,
             (const void*)&optval, sizeof(optval));

  UftpUtils::CheckErr(
      bind(sock_handle_.sockfd, (struct sockaddr*)&sock_handle_.addr,
           sizeof(sock_handle_.addr)),
      "Error binding to port");

  DEBUG_LOG("Binded to port: ", server_port_);
  open_ = true;
}

///////////////////////////////////////////////////////////////////////////////
void UftpServer::Close() {
  if (!open_) {
    return;
  }

  UftpUtils::CheckErr(close(sock_handle_.sockfd), "Error closing udp socket");
  DEBUG_LOG("Closed socket on port: ", server_port_);
  open_ = false;
}

///////////////////////////////////////////////////////////////////////////////
bool UftpServer::ReceiveCommand() {
  UftpMessage request;

  while (!UftpUtils::ReceiveMessage(sock_handle_, request)) {
    std::cout << "Error receiving message" << std::endl;
  }

  std::cout << "Received message" << std::endl;
  HandleRequest(request);
  while (!UftpUtils::SendMessage(sock_handle_, response_)) {
    std::cout << "Error sending message";
    return true;
  }

  return (response_.command != "exit");
}

///////////////////////////////////////////////////////////////////////////////
void UftpServer::HandleRequest(const UftpMessage& request) {
  if (request.header.sequence_num == response_.header.sequence_num) {
    // If the sequence numbers match then this is a re-transmit. Send the last
    // response.
    DEBUG_LOG("Sequence numbers match!", request.header.sequence_num);
    return;
  }

  // These fields are usually the same in request/response_ pair.
  response_ = UftpMessage();
  response_.command = request.command;
  response_.argument = request.argument;
  response_.header.sequence_num = request.header.sequence_num;

  if (request.command == "exit") {
    response_.header.status_code = UftpStatusCode::NO_ERR;

  } else if (request.command == "ls") {
    response_.header.status_code = HandleLsRequest(response_.message);

  } else if (request.command == "put") {
    response_.header.status_code =
        UftpUtils::WriteFile(request.argument, request.message);

  } else if (request.command == "get") {
    response_.header.status_code =
        UftpUtils::ReadFile(request.argument, response_.message);

  } else if (request.command == "delete") {
    response_.header.status_code = HandleDeleteRequest(request.argument);

  } else {
    response_.header.status_code = UftpStatusCode::ERR_BAD_COMMAND;
  }
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
  if (argc != 2) {
    std::cout << "uftp_server: missing argument\n\tUsage: uftp_server "
                 "<port_number>\n";
    std::exit(1);
  }

  const uint16_t port_number = atoi(argv[1]);

  UftpServer uftp_server(port_number);
  uftp_server.Open();

  while (uftp_server.ReceiveCommand()) {
    //    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  uftp_server.Close();
  std::exit(0);
}
