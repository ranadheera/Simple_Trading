#include "Message.h"
#include <cstdio>
#include <cstring>

#include "Message.h"
#include <cstdio>
#include <cstring>
#include <array>


std::string_view to_string(FixVersion version) {
    static const std::array<std::string_view, static_cast<int>(FixVersion::END)> stringValus{"FIX.4.2", "FIX.4.2"};
    return stringValus[static_cast<int>(version)];
}

OutMessage::UpdateStatus OutMessage::addTagValue(int tag, std::string_view value)
{
    auto available = buffer_.size() - dataSize_;
    auto written = snprintf(buffer_.data() + dataSize_, available,
                            "%d=%s%c", tag, value.data(), SOH);

    if (written < 0)
        return UpdateStatus::FORMAT_ERROR;

    if (static_cast<size_t>(written) >= available) {
        status_ = MessageStatus::INVALID;
        return UpdateStatus::OVERFLOW;
    }

    dataSize_ += written;
    return UpdateStatus::SUCCESS;
}

OutMessage::UpdateStatus OutMessage::addTagValue(int tag, int value)
{
    auto available = buffer_.size() - dataSize_;
    auto written = snprintf(buffer_.data() + dataSize_, available,
                            "%d=%d%c", tag, value, SOH);

    if (written < 0)
        return UpdateStatus::FORMAT_ERROR;

    if (static_cast<size_t>(written) >= available) {
        status_ = MessageStatus::INVALID;
        return UpdateStatus::OVERFLOW;
    }

    dataSize_ += written;
    return UpdateStatus::SUCCESS;
}


OutMessage::UpdateStatus OutMessage::addTagValue(int tag, size_t value)
{
    auto available = buffer_.size() - dataSize_;
    auto written = snprintf(buffer_.data() + dataSize_, available,
                            "%d=%zu%c", tag, value, SOH);

    if (written < 0)
        return UpdateStatus::FORMAT_ERROR;

    if (static_cast<size_t>(written) >= available) {
        status_ = MessageStatus::INVALID;
        return UpdateStatus::OVERFLOW;
    }

    dataSize_ += written;
    return UpdateStatus::SUCCESS;
}

OutMessage::UpdateStatus OutMessage::addTagValue(int tag, char value)
{
    auto available = buffer_.size() - dataSize_;
    auto written = snprintf(buffer_.data() + dataSize_, available,
                            "%d=%c%c", tag, value, SOH);

    if (written < 0)
        return UpdateStatus::FORMAT_ERROR;

    if (static_cast<size_t>(written) >= available) {
        status_ = MessageStatus::INVALID;
        return UpdateStatus::OVERFLOW;
    }

    dataSize_ += written;
    return UpdateStatus::SUCCESS;
}

OutMessage::UpdateStatus OutMessage::addTagValue(int tag, double value)
{
    auto available = buffer_.size() - dataSize_;
    auto written = snprintf(buffer_.data() + dataSize_, available,
                            "%d=%.4f%c", tag, value, SOH);

    if (written < 0)
        return UpdateStatus::FORMAT_ERROR;

    if (static_cast<size_t>(written) >= available) {
        status_ = MessageStatus::INVALID;
        return UpdateStatus::OVERFLOW;
    }

    dataSize_ += written;
    return UpdateStatus::SUCCESS;
}



const char* OutMessage::getDataToBeSent(size_t& length) const
{
    length = dataSize_ - sentCount_;
    return buffer_.data() + sentCount_;
}

void OutMessage::reset()
{
    dataSize_ = 0;
    sentCount_ = 0;
    status_ = MessageStatus::INITIAL;
}