#include "Dispatcher.h"

void Dispatcher::start()
{
    auto func = [&] {
        Marketdata data;
        while (true) {
            if (ringBuffer_.read(data)) {
                auto updaterID = data.symbol % numUpdaters_;
                auto updater = updaterList_.getUpdater(updaterID);
                if (!updater)
                    std::cout << "can not find updater for " << updaterID << " ";
                else
                 updater->addData(data);
            }
        }
    };
    std::thread t(func);
    t.detach();
}