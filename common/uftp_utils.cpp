#include <uftp_utils.h>

#include <dirent.h>
#include <errno.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "uftp_defs.h"

///////////////////////////////////////////////////////////////////////////////
const int UftpUtils::program_start_time_ =
    std::chrono::time_point_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now())
        .time_since_epoch()
        .count();

const std::map<UftpStatusCode, std::string> UftpUtils::UftpStatusCodeStrings{
    {NO_ERR, "No Error"},
    {ERR_FILE_NOT_FOUND, "File Not Found"},
    {ERR_BAD_PERMISSIONS, "Bad Permissions"},
    {ERR_FILE_ALREADY_EXISTS, "File Already Exists"},
    {ERR_BAD_COMMAND, "Bad Command"},
    {ERR_BAD_CRC, "Bad CRC"},
    {ERR_UNKNOWN, "Unknown Error"}};

///////////////////////////////////////////////////////////////////////////////
const std::string& UftpUtils::StatusCodeToString(UftpStatusCode status_code) {
  return UftpStatusCodeStrings.at(status_code);
}

///////////////////////////////////////////////////////////////////////////////
std::ostream& operator<<(std::ostream& ostream, const UftpHeader& uftp_header) {
  ostream << "| status_code: "
          << UftpUtils::StatusCodeToString(
                 static_cast<UftpStatusCode>(uftp_header.status_code))
          << " | command_length: " << +uftp_header.command_length
          << " | argument_length: " << +uftp_header.argument_length
          << " | message_length: " << uftp_header.message_length
          << " | crc: " << +uftp_header.crc << " | ";
  return ostream;
}

///////////////////////////////////////////////////////////////////////////////
std::ostream& operator<<(std::ostream& ostream,
                         const UftpMessage& uftp_message) {
  ostream << "Header: " << uftp_message.header
          << "Command: " << uftp_message.command
          << " | Argument: " << uftp_message.argument << " | Message: ";
  for (const uint8_t byte : uftp_message.message) {
    ostream << byte;
  }
  ostream << "|";
  return ostream;
}

///////////////////////////////////////////////////////////////////////////////
const std::string UftpUtils::GetLogPrefix(const std::string& file,
                                          const std::string& func, int line) {
  const auto time_now = std::chrono::time_point_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now())
                            .time_since_epoch()
                            .count();
  const auto time_since_start = time_now - program_start_time_;

  std::ostringstream osstream;
  osstream << time_since_start << " [" << file << "::" << func << "." << line
           << "] ";
  return osstream.str();
}

///////////////////////////////////////////////////////////////////////////////
int UftpUtils::CheckErr(int ret, const std::string& error_str) {
  if (ret < 0) {
    std::cerr << error_str << ", errno = " << std::strerror(errno) << std::endl;
    std::exit(errno);
  }
  return ret;
}

//////////////////////////////////////////////////////////////////////////////
UftpStatusCode UftpUtils::ReadFile(const std::string& filename,
                                   std::vector<uint8_t>& buffer) {
  std::ifstream file_stream(filename, std::ios::in | std::ios::binary);
  if (!file_stream.is_open()) {
    DEBUG_LOG("Couldn't open file:", filename);
    // TODO: Better error codes in ReadFile
    return UftpStatusCode::ERR_FILE_NOT_FOUND;
  }
  file_stream.seekg(0, std::ios::end);
  const uint32_t file_size = file_stream.tellg();
  DEBUG_LOG("File Size:", file_size);
  file_stream.seekg(0, std::ios::beg);
  buffer.resize(file_size);
  file_stream.read(reinterpret_cast<char*>(buffer.data()), file_size);

  return UftpStatusCode::NO_ERR;
}

//////////////////////////////////////////////////////////////////////////////
UftpStatusCode UftpUtils::WriteFile(const std::string& filename,
                                    const std::vector<uint8_t>& buffer) {
  std::ofstream file_stream(filename, std::ios::out | std::ios::binary);
  if (!file_stream.is_open()) {
    DEBUG_LOG("Couldn't open file:", filename);
    return UftpStatusCode::ERR_FILE_NOT_FOUND;
  }
  file_stream.write(reinterpret_cast<const char*>(buffer.data()),
                    buffer.size());

  return UftpStatusCode::NO_ERR;
}

///////////////////////////////////////////////////////////////////////////////
void UftpUtils::ConstructUftpHeader(UftpMessage& uftp_message) {
  UftpHeader& header = uftp_message.header;
  header.crc = 0;
  header.status_code = NO_ERR;
  header.message_length = uftp_message.message.size();
  header.command_length = uftp_message.command.size();
  header.argument_length = uftp_message.argument.size();
  header.crc = GetCrc(uftp_message);
}

///////////////////////////////////////////////////////////////////////////////
uint8_t UftpUtils::GetCrc(const UftpMessage& uftp_message) {
  const UftpHeader& header = uftp_message.header;
  uint8_t crc = 0;
  for (int ii = 0; ii < header.message_length; ++ii) {
    crc ^= uftp_message.message[ii];
  }
  for (int ii = 0; ii < header.command_length; ++ii) {
    crc ^= uftp_message.command[ii];
  }
  for (int ii = 0; ii < header.argument_length; ++ii) {
    crc ^= uftp_message.argument[ii];
  }
  const uint8_t* header_byte_ptr = reinterpret_cast<const uint8_t*>(&header);
  for (int ii = 0; ii < sizeof(UftpHeader); ++ii) {
    crc ^= header_byte_ptr[ii];
  }
  return crc;
}

///////////////////////////////////////////////////////////////////////////////
void UftpUtils::UdpSendTo(const UftpSocketHandle& sock_handle, const void* buff,
                          int buff_len) {
  if (buff_len == 0) return;

  sendto(sock_handle.sockfd, buff, buff_len, 0,
         (struct sockaddr*)&sock_handle.addr, sizeof(sock_handle.addr));
}

///////////////////////////////////////////////////////////////////////////////
void UftpUtils::SendMessage(const UftpSocketHandle& sock_handle,
                            UftpMessage& uftp_message) {
  // Fill out header field (command_length, message_lenght)
  ConstructUftpHeader(uftp_message);

  // Send header.
  const UftpHeader& header = uftp_message.header;
  UdpSendTo(sock_handle, &header, sizeof(header));
  DEBUG_LOG("Sent header:", header);
  // Send command
  UdpSendTo(sock_handle, uftp_message.command.data(), header.command_length);
  DEBUG_LOG("Sent command:", uftp_message.command);
  // Send argument
  UdpSendTo(sock_handle, uftp_message.argument.data(), header.argument_length);
  DEBUG_LOG("Sent command:", uftp_message.argument);
  // Send message
  UdpSendTo(sock_handle, uftp_message.message.data(), header.message_length);
  DEBUG_LOG("Sent message:", uftp_message);
}

///////////////////////////////////////////////////////////////////////////////
void UftpUtils::UdpRecvFrom(UftpSocketHandle& sock_handle, void* buff,
                            int buff_len) {
  if (buff_len == 0) return;

  socklen_t socklen = sizeof(sock_handle.addr);
  recvfrom(sock_handle.sockfd, buff, buff_len, 0,
           (struct sockaddr*)&sock_handle.addr, &socklen);
}

///////////////////////////////////////////////////////////////////////////////
void UftpUtils::ReceiveMessage(UftpSocketHandle& sock_handle,
                               UftpMessage& uftp_message) {
  // Receive header.
  UftpHeader& header = uftp_message.header;
  UdpRecvFrom(sock_handle, &header, sizeof(header));
  DEBUG_LOG("Received header:", header);
  // Receive command.
  uftp_message.command.resize(header.command_length);
  UdpRecvFrom(sock_handle, (void*)uftp_message.command.data(),
              header.command_length);
  DEBUG_LOG("Received command:", uftp_message.command);
  // Receive argument.
  uftp_message.argument.resize(header.argument_length);
  UdpRecvFrom(sock_handle, (void*)uftp_message.argument.data(),
              header.argument_length);
  DEBUG_LOG("Received argument:", uftp_message.argument);
  // Receive message.
  uftp_message.message.resize(header.message_length);
  UdpRecvFrom(sock_handle, uftp_message.message.data(), header.message_length);
  DEBUG_LOG("Received message:", uftp_message);
  // TODO: Verify Checksum.
}

///////////////////////////////////////////////////////////////////////////////
UftpSocketHandle UftpUtils::GetSocketHandle(const std::string& ip_addr,
                                            uint16_t port_number) {
  UftpSocketHandle sock_handle;
  sock_handle.sockfd =
      CheckErr(socket(AF_INET, SOCK_DGRAM, 0), "Error creating UDP socket.");

  sockaddr_in& addr = sock_handle.addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port_number);
  if (ip_addr.empty()) {
    addr.sin_addr.s_addr = INADDR_ANY;
  } else {
    addr.sin_addr.s_addr = inet_addr(ip_addr.c_str());
  }

  return sock_handle;
}
