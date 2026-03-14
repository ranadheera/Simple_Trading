#ifndef MESSAGE_H
#define MESSAGE_H

#include <cstdint>
#include <vector>
#include <string>

struct FixTags {
   enum {
      BeginString = 8,
      BodyLength = 9,
      CheckSum = 10,
      SecuirtyIDSource = 22,
      LastPrice = 31,
      LastQuantity = 32,
      MsgSeqNum = 34,
      MsgType = 35,
      PossDupFlag = 43,
      SecuirtyID = 48,
      SenderCompID = 49,
      SenderSubID = 50,
      SendingTime = 52,
      Symbol = 55,
      TargetCompID = 56,
      TargetSubID = 57,
      TransactionTime = 60,
      RptSeq = 83,
      PossResend = 97,
      EncryptMethod = 98,
      HeartBtInt = 108,
      OrigSendingTime = 122,
      ResetSeqNumFlag = 141,
      NoEntries = 268,
      EntryType = 269,
      EntryPrice = 270,
      EntrySize = 271,
      EntryDate = 272,
      EntryTime = 273,
      EntryID = 278,
      UpdateAction = 279,
      EntryPosition = 290,
      TradingSessionID = 336,
      NumberOfOrders = 346,
      TradinSessionSubID = 625,
      NoFeedTypes = 1021,
      FeedType = 1022,
      PriceLevel = 1023,
      LastMsgSeqNumProcessed = 369,
      Username = 553,
      Password = 554,
      LastLiquidityIndicator = 851,
      AggressorIndicator = 5797,

   };
};

enum class EntryType {
   BID = '0',
   OFFER = '1',
   TRADE = '2',
   UNDEFINED 
};

enum class UpdateAction {
   NEW = 0,
   CHANGE,
   DELETE,
   UNDEFINED
};

enum class AggressorIndicator {
   PASSIVE = 'N',
   AGGRESSOR = 'Y',
   UNDEFINED
};

enum class FixVersion {
   FIX42 = 0,
   FIX44,
   END
};

std::string_view to_string(FixVersion version);

class OutMessage {
   
friend class Session;

public:
   enum class UpdateStatus { SUCCESS, FORMAT_ERROR, OVERFLOW};
   enum class MessageStatus { INITIAL, DATA_FILLED, FINALIZED, INVALID };
   static constexpr char SOH = '\x01';
public:
   OutMessage(std::size_t size = 2048) : buffer_(size) {}
   UpdateStatus addTagValue(int tag, std::string_view value);
   UpdateStatus addTagValue(int tag, int value);
   UpdateStatus addTagValue(int tag, size_t value);
   UpdateStatus addTagValue(int tag, double value);
   UpdateStatus addTagValue(int tag, char value);
   const char* getDataToBeSent(size_t &length) const;
   MessageStatus getSatus() { return status_;}
   void addSentCount(size_t size) { sentCount_ += sentCount_;}
   void reset();
private:
   std::vector<char> buffer_;
   std::size_t dataSize_ = 0;
   std::size_t sentCount_ = 0;
   std::size_t msgLenPos_ = 0;
   std::size_t bodyStartPos_ = 0;
   MessageStatus status_ = MessageStatus::INITIAL;
};

#endif