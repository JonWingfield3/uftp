#include <uftp_utils.h>

#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
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
const std::size_t UftpUtils::program_start_time_ =
    std::chrono::time_point_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now())
        .time_since_epoch()
        .count();

const std::map<UftpStatusCode, std::string> UftpUtils::UftpStatusCodeStrings{
    {UftpStatusCode::NO_ERR, "No Error"},
    {UftpStatusCode::ERR_FILE_NOT_FOUND, "File Not Found"},
    {UftpStatusCode::ERR_BAD_PERMISSIONS, "Bad Permissions"},
    {UftpStatusCode::ERR_BAD_COMMAND, "Unknown Command"},
    {UftpStatusCode::ERR_UNKNOWN, "Unknown Error"}};

const std::map<int, UftpStatusCode> UftpUtils::ErrnoToStatusCodeMap{
    {ENOENT, UftpStatusCode::ERR_FILE_NOT_FOUND},
    {EACCES, UftpStatusCode::ERR_BAD_PERMISSIONS},
    {0, UftpStatusCode::NO_ERR}};

///////////////////////////////////////////////////////////////////////////////
const std::string& UftpUtils::StatusCodeToString(UftpStatusCode status_code) {
  const auto ite = UftpStatusCodeStrings.find(status_code);
  if (ite == UftpStatusCodeStrings.end()) {
    DEBUG_LOG("ERROR! StatusCode:", status_code);
    return UftpStatusCodeStrings.at(UftpStatusCode::ERR_UNKNOWN);
  } else {
    return UftpStatusCodeStrings.at(status_code);
  }
}

///////////////////////////////////////////////////////////////////////////////
UftpStatusCode UftpUtils::ErrnoToStatusCode(int errno_val) {
  const auto status_code_ite = ErrnoToStatusCodeMap.find(errno_val);
  if (status_code_ite == ErrnoToStatusCodeMap.end()) {
    return UftpStatusCode::ERR_UNKNOWN;
  } else {
    return status_code_ite->second;
  }
}

///////////////////////////////////////////////////////////////////////////////
std::ostream& operator<<(std::ostream& ostream, const UftpHeader& uftp_header) {
  ostream << "| status_code: "
          << UftpUtils::StatusCodeToString(
                 static_cast<UftpStatusCode>(uftp_header.status_code))
          << " | command_length: " << uftp_header.command_length
          << " | argument_length: " << uftp_header.argument_length
          << " | message_length: " << uftp_header.message_length
          << " | sequence number: " << uftp_header.sequence_num << " | ";
  return ostream;
}

///////////////////////////////////////////////////////////////////////////////
std::ostream& operator<<(std::ostream& ostream,
                         const UftpMessage& uftp_message) {
  ostream << "Header: " << uftp_message.header
          << " | Command: " << uftp_message.command
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

  if (!file_stream.good()) {
    DEBUG_LOG("Couldn't open file:", filename);
    return ErrnoToStatusCode(errno);
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
    if (errno == ENOENT) {
      return UftpStatusCode::ERR_FILE_NOT_FOUND;
    } else if (errno == EACCES) {
      return UftpStatusCode::ERR_BAD_PERMISSIONS;
    } else {
      return UftpStatusCode::ERR_UNKNOWN;
    }
  }

  file_stream.write(reinterpret_cast<const char*>(buffer.data()),
                    buffer.size());

  return UftpStatusCode::NO_ERR;
}

///////////////////////////////////////////////////////////////////////////////
void UftpUtils::ConstructUftpHeader(UftpMessage& uftp_message) {
  UftpHeader& header = uftp_message.header;
  header.message_length = uftp_message.message.size();
  header.command_length = uftp_message.command.size();
  header.argument_length = uftp_message.argument.size();
}

///////////////////////////////////////////////////////////////////////////////
bool UftpUtils::UdpSendTo(const UftpSocketHandle& sock_handle,
                          const SendDataBuffer& send_buff) {
  if (send_buff.buff_len == 0) return true;

  if (sendto(sock_handle.sockfd, send_buff.buff, send_buff.buff_len, 0,
             (struct sockaddr*)&sock_handle.addr,
             sizeof(sock_handle.addr)) == -1) {
    DEBUG_LOG("Time out sending buffer: ", send_buff.buff_name);
    return false;
  }

  DEBUG_LOG("Sent buffer: ", send_buff.buff_name);
  return true;
}

///////////////////////////////////////////////////////////////////////////////
bool UftpUtils::SendMessage(const UftpSocketHandle& sock_handle,
                            UftpMessage& uftp_message) {
  static constexpr std::size_t MAX_NUM_RETRY_ATTEMPTS = 3;
  std::size_t retry_count = 0;

  // Fill out header field (command_length, message_lenght)
  ConstructUftpHeader(uftp_message);

  DEBUG_LOG("Sending Message:", uftp_message);

  const UftpHeader& header = uftp_message.header;

  const std::vector<SendDataBuffer> send_buffers{
      SendDataBuffer(&header, sizeof(header), "header"),
      SendDataBuffer(uftp_message.command.data(), header.command_length,
                     "command"),
      SendDataBuffer(uftp_message.argument.data(), header.argument_length,
                     "argument"),
      SendDataBuffer(uftp_message.message.data(), header.message_length,
                     "message")};

  for (const SendDataBuffer& send_buff : send_buffers) {
    retry_count = 0;
    while (!UdpSendTo(sock_handle, send_buff)) {
      if (++retry_count >= MAX_NUM_RETRY_ATTEMPTS) {
        return false;
      }
    }
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////
bool UftpUtils::UdpRecvFrom(UftpSocketHandle& sock_handle,
                            ReceiveDataBuffer& recv_buff) {
  if (recv_buff.buff_len == 0) return true;

  socklen_t socklen = sizeof(sock_handle.addr);
  if (recvfrom(sock_handle.sockfd, recv_buff.buff, recv_buff.buff_len, 0,
               (struct sockaddr*)&sock_handle.addr, &socklen) == -1) {
    DEBUG_LOG("Time out receiving buff: ", recv_buff.buff_name);
    return false;
  }

  DEBUG_LOG("Received buff: ", recv_buff.buff_name);
  return true;
}

///////////////////////////////////////////////////////////////////////////////
bool UftpUtils::ReceiveMessage(UftpSocketHandle& sock_handle,
                               UftpMessage& uftp_message) {
  // Receive header.
  UftpHeader& header = uftp_message.header;
  ReceiveDataBuffer header_buff(&header, sizeof(header), "header");
  if (!UdpRecvFrom(sock_handle, header_buff)) {
    return false;
  }

  // Adjust sizes of payload buffers based on header
  uftp_message.command.resize(header.command_length);
  uftp_message.argument.resize(header.argument_length);
  uftp_message.message.resize(header.message_length);

  std::vector<ReceiveDataBuffer> recv_buffers{
      ReceiveDataBuffer((void*)uftp_message.command.data(),
                        header.command_length, "command"),
      ReceiveDataBuffer((void*)uftp_message.argument.data(),
                        header.argument_length, "argument"),
      ReceiveDataBuffer(uftp_message.message.data(), header.message_length,
                        "message")};

  for (auto& recv_buff : recv_buffers) {
    if (!UdpRecvFrom(sock_handle, recv_buff)) {
      return false;
    }
  }

  DEBUG_LOG("Received Message:", uftp_message);
  return true;
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

  // Set send timeout.
  timeval send_tv;
  send_tv.tv_sec = 2;
  send_tv.tv_usec = 0;
  CheckErr(setsockopt(sock_handle.sockfd, SOL_SOCKET, SO_SNDTIMEO, &send_tv,
                      sizeof(send_tv)),
           "Failed to set send timeout");

  return sock_handle;
}
