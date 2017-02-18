/*
 bitSet.hpp
 Katsuki Ohto
 */

#ifndef UTIL_BITSET_HPP_
#define UTIL_BITSET_HPP_

// ビットセットを定義
// サイズ固定
// std::bitsetより色々機能が多い

#include <cassert>
#include <string>
#include <iostream>
#include <sstream>

#include "../defines.h"
//#include "type.hpp"
#include "bitOperation.hpp"
#include "container.hpp"

#include "bitArray.hpp"

template<typename data_t, int MAX_SIZE_ = (sizeof(data_t) * 8), typename atomic_data_t = data_t>
class alignas(data_t) BitSetInRegister{
    
public:
    using size_t = std::size_t; // unsigned int;
    using entry_t = data_t;
    using this_t = BitSetInRegister<data_t, MAX_SIZE_, atomic_data_t>;
    
    // constructor
    constexpr BitSetInRegister() :dat_(){}
    constexpr BitSetInRegister(const data_t arg) : dat_(arg){}
    
    template<typename ... args_t>
    explicit constexpr BitSetInRegister(const size_t p0, args_t ... others)
    : dat_(mask(p0, others...)) {}
    
    
    // operator
    constexpr operator data_t()const noexcept{ return static_cast<data_t>(dat_); }
    
    /*constexpr this_t operator ~()const noexcept{ return this_t(~dat_); }
    
    constexpr this_t operator +(const data_t arg)const noexcept{ return this_t(dat_ + arg); }
    constexpr this_t operator -(const data_t arg)const noexcept{ return this_t(dat_ - arg); }
    constexpr this_t operator &(const data_t arg)const noexcept{ return this_t(dat_ & arg); }
    constexpr this_t operator |(const data_t arg)const noexcept{ return this_t(dat_ | arg); }
    constexpr this_t operator ^(const data_t arg)const noexcept{ return this_t(dat_ ^ arg); }*/
    
    this_t& operator=(const data_t arg)noexcept{ dat_ = arg; return *this; }
    this_t& operator+=(const data_t arg)noexcept{ dat_ += arg; return *this; }
    this_t& operator-=(const data_t arg)noexcept{ dat_ -= arg; return *this; }
    this_t& operator*=(const data_t arg)noexcept{ dat_ += arg; return *this; }
    this_t& operator/=(const data_t arg)noexcept{ dat_ -= arg; return *this; }
    this_t& operator|=(const data_t arg)noexcept{ dat_ |= arg; return *this; }
    this_t& operator&=(const data_t arg)noexcept{ dat_ &= arg; return *this; }
    this_t& operator^=(const data_t arg)noexcept{ dat_ ^= arg; return *this; }
    this_t& operator<<=(const size_t arg)noexcept{ dat_ <<= arg; return *this; }
    this_t& operator>>=(const size_t arg)noexcept{ dat_ >>= arg; return *this; }
        
    static void assert_index(const size_t p){
        ASSERT(0 <= p && p < MAX_SIZE_, std::cerr << p << " in " << MAX_SIZE_ << std::endl; );
    }
        
    constexpr static data_t mask(const size_t p)noexcept{ return (MASK_BIT_ << p); }
        
    template<typename ... args_t>
    constexpr static data_t mask(const size_t p0, args_t ... others) { // TODO:間のmask作成がmask_betweenに変わった為、もし前の使い方の関数があればバグる
        return mask(p0) | mask(others...);
    }
        
    constexpr static data_t lower_mask(const size_t p)noexcept{ return (MASK_BIT_ << p) - static_cast<data_t>(1); }
    constexpr static data_t mask_between(const size_t p0, const size_t p1)noexcept{ // |p0| ~ |p1|間のマスク
        return lower_mask(p1 + 1) & (~lower_mask(p0));
    }
    constexpr static data_t full_mask()noexcept{ return static_cast<data_t>(-1); }
    
        
    // 状態を変化させる関数
    this_t& set(const size_t p)noexcept{ // 位置|p|に1を立てる
        assert_index(p);
        dat_ |= mask(p);
        return *this;
    }
    this_t& flip(const size_t p)noexcept{ // 位置|p|を反転
        assert_index(p);
        dat_ ^= mask(p);
        return *this;
    }
    this_t& reset(const size_t p)noexcept{ // 位置|p|を0にする
        assert_index(p);
        dat_ &= ~mask(p);
        return *this;
    }
    this_t& set_value(const size_t p, const entry_t v)noexcept{ // 位置|p|の値を|v|にする(元々あった場合には消さない)
        assert_index(p);
        dat_ |= v << p;
        return *this;
    }
    this_t& assign(const size_t p, const entry_t v)noexcept{ // 位置|p|の値を|v|にする
        assert_index(p);
        dat_ = (dat_ & ~mask(p)) | (v << p);
        return *this;
    }
    
    this_t& push(const size_t p, const entry_t v)noexcept{ // 位置|p|に|v|を押し込む
        assert_index(p);
        dat_ = (dat_ & lower_mask(p)) | ((dat_ & (~lower_mask(p))) << 1) | mask(p);
        return *this;
    }
    this_t& push0(const size_t p)noexcept{ // 位置|p|に0を押し込む
        assert_index(p);
        dat_ = (dat_ & lower_mask(p)) | ((dat_ & (~lower_mask(p))) << 1);
        return *this;
    }
    this_t& push1(const size_t p)noexcept{ // 位置|p|に1を押し込む
        assert_index(p);
        dat_ = (dat_ & lower_mask(p)) | ((dat_ & (~lower_mask(p))) << 1) | mask(p);
        return *this;
    }
        
    this_t& remove(const size_t p)noexcept{ // 位置|p|から1つ抜いて寄せる
        assert_index(p);
        dat_ = (dat_ & lower_mask(p)) | ((dat_ >> 1) & (~lower_mask(p)));
        return *this;
    }
    //void remove_and_fill_by_msb(const size_t p)noexcept{
    //    assert_index(p);
    //    dat_ = (dat_ & lower_mask(p)) | ((static_cast<signed_type<data_t>::type>(dat_) >> 1) & (~lower_mask(p)));
    //}

    this_t& reset()noexcept{ // 全て0に
        dat_ = static_cast<data_t>(0); return *this;
    }
    this_t& flip()noexcept{ // 全反転
        dat_ = ~dat_; return *this;
    }

    void set(const size_t p0, const size_t p1)noexcept{
        dat_ |= mask(p0) | mask(p1);
    }
    void set(const size_t p0, const size_t p1, const size_t p2)noexcept{
        dat_ |= mask(p0) | mask(p1) | mask(p2);
    }
    void set(const size_t p0, const size_t p1, const size_t p2, const size_t p3)noexcept{
        dat_ |= mask(p0) | mask(p1) | mask(p2) | mask(p3);
    }
    void set(const size_t p0, const size_t p1, const size_t p2, const size_t p3, const size_t p4)noexcept{
        dat_ |= mask(p0) | mask(p1) | mask(p2) | mask(p3) | mask(p4);
    }
    void set(const size_t p0, const size_t p1, const size_t p2, const size_t p3, const size_t p4, const size_t p5)noexcept{
        dat_ |= mask(p0) | mask(p1) | mask(p2) | mask(p3) | mask(p4) | mask(p5);
    }

    this_t& fill()noexcept{ // 全て1
        dat_ = full_mask();
        return *this;
    }
    this_t& fill_through_back(const size_t p)noexcept{ // 最後まで全て1に
        assert_index(p);
        dat_ |= ~lower_mask(p);
        return *this;
    }
    this_t& fill(const size_t p0, const size_t p1)noexcept{ // 位置|p0|から|p1|まで(両端含む)の全てを1に
        assert_index(p0); assert_index(p1);
        dat_ |= lower_mask(p1 + 1) & (~lower_mask(p0));
        return *this;
    }
    this_t& reset_through_back(const size_t p)noexcept{ // 最後まで全て0に
        assert_index(p);
        dat_ &= lower_mask(p);
        return *this;
    }
        
    data_t any_through_back(const size_t p)const noexcept{ // 最後までにあるか
        assert_index(p);
        return dat_ & (~lower_mask(p));
    }
    int find_through_back(const size_t p)const noexcept{ // 最後までにあるか探す
        assert_index(p);
        data_t d = dat_ & (~lower_mask(p));
        if(d){
            return static_cast<this_t>(d).bsf();
        }
        return -1;
    }
        
    this_t& pop_lsb()noexcept{
        dat_ &= (dat_ - static_cast<data_t>(1)); return *this;
    }
        
    this_t& init()noexcept{
        dat_ = static_cast<data_t>(0); return *this;
    }

    // 判定する関数
    constexpr data_t test(const size_t p)const noexcept{
        return dat_ & mask(p);
    }
    constexpr entry_t get(const size_t p)const noexcept{
        return (dat_ >> p) & MASK_BIT_;
    }

    template<int NN, typename other_atomic_data_t>
    constexpr bool holds(const BitSetInRegister<data_t, NN, other_atomic_data_t>& arg)const noexcept{
        return holdsBits<data_t>(dat_, arg.dat_);
    }

    constexpr data_t holds(const size_t p0)const noexcept{ // 包括性
        return test(p0);
    }
    constexpr bool holds(const size_t p0, const size_t p1)const noexcept{
        return holdsBits<data_t>(dat_, mask(p0) | mask(p1));
    }
    constexpr bool holds(const size_t p0, const size_t p1, const size_t p2)const noexcept{
        return holdsBits<data_t>(dat_, mask(p0) | mask(p1) | mask(p2));
    }
    constexpr bool holds(const size_t p0, const size_t p1, const size_t p2, const size_t p3)const noexcept{
        return holdsBits<data_t>(dat_, mask(p0) | mask(p1) | mask(p2) | mask(p3));
    }
    constexpr bool holds(const size_t p0, const size_t p1, const size_t p2, const size_t p3, const size_t p4)const noexcept{
        return holdsBits<data_t>(dat_, mask(p0) | mask(p1) | mask(p2) | mask(p3) | mask(p4));
    }
    constexpr bool holds(const size_t p0, const size_t p1, const size_t p2, const size_t p3, const size_t p4, const size_t p5)const noexcept{
        return holdsBits<data_t>(dat_, mask(p0) | mask(p1) | mask(p2) | mask(p3) | mask(p4) | mask(p5));
    }

    constexpr bool is_only(const size_t p)const noexcept{ // |p|の位置のみ立っている状態
        return (dat_ == (static_cast<data_t>(1)<<p));
    }
    
    constexpr data_t any_except(const size_t p)const noexcept{ // |p|以外で立っているかどうか
        return dat_ & (~mask(p));
    }
    
    constexpr data_t any()const noexcept{ return dat_; }
    constexpr data_t any2()const noexcept{ return any2Bits(dat_); }
    constexpr bool none()const noexcept{ return (dat_ == 0); }
    constexpr bool empty()const noexcept{ return (dat_ == 0); }
    constexpr bool all()const noexcept{ return (dat_ == full_mask()); }
    
    size_t count()const noexcept{ return countBits<data_t>(dat_); }
    size_t bsf()const noexcept{ return ::bsf<data_t>(dat_); }
    size_t bsr()const noexcept{ return ::bsr<data_t>(dat_); }

    constexpr data_t data()const noexcept{ return dat_; }
        
    // 配列化。ビット数やデータ数がオーバーしたときの動作は未定義
    template<int N, int SIZE = 64 / N>
    BitArray64<N, SIZE> to_array64()const{
        BitArray64<N, SIZE> ret(0);
        auto bs = *this;
        while (bs.any()){
            ret.insert(0, bs.bsf());
            bs.pop_lsb();
        }
        return ret;
    }
    template<int N, int SIZE = 32 / N>
    BitArray32<N, SIZE> to_array32()const{
        BitArray32<N, SIZE> ret(0);
        auto bs = *this;
        while (bs.any()){
            ret.insert(0, bs.bsf());
            bs.pop_lsb();
        }
        return ret;
    }
        
        
    std::string to_set_string()const{
        std::ostringstream oss;
        this_t tmp = data();
        oss << "{";
        while(tmp.any()){
            size_t i = tmp.bsf();
            oss << i;
            tmp.pop_lsb();
            if(tmp.any()){
                oss << ",";
            }
        }
        oss << "}";
        return oss.str();
    }

private:
    constexpr static data_t MASK_BIT_ = static_cast<data_t>(1);
    
    atomic_data_t dat_;
};
    
// operators
    
template<typename data_t, int MAX_SIZE>
BitSetInRegister<data_t, MAX_SIZE> operator~(const BitSetInRegister<data_t, MAX_SIZE>& rhs)noexcept{
    return BitSetInRegister<data_t, MAX_SIZE>(~rhs.data());
}
/*template<typename data_t, int MAX_SIZE>
BitSetInRegister<data_t, MAX_SIZE> operator|(const BitSetInRegister<data_t, MAX_SIZE>& rhs)noexcept{
    return BitSetInRegister<data_t, MAX_SIZE>(rhs.data());
}
template<typename data_t, int MAX_SIZE>
BitSetInRegister<data_t, MAX_SIZE> operator&(const BitSetInRegister<data_t, MAX_SIZE>& rhs)noexcept{
    return BitSetInRegister<data_t, MAX_SIZE>(~rhs.data());
}
template<typename data_t, int MAX_SIZE>
    BitSetInRegister<data_t, MAX_SIZE> operator^(const BitSetInRegister<data_t, MAX_SIZE>& rhs)noexcept{
    return BitSetInRegister<data_t, MAX_SIZE>(~rhs.data());
}*/

template<typename data_t, int MAX_SIZE>
std::ostream& operator<<(std::ostream& out, const BitSetInRegister<data_t, MAX_SIZE>& arg){ // output
    out << "[";
    for (int i = 0; i < MAX_SIZE; ++i){
        out << int(arg.get(i));
    }
    out << "]";
    return out;
}

template<typename data_t, int MAX_SIZE, typename callback_t>
void iterate(BitSetInRegister<data_t, MAX_SIZE> bs, const callback_t& callback)noexcept{
    while (bs.any()){
        callback(bs.bsf());
        bs.pop_lsb();
    }
}


template<typename data_t, int MAX_SIZE, typename callback_t>
void iterateWithIndex(BitSetInRegister<data_t, MAX_SIZE> bs, const callback_t& callback)noexcept{ // カウント付き TODO : with countの方がいいかも
    int cnt = 0;
    while (bs.any()){
        callback(cnt, bs.bsf());
        bs.pop_lsb();
        ++cnt;
    }
}

template<typename data_t, int MAX_SIZE, typename callback_t>
void iterateWithoutFirstCheck(BitSetInRegister<data_t, MAX_SIZE> bs, const callback_t& callback)noexcept{ // any()チェックなし
    do{
        callback(bs.bsf());
        bs.pop_lsb();
    } while (bs.any());
}

template<typename data_t, int MAX_SIZE, typename callback_t>
void iterateExcept(BitSetInRegister<data_t,MAX_SIZE> bs, const std::size_t p, const callback_t& callback)noexcept{ // |p|以外
    BitSetInRegister<data_t, MAX_SIZE>::assert_index(p);
    bs.reset(p);
    while (bs.any()){
        callback(bs.bsf());
        bs.pop_lsb();
    }
}

template<typename data_t, int MAX_SIZE, typename callback_t>
void iterateWithoutFirstCheckExcept(BitSetInRegister<data_t, MAX_SIZE> bs, const std::size_t p, const callback_t& callback)noexcept{ // |p|以外, any()チェックなし
    BitSetInRegister<data_t, MAX_SIZE>::assert_index(p);
    bs.reset(p);
    do{
        callback(bs.bsf());
        bs.pop_lsb();
    } while (bs.any());
}

template<typename data_t, int MAX_SIZE, typename callback_t>
int search(BitSetInRegister<data_t, MAX_SIZE> bs, const callback_t& callback)noexcept{ // 発見でインデックスを返し無ければ-1を返す
    while(bs.any()){
        const std::size_t idx = bs.bsf();
        if (callback(idx)){ return idx; }
        bs.pop_lsb();
    }
    return -1;
}

using BitSet8 = BitSetInRegister<uint8_t>;
using BitSet16 = BitSetInRegister<uint16_t>;
using BitSet32 = BitSetInRegister<uint32_t>;
using BitSet64 = BitSetInRegister<uint64_t>;

#endif // UTIL_BITSET_HPP_