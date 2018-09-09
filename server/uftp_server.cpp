#include "uftp_server.h"

#include <dirent.h>
#include <chrono>
#include <cstdio>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <uftp_defs.h>
#include <uftp_utils.h>

/////////////////////////////////////////////////////////////////////////////////
// int UftpUtils::HandleLsRequest(std::vector<uint8_t>& message) {
//  // Do this the good ole C way since support for
//  std::experimental::filesystem
//  // isn't great.
//  std::ostringstream osstream;
//  DIR* dir = nullptr;
//  if ((dir = opendir(".")) != nullptr) {
//    dirent* dir_entry = nullptr;
//    while ((dir_entry = readdir(dir)) != nullptr) {
//      osstream << dir_entry->d_name << "\n";
//    }
//  } else {
//    // Couldn't open dir. Write error mesage
//    osstream << "Couldn't read directory!\n";
//  }
//  const std::string& str = osstream.str();
//  message.insert(message.end(), str.begin(), str.end());
//}
//
/////////////////////////////////////////////////////////////////////////////////
// int UftpUtils::HandleDeleteRequest(const std::string& filename,
//                                   std::vector<uint8_t>& message) {
//  const int ret = std::remove(filename.c_str());
//  if (ret != 0) {
//    std::ostringstream osstream;
//    osstream << "Unable to remove file " << filename << ", "
//             << std::strerror(errno);
//    const std::string& str = osstream.str();
//    message.insert(message.end(), str.begin(), str.end());
//  }
//}

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

  DEBUG_LOG("Binded to port:", server_port_);
  open_ = true;
}

///////////////////////////////////////////////////////////////////////////////
void UftpServer::Close() {
  if (!open_) {
    return;
  }
  UftpUtils::CheckErr(close(sock_handle_.sockfd), "Error closing udp socket");
  DEBUG_LOG("Closed socket on port:", server_port_);
  open_ = false;
}

///////////////////////////////////////////////////////////////////////////////
void UftpServer::HandleRequest() {
  //  std::vector<uint8_t> buffer(1);
  //  sockaddr_in clientaddr;
  //  socklen_t clientlen = sizeof(clientaddr);
  //  recvfrom(sockfd_, buffer.data(), buffer.size(), 0,
  //           (struct sockaddr*)&clientaddr, &clientlen);
  //  DEBUG_LOG("Received:", buffer[0]);
  //  sendto(sockfd_, buffer.data(), buffer.size(), 0,
  //         (struct sockaddr*)&clientaddr, clientlen)
  UftpMessage request, response;
  UftpUtils::ReceiveMessage(sock_handle_, request);
  // Handle command based on command, payload

  UftpUtils::SendMessage(sock_handle_, response);
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
  if (argc != 2) {
    std::cout
        << "uftp_server: missing argument\n\tUsage: uftp_server <port_number>";
    std::exit(1);
  }

  const uint16_t port_number = atoi(argv[1]);
  DEBUG_LOG("Port number:", port_number);

  UftpServer uftp_server(port_number);
  uftp_server.Open();

  while (true) {
    uftp_server.HandleRequest();
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
  }

  uftp_server.Close();
  std::exit(0);
}
