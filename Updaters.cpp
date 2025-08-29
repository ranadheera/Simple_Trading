#include "Updaters.h"

BookUpdaters::BookUpdaters(L1Book &l1book, L2Book &l2book, std::size_t numupdaters, std::size_t bufferSize):
 l1book_(l1book), l2book_(l2book), updaters_(numupdaters), updaterThreads_(numupdaters), bufferSize_(bufferSize) {}

 BookUpdaters::~BookUpdaters()
 {
    for (auto updater : updaters_) {
        delete updater;
    }
 }

void BookUpdaters::start()
{
    for (int i = 0; i < updaters_.size(); ++i) {

        updaters_[i] = new Updater(*this, l1book_, l2book_, bufferSize_);
        updaterThreads_[i] =  std::thread(&Updater::exec, updaters_[i]);
    }
    numUpdaers_.store(updaters_.size(), std::memory_order_release);
}

void BookUpdaters::stop()
{
    for (int i = 0; i < updaters_.size(); ++i) {
        updaters_[i]->setStopFlag();
    }
    
    while (numUpdaers_.load(std::memory_order_acquire));

    for (auto &thread : updaterThreads_) {
        if (thread.joinable())
            thread.join();
    }
}


void BookUpdaters::sendToUpdater(const Marketdata& data)
{
    auto updaterID = data.symbol % updaters_.size();
    auto updater = updaters_[updaterID];

    if (!updater)
        std::cout << "can not find updater for " << updaterID << " ";
    else
        updater->addData(data);
}