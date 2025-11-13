#pragma once
#include <atomic>
#include <array>
#include <optional>


template<typename T,size_t Capacity>
class LockFreeQueue{
public:
    LockFreeQueue(): head_(0),tail_(0) {}

    bool push(const T& value){
        const auto head=head_.load(std::memory_order_relaxed);
        const auto next=increment(head);
        // queue is full - happens-before relationship
        if (next==tail_.load(std::memory_order_acquire)){
            return false;
        }
        buffer_[head]=value;
        head_.store(next,std::memory_order_release);
        return true;
    }

    std::optional<T>pop(){
        const auto tail=tail_.load(std::memory_order_relaxed);
        // queue is empty
        if (tail==head_.load(std::memory_order_acquire)){
            return std::nullopt;
        }

        auto value=buffer_[tail];
        tail_.store(increment(tail),std::memory_order_release);
        return value;
    }

private:
    static size_t increment(size_t i) noexcept {
        return (i+1)%Capacity;
    }

    std::array<T, Capacity>buffer_{};
    std::atomic<size_t>head_;
    std::atomic<size_t>tail_;
};
