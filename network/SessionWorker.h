#ifndef SESSION_WORKER
#define SESSION_WORKER

#include "Session.h"
#include "MessageParser.h"
#include "MessageBuilder.h"
#include <variant>
#include <thread>

using Session44 = Session<SessionConfig<MessageParser, MessageBuilder>>;
using SessionPtrHolder = std::variant<Session44*>;

class SessionWorker
{
public:
    SessionWorker()=default;
    SessionWorker(const SessionWorker&) = delete;
    SessionWorker& operator=(const SessionWorker&) = delete;
public:
    template<typename T> bool addSessionPtr(T* session);
    void start();
    void stop();
private:
    void exec();
private:
    std::vector<SessionPtrHolder> sessionList_;
    std::jthread workerThread_;
    bool running_ = false;
};

template<typename T> bool SessionWorker::addSessionPtr(T *session)
{
    if (running_)
        return false;

    sessionList_.push_back(session);
    return true;
}

#endif