#ifndef MESSAGE_H
#define MESSAGE_H

#include <vector>
#include <string>

struct FixTags {
   enum {
      BeginString = 8,
      BodyLength = 9,
      CheckSum = 10,
      MsgSeqNum = 34,
      MsgType = 35,
      SenderCompID = 49,
      SendingTime = 52,
      TargetCompID = 56,
      EncryptMethod = 98,
      HeartBtInt = 108,
      ResetSeqNumFlag = 141,
      Username = 553,
      Password = 554
   };
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
   static constexpr char SOH = '\x01';
   std::vector<char> buffer_;
   std::size_t dataSize_ = 0;
   std::size_t sentCount_ = 0;
   std::size_t msgLenPos_ = 0;
   std::size_t bodyStartPos_ = 0;
   MessageStatus status_ = MessageStatus::INITIAL;
};

#endif