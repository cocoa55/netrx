#include "netrx/itch/decoder.hpp"

#include <algorithm>
#include <cstdint>
#include <span>
#include <cassert>
#include "netrx/bytes.hpp"
#include "netrx/itch/messages.hpp"
#include <print>
#include <ranges>
namespace netrx::itch {
// Header field lengths.
constexpr auto kMessageTypeLength = 1;
constexpr auto kStockLocateLength = 2;
constexpr auto kTrackingNumberLength = 2;
constexpr auto kTimestampLength = 6;
constexpr auto kLow48Bits = (uint64_t{1} << 48) - 1;

// Add Order 'A' field lengths.
constexpr auto kOrderRefNumberLength = 8;
constexpr auto kBuySellIndicatorLength = 1;
constexpr auto kSharesLength = 4;
constexpr auto kStockLength = 8;
constexpr auto kPriceLength = 4;

// System Event 'S' field lengths.
constexpr auto kEventCodeLength = 1;

// Stock Directory 'R' field lengths.
constexpr auto kMarketCategoryLength = 1;
constexpr auto kFinancialStatusIndicatorLength = 1;
constexpr auto kRoundLotSizeLength = 4;
constexpr auto kRoundLotsOnlyLength = 1;
constexpr auto kIssueClassificationLength = 1;
constexpr auto kIssueSubTypeLength = 2;
constexpr auto kAuthenticityLength = 1;
constexpr auto kShortSaleThresholdIndicatorLength = 1;
constexpr auto kIpoFlagLength = 1;
constexpr auto kLuldReferencePriceTierLength = 1;
constexpr auto kEtpFlagLength = 1;
constexpr auto kEtpLeverageFactorLength = 4;
constexpr auto kInverseIndicatorLength = 1;

// Add Order with MPID 'F' field lengths.
constexpr auto kAttributionLength = 4;

// Order Executed 'E' / Order Executed With Price 'C' field lengths.
constexpr auto kMatchNumberLength = 8;
constexpr auto kPrintableLength = 1;

Message itchDecoder(std::span<const char> bytes) {
  size_t i = 0;
  const auto msgType = static_cast<MessageType>(bytes[i]);
  i += kMessageTypeLength;

  const auto stockLocate = netrx::readBigEndian<StockLocate>(bytes.subspan(i));
  i += kStockLocateLength;

  const auto trackingNumber =
      netrx::readBigEndian<TrackingNumber>(bytes.subspan(i));
  i += kTrackingNumberLength;

  // Take in full 8 bytes
  const auto raw =
      netrx::readBigEndian<Timestamp>(bytes.subspan(i - kTrackingNumberLength));

  // Drop the first 2 bytes to get 6.
  const Timestamp timestamp = raw & kLow48Bits;
  i += kTimestampLength;

  const MessageHeader header{.messageType = msgType,
                             .stockLocate = stockLocate,
                             .trackingNumber = trackingNumber,
                             .timestamp = timestamp};

  switch (msgType) {
  case MessageType::SystemEvent:
    return systemEvent(header, bytes.subspan(i));
  case MessageType::StockDirectory:
    return stockDirectory(header, bytes.subspan(i));
  case MessageType::AddOrder:
    return addOrder(header, bytes.subspan(i));
  case MessageType::AddOrderMpid:
    return addOrderMpid(header, bytes.subspan(i));
  case MessageType::OrderExecuted:
    return orderExecuted(header, bytes.subspan(i));
  case MessageType::OrderExecutedWithPrice:
    return orderExecutedWithPrice(header, bytes.subspan(i));
  case MessageType::OrderCancel:
    return orderCancel(header, bytes.subspan(i));
  case MessageType::OrderDelete:
    return orderDelete(header, bytes.subspan(i));
  case MessageType::OrderReplace:
    return orderReplace(header, bytes.subspan(i));
  default:
    return std::monostate{};
    break;
  }
  std::unreachable();
}

AddOrder addOrder(const MessageHeader &msgHeader,
                  const std::span<const char> bytes) {
  size_t i = 0;
  const auto orderRefNumber =
      readBigEndian<OrderReferenceNumber>(bytes.subspan(i));
  i += kOrderRefNumberLength;

  const auto buySellIndicator = bytes[i];
  i += kBuySellIndicatorLength;

  const auto shares = readBigEndian<Shares>(bytes.subspan(i));
  i += kSharesLength;

  Stock stock;
  std::ranges::copy(bytes.subspan(i, kStockLength), stock.begin());
  i += kStockLength;

  const auto price = readBigEndian<Price>(bytes.subspan(i));
  i += kPriceLength;
  assert(i == bytes.size());
  return {.header = msgHeader,
          .stock = stock,
          .orderReferenceNumber = orderRefNumber,
          .shares = shares,
          .price = price,
          .buySellIndicator = buySellIndicator};
}

SystemEvent systemEvent(const MessageHeader &msgHeader,
                        const std::span<const char> bytes) {
  size_t i = 0;
  const auto eventCode = bytes[i];
  i += kEventCodeLength;

  assert(i == bytes.size());
  return {.header = msgHeader, .eventCode = eventCode};
}

StockDirectory stockDirectory(const MessageHeader &msgHeader,
                              const std::span<const char> bytes) {
  size_t i = 0;
  Stock stock;
  std::ranges::copy(bytes.subspan(i, kStockLength), stock.begin());
  i += kStockLength;

  const auto marketCategory = bytes[i];
  i += kMarketCategoryLength;

  const auto financialStatusIndicator = bytes[i];
  i += kFinancialStatusIndicatorLength;

  const auto roundLotSize = readBigEndian<RoundLotSize>(bytes.subspan(i));
  i += kRoundLotSizeLength;

  const auto roundLotsOnly = bytes[i];
  i += kRoundLotsOnlyLength;

  const auto issueClassification = bytes[i];
  i += kIssueClassificationLength;

  IssueSubType issueSubType;
  std::ranges::copy(bytes.subspan(i, kIssueSubTypeLength),
                    issueSubType.begin());
  i += kIssueSubTypeLength;

  const auto authenticity = bytes[i];
  i += kAuthenticityLength;

  const auto shortSaleThresholdIndicator = bytes[i];
  i += kShortSaleThresholdIndicatorLength;

  const auto ipoFlag = bytes[i];
  i += kIpoFlagLength;

  const auto luldReferencePriceTier = bytes[i];
  i += kLuldReferencePriceTierLength;

  const auto etpFlag = bytes[i];
  i += kEtpFlagLength;

  const auto etpLeverageFactor =
      readBigEndian<EtpLeverageFactor>(bytes.subspan(i));
  i += kEtpLeverageFactorLength;

  const auto inverseIndicator = bytes[i];
  i += kInverseIndicatorLength;

  assert(i == bytes.size());
  return {.header = msgHeader,
          .stock = stock,
          .roundLotSize = roundLotSize,
          .etpLeverageFactor = etpLeverageFactor,
          .issueSubType = issueSubType,
          .marketCategory = marketCategory,
          .financialStatusIndicator = financialStatusIndicator,
          .roundLotsOnly = roundLotsOnly,
          .issueClassification = issueClassification,
          .authenticity = authenticity,
          .shortSaleThresholdIndicator = shortSaleThresholdIndicator,
          .ipoFlag = ipoFlag,
          .luldReferencePriceTier = luldReferencePriceTier,
          .etpFlag = etpFlag,
          .inverseIndicator = inverseIndicator};
}

AddOrderMpid addOrderMpid(const MessageHeader &msgHeader,
                          const std::span<const char> bytes) {
  size_t i = 0;
  const auto orderRefNumber =
      readBigEndian<OrderReferenceNumber>(bytes.subspan(i));
  i += kOrderRefNumberLength;

  const auto buySellIndicator = bytes[i];
  i += kBuySellIndicatorLength;

  const auto shares = readBigEndian<Shares>(bytes.subspan(i));
  i += kSharesLength;

  Stock stock;
  std::ranges::copy(bytes.subspan(i, kStockLength), stock.begin());
  i += kStockLength;

  const auto price = readBigEndian<Price>(bytes.subspan(i));
  i += kPriceLength;

  Attribution attribution;
  std::ranges::copy(bytes.subspan(i, kAttributionLength), attribution.begin());
  i += kAttributionLength;

  assert(i == bytes.size());
  return {.header = msgHeader,
          .stock = stock,
          .orderReferenceNumber = orderRefNumber,
          .shares = shares,
          .price = price,
          .attribution = attribution,
          .buySellIndicator = buySellIndicator};
}

OrderExecuted orderExecuted(const MessageHeader &msgHeader,
                            const std::span<const char> bytes) {
  size_t i = 0;
  const auto orderRefNumber =
      readBigEndian<OrderReferenceNumber>(bytes.subspan(i));
  i += kOrderRefNumberLength;

  const auto executedShares = readBigEndian<Shares>(bytes.subspan(i));
  i += kSharesLength;

  const auto matchNumber = readBigEndian<MatchNumber>(bytes.subspan(i));
  i += kMatchNumberLength;

  assert(i == bytes.size());
  return {.header = msgHeader,
          .orderReferenceNumber = orderRefNumber,
          .matchNumber = matchNumber,
          .executedShares = executedShares};
}

OrderExecutedWithPrice
orderExecutedWithPrice(const MessageHeader &msgHeader,
                       const std::span<const char> bytes) {
  size_t i = 0;
  const auto orderRefNumber =
      readBigEndian<OrderReferenceNumber>(bytes.subspan(i));
  i += kOrderRefNumberLength;

  const auto executedShares = readBigEndian<Shares>(bytes.subspan(i));
  i += kSharesLength;

  const auto matchNumber = readBigEndian<MatchNumber>(bytes.subspan(i));
  i += kMatchNumberLength;

  const auto printable = bytes[i];
  i += kPrintableLength;

  const auto executionPrice = readBigEndian<Price>(bytes.subspan(i));
  i += kPriceLength;

  assert(i == bytes.size());
  return {.header = msgHeader,
          .orderReferenceNumber = orderRefNumber,
          .matchNumber = matchNumber,
          .executedShares = executedShares,
          .executionPrice = executionPrice,
          .printable = printable};
}

OrderCancel orderCancel(const MessageHeader &msgHeader,
                        const std::span<const char> bytes) {
  size_t i = 0;
  const auto orderRefNumber =
      readBigEndian<OrderReferenceNumber>(bytes.subspan(i));
  i += kOrderRefNumberLength;

  const auto cancelledShares = readBigEndian<Shares>(bytes.subspan(i));
  i += kSharesLength;


  assert(i == bytes.size());
  return {.header = msgHeader,
          .orderReferenceNumber = orderRefNumber,
          .cancelledShares = cancelledShares};
}

OrderDelete orderDelete(const MessageHeader &msgHeader,
                        const std::span<const char> bytes) {
  size_t i = 0;
  const auto orderRefNumber =
      readBigEndian<OrderReferenceNumber>(bytes.subspan(i));
  i += kOrderRefNumberLength;

  assert(i == bytes.size());
  return {.header = msgHeader, .orderReferenceNumber = orderRefNumber};
}

OrderReplace orderReplace(const MessageHeader &msgHeader,
                          const std::span<const char> bytes) {
  size_t i = 0;
  const auto originalOrderRefNumber =
      readBigEndian<OrderReferenceNumber>(bytes.subspan(i));
  i += kOrderRefNumberLength;

  const auto newOrderRefNumber =
      readBigEndian<OrderReferenceNumber>(bytes.subspan(i));
  i += kOrderRefNumberLength;

  const auto shares = readBigEndian<Shares>(bytes.subspan(i));
  i += kSharesLength;

  const auto price = readBigEndian<Price>(bytes.subspan(i));
  i += kPriceLength;

  assert(i == bytes.size());
  return {.header = msgHeader,
          .originalOrderReferenceNumber = originalOrderRefNumber,
          .newOrderReferenceNumber = newOrderRefNumber,
          .shares = shares,
          .price = price};
}
} // namespace netrx::itch
