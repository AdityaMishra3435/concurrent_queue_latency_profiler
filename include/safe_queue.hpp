#pragma once
#include <queue>
#include <mutex>
#include <optional>

template<typename T>
class SafeQueue{
public:
    void push(const T& value){
        std::scoped_lock lock(mutex_);
        queue_.push(value);
    }

    std::optional<T> pop(){
        std::scoped_lock lock(mutex_);
        if (queue_.empty())return std::nullopt;
        auto val=queue_.front();
        queue_.pop();
        return val;
    }

private:
    std::queue<T>queue_;
    std::mutex mutex_;
};
