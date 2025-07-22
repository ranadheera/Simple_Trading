#include "Updaters.h"

UpdatersList::UpdatersList(L1Buffer &l1buffer, L1Book &l1book, std::size_t numupdaters, std::size_t bufferSize):
 l1buffer_(l1buffer), l1book_(l1book), updaters_(numupdaters), bufferSize_(bufferSize) {}

void UpdatersList::start()
{
    for (int i = 0; i < updaters_.size(); ++i) {

        updaters_[i] = new Updater(l1buffer_, l1book_, bufferSize_);
        std::thread t(&Updater::exec, updaters_[i]);
        t.detach();
    }
}