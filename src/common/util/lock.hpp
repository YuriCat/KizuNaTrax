/*
 lock.hpp
 Katsuki Ohto
 */

#ifndef UTIL_LOCK_HPP_
#define UTIL_LOCK_HPP_

#include "../defines.h"

// 0-1スピンロック
template<class data_t = int>
class SpinLock{
    // 0 unlock 1 lock
public:
    constexpr bool is_locked()const noexcept{
        return data_;
    }
    void lock()noexcept{
        for(;;){
            while(data_ != UNLOCK_);
            data_t tmp = LOCK_;
            if(data_.exchange(tmp, std::memory_order_acquire) == UNLOCK_){ return; }
        }
    }
    void unlock()noexcept{
        data_ = UNLOCK_;
    }
private:
    constexpr static data_t UNLOCK_ = 0;
    constexpr static data_t LOCK_ = 1;
    
    std::atomic<data_t> data_;
};

template<class data_t = int>
class NullLock{
public:
    static constexpr bool is_locked()noexcept{
        return false;
    }
    static void lock()noexcept{}
    static void unlock()noexcept{}
private:
    data_t data_;
};

// シングルスレッド時のダミークラス
class NullNodeLock{
public:
    constexpr static int lock_size()noexcept{ return 0; }
    constexpr static uint32_t lock_mask()noexcept{ return 0; }
private:
};

// 排他処理用スピンロック兼局面の同一性確保用ビット
template<class data_t>
class NodeLock{
public:
    constexpr NodeLock() : data_(){}
    constexpr NodeLock(data_t arg) : data_(arg){}
    
    constexpr static int lock_size()noexcept{ return N_LOCK_BITS; }
    constexpr static data_t lock_mask()noexcept{ return ALL_MASK; }
    
    constexpr data_t data()const noexcept{ return data_.load(); }
    
    constexpr static bool compare(data_t stored_data, data_t compared_data)noexcept{
        return !((stored_data ^ compared_data) & (~ALL_MASK));
    }
    
    constexpr bool compare(data_t compared_data)const noexcept{
        return !((data_ ^ compared_data) & (~ALL_MASK));
    }
    
    constexpr bool is_locked()const noexcept{
        return data_ & ALL_MASK;
    }
    void lock()noexcept{
        data_t cur = data_;
        for(;;){
            if(!(data_.exchange(cur | ALL_MASK, std::memory_order_acquire) & ALL_MASK)){ return; }
            while(data_ & ALL_MASK);
        }
    }
    void unlock()noexcept{
        data_ &= (~ALL_MASK);
    }
    
    void replace(data_t dat)noexcept{ data_ = dat & (~ALL_MASK); }
    void replace_lock(data_t dat)noexcept{ data_ = dat | ALL_MASK; }
    
    bool regist(data_t dat)noexcept{
        data_t org = static_cast<data_t>(0);
        return data_.compare_exchange_strong(org, dat & (~ALL_MASK));
    }
    void force_regist(data_t dat)noexcept{
        const data_t stored_data = dat & (~ALL_MASK);
        data_ = stored_data;
    }
    bool regist_lock(data_t dat)noexcept{
        data_t org = static_cast<data_t>(0);
        return data_.compare_exchange_strong(org, dat | ALL_MASK);
    }
    
    constexpr bool is_being_read()const noexcept{ return is_locked(); }
    constexpr bool is_being_fed()const noexcept{ return is_locked(); }
    constexpr bool is_being_made()const noexcept{ return is_locked(); }
    
    void start_read()noexcept{ lock(); }
    void start_feed()noexcept{ lock(); }
    void start_make()noexcept{ lock(); }
    
    void finish_read()noexcept{ unlock(); }
    void finish_feed()noexcept{ unlock(); }
    void finish_make()noexcept{ unlock(); }
    
    bool regist_start_read(data_t dat)noexcept{ return regist_lock(dat); }
    bool regist_start_feed(data_t dat)noexcept{ return regist_lock(dat); }
    bool regist_start_make(data_t dat)noexcept{ return regist_lock(dat); }
    
private:
    constexpr static int N_LOCK_BITS = 1;
    constexpr static data_t ALL_MASK = 1;
    
    std::atomic<data_t> data_;
};

// 排他処理用スピンロック兼局面の同一性確保用ビット、ReadとMakeの２種類のロック
template<class data_t, int N_READ_BITS = 8>
class NodeRMLock{
public:
    constexpr NodeRMLock() : data_(){}
    constexpr NodeRMLock(data_t arg) : data_(arg){}
    
    constexpr static int lock_size()noexcept{ return N_LOCK_BITS; }
    constexpr static data_t lock_mask()noexcept{ return ALL_MASK; }

    constexpr data_t data()const noexcept{ return data_.load(); }
    
    constexpr static bool compare(data_t stored_data, data_t compared_data)noexcept{
        return !((stored_data ^ compared_data) & (~ALL_MASK));
    }
    
    constexpr bool compare(data_t compared_data)const noexcept{
        return !((data_ ^ compared_data) & (~ALL_MASK));
    }
    
    constexpr bool is_being_read()const noexcept{
        return data_ & READ_MASK;
    }
    
    constexpr bool is_being_fed()const noexcept{
        return is_being_read();
    }
    
    constexpr bool is_being_made()const noexcept{
        return data_ & MAKE_MASK;
    }
    
    void replace(data_t dat)noexcept{
        const data_t stored_data = dat & (~ALL_MASK);
        data_ = stored_data;
    }
    
    void replace_make(data_t dat)noexcept{
        const data_t stored_data = (dat & (~ALL_MASK)) | MAKE_MASK;
        return data_ = stored_data;
    }
    
    bool regist(data_t dat)noexcept{
        data_t org = static_cast<data_t>(0);
        const data_t stored_data = dat & (~ALL_MASK);
        return data_.compare_exchange_strong(org, stored_data);
    }
    void force_regist(data_t dat)noexcept{
        const data_t stored_data = dat & (~ALL_MASK);
        data_ = stored_data;
    }
    
    bool regist_start_make(data_t dat)noexcept{
        data_t org = static_cast<data_t>(0);
        const data_t stored_data = (dat & (~ALL_MASK)) | MAKE_MASK;
        return data_.compare_exchange_strong(org, stored_data);
    }
    
    void start_read()noexcept{
        for(;;){
            data_t cur, next;
            do{
                cur = data_;
            }while(cur & READ_STOP);
            next = cur + 1;
            if(data_.compare_exchange_weak(cur, next)){ return; }
        }
    }
    void start_feed()noexcept{
        start_read();
    }
    void start_make()noexcept{
        for(;;){
            data_t cur, next;
            do{
                cur = data_;
            }while(cur & MAKE_STOP);
            next = cur | MAKE_MASK;
            if(data_.compare_exchange_weak(cur, next)){ return; }
        }
    }
    
    void finish_read()noexcept{
        --data_;
    }
    void finish_feed()noexcept{
        finish_read();
    }
    void finish_make()noexcept{
        data_ &= (~MAKE_MASK);
    }
    
    template<class callback_t>
    void try_read_to_make(const callback_t& callback){
        data_t cur, next;
        do{
            callback();
            cur = data_;
            next = cur | MAKE_MASK;
        }while((!is_being_made()) && data_.compare_exchange_weak(cur, next));
    }
    
    void make_to_read()noexcept{
        data_ = (data_ & (~MAKE_MASK)) + 1;
    }
    void read_to_make()noexcept{
        for(;;){
            data_t cur, next;
            do{
                cur = data_;
            }while(cur & MAKE_STOP);
            next = cur | MAKE_MASK;
            if(data_.compare_exchange_weak(cur, next)){ return; }
        }
    }
    
private:
    constexpr static int N_MAKE_BITS = 1;
    constexpr static int N_LOCK_BITS = N_READ_BITS + N_MAKE_BITS;
    
    constexpr static data_t READ_MASK = ((1ULL << N_READ_BITS) - 1ULL) << 0;
    constexpr static data_t MAKE_MASK = ((1ULL << N_MAKE_BITS) - 1ULL) << N_READ_BITS;
    constexpr static data_t ALL_MASK = READ_MASK | MAKE_MASK;
    
    constexpr static data_t READ_STOP = MAKE_MASK;
    constexpr static data_t MAKE_STOP = MAKE_MASK | READ_MASK;
    
    std::atomic<data_t> data_;
};


#endif // UTIL_LOCK_HPP_