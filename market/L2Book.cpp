#include "L2Book.h"


L2Book::L2Book(std::size_t size, double tickSize) : bidLiquidity_(size, tickSize), askLiquidity_(size, tickSize)
{

}

L2Book::L2Book() : L2Book(2048, 0.01) {}

bool L2Book::setSize(std::size_t size)
{
    bool st = askLiquidity_.setSize(size);
    st &= bidLiquidity_.setSize(size);
    return st;
}

bool L2Book::setTickSize(double tickSize) {
    bool st = askLiquidity_.setTickSize(tickSize);
    st &= bidLiquidity_.setTickSize(tickSize);
    return st;
}

bool L2Book::init()
{
    bool st = askLiquidity_.init();
    st &= bidLiquidity_.init();
    return st;
}

bool L2Book::update(const Marketdata &data) {

    bool updateSuccess = false;

    if (data.getAskPrice() > 0)  {
        updateSuccess = askLiquidity_.update(data.getAskPrice(), data.getAskVolume());
    }

    if (data.getBidPrice() > 0) {
        updateSuccess |= bidLiquidity_.update(data.getBidPrice(), data.getBidVolume());
    }

    if (updateSuccess)
        lastUpdatedTime_ = data.getTimeStamp();

    return updateSuccess;
}

void L2Book::getSnapShot(L2Book &l2Book) const
{
    bidLiquidity_.copyTo(l2Book.bidLiquidity_);
    askLiquidity_.copyTo(l2Book.askLiquidity_);
    l2Book.lastUpdatedTime_ = lastUpdatedTime_;
}
