/*
 thread.hpp
 */

#ifndef TRAX_THREAD_HPP_
#define TRAX_THREAD_HPP_

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include "trax.hpp"

using namespace Trax;

namespace Trax{
    namespace KizuNa{
        
        SearchThread::SearchThread(size_t thread_id, Node& node/*, SharedData& shared_data,
                                                                ThreadManager& thread_manager*/)
        : //thread_manager_(thread_manager),
        search_(/*shared_data, */thread_id),
        root_node_(node/*Position::CreateStartPosition()*/),
        searching_{false},
        exit_{false},
        native_thread_([&](){ IdleLoop(); }) {
            // マスタースレッドではなく、ワーカースレッドに限る
            assert(!search_.isMasterThread());
        }
        
        SearchThread::~SearchThread() {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                exit_ = true;
                sleep_condition_.notify_one();
            }
            native_thread_.join();
        }
        
        void SearchThread::IdleLoop() {
            while (!exit_) {
                // exit_ か searching_ が true になるまでスリープする
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    sleep_condition_.wait(lock, [this](){ return exit_ || searching_; });
                }
                
                if (exit_) {
                    break;
                }
                
                search_.iterativeDeepening(root_node_/*, thread_manager_*/);
                
                // 探索終了後の処理
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    // searching_フラグを元に戻しておく
                    searching_ = false;
                    // WaitUntilSearchFinished()で待機している場合があるので、探索終了を知らせる必要がある
                    sleep_condition_.notify_one();
                }
            }
        }
        
        void SearchThread::SetRootNode(const Node& root_node) {
            root_node_ = root_node;
        }
        
        void SearchThread::StartSearching() {
            std::unique_lock<std::mutex> lock(mutex_);
            searching_ = true;
            sleep_condition_.notify_one();
        }
        
        void SearchThread::WaitUntilSearchIsFinished() {
            std::unique_lock<std::mutex> lock(mutex_);
            sleep_condition_.wait(lock, [this](){ return !searching_; });
        }
        
        ThreadManager::ThreadManager(/*SharedData& shared_data, TimeManager& time_manager*/)
        /*:shared_data_(shared_data),
         time_manager_(time_manager)*/ {
         }
        
        
        
        /*uint64_t ThreadManager::CountNodesSearchedByWorkerThreads() const {
         uint64_t total = 0;
         for (const std::unique_ptr<SearchThread>& worker : worker_threads_) {
         total += worker->search_.num_nodes_searched();
         }
         return total;
         }
         
         uint64_t ThreadManager::CountNodesUnder(Move move) const {
         uint64_t total = 0;
         for (const std::unique_ptr<SearchThread>& worker : worker_threads_) {
         total += worker->search_.GetNodesUnder(move);
         }
         return total;
         }*/
        
        //RootMove
        
        
    }
}


#endif // TRAX_THREAD_HPP_
