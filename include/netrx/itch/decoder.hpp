#pragma once

#include <span>
#include <expected>
#include "netrx/itch/messages.hpp"

namespace netrx::itch {

enum class DecodeError {
  TooShort,
  WrongLength
};

std::expected<Message, DecodeError> itchDecoder(std::span<const char> bytes);

// Per-message-type decoders.
AddOrder addOrder(const MessageHeader &msgHeader, std::span<const char> bytes);
SystemEvent systemEvent(const MessageHeader &msgHeader,
                        std::span<const char> bytes);
StockDirectory stockDirectory(const MessageHeader &msgHeader,
                              std::span<const char> bytes);
AddOrderMpid addOrderMpid(const MessageHeader &msgHeader,
                          std::span<const char> bytes);
OrderExecuted orderExecuted(const MessageHeader &msgHeader,
                            std::span<const char> bytes);
OrderExecutedWithPrice
orderExecutedWithPrice(const MessageHeader &msgHeader,
                       std::span<const char> bytes);
OrderCancel orderCancel(const MessageHeader &msgHeader,
                        std::span<const char> bytes);
OrderDelete orderDelete(const MessageHeader &msgHeader,
                        std::span<const char> bytes);
OrderReplace orderReplace(const MessageHeader &msgHeader,
                          std::span<const char> bytes);
} // namespace netrx::itch
