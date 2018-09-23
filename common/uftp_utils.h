#pragma once

#include <iostream>
#include <map>
#include <string>

#include <uftp_defs.h>

#ifdef __DEBUG__
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

  static UftpStatusCode ReadFile(const std::string& filename,
                                 std::vector<uint8_t>& buffer);

  static UftpStatusCode WriteFile(const std::string& filename,
                                  const std::vector<uint8_t>& buffer);

  static bool SendMessage(UftpSocketHandle& sock_handle,
                          UftpMessage& uftp_message);
  static bool ReceiveMessage(UftpSocketHandle& sock_handle,
                             UftpMessage& message);

  static UftpStatusCode ErrnoToStatusCode(int errno_val);

 private:
  template <typename T>
  struct DataBuffer {
    DataBuffer() {}
    DataBuffer(T buff_in, uint64_t buff_len_in,
               const std::string&& buff_name_in)
        : buff(buff_in), buff_len(buff_len_in), buff_name(buff_name_in) {}
    T buff = T();
    uint64_t buff_len = 0;
    const std::string buff_name;
  };

  using SendDataBuffer = DataBuffer<const void*>;
  using ReceiveDataBuffer = DataBuffer<void*>;

  static bool UdpSendTo(UftpSocketHandle& sock_handle,
                        const SendDataBuffer& send_buff);
  static bool UdpRecvFrom(UftpSocketHandle& sock_handle,
                          ReceiveDataBuffer& recv_buff);

  static bool SendAck(UftpSocketHandle& sock_handle);
  static bool RecvAck(UftpSocketHandle& sock_handle);

  static void ConstructUftpHeader(UftpMessage& uftp_message);
  static const std::string GetLogPrefix(const std::string& file,
                                        const std::string& func, int line);

  static void _DebugLog() { std::cerr << "\n"; }

  template <typename T, typename... Args>
  static void _DebugLog(T first_arg, Args... remaining_args) {
    std::cerr << first_arg;
    _DebugLog(remaining_args...);
  }

  static const std::size_t program_start_time_;
  static const std::map<UftpStatusCode, std::string> UftpStatusCodeStrings;
  static const std::map<int, UftpStatusCode> ErrnoToStatusCodeMap;
};

std::ostream& operator<<(std::ostream& ostream, const UftpHeader& uftp_header);
std::ostream& operator<<(std::ostream& ostream,
                         const UftpMessage& uftp_message);
