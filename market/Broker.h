#ifndef BROKER_H
#define BROKER_H

#include <memory>
#include "Session.h"
#include "CommonUtils.h"
#include "Events.h"
#include "SessionWorker.h"

template<typename MDSessionConfig, typename OESessionConfig> class Broker
{
public:
    Broker(const MDSessionConfig& marketDataSessionConfig, const OESessionConfig& orderExecSessionConfig, 
        const TradeSymbols& symbols, std::vector<SymbolID>&& interestedSymbols);
public:
    void notify(const EventBase& event);
    bool openSessoins(SessionWorker& marketDataWorker, SessionWorker& orderExecWorker);    
private:
    std::unique_ptr<Session<MDSessionConfig>> marketDataSession_;
    std::unique_ptr<Session<OESessionConfig>> orderExecSession_;
    const TradeSymbols& symbols_;
    std::vector<SymbolID> interestedSymbols_;
};


template<typename MDSessionConfig, typename OESessionConfig>
 Broker<MDSessionConfig, OESessionConfig>::Broker(const MDSessionConfig& marketDataSessionConfig, const OESessionConfig& orderExecSessionConfig,
        const TradeSymbols &symbols, std::vector<SymbolID>&& interestedSymbols) : 
        symbols_(symbols), interestedSymbols_(std::move(interestedSymbols))
{
    auto mdMsgParser = std::make_unique<typename MDSessionConfig::MsgParser>(symbols, interestedSymbols);
    auto mdMsgBuilder = std::make_unique<typename MDSessionConfig::MsgBuilder>(marketDataSessionConfig.outMessageMaxBodyLength_, marketDataSessionConfig.maxOutMessageSeqNo_, marketDataSessionConfig.timeAccuracy_);
    marketDataSession_ = std::make_unique<Session<MDSessionConfig>>(std::move(mdMsgParser), std::move(mdMsgBuilder), marketDataSessionConfig);

    auto oeMsgParser = std::make_unique<typename MDSessionConfig::MsgParser>(symbols, interestedSymbols);
    auto oeMsgBuilder = std::make_unique<typename MDSessionConfig::MsgBuilder>(orderExecSessionConfig.outMessageMaxBodyLength_, orderExecSessionConfig.maxOutMessageSeqNo_, orderExecSessionConfig.timeAccuracy_);
    orderExecSession_ = std::make_unique<Session<OESessionConfig>>(std::move(oeMsgParser), std::move(oeMsgBuilder), orderExecSessionConfig);

    Subscriber subscriber(this, ::notify<Broker<MDSessionConfig, OESessionConfig>>);
    marketDataSession_->registerForSessionEvents(subscriber);
     orderExecSession_->registerForSessionEvents(subscriber);
}


template<typename MDSessionConfig, typename OESessionConfig>
 bool Broker<MDSessionConfig, OESessionConfig>::openSessoins(SessionWorker& marketDataWorker, SessionWorker& orderExecWorker)
 {
    if (!marketDataSession_->openSession())
        return false;

    if (!orderExecSession_->openSession())
        return false;

    marketDataWorker.addSessionPtr(marketDataSession_.get());
    orderExecWorker.addSessionPtr(orderExecSession_.get());
    return true;
 }

template<typename MDSessionConfig, typename OESessionConfig>
void Broker<MDSessionConfig, OESessionConfig>::notify(const EventBase& event)
 {

 }

 

#endif