#pragma once

#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <map>
#include <string>

#include "uftp_defs.h"

#ifdef __DEBUG__
//#define DEBUG_LOG(...)                                                       \
//  UftpUtils::DebugLog(UftpUtils::GetLogPrefix(__FILE__, __func__, __LINE__), \
//                      __VA_ARGS__)
#define DEBUG_LOG(...) \
  UftpUtils::DebugLog(__FILE__, __func__, __LINE__, __VA_ARGS__)

#else
#define DEBUG_LOG(...)
#endif

class UftpUtils {
 public:
  UftpUtils();

  static const std::string& StatusCodeToString(UftpStatusCode status_code);

  template <typename T, typename... Args>
  static void DebugLog(const std::string& file, const std::string& func,
                       int line, T first, Args... args) {
    std::cerr << GetLogPrefix(file, func, line) << first;
    _DebugLog(args...);
  }

  static int CheckErr(int ret, const std::string& error_str);

  static UftpSocketHandle GetSocketHandle(const std::string& ip_addr,
                                          uint16_t port_number);

  static void ReadFile(const std::string& filename,
                       std::vector<uint8_t>& buffer);

  static void SendMessage(const UftpSocketHandle& sock_handle,
                          UftpMessage& uftp_message);
  static void ReceiveMessage(UftpSocketHandle& sock_handle,
                             UftpMessage& message);

 private:
  static void UdpSendTo(const UftpSocketHandle& sock_handle, const void* buff,
                        int buff_len);
  static void UdpRecvFrom(UftpSocketHandle& sock_handle, void* buff,
                          int buff_len);

  static void ConstructUftpHeader(UftpMessage& uftp_message);
  static uint8_t GetCrc(const UftpMessage& uftp_message);
  static const std::string GetLogPrefix(const std::string& file,
                                        const std::string& func, int line);

  static void _DebugLog() { std::cerr << "\n"; }

  template <typename T, typename... Args>
  static void _DebugLog(T first, Args... args) {
    std::cerr << " " << first;
    _DebugLog(args...);
  }

  static const int program_start_time_;
  static const std::map<UftpStatusCode, std::string> UftpStatusCodeStrings;
};

static std::ostream& operator<<(std::ostream& ostream,
                                const struct UftpHeader& uftp_header);
static std::ostream& operator<<(std::ostream& ostream,
                                const struct UftpMessage& uftp_message);
