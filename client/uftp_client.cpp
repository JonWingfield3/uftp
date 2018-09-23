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
  DEBUG_LOG("Opened socket to host: ", server_addr_str_, ", port: ",
            server_port_);

  // Set receive timeout
  timeval receive_tv;
  receive_tv.tv_sec = 2;
  receive_tv.tv_usec = 0;
  UftpUtils::CheckErr(setsockopt(sock_handle_.sockfd, SOL_SOCKET, SO_RCVTIMEO,
                                 &receive_tv, sizeof(receive_tv)),
                      "Failed to set receive timeout");

  open_ = true;
}

///////////////////////////////////////////////////////////////////////////////
void UftpClient::Close() {
  if (!open_) {
    return;
  }
  UftpUtils::CheckErr(close(sock_handle_.sockfd), "Error closing udp socket");
  DEBUG_LOG("Closed socket to host: ", server_addr_str_, ", port: ",
            server_port_);
  open_ = false;
}

///////////////////////////////////////////////////////////////////////////////
bool UftpClient::HandleResponse(const UftpMessage& response) {
  const auto response_code =
      static_cast<UftpStatusCode>(response.header.status_code);

  if (response.command == "exit") {
    std::cout << "Closing connection with server...\n";
    return false;

  } else if (response.command == "ls") {
    for (const auto byte : response.message) {
      std::cout << byte;
    }

  } else if (response.command == "get") {
    if (response.header.status_code == UftpStatusCode::ERR_FILE_NOT_FOUND) {
      std::cout << "File not found: " << response.argument << "\n";
    } else {
      UftpUtils::WriteFile(response.argument, response.message);
    }

  } else if (response.header.status_code == UftpStatusCode::ERR_BAD_COMMAND) {
    std::cout << UftpUtils::StatusCodeToString(response_code) << "\n";

  } else if (response_code != UftpStatusCode::NO_ERR) {
    std::cout << UftpUtils::StatusCodeToString(response_code) << "\n";
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////
bool UftpClient::SendCommand(const std::string& command,
                             const std::string& argument) {
  UftpMessage request, response;

  if (command == "put") {  // need to check argument and try to read in file.
    const auto status = UftpUtils::ReadFile(argument, request.message);
    if (status == UftpStatusCode::ERR_FILE_NOT_FOUND) {
      std::cout << "Unknown file: " << argument << "\n";
      return true;
    }
  }

  request.header.status_code = UftpStatusCode::NO_ERR;
  request.command = command;
  request.argument = argument;
  request.header.sequence_num = current_sequence_num_;
  bool response_received = false;
  bool matching_seq_nums = false;

  do {
    // Try sending message. Don't expect errors.
    if (!UftpUtils::SendMessage(sock_handle_, request)) {
      std::cout << "Error sending message to: " << server_addr_str_;
      continue;
    }

    response_received = UftpUtils::ReceiveMessage(sock_handle_, response);
    if (!response_received) {
      DEBUG_LOG("No Response Received!");
    }

    matching_seq_nums =
        response.header.sequence_num == request.header.sequence_num;
    if (!matching_seq_nums) {
      DEBUG_LOG("Mismatched Sequence numbers.");
    }

  } while (!response_received || !matching_seq_nums);

  ++current_sequence_num_;

  return HandleResponse(response);
}

///////////////////////////////////////////////////////////////////////////////
static bool ReadCLIInput(std::string& command, std::string& argument) {
  // Empty out command and argument.
  command = std::string();
  argument = std::string();

  // Print command prompt.
  std::cout << ">> ";

  // Buffer for user input.
  std::string user_input;
  // Read next line of user input.
  std::getline(std::cin, user_input);

  enum class ParserState {
    LOOKING_FOR_COMMAND,
    READING_COMMAND,
    LOOKING_FOR_ARG,
    READING_ARG,
    DONE,
  };

  ParserState parser_state = ParserState::LOOKING_FOR_COMMAND;

  for (const auto character : user_input) {
    if (parser_state == ParserState::DONE) break;

    switch (parser_state) {
      case ParserState::LOOKING_FOR_COMMAND:
        if (character != ' ') {
          command.push_back(character);
          parser_state = ParserState::READING_COMMAND;
        }
        break;
      case ParserState::READING_COMMAND:
        if (character != ' ') {
          command.push_back(character);
        } else {
          parser_state = ParserState::LOOKING_FOR_ARG;
        }
        break;
      case ParserState::LOOKING_FOR_ARG:
        if (character != ' ') {
          argument.push_back(character);
          parser_state = ParserState::READING_ARG;
        }
        break;
      case ParserState::READING_ARG:
        if (character != ' ') {
          argument.push_back(character);
        } else {
          parser_state = ParserState::DONE;
        }
        break;
    }
  }

  DEBUG_LOG("next_command: <", command, ">");
  DEBUG_LOG("next_argument: <", argument, ">");

  return (command.size() > 0);
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

  UftpClient uftp_client(server_address, server_port_number);
  uftp_client.Open();

  std::string next_command, next_argument;
  do {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    while (!ReadCLIInput(next_command, next_argument)) {
    }
  } while (uftp_client.SendCommand(next_command, next_argument));

  uftp_client.Close();
  std::exit(0);
}
