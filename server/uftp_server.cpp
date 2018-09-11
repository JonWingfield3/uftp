#include "uftp_server.h"

#include <dirent.h>
#include <errno.h>
#include <chrono>
#include <cstdio>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <uftp_defs.h>
#include <uftp_utils.h>

/////////////////////////////////////////////////////////////////////////////////
UftpStatusCode UftpServer::HandleLsRequest(std::vector<uint8_t>& message) {
  UftpStatusCode return_code = UftpStatusCode::NO_ERR;
  std::ostringstream osstream;

  DIR* dir = nullptr;
  if ((dir = opendir(".")) != nullptr) {
    dirent* dir_entry = nullptr;
    while ((dir_entry = readdir(dir)) != nullptr) {
      osstream << dir_entry->d_name << "\n";
    }
  } else {
    // Couldn't open dir. Write error mesage
    osstream << "Couldn't read directory!\n";
    return_code = UftpStatusCode::ERR_BAD_PERMISSIONS;
  }
  const std::string& str = osstream.str();
  message.insert(message.end(), str.begin(), str.end());
}

//////////////////////////////////////////////////////////////////////////////
UftpStatusCode UftpServer::HandleDeleteRequest(const std::string& filename) {
  const int ret = std::remove(filename.c_str());
  if (ret == 0) {
    return UftpStatusCode::NO_ERR;
  } else if (ret == ENOENT) {
    return UftpStatusCode::ERR_FILE_NOT_FOUND;
  } else if (ret == EACCES) {
    return UftpStatusCode::ERR_BAD_PERMISSIONS;
  } else {
    return UftpStatusCode::ERR_UNKNOWN;
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
  UftpMessage request, response;

  UftpUtils::ReceiveMessage(sock_handle_, request);
  HandleRequest(request, response);
  DEBUG_LOG("Response header:", response.header);
  UftpUtils::SendMessage(sock_handle_, response);

  return (response.command != "exit");
}

///////////////////////////////////////////////////////////////////////////////
void UftpServer::HandleRequest(const UftpMessage& request,
                               UftpMessage& response) {
  // These fields are usually the same in request/response pair.
  response.command = request.command;
  response.argument = request.argument;

  if (request.command == "exit") {
    response.header.status_code = UftpStatusCode::NO_ERR;

  } else if (request.command == "ls") {
    response.header.status_code = HandleLsRequest(response.message);

  } else if (request.command == "put") {
    response.header.status_code =
        UftpUtils::WriteFile(request.argument, request.message);

  } else if (request.command == "get") {
    response.header.status_code =
        UftpUtils::ReadFile(request.argument, response.message);

  } else if (request.command == "delete") {
    response.header.status_code = HandleDeleteRequest(request.argument);

  } else {
    response.header.status_code = UftpStatusCode::ERR_BAD_COMMAND;
  }
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
  if (argc != 2) {
    std::cout
        << "uftp_server: missing argument\n\tUsage: uftp_server <port_number>";
    std::exit(1);
  }

  const uint16_t port_number = atoi(argv[1]);

  UftpServer uftp_server(port_number);
  uftp_server.Open();

  while (uftp_server.ReceiveCommand()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  uftp_server.Close();
  std::exit(0);
}
