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
    UftpUtils::WriteFile(response.argument, response.message);

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
    UftpUtils::ReadFile(argument, request.message);
  }

  request.command = command;
  request.argument = argument;

  UftpUtils::SendMessage(sock_handle_, request);
  UftpUtils::ReceiveMessage(sock_handle_, response);
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
  DEBUG_LOG("Received user_input:", user_input);

  enum class ParserState {
    LOOKING_FOR_COMMAND,
    READING_COMMAND,
    LOOKING_FOR_ARG,
    READING_ARG,
    DONE,
  };

  ParserState parser_state;
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
