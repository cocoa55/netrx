#pragma once
#include "netrx/config.hpp"
#include "netrx/udpsocket.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>


namespace netrx {
class ReplaySender {
public:
  [[nodiscard]] ReplaySender() {
    _dest.sin_family = AF_INET;
    _dest.sin_port = htons(netrx::kPort);
    if (inet_pton(AF_INET, "127.0.0.1", &_dest.sin_addr) != 1) {
      throw std::runtime_error("ReplaySender(): Invalid address\n");
    }
  }

  //Reads ITCH message from file
  auto ReadItchMessage();

  //Wraps ITCH Message into a MoldUDP64 packet
  auto FrameMessage();

  //Sends Packet
  auto SendPacket();

  //Answers retransmission requests by the feed handler
  auto AnswerRequest();

  //
private:
  UdpSocket _sock{};
  sockaddr_in _dest{};

};
}
