#pragma once

#include <array>
#include <cstdint>
#include <variant>

namespace netrx::itch {
//Message header definition and header typedefs
using StockLocate = uint16_t;
using TrackingNumber = uint16_t;
using Timestamp = uint64_t;

enum class MessageType : char {
  SystemEvent = 'S',
  StockDirectory = 'R',
  AddOrder = 'A',
  AddOrderMpid = 'F',
  OrderExecuted = 'E',
  OrderExecutedWithPrice = 'C',
  OrderCancel = 'X',
  OrderDelete = 'D',
  OrderReplace = 'U',
};

struct MessageHeader {
  MessageType messageType;
  StockLocate stockLocate;
  TrackingNumber trackingNumber;
  Timestamp timestamp;
};

//Message Body definitions and typedefs
using OrderReferenceNumber = uint64_t;
using BuySellIndicator = char;
using Shares = uint32_t;
using Stock = std::array<char, 8>;
using Price = uint32_t;

struct AddOrder {
  MessageHeader header;
  Stock stock;
  OrderReferenceNumber orderReferenceNumber;
  Shares shares;
  Price price;
  BuySellIndicator buySellIndicator;
};
static_assert(sizeof(AddOrder) == 48);

// System Event 'S'
using EventCode = char;

// Stock Directory 'R'
using MarketCategory = char;
using FinancialStatusIndicator = char;
using RoundLotSize = uint32_t;
using RoundLotsOnly = char;
using IssueClassification = char;
using IssueSubType = std::array<char, 2>;
using Authenticity = char;
using ShortSaleThresholdIndicator = char;
using IpoFlag = char;
using LuldReferencePriceTier = char;
using EtpFlag = char;
using EtpLeverageFactor = uint32_t;
using InverseIndicator = char;

// Add Order with MPID 'F'
using Attribution = std::array<char, 4>;

// Order Executed 'E' / Order Executed With Price 'C'
using MatchNumber = uint64_t;
using Printable = char;

struct SystemEvent {
  MessageHeader header;
  EventCode eventCode;
};
static_assert(sizeof(SystemEvent) == 24);

struct StockDirectory {
  MessageHeader header;
  Stock stock;
  RoundLotSize roundLotSize;
  EtpLeverageFactor etpLeverageFactor;
  IssueSubType issueSubType;
  MarketCategory marketCategory;
  FinancialStatusIndicator financialStatusIndicator;
  RoundLotsOnly roundLotsOnly;
  IssueClassification issueClassification;
  Authenticity authenticity;
  ShortSaleThresholdIndicator shortSaleThresholdIndicator;
  IpoFlag ipoFlag;
  LuldReferencePriceTier luldReferencePriceTier;
  EtpFlag etpFlag;
  InverseIndicator inverseIndicator;
};
static_assert(sizeof(StockDirectory) == 48);

struct AddOrderMpid {
  MessageHeader header;
  Stock stock;
  OrderReferenceNumber orderReferenceNumber;
  Shares shares;
  Price price;
  Attribution attribution;
  BuySellIndicator buySellIndicator;
};
static_assert(sizeof(AddOrderMpid) == 48);

struct OrderExecuted {
  MessageHeader header;
  OrderReferenceNumber orderReferenceNumber;
  MatchNumber matchNumber;
  Shares executedShares;
};
static_assert(sizeof(OrderExecuted) == 40);

struct OrderExecutedWithPrice {
  MessageHeader header;
  OrderReferenceNumber orderReferenceNumber;
  MatchNumber matchNumber;
  Shares executedShares;
  Price executionPrice;
  Printable printable;
};
static_assert(sizeof(OrderExecutedWithPrice) == 48);

struct OrderCancel {
  MessageHeader header;
  OrderReferenceNumber orderReferenceNumber;
  Shares cancelledShares;
};
static_assert(sizeof(OrderCancel) == 32);

struct OrderDelete {
  MessageHeader header;
  OrderReferenceNumber orderReferenceNumber;
};
static_assert(sizeof(OrderDelete) == 24);

struct OrderReplace {
  MessageHeader header;
  OrderReferenceNumber originalOrderReferenceNumber;
  OrderReferenceNumber newOrderReferenceNumber;
  Shares shares;
  Price price;
};
static_assert(sizeof(OrderReplace) == 40);

using Message =
    std::variant<std::monostate, AddOrder, SystemEvent, StockDirectory,
                 AddOrderMpid, OrderExecuted, OrderExecutedWithPrice,
                 OrderCancel, OrderDelete, OrderReplace>;
} // namespace netrx::itch
