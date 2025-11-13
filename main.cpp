#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <optional>
#include <string_view>
#include <type_traits>
#include <fstream>
#include "./include/safe_queue.hpp"
#include "./include/lock_free_queue.hpp"

using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;
using namespace std::chrono_literals;

// compute percentile
template<typename T>
T percentile(std::vector<T>& data,double pct){
    if (data.empty())return T{};
    size_t idx = static_cast<size_t>(pct*data.size());
    idx=std::min(idx,data.size()-1);
    return data[idx];
}

// benchmark function
template <typename QueueType>
void runBenchmark(const std::string_view name){
    constexpr int NUM_MESSAGES=1000000;
    QueueType queue;
    std::vector<long long>latencies;
    latencies.reserve(NUM_MESSAGES);

    std::cout<<"Running "<<name<<" benchmark with "<< NUM_MESSAGES<< " messages...\n";

    // producer thread
    std::thread producer([&]{
        for (int i=0;i<NUM_MESSAGES;++i){
            // compile-time check for return type
            if constexpr(std::is_same_v<decltype(queue.push(Clock::now())),void>){
                queue.push(Clock::now());
            }
            else{
                while(!queue.push(Clock::now())){
                    // busy-wait if full => lock-free case
                }
            }
            std::this_thread::sleep_for(1us);
        }
    });
    // consumer thread
    std::thread consumer([&]{
        for (int i=0;i<NUM_MESSAGES;++i){
            while(true){
                if(auto t=queue.pop()){
                    auto now=Clock::now();
                    auto latency=std::chrono::duration_cast<std::chrono::nanoseconds>(now-*t).count();
                    latencies.push_back(latency);
                    break;
                }
            }
        }
    });
    producer.join();
    consumer.join();

    std::ofstream outfile(std::string(name) + "_latencies.txt");
    std::sort(latencies.begin(), latencies.end());
    // save latency values for data analysis
    for (long long l : latencies) {
        outfile << l << "\n";
    }
    double avg = std::accumulate(latencies.begin(),latencies.end(),0.0)/latencies.size();
    long long p50=percentile(latencies,0.50);
    long long p99=percentile(latencies,0.99);

    std::cout<<"\n["<< name<<" Results]\n";
    std::cout<<"Average Latency: "<<avg <<" ns\n";
    std::cout<<"P50: "<<p50<< " ns\n";
    std::cout<<"P99: "<<p99<< " ns\n";
    std::cout<<"Total: "<<latencies.size()<<"\n\n";
}

int main(int argc,char** argv){
    if(argc<2){
        std::cerr<<"Usage: ./latency_profiler [mutex|lockfree]\n";
        return 1;
    }

    std::string_view mode=argv[1];
    if(mode=="mutex"){
        runBenchmark<SafeQueue<TimePoint>>("Mutex Queue");
    }
    else if(mode=="lockfree"){
        runBenchmark<LockFreeQueue<TimePoint,1024>>("Lock-Free Queue");
    } 
    else{
        std::cerr<<"Unknown mode: "<<mode<<"\n";
        return 1;
    }
}

