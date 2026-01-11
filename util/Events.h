#ifndef EVENTS_H
#define EVENTS_H

enum class EventType {
    MARKET_CHANGE,
};

class EventBase {
public:
    EventBase(EventType type) : type_(type) { }
    EventType getEventType() { return type_; }
private:
    EventType type_;
};

using notifyFuncType = void(*)(void*, const EventBase&);

template <typename T> void notify(void* obj, const EventBase& event)
{
    T* subscriber = static_cast<T*>(obj);
    subscriber->notify(event);
}

class Subscriber
{
public:
    Subscriber(void* object, notifyFuncType func) : object_(object), func_(func) {}
    void notify(const EventBase& event) { func_(object_, event); }
private:
    void *object_;
    notifyFuncType func_;
};

#endif