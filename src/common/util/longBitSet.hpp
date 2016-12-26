/*
 longBitSet.hpp
 Katsuki Ohto
 */

#ifndef UTIL_LONGBITSET_HPP_
#define UTIL_LONGBITSET_HPP_

// 64ビットレジスタに収まらないビットセット

#include <cassert>
#include <string>
#include <iostream>
#include <sstream>

#include "../defines.h"
//#include "type.hpp"
#include "bitOperation.hpp"
#include "container.hpp"

#include "bitArray.hpp"
#include "bitSet.hpp"

template<int N>
class LongBitSet{
private:
    using size_t = std::size_t;
    using entry_t = bool;
    using data_t = std::uint64_t;
    
    static constexpr size_t INT_SIZE = 64;
    static constexpr size_t SIZE = (N + INT_SIZE - 1) / INT_SIZE;
    
    static void assert_index(const size_t p){
        ASSERT(0 <= p && p < N, std::cerr << p << " in " << N << std::endl; );
    }
    
public:
    
    static constexpr size_t int_size()noexcept{ return INT_SIZE; }
    
    LongBitSet& set(size_t index){
        assert_index(index);
        size_t dataIndex = index / INT_SIZE;
        size_t bitIndex = index % INT_SIZE;
        bits_[dataIndex].set(bitIndex);
        return *this;
    }
    
    LongBitSet& flip(size_t index){
        assert_index(index);
        size_t dataIndex = index / INT_SIZE;
        size_t bitIndex = index % INT_SIZE;
        bits_[dataIndex].flip(bitIndex);
        return *this;
    }
    
    LongBitSet& reset(size_t index){
        assert_index(index);
        size_t dataIndex = index / INT_SIZE;
        size_t bitIndex = index % INT_SIZE;
        bits_[dataIndex].reset(bitIndex);
        return *this;
    }
    
    LongBitSet& reset()noexcept{
        bits_.fill(0);
        return *this;
    }
    
    LongBitSet& flip()noexcept{ // 全反転
        for(auto& b : bits_){
            b.flip();
        }
        return *this;
    }
    
    LongBitSet& fill()noexcept{
        bits_.fill(-1);
        return *this;
    }
    
    LongBitSet& fill_through_back(const size_t index){
        assert_index(index);
        size_t dataIndex = index / INT_SIZE;
        size_t bitIndex = index % INT_SIZE;
        bits_[dataIndex].fill_through_back(bitIndex);
        for(size_t i = dataIndex + 1; i < SIZE; ++i){
            bits_[i].fill();
        }
        return *this;
    }
    
    LongBitSet& reset_through_back(const size_t index){
        assert_index(index);
        size_t dataIndex = index / INT_SIZE;
        size_t bitIndex = index % INT_SIZE;
        bits_[dataIndex].reset_through_back(bitIndex);
        for(size_t i = dataIndex + 1; i < SIZE; ++i){
            bits_[i].reset();
        }
        return *this;
    }
    
    bool any_through_back(const size_t index)const{
        assert_index(index);
        size_t dataIndex = index / INT_SIZE;
        size_t bitIndex = index % INT_SIZE;
        if(bits_[dataIndex].any_through_back(bitIndex)){
            return true;
        }
        for(size_t i = dataIndex + 1; i < SIZE; ++i){
            if(bits_[i].any()){
                return true;
            }
        }
        return false;
    }
    
    int find_through_back(const size_t index)const{
        assert_index(index);
        size_t dataIndex = index / INT_SIZE;
        size_t bitIndex = index % INT_SIZE;
        int p = bits_[dataIndex].find_through_back(bitIndex);
        size_t baseIndex = dataIndex * INT_SIZE;
        if(p >= 0){
            return p + baseIndex;
        }
        for(size_t i = dataIndex + 1; i < SIZE; ++i){
            baseIndex += INT_SIZE;
            if(bits_[i].any()){
                return bits_[i].bsf() + baseIndex;
            }
        }
        return -1;
    }
    
    constexpr data_t test(const size_t index)const noexcept{
        assert_index(index);
        size_t dataIndex = index / INT_SIZE;
        size_t bitIndex = index % INT_SIZE;
        return bits_[dataIndex].test(bitIndex);
    }
    constexpr entry_t get(const size_t index)const noexcept{
        assert_index(index);
        size_t dataIndex = index / INT_SIZE;
        size_t bitIndex = index % INT_SIZE;
        return bits_[dataIndex].get(bitIndex);
    }
    
    size_t count()const noexcept{
        size_t ret = 0;
        for(BitSet64 bs : data()){
            ret += bs.count();
        }
        return ret;
    }
    
    bool any()const noexcept{
        for(BitSet64 bs : data()){
            if(bs.any()){
                return true;
            }
        }
        return false;
    }
    
    template<int MODE = 0>
    bool equals(const LongBitSet& rhs)const{
        return bits_ == rhs.bits_;
    }
    
    bool operator==(const LongBitSet& rhs)const noexcept{
        return equals<0>(rhs);
    }
    bool operator!=(const LongBitSet& rhs)const noexcept{
        return !((*this) == rhs);
    }
    
    std::array<BitSet64, SIZE>& data()noexcept{
        return bits_;
    }
    const std::array<BitSet64, SIZE>& data()const noexcept{
        return bits_;
    }
    
private:
    std::array<BitSet64, SIZE> bits_; // 端数の分も含める
};

template<int N, class callback_t>
void iterate(const LongBitSet<N>& lbs, const callback_t& callback){
    size_t baseIndex = 0;
    for(BitSet64 bs : lbs.data()){
        iterate(bs, [baseIndex, &callback](size_t index)->void{
            callback(baseIndex + index);
        });
        baseIndex += LongBitSet<N>::int_size();
    }
}


#endif // UTIL_LONGBITSET_HPP_