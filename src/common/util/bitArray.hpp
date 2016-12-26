/*
 bitArray.hpp
 Katsuki Ohto
 */

#ifndef UTIL_BITARRAY_HPP_
#define UTIL_BITARRAY_HPP_

#include <iostream>
#include <cstdio>
#include <cassert>
#include <cstdlib>

#include "../defines.h"

#include "container.hpp"

// 整数のNビットずつを使ってデータを保存する
// アクセスは配列より遅いだろうが、
// パラレルに処理出来るので速くなる部分もあると期待

template<typename data_t, int MAX_SIZE_, int N_, int ARG_SIZE_, typename atomic_data_t = data_t>
class BitArrayInRegister{

public:
    
    using size_t = std::size_t; // unsigned int;
    using entry_t = data_t;
    using this_type_t = BitArrayInRegister<data_t, MAX_SIZE_, N_, ARG_SIZE_, atomic_data_t>;

    // validator
    static void assert_index(const size_t n){ ASSERT(0 <= n && n < SIZE_, std::cerr << n << std::endl;); }
    static void assert_entry(const entry_t entry){ ASSERT(entry <= entry_max(), std::cerr << entry << std::endl;); }

    constexpr BitArrayInRegister() :dat_(){}
    constexpr BitArrayInRegister(const data_t arg) : dat_(arg){}
    constexpr BitArrayInRegister(const BitArrayInRegister<data_t, MAX_SIZE_, N_, ARG_SIZE_, atomic_data_t>& arg) : dat_(arg.data()){}

    // operator
    constexpr operator data_t()const noexcept{ return static_cast<data_t>(dat_); }

    BitArrayInRegister& operator =(const data_t arg)noexcept{ dat_ = arg; return *this; }
    BitArrayInRegister& operator +=(const data_t arg)noexcept{ dat_ += arg; return *this; }
    BitArrayInRegister& operator -=(const data_t arg)noexcept{ dat_ -= arg; return *this; }
    BitArrayInRegister& operator *=(const data_t arg)noexcept{ dat_ += arg; return *this; }
    BitArrayInRegister& operator /=(const data_t arg)noexcept{ dat_ -= arg; return *this; }
    BitArrayInRegister& operator |=(const data_t arg)noexcept{ dat_ |= arg; return *this; }
    BitArrayInRegister& operator &=(const data_t arg)noexcept{ dat_ &= arg; return *this; }
    BitArrayInRegister& operator ^=(const data_t arg)noexcept{ dat_ ^= arg; return *this; }
    BitArrayInRegister& operator <<=(const data_t arg)noexcept{ dat_ <<= arg; return *this; }
    BitArrayInRegister& operator >>=(const data_t arg)noexcept{ dat_ >>= arg; return *this; }
            
    constexpr data_t any()const noexcept{ return dat_; }

    constexpr entry_t operator[](const size_t n)const noexcept{ // 境界チェックなし
        return (dat_ >> place(n)) & MASK_ENTRY_;
    }

    constexpr entry_t front()const noexcept{ return dat_ & MASK_ENTRY_; }

    // set special value
    this_type_t& clear()noexcept{
        dat_ = static_cast<data_t>(0); return *this;
    }
    this_type_t& fill(const entry_t entry)noexcept{ // 同じ値を範囲内に詰める
        dat_ = fillBits<data_t, N_>(entry) & full_mask(); return *this;
    }
    this_type_t& fill_all(const entry_t entry)noexcept{ // 同じ値を範囲外まで詰める
        dat_ = fillBits<data_t, N_>(entry); return *this;
    }

    this_type_t& setSequence()noexcept{ // TODO:コンパイル時計算されてる?
        clear();
        for(size_t i = 0; i < size(); ++i){
            set(i, i & MASK_ENTRY_);
        }
        return *this;
    }
    
    constexpr data_t data()const noexcept{ return dat_; }
    data_t& data()noexcept{ return dat_; }

    // static関数
    static constexpr size_t place(const size_t n)noexcept{ return (n * N_); }

    static constexpr data_t mask(const size_t n)noexcept{ return MASK_ENTRY_ << place(n); }

    static constexpr data_t mask_by_bit_index(const size_t n)noexcept{ return MASK_ENTRY_ << n; }
    
    static constexpr data_t mask(const size_t n0, const size_t n1)noexcept{
        return ((MASK_BIT_ << place(n1 + 1)) - MASK_BIT_) & (~((MASK_BIT_ << place(n0)) - MASK_BIT_));
    }
    static constexpr data_t full_mask()noexcept{
        return (place(SIZE_) >= MAX_SIZE_) ? static_cast<data_t>(-1) : ((static_cast<data_t>(1) << place(SIZE_)) - static_cast<data_t>(1)); }

    static constexpr data_t lower_mask(const size_t n)noexcept{
        return (MASK_BIT_ << place(n)) - MASK_BIT_;
    }

    static constexpr entry_t entry_max()noexcept{ return MASK_ENTRY_; }
    
    static constexpr size_t size()noexcept{ return SIZE_; }
    
    constexpr data_t get_part(const size_t n)const noexcept{ return dat_ & mask(n); }
        
    constexpr data_t get_part(const size_t n, const size_t l)const noexcept{ return dat_ & (lower_mask(l) << place(n)); }
        
    constexpr data_t any(const size_t n)const noexcept{ return get_part(n); }

    entry_t at_by_bit_index(const size_t n)const noexcept{ // ベースのビット位置で取り出す
        return (dat_ >> n) & MASK_ENTRY_;
    }
    entry_t at(const size_t n)const noexcept{ // 境界チェックあり
        assert_index(n);
        return (dat_ >> place(n)) & MASK_ENTRY_;
    }
    entry_t at(const size_t n, const size_t l)const noexcept{ // 長さ|l|の分のデータを拾う
        assert_index(n); assert_index(n + l - 1);
        return (dat_ >> place(n)) & lower_mask(l);
    }
    entry_t at_flag(const size_t n, const size_t fn)const noexcept{ // インデックス|n|の|fn|ビット目の値を返す
        assert_index(n);
        return (dat_ >> (place(n) + fn)) & MASK_BIT_;
    }

    this_type_t& set(const size_t n, const entry_t entry)noexcept{
        assert_index(n);
        dat_ |= (entry << place(n));
        return *this;
    }
    this_type_t& replace(const size_t n, const entry_t entry)noexcept{
        assert_index(n);
        dat_ = (dat_ & (~mask(n))) | (entry << place(n));
        return *this;
    }
    this_type_t& replace(const size_t n, const entry_t entry, const size_t l)noexcept{ // 長さ|l|分のデータを入れ替え
        assert_index(n);
        data_t msk = lower_mask(l) << place(n);
        dat_ = (dat_ & (~msk)) | (entry << place(n));
        return *this;
    }
    this_type_t& clear(const size_t n)noexcept{
        assert_index(n);
        dat_ &= ~mask(n);
        return *this;
    }
    this_type_t& assign(const size_t n, const entry_t entry)noexcept{
        replace(n, entry);
        return *this;
    }
    this_type_t& assign(const size_t n, const entry_t entry, const size_t l)noexcept{
        replace(n, entry, l);
        return *this;
    }
    this_type_t& set_flag(const size_t n, const size_t fn)noexcept{
        assert_index(n);
        dat_ |= (MASK_BIT_ << (place(n) + fn));
        return *this;
    }
        
    this_type_t& set_by_bit_index(const size_t n, const entry_t entry)noexcept{
        dat_ |= (entry << n);
        return *this;
    }
    this_type_t& assign_by_bit_index(const size_t n, const entry_t entry)noexcept{
        dat_ = (dat_ & (~mask_by_bit_index(n))) | (entry << n) ;
        return *this;
    }
        
    // 0でない最小最大検索
    size_t min_index()const{
        return bsf(dat_) / N_;
    }
    size_t max_index()const{
        return bsr(dat_) / N_;
    }
        
    // 他のビット配列の同じ位置をコピー
    template<int N_A, int SIZE_A, typename other_atomic_data_t>
    this_type_t& set_part(const size_t n, const BitArrayInRegister<data_t, MAX_SIZE_, N_A, SIZE_A, other_atomic_data_t> arg)noexcept{
        assert_index(n);
        dat_ |= arg.get_part(n);
        return *this;
    }
    template<int N_A, int SIZE_A, typename other_atomic_data_t>
    this_type_t& replace_part(const size_t n, const BitArrayInRegister<data_t, MAX_SIZE_, N_A, SIZE_A, other_atomic_data_t> arg)noexcept{
        assert_index(n);
        dat_ = (dat_ & (~mask(n))) | (arg.data() & mask(n));
        return *this;
    }
    template<int N_A, int SIZE_A, typename other_atomic_data_t>
    this_type_t& assign_part(const size_t n, const BitArrayInRegister<data_t, MAX_SIZE_, N_A, SIZE_A, other_atomic_data_t> arg)noexcept{
        return replace_part<N_A, SIZE_A>(n, arg);
    }

    // 回転
    this_type_t& rotate_front(const size_t n)noexcept{
        assert_index(n);
        dat_ = ((dat_ & lower_mask(n)) << ((N_ * SIZE_) - place(n)))
            | ((dat_ & (~lower_mask(n))) >> place(n));
        return *this;
    }
    this_type_t& rotate_back(const size_t n)noexcept{
        return rotate_front(SIZE_ - n);
    }
    this_type_t& rotate(const size_t n)noexcept{
        return rotate_front(n);
    }

    // 加減算
    this_type_t& plus(const size_t n, const entry_t entry)noexcept{
        dat_ += (entry << place(n));
        return *this;
    }
    this_type_t& minus(const size_t n, const entry_t entry)noexcept{
        dat_ -= (entry << place(n));
        return *this;
    }
    
    this_type_t& add(const size_t n, const entry_t entry)noexcept{
        dat_ += (entry << place(n));
        return *this;
    }
    this_type_t& subtr(const size_t n, const entry_t entry)noexcept{
        dat_ -= (entry << place(n));
        return *this;
    }

    // データ押し込み
    this_type_t& insert_push_into_back(const size_t n, const entry_t entry)noexcept{
        assert_index(n);
        assert_entry(entry);
        dat_ = (dat_ & lower_mask(n))
            | ((dat_ & (~lower_mask(n))) << N_)
            | (entry << place(n));
        return *this;
    }
    this_type_t& insert_push_into_front(const size_t n, const entry_t entry)noexcept{
        assert_index(n);
        assert_entry(entry);
        dat_ = ((dat_ & lower_mask(n)) >> N_)
            | (dat_ & (~lower_mask(n)))
            | (entry << place(n));
        return *this;
    }

    this_type_t& insert(const size_t n, const entry_t entry)noexcept{
        return insert_push_into_back(n, entry);
    }

    // ある位置のものを抜いて、1つ詰める
    this_type_t& remove_front()noexcept{
        dat_ >>= N_; return *this;
    }
    this_type_t& remove_back()noexcept{
        dat_ &= (~mask(SIZE_ - 1)); return *this;
    }
    this_type_t& remove_pull_back(const size_t n)noexcept{
        assert_index(n);
        dat_ = (dat_ & mask(0, n - 1)) | ((dat_ >> N_) & (~mask(0, n - 1)));
        return *this;
    }
    this_type_t& remove_pull_front(const size_t n)noexcept{
        assert_index(n);
        dat_ = ((dat_ << N_) & mask(0, n - 1)) | (dat_ & (~mask(0, n - 1)));
        return *this;
    }

    this_type_t& remove(const size_t n)noexcept{ return remove_pull_back(n); }

    // スワップ
    this_type_t& swap(const size_t n0, const size_t n1)noexcept{
        dat_ = swapBits(dat_, place(n0), place(n1), N_);
        return *this;
    }

    entry_t sum()const noexcept;
    entry_t sumExcept(const size_t n)const noexcept{
        auto tmp = *this;
        tmp.replace(n, 0);
        return tmp.sum();
    }

private:
    
    atomic_data_t dat_;

    static constexpr data_t MASK_BIT_ = static_cast<data_t>(1);
    static constexpr data_t MASK_ENTRY_ = (MASK_BIT_ << N_) - MASK_BIT_;
    static constexpr size_t SIZE_ = cmax(0, cmin(ARG_SIZE_, MAX_SIZE_ / N_)); // 指定サイズがおかしい時は勝手に調整
    static constexpr size_t LOG2_SIZE_ =
    (SIZE_ <= 1) ? 0 :
    ((SIZE_ <= 2) ? 1 :
     ((SIZE_ <= 4) ? 2 :
      ((SIZE_ <= 8) ? 3 :
       ((SIZE_ <= 16) ? 4 :
        ((SIZE_ <= 32) ? 5 : 6
         )))));

    static constexpr size_t NWtoS(size_t n, size_t w)noexcept{ return (1 << w) * n; }
};

template<typename entry_t, typename data_t, int MAX_SIZE, int R, int N>
entry_t sumOfBitArrayInRegister(data_t ba)noexcept{
    if (R <= 0){ return ba; }
    constexpr int L = (1 << ((R > 0) ? (R - 1) : 0)) * N;
    data_t r = sumOfBitArrayInRegister<entry_t, data_t, MAX_SIZE, (R > 0) ? (R - 1) : 0, N>(ba);
    data_t msk = fillBits<data_t, L * 2>((static_cast<data_t>(1) << L) - static_cast<data_t>(1));
    return (r & msk) + ((r >> L) & msk);
}

template<typename data_t, int MAX_SIZE, int N, int SIZE, typename atomic_data_t>
typename BitArrayInRegister<data_t, MAX_SIZE, N, SIZE, atomic_data_t>::entry_t
BitArrayInRegister<data_t, MAX_SIZE, N, SIZE, atomic_data_t>::sum()const noexcept{
    return sumOfBitArrayInRegister<entry_t, data_t, MAX_SIZE, LOG2_SIZE_, N>(data());
}

template<typename data_t, int MAX_SIZE, int N, int SIZE, typename atomic_data_t>
BitArrayInRegister<data_t, MAX_SIZE, N, SIZE, atomic_data_t>
invert(const BitArrayInRegister<data_t, MAX_SIZE, N, SIZE, atomic_data_t> ba)noexcept{
    BitArrayInRegister<data_t, MAX_SIZE, N, SIZE> ret(0);
    for(int i = 0; i < SIZE; ++i){
        ret.set(ba[i], i);
    }
    return ret;
}

template<typename data_t, int MAX_SIZE, int N, int SIZE, typename atomic_data_t>
ostream& operator<<(ostream& out, const BitArrayInRegister<data_t, MAX_SIZE, N, SIZE, atomic_data_t>& arg){//出力
    out << "{";
    if (arg.size() >= 1){
        out << (uint64_t)(arg[0]);
        for (int i = 1, n = arg.size(); i < n; ++i){
            out << "," << (uint64_t)(arg[i]);
        }
    }
    out << "}";
    return out;
}

template<typename data_t, int MAX_SIZE, int N, int SIZE, typename atomic_data_t, typename callback_t>
void iterateAll(BitArrayInRegister<data_t, MAX_SIZE, N, SIZE, atomic_data_t> ba,
                const callback_t& callback)noexcept{ // 全て
    for(int i = 0;; ++i){
        callback(ba.front());
        if(i == SIZE - 1){ break; }
        ba.remove_front();
    }
}

template<typename data_t, int MAX_SIZE, int N, int SIZE, typename atomic_data_t, typename callback_t>
void iterateAny(BitArrayInRegister<data_t, MAX_SIZE, N, SIZE, atomic_data_t> ba,
                const callback_t& callback)noexcept{ // 以降0なら終了
    while(ba.any()){
        callback(ba.front());
        ba.remove_front();
    }
}

template<typename data_t, int MAX_SIZE, int N, int SIZE, typename atomic_data_t, typename callback_t>
void iterateAnyWithIndex(BitArrayInRegister<data_t, MAX_SIZE, N, SIZE, atomic_data_t> ba,
                         const callback_t& callback)noexcept{ // 以降0なら終了
    for(int i = 0;; ++i){
        callback(i, ba.front());
        ba.remove_front();
        if(!ba.any()){ break; }
    }
}

template<typename data_t, int MAX_SIZE, int N, int SIZE, typename atomic_data_t, typename callback_t>
void iterateAnyWithoutFirstCheck(BitArrayInRegister<data_t, MAX_SIZE, N, SIZE, atomic_data_t> ba,
                                 const callback_t& callback)noexcept{ // 以降0なら終了, 全0の判定なし
    do{
        callback(ba.front());
        ba.remove_front();
    }while(ba.any());
}
       
template<typename data_t, int MAX_SIZE, int N, int SIZE, typename atomic_data_t, typename callback_t>
void iterateOn(BitArrayInRegister<data_t, MAX_SIZE, N, SIZE, atomic_data_t> ba,
               const callback_t& callback)noexcept{ // 0でない箇所のみ
    constexpr data_t mask = BitArrayInRegister<data_t, MAX_SIZE, N, SIZE, atomic_data_t>::mask(0);
    while(ba.any()){
        unsigned int i = bsf(ba.data());
        unsigned int p = i / N;
        int n = ba[p];
        callback(p, n);
        ba.assign(p, 0);
    }
}
        
template<typename data_t, int MAX_SIZE, int N, int SIZE, typename atomic_data_t, typename callback_t>
int iterateOnWithCount(BitArrayInRegister<data_t, MAX_SIZE, N, SIZE, atomic_data_t> ba,
                        const callback_t& callback)noexcept{ // 0でない箇所のみ, カウントあり, カウント返す
    int cnt = 0;
    constexpr data_t mask = BitArrayInRegister<data_t, MAX_SIZE, N, SIZE, atomic_data_t>::mask(0);
    while(ba.any()){
        unsigned int i = bsf(ba.data());
        unsigned int p = i / N;
        int n = ba[p];
        callback(cnt, p, n);
        ba.assign(p, 0);
        cnt += 1;
    }
    return cnt;
}
        
template<typename data_t, int MAX_SIZE, int N, int SIZE, typename atomic_data_t, typename callback_t>
void iterateOnTimes(BitArrayInRegister<data_t, MAX_SIZE, N, SIZE, atomic_data_t> ba,
                    const callback_t& callback)noexcept{ // 0でない箇所のみ, 回数分イテレート
    constexpr data_t mask = BitArrayInRegister<data_t, MAX_SIZE, N, SIZE, atomic_data_t>::mask(0);
    while(ba.any()){
        unsigned int i = bsf(ba.data());
        unsigned int p = i / N;
        int n = ba[p];
        callback(p);
        for(int k = 1; k < n; ++k){
            callback(p);
        }
        ba.assign(p, 0);
    }
}
        
template<typename data_t, int MAX_SIZE, int N, int SIZE, typename atomic_data_t, typename callback_t>
int iterateOnTimesWithCount(BitArrayInRegister<data_t, MAX_SIZE, N, SIZE, atomic_data_t> ba,
                             const callback_t& callback)noexcept{ // 0でない箇所のみ, 回数分イテレート, カウントあり, カウント返す
    int cnt = 0;
    constexpr data_t mask = BitArrayInRegister<data_t, MAX_SIZE, N, SIZE, atomic_data_t>::mask(0);
    while(ba.any()){
        unsigned int i = bsf(ba.data());
        unsigned int p = i / N;
        int n = ba[p];
        callback(cnt, p);
        cnt += 1;
        for(int k = 1; k < n; ++k){
            callback(cnt, p);
            cnt += 1;
        }
        ba.assign(p, 0);
    }
    return cnt;
}
        
template<typename data_t, int MAX_SIZE, int N, int SIZE, typename atomic_data_t>
int countEqual(BitArrayInRegister<data_t, MAX_SIZE, N, SIZE, atomic_data_t> a,
            BitArrayInRegister<data_t, MAX_SIZE, N, SIZE, atomic_data_t> b,
            int s, int g)noexcept{ // |s|<=|i|<|g|内で同じ値の個数を返す
    int cnt = 0;
    for(int i = s; i < g; ++i){
        if(a[i] == b[i]){ ++cnt; }
    }
    return cnt;
}
template<typename data_t, int MAX_SIZE, int N, int SIZE, typename atomic_data_t, typename callback_t>
data_t mul(BitArrayInRegister<data_t, MAX_SIZE, N, SIZE, atomic_data_t> ba,
           int s, int g)noexcept{ // |s|<=|i|<|g|内の全ての値の積を返す
    uint32_t ret = 1;
    for(int i = s; i < g; ++i){
        ret *= ba[i];
    }
    return ret;
}


template<typename data_t, int MAX_SIZE, int N, int SIZE, typename atomic_data_t>
BitArrayInRegister<data_t, MAX_SIZE, N, SIZE, atomic_data_t>
splitHorizon(const BitArrayInRegister<data_t, MAX_SIZE, N, SIZE, atomic_data_t>& c, const size_t n)noexcept{ // 基準をずらす(不可逆)
    return BitArrayInRegister<data_t, MAX_SIZE, N, SIZE, atomic_data_t>(c.data() >> c.place(n));
}

template<int N = 2, int SIZE = 8 / N>
using BitArray8 = BitArrayInRegister<uint8_t, 8, N, SIZE>;

template<int N = 4, int SIZE = 16 / N>
using BitArray16 = BitArrayInRegister<uint16_t, 16, N, SIZE>;

template<int N = 4, int SIZE = 32 / N>
using BitArray32 = BitArrayInRegister<uint32_t, 32, N, SIZE>;

template<int N = 4, int SIZE = 64 / N>
using BitArray64 = BitArrayInRegister<uint64_t, 64, N, SIZE>;

#endif // UTIL_BITARRAY_HPP_