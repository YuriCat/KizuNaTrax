/*
 bitOperation.hpp
 Katsuki Ohto
 */

#ifndef UTIL_BIT_OPERATION_HPP_
#define UTIL_BIT_OPERATION_HPP_

#include <cassert>

#include "../defines.h"

// ビット演算ユーティリティ
// 環境（プロセッサ、コンパイラ）依存する命令を使うものと、使わないものをいずれも用意したいところだが
// 現在はSSE4.?以降かつgccのみ対応

/**************************基本演算**************************/

// 包括性
template<typename T>
static inline constexpr bool holdsBits(const T a, const T b)noexcept{
    return (((~a) & b) == T(0));
}

// 排他性
template<typename T>
static inline constexpr bool isExclusive(const T a, const T b)noexcept{
    return ((a & b) == T(0));
}

/**************************ビット数判定**************************/

template<class T>
static constexpr T any2Bits(const T a)noexcept{
    return a & (a - static_cast<T>(1));
}

/**************************ビット数計算**************************/

// 環境依存なし

//畳み込み
static inline uint32_t countManyBits64(uint64_t a)noexcept{
    a = (a & 0x5555555555555555) + ((a >> 1) & 0x5555555555555555);
    a = (a & 0x3333333333333333) + ((a >> 2) & 0x3333333333333333);
    a = (a & 0x0f0f0f0f0f0f0f0f) + ((a >> 4) & 0x0f0f0f0f0f0f0f0f);
    a = (a & 0x00ff00ff00ff00ff) + ((a >> 8) & 0x00ff00ff00ff00ff);
    a = (a & 0x0000ffff0000ffff) + ((a >> 16) & 0x0000ffff0000ffff);
    return (int)((a & 0x00000000ffffffff) + ((a >> 32) & 0x00000000ffffffff));
}

static inline uint32_t countManyBits32(uint32_t a)noexcept{
    a = (a & 0x55555555) + ((a >> 1) & 0x55555555);
    a = (a & 0x33333333) + ((a >> 2) & 0x33333333);
    a = (a & 0x0f0f0f0f) + ((a >> 4) & 0x0f0f0f0f);
    a = (a & 0x00ff00ff) + ((a >> 8) & 0x00ff00ff);
    return (int)((a & 0x0000ffff) + ((a >> 16) & 0x0000ffff));
}

// 1ビットずつ数える
static inline constexpr uint32_t countFewBits32(uint32_t a)noexcept{
    return a ? (1 + countFewBits32(a & (a - 1))) : 0;
    /*int count = 0;
    for (; a; a &= (a - 1)){ ++count; }
    return count;*/
}

static inline constexpr uint32_t countFewBits64(uint64_t a)noexcept{
    return a ? (1 + countFewBits64(a & (a - 1))) : 0;
    /*int count = 0;
    for (; a; a &= (a - 1)){ ++count; }
    return count;*/
}

// 環境依存あり

#if defined(_MSC_VER)

static inline uint32_t countBits32(uint32_t a)noexcept{
    return __popcnt(a);
}
static inline uint32_t countBits64(uint64_t a)noexcept{
    return __popcnt64(a);
}

#elif defined(__GNUC__) && ( defined(__i386__) || defined(__x86_64__) )

static inline uint32_t countBits32(uint32_t a)noexcept{
    return __builtin_popcount(a);
}
static inline uint32_t countBits64(uint64_t a)noexcept{
    return __builtin_popcountll(a);
}

#else

static inline uint32_t countBits32(uint32_t a)noexcept{
    return countManyBits32(a);
}
static inline uint32_t countBits64(uint64_t a)noexcept{
    return countManyBits64(a);
}

#endif

template<typename T>inline uint32_t countBits(T a)noexcept;

template<>inline uint32_t countBits<uint8_t>(uint8_t a)noexcept{ return countBits32(a); }
template<>inline uint32_t countBits<uint16_t>(uint16_t a)noexcept{ return countBits32(a); }
template<>inline uint32_t countBits<uint32_t>(uint32_t a)noexcept{ return countBits32(a); }
template<>inline uint32_t countBits<uint64_t>(uint64_t a)noexcept{ return countBits64(a); }


/**************************4ビットごと演算**************************/

// 部分ごとのビット数
template<typename T>static T countAll4Bits(T a)noexcept; // 4ビットごとのビット数

template<>uint64_t countAll4Bits(uint64_t a)noexcept{
    a = (a & 0x5555555555555555) + ((a >> 1) & 0x5555555555555555);
    return (a & 0x3333333333333333) + ((a >> 2) & 0x3333333333333333);
}

// 部分ごとのビット存在
template<typename T>static T gatherAll4Bits(T a)noexcept; // 4ビットごとのビット存在

template<>uint64_t gatherAll4Bits(uint64_t a)noexcept{
    a = a | (a >> 1);
    return (a | (a >> 2)) & 0x1111111111111111;
}

// 部分ごとの全ビット存在
template<typename T>static T andAll4Bits(T a)noexcept; // 4ビットごとのビット存在

template<>uint64_t andAll4Bits(uint64_t a)noexcept{
    a = a & (a >> 1);
    return a & (a >> 2) & 0x1111111111111111;
}

// 部分ごとに存在するビットを埋め尽くす
template<typename T>static T fillAll4Bits(T a)noexcept; // 4ビットごとの埋め尽くし

template<>uint64_t fillAll4Bits(uint64_t a)noexcept{
    a = ((a & 0x5555555555555555) << 1) | ((a >> 1) & 0x5555555555555555);
    a = ((a & 0x3333333333333333) << 2) | ((a >> 2) & 0x3333333333333333);
    assert(((uint32_t)countBits(a)) % 4 == 0);
    return a;
}

// 部分ごとに存在するビット枚数に対応した位置にのみビットを立てる
template<typename T>static T rankAll4Bits(const T& arg)noexcept;

template<>uint64_t rankAll4Bits(const uint64_t& arg)noexcept{
    // 2ビットごとの枚数を計算
    uint64_t a = (arg & 0x5555555555555555) + ((arg >> 1) & 0x5555555555555555);
    // 4ビットあったところを3に配置
    uint64_t r = (a & 0x8888888888888888) & ((a << 2) & 0x8888888888888888);
    // 3ビットあったところを2に配置
    uint64_t r3 = ((a << 2) & 0x4444444444444444) & ((a >> 1) & 0x4444444444444444);
    r3 |= ((a << 1) & 0x4444444444444444) & (a & 0x4444444444444444);

    // 残りは足すだけ。ただし3,4ビットがすでにあったところにはビットを置かない。
    uint64_t r12 = (((a & 0x3333333333333333) + ((a >> 2) & 0x3333333333333333))) & 0x3333333333333333;
    if (r3){
        r |= r3;
        r |= (~((r3 >> 1) | (r3 >> 2))) & r12;
    }
    else{
        r |= r12;
    }

    assert(countBits(r) == countBits(gatherAll4Bits(arg)));

    return r;
}

/**************************ビット位置**************************/

// 名前と処理の関係がややこしいので注意
// ntz = ctz = bsf 最も下位のビットが下から何番目か
// clz 最も上位のビットが上から何番目か
// bsr 最も上位のビットが下から何番目か

// 環境依存無し
constexpr int NTZ64Table[64] = {
    0, 1, 59, 2, 60, 40, 54, 3,
    61, 32, 49, 41, 55, 19, 35, 4,
    62, 52, 30, 33, 50, 12, 14, 42,
    56, 16, 27, 20, 36, 23, 44, 5,
    63, 58, 39, 53, 31, 48, 18, 34,
    51, 29, 11, 13, 15, 26, 22, 43,
    57, 38, 47, 17, 28, 10, 25, 21,
    37, 46, 9, 24, 45, 8, 7, 6,
};

static constexpr inline int ntz64(const uint64_t a)noexcept{
    return NTZ64Table[(int)(((a & (-a)) * 0x03F566ED27179461ULL) >> 58)];
}


//環境依存あり

#if defined(_MSC_VER)

static inline int bsf32(uint32_t a)noexcept{
    unsigned long i;
    _BitScanForward(&i, a);
    return i;
}

static inline int bsf64(uint64_t a)noexcept{
    unsigned long i;
    _BitScanForward64(&i, a);
    return i;
}

static inline int bsr32(uint32_t a)noexcept{
    unsigned long i;
    _BitScanReverse(&i, a);
    return i;
}

static inline int bsr64(uint64_t a)noexcept{
    unsigned long i;
    _BitScanReverse64(&i, a);
    return i;
}

static inline int ctz32(uint32_t a)noexcept{
    return bsf32(a);
}

static inline int ctz64(uint64_t a)noexcept{
    return bsf64(a);
}

static inline int clz32(uint32_t a)noexcept{
    return 31 - bsr32(a);
}

static inline int clz64(uint64_t a)noexcept{
    return 63 - bsr64(a);
}

#elif defined(__GNUC__) && ( defined(__i386__) || defined(__x86_64__) )

static inline int bsf32(uint32_t a)noexcept{
    return __builtin_ctz(a);
}

static inline int bsf64(uint64_t a)noexcept{
    return __builtin_ctzll(a);
}

static inline int bsr32(uint32_t a)noexcept{
    int r;
    __asm__("bsrl %1, %0;" :"=r"(r) : "r"(a));
    return r;
}

static inline int bsr64(uint64_t a)noexcept{
    int64_t r;
    __asm__("bsrq %1, %0;" :"=r"(r) : "r"(a));
    return (int)r;
}

static inline int ctz32(uint32_t a)noexcept{
    return __builtin_ctzl(a);
}

static inline int ctz64(uint64_t a)noexcept{
    return __builtin_ctzll(a);
}

static inline int clz32(uint32_t a)noexcept{
    return __builtin_clzl(a);
}

static inline int clz64(uint64_t a)noexcept{
    return __builtin_clzll(a);
}

#else

static_assert(0, "no bsf-bsr.");

#endif

template<typename T>inline uint32_t bsf(T a)noexcept;
template<typename T>inline uint32_t bsr(T a)noexcept;

template<>inline uint32_t bsf<uint8_t>(uint8_t a)noexcept{ return bsf32(a); }
template<>inline uint32_t bsf<uint16_t>(uint16_t a)noexcept{ return bsf32(a); }
template<>inline uint32_t bsf<uint32_t>(uint32_t a)noexcept{ return bsf32(a); }
template<>inline uint32_t bsf<uint64_t>(uint64_t a)noexcept{ return bsf64(a); }
template<>inline uint32_t bsr<uint8_t>(uint8_t a)noexcept{ return bsr32(a); }
template<>inline uint32_t bsr<uint16_t>(uint16_t a)noexcept{ return bsr32(a); }
template<>inline uint32_t bsr<uint32_t>(uint32_t a)noexcept{ return bsr32(a); }
template<>inline uint32_t bsr<uint64_t>(uint64_t a)noexcept{ return bsr64(a); }

/**************************ビット取り出し、性質ビット抽出**************************/

// 上位ビットの取り出し

template<typename T>
static inline T highestBit(const T a)noexcept;

template<>inline int32_t highestBit(const int32_t a)noexcept{
    return 1 << bsr32(a);
}
template<>inline uint32_t highestBit(const uint32_t a)noexcept{
    return 1U << bsr32(a);
}
template<>inline int64_t highestBit(const int64_t a)noexcept{
    return (int64_t)(1ULL << bsr64(a)); // 論理シフト
}
template<>inline uint64_t highestBit(const uint64_t a)noexcept{
    return 1ULL << bsr64(a);
}

// 下位ビット取り出し
template<typename T>
constexpr inline T lowestBit(const T a)noexcept{
    return (a & (-a));
}

template<typename T>
constexpr static T allLowerBits(T a)noexcept{
    // 最下位ビットより下位のビット全て
    return ((~a) & (a - static_cast<T>(1)));
}

template<typename T>
constexpr T allLowerBitsThan1Bit64(T a)noexcept{
    return a - static_cast<T>(1);
}

template<typename T>
static inline T allHigherBits(T a)noexcept;

template<>
inline uint64_t allHigherBits<uint64_t>(uint64_t a)noexcept{
    return ~((1ULL << bsr64(a)) - 1ULL);
}

template<typename T>
constexpr static T allHigherBitsThan1Bit64(T a)noexcept{
    return (~((a - static_cast<T>(1)) << 1));
}

static inline uint32_t msb32(const uint32_t a)noexcept{ return 1U << bsr32(a); }
static inline uint64_t msb64(const uint64_t a)noexcept{ return 1ULL << bsr64(a); }
static inline uint32_t lsb32(const uint32_t a)noexcept{ return (a & (-a)); }
static inline uint64_t lsb64(const uint64_t a)noexcept{ return (a & (-a)); }

static inline uint32_t msb(const uint32_t a)noexcept{ return msb32(a); }
static inline uint64_t msb(const uint64_t a)noexcept{ return msb64(a); }
static inline uint32_t lsb(const uint32_t a)noexcept{ return lsb32(a); }
static inline uint64_t lsb(const uint64_t a)noexcept{ return lsb64(a); }

/**************************ビットクロス**************************/

template<int N>static uint64_t genCrossNumber(const int p)noexcept;
    
template<int N>
static uint64_t genCrossNumber(const int p)noexcept{

    assert(p >= 0 && p < N);

    uint64_t r;
    switch (N){
        case 0: UNREACHABLE; break;
        case 1: r = 0xFFFFFFFFFFFFFFFF << p; break;
        case 2: r = 0x5555555555555555 << p; break;
        case 3: r = 0x9249249249249249 << p; break;
        case 4: r = 0x1111111111111111 << p; break;
        case 5: r = 0x1084210842108421 << p; break;
        case 6: r = 0x4141414141414141 << p; break;
        case 7: r = 0x8102040810204081 << p; break;
        case 8: r = 0x0101010101010101 << p; break;
        default: UNREACHABLE; break;
    }
    return r;
}

constexpr static inline uint64_t crossBits64(const uint64_t a, const uint64_t b)noexcept{
    return (a & 0x5555555555555555) | (b & 0xAAAAAAAAAAAAAAAA);
}

constexpr static inline uint64_t crossBits64(const uint64_t a, const uint64_t b, const uint64_t c)noexcept{
    return (a & 0x9249249249249249) | (b & 0x2492492492492492) | (c & 0x4924924924924924);
}

constexpr static inline uint64_t crossBits64(const uint64_t a, const uint64_t b, const uint64_t c, const uint64_t d)noexcept{
    return (a & 0x1111111111111111) | (b & 0x2222222222222222) | (c & 0x4444444444444444) | (d & 0x8888888888888888);
}

constexpr static inline  uint64_t crossBits64(const uint64_t a, const uint64_t b, const uint64_t c, const uint64_t d, const uint64_t e)noexcept{
    return (a & 0x1084210842108421) | (b & 0x2108421084210842) | (c & 0x4210842108421084) | (d & 0x8421084210842108) | (e & 0x0842108421084210);
}

template<int N>static uint64_t crossBits64(const uint64_t a[]);

template<>uint64_t crossBits64<2>(const uint64_t a[]){
    return crossBits64(a[0], a[1]);
}

template<>uint64_t crossBits64<3>(const uint64_t a[]){
    return crossBits64(a[0], a[1], a[2]);
}

template<>uint64_t crossBits64<4>(const uint64_t a[]){
    return crossBits64(a[0], a[1], a[2], a[3]);
}

template<>uint64_t crossBits64<5>(const uint64_t a[]){
    return crossBits64(a[0], a[1], a[2], a[3], a[4]);
}

/**************************ビットマスク**************************/

template<typename T, int W, int MAX_SIZE = sizeof(T) * 8>T fillBits(T a)noexcept;
template<typename T, int W, int MAX_SIZE = sizeof(T) * 8>T fillBits_sub(T a, int n)noexcept;

template<typename T, int W, int MAX_SIZE>
T fillBits(T a)noexcept{
    return
        (W <= 0) ? static_cast<T>(0) :
        ((W >= MAX_SIZE) ? a :
        (((fillBits_sub<T, W, MAX_SIZE>(a, (MAX_SIZE - 1) / W)) << W) | a));
}

template<typename T, int W, int MAX_SIZE>
T fillBits_sub(T a, int n)noexcept{
    return
        (n <= 0) ? a :
        ((W <= 0) ? static_cast<T>(0) :
        ((W >= MAX_SIZE) ? a :
        ((fillBits_sub<T, W, MAX_SIZE>(a, n - 1) << W) | a)));
}

template<int W>
uint32_t fillBits32(uint32_t a)noexcept{
    return fillBits<uint32_t, W>(a);
}
template<int W>
uint64_t fillBits64(uint64_t a)noexcept{
    return fillBits<uint64_t, W>(a);
}

/**************************ビットを利用した基本演算**************************/

// 対数
template<typename T>static uint32_t log2i(T a)noexcept;

template<>uint32_t log2i(uint32_t a)noexcept{
    return (uint32_t)bsr32(a);
}
template<>uint32_t log2i(uint64_t a)noexcept{
    return (uint32_t)bsr64(a);
}

// 2の累乗に切り上げ
static inline uint32_t roundUpPow2(uint32_t v)noexcept{ return msb(v - 1) << 1; } // 2の累乗に切り上げ
static inline uint64_t roundUpPow2(uint64_t v)noexcept{ return msb(v - 1) << 1; } // 2の累乗に切り上げ

/**************************ビットによる組み合わせ表現**************************/

// bit permutation。nビット立っているものを列挙する
static inline uint32_t getNextBitPermutation(uint32_t x)noexcept{
    int t = x | (x - 1);
    return (t + 1) | (uint32_t)((~t & -~t) - 1) >> (bsf(x) + 1);
}

// bit permutation。nビット立っているものを列挙する
static inline uint64_t getNextBitPermutation(uint64_t x)noexcept{
    uint64_t t = x | (x - 1);
    return (t + 1) | (uint64_t)((~t & -~t) - 1) >> (bsf(x) + 1);
}

/**************************ビット入れ替え**************************/

template<class T>
T swapBits(T x, std::size_t p1, std::size_t p2, std::size_t len)noexcept{
    // x のp1からlen bitとp2からlen bitを入れ替え
    T mask = (static_cast<T>(1) << len) - 1;
    T ope = (x >> p1 ^ x >> p2) & mask;
    return x ^ (ope << p1 | ope << p2);
}

/**************************ビット逆順化**************************/

static inline uint32_t reverseBits32(uint32_t v)noexcept{
    v = (v >> 16) | (v << 16);
    v = ((v >> 8) & 0x00ff00ff) | ((v & 0x00ff00ff) << 8);
    v = ((v >> 4) & 0x0f0f0f0f) | ((v & 0x0f0f0f0f) << 4);
    v = ((v >> 2) & 0x33333333) | ((v & 0x33333333) << 2);
    return ((v >> 1) & 0x55555555) | ((v & 0x55555555) << 1);
}

static inline uint64_t reverseBits64(uint64_t v)noexcept{
    v = (v >> 32) | (v << 32);
    v = ((v >> 16) & 0x0000ffff0000ffff) | ((v & 0x0000ffff0000ffff) << 16);
    v = ((v >> 8) & 0x00ff00ff00ff00ff) | ((v & 0x00ff00ff00ff00ff) << 8);
    v = ((v >> 4) & 0x0f0f0f0f0f0f0f0f) | ((v & 0x0f0f0f0f0f0f0f0f) << 4);
    v = ((v >> 2) & 0x3333333333333333) | ((v & 0x3333333333333333) << 2);
    return  ((v >> 1) & 0x5555555555555555) | ((v & 0x5555555555555555) << 1);
}

static inline uint32_t reverseBits(uint32_t v)noexcept{ return reverseBits32(v); }
static inline uint64_t reverseBits(uint64_t v)noexcept{ return reverseBits64(v); }

/**************************PEXT,PDEP**************************/

#if defined (HAVE_BMI2)
static inline uint32_t pext32BMI2(uint32_t a, uint32_t msk)noexcept{
    return _pext_u32(a, msk);
}

static inline uint64_t pext64BMI2(uint64_t a, uint64_t msk)noexcept{
    return _pext_u64(a, msk);
}

static inline uint32_t pdep32BMI2(uint32_t a, uint32_t msk)noexcept{
    return _pdep_u32(a, msk);
}

static inline uint64_t pdep64BMI2(uint64_t a, uint64_t msk)noexcept{
    return _pdep_u64(a, msk);
}
#endif

static inline uint64_t pextNoBMI2(uint64_t bits, uint64_t mask)noexcept{
    uint64_t ans;
    if(mask){
        int a = bsf(mask);
        ans = (bits >> a) & 1;
        uint64_t tmask = mask & (mask - 1ULL);
        if(tmask){
            int cnt = 1;
            while(1){
                int a = bsf(tmask);
                ans |= ((bits >> a) & 1) << cnt;
                tmask &= (tmask - 1ULL);
                if(!tmask)break;
                ++cnt;
            }
        }
    }else{
        ans = 0;
    }
    return ans;
}

static inline uint64_t pdepNoBMI2(uint64_t bits, uint64_t mask)noexcept{
    uint64_t ans;
    if(mask){
        int a = bsf(mask);
        ans = ((bits & 1ULL) << a);
        uint64_t tmask = mask & (mask - 1ULL);
        uint64_t tbits = bits >> 1;
        while(tmask){
            int a = bsf(tmask);
            ans |= ((tbits & 1ULL) << a);
            tmask &= (tmask - 1ULL);
            tbits >>= 1;
        }
    }else{
        ans = 0;
    }
    return ans;
}

#if defined (HAVE_BMI2)
static inline uint32_t pext(uint32_t a, uint32_t msk)noexcept{ return pext32BMI2(a, msk); }
static inline uint64_t pext(uint64_t a, uint64_t msk)noexcept{ return pext64BMI2(a, msk); }
static inline uint32_t pdep(uint32_t a, uint32_t msk)noexcept{ return pdep32BMI2(a, msk); }
static inline uint64_t pdep(uint64_t a, uint64_t msk)noexcept{ return pdep64BMI2(a, msk); }
#else
static inline uint64_t pext(uint64_t a, uint64_t msk)noexcept{ return pextNoBMI2(a, msk); }
static inline uint64_t pdep(uint64_t a, uint64_t msk)noexcept{ return pdepNoBMI2(a, msk); }
#endif
    
/**************************複数ビット取り出し**************************/

// pdepを使うと高速な場合が多い

// pdep版
template<typename T>
static T lowestNBitsBMI2(T a, int n)noexcept{
    return pdep((static_cast<T>(1) << n) - static_cast<T>(1), a);
}
template<typename T>
static T highestNBitsBMI2(T a, int n)noexcept{
    return a - pdep((static_cast<T>(1) << (countBits(a) - n)) - static_cast<T>(1), a);
}
template<typename T>
static inline T NthLowestBitBMI2(T a, int n)noexcept{
    return lowestBit(a - pdep((static_cast<T>(1) << (n - 1)) - static_cast<T>(1), a));
}
template<typename T>
static inline T NthHighestBitBMI2(T a, int n)noexcept{
    return lowestBit(a - pdep((static_cast<T>(1) << (countBits(a) - n)) - static_cast<T>(1), a));
}
    
// 自前
template<typename T>
static T lowestNBitsNoBMI2(T a, int n)noexcept{
    // n <= 0には未対応
    assert(n > 0);
    T ans = static_cast<T>(0);
    T l;
    while(1){
        l = lowestBit<T>(a);
        ans |= l;
        if(n == 1){ break; }
        --n;
        a -= l;
    }
    return ans;
}
    
template<typename T>
static T highestNBitsNoBMI2(T a, int n)noexcept{
    assert(n > 0);
    // n <= 0には未対応
    T ans = static_cast<T>(0);
    T h;
    while(1){
        h = highestBit<T>(a);
        ans |= h;
        if(n == 1){ break; }
        --n;
        a -= h;
    }
    return ans;
}
    
template<typename T>
static inline T NthLowestBitNoBMI2(T a, int n)noexcept{
    T bit = 0;
    while(n){
        if(n == 1){ bit = lowestBit<T>(a); return bit; }
        a = a & (a - static_cast<T>(1));
        n--;
    }
    return bit;
}

template<typename T>
static inline T NthHighestBitNoBMI2(T a, int n)noexcept{
    T bit = 0;
    while(n){
        assert(a);
        if (n == 1){ bit = highestBit<T>(a); return bit; }
        a &= ((static_cast<T>(1) << bsr(a)) - static_cast<T>(1)); // 最上位ビットだけを外す
        --n;
    }
    return bit;
}

// BMI2版の方が遅そう...
/*#if defined (HAVE_BMI2)
template<typename T>
static T lowestNBits(T a, int n)noexcept{ return lowestNBitsBMI2(a, n); }
template<typename T>
static T highestNBits(T a, int n)noexcept{ return highestNBitsBMI2(a, n); }
template<typename T>
static T NthLowestBit(T a, int n)noexcept{ return NthLowestBitBMI2(a, n); }
template<typename T>
static T NthHighestBit(T a, int n)noexcept{ return NthHighestBitBMI2(a, n); }
    
#else*/
    
template<typename T>
static T lowestNBits(T a, int n)noexcept{ return lowestNBitsNoBMI2(a, n); }
template<typename T>
static T highestNBits(T a, int n)noexcept{ return highestNBitsNoBMI2(a, n); }
template<typename T>
static T NthLowestBit(T a, int n)noexcept{ return NthLowestBitNoBMI2(a, n); }
template<typename T>
static T NthHighestBit(T a, int n)noexcept{ return NthHighestBitNoBMI2(a, n); }
    
//#endif
    
/**************************ベクトル演算**************************/

#ifdef HAVE_AVX2
class bits128_t{
public:
    __m128i xmm_;
    
private:

    struct not_type{
        const __m128i not_xmm_;

        // ~b0 & b1
        bits128_t operator&(const bits128_t& b)const{
            return bits128_t(_mm_andnot_si128(not_xmm_, b.xmm_));
        }
        // 通常のnot
        operator bits128_t()const{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
            __m128i temp;
            const __m128i mask = _mm_cmpeq_epi64(temp, temp);
#pragma GCC diagnostic pop
            //const __m128i mask = _mm_set1_epi8(-1);
            return bits128_t(_mm_xor_si128(not_xmm_, mask));
        }
        explicit not_type(const bits128_t& b) : not_xmm_(b.xmm_) {}
    };

public:
    static bits128_t zero()noexcept{
        return bits128_t(_mm_setzero_si128());
    }
    
    bits128_t::not_type operator ~()const noexcept{
        return bits128_t::not_type(*this);
    }
    bits128_t operator &(const bits128_t::not_type& not_b)const noexcept{
        return bits128_t(_mm_andnot_si128(not_b.not_xmm_, xmm_));
    }
    bits128_t operator &(const bits128_t& b)const noexcept{
        return bits128_t(_mm_and_si128(xmm_, b.xmm_));
    }
    bits128_t operator |(const bits128_t& b)const noexcept{
        return bits128_t(_mm_or_si128(xmm_, b.xmm_));
    }
    bits128_t operator ^(const bits128_t& b)const noexcept{
        return bits128_t(_mm_xor_si128(xmm_, b.xmm_));
    }
    bits128_t operator +(const bits128_t& b)const noexcept{
        return bits128_t(xmm_ + b.xmm_);
    }
    bits128_t operator -(const bits128_t& b)const noexcept{
        return bits128_t(xmm_ - b.xmm_);
    }
    bits128_t operator <<(const int i)const noexcept{
        return bits128_t(xmm_ << i);
    }
    bits128_t operator >>(const int i)const noexcept{
        return bits128_t(xmm_ >> i);
    }

    bits128_t& operator |=(const bits128_t& b)noexcept{
        xmm_ = _mm_or_si128(xmm_, b.xmm_);
        return *this;
    }
    bits128_t& operator &=(const bits128_t::not_type& not_b)noexcept{
        xmm_ = _mm_andnot_si128(not_b.not_xmm_, xmm_);
        return *this;
    }
    bits128_t& operator &=(const bits128_t& b)noexcept{
        xmm_ = _mm_and_si128(xmm_, b.xmm_);
        return *this;
    }
    bits128_t& operator ^=(const bits128_t& b)noexcept{
        xmm_ = _mm_xor_si128(xmm_, b.xmm_);
        return *this;
    }
    bits128_t& operator +=(const bits128_t& b)noexcept{
        xmm_ = xmm_ + b.xmm_;
        return *this;
    }
    bits128_t& operator -=(const bits128_t& b)noexcept{
        xmm_ = xmm_ - b.xmm_;
        return *this;
    }

    bool operator ==(const bits128_t& b)const noexcept{
        __m128i temp = xmm_ - b.xmm_;
        return _mm_testz_si128(temp, temp);
    }
    bool operator !=(const bits128_t& b)const noexcept{
        return !((*this) == b);
    }

    constexpr bits128_t(): xmm_(){}
    explicit constexpr bits128_t(const __m128i& i): xmm_(i){}
    constexpr bits128_t(const bits128_t& b): xmm_(b.xmm_){}
};

bool holds(const bits128_t& b0, const bits128_t& b1)noexcept{
    return _mm_testc_si128(b0.xmm_, b1.xmm_);
}
bool isExclusive(const bits128_t& b0, const bits128_t& b1)noexcept{
    return _mm_testz_si128(b0.xmm_, b1.xmm_);
}
    
/*std::string toString(const bits128_t& b){
    std::ostringstream oss;
    
}
    
std::ostream& operator <<(std::ostream& ost, const bits128_t& b){
    ost << toStirng(b);
    return ost;
}*/
    
class bits256_t{
public:
    __m256i ymm_;

private:

    struct not_type{
        const __m256i not_ymm_;
            
        // ~b0 & b1
        bits256_t operator&(const bits256_t& b)const{
            return bits256_t(_mm256_andnot_si256(not_ymm_, b.ymm_));
        }
        // 通常のnot
        operator bits256_t()const{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
            __m256i temp;
            const __m256i mask = _mm256_cmpeq_epi64(temp, temp);
#pragma GCC diagnostic pop
            //const __m256i mask = _mm256_set1_epi8(-1);
            return bits256_t(_mm256_xor_si256(not_ymm_, mask));
        }
        explicit not_type(const bits256_t& b) : not_ymm_(b.ymm_) {}
    };
        
public:
    static bits256_t zero()noexcept{
        return bits256_t(_mm256_setzero_si256());
    }
    
    bits256_t::not_type operator ~()const noexcept{
        return bits256_t::not_type(*this);
    }
    bits256_t operator &(const bits256_t::not_type& not_b)const noexcept{
        return bits256_t(_mm256_andnot_si256(not_b.not_ymm_, ymm_));
    }
    bits256_t operator &(const bits256_t& b)const noexcept{
        return bits256_t(_mm256_and_si256(ymm_, b.ymm_));
    }
    bits256_t operator |(const bits256_t& b)const noexcept{
        return bits256_t(_mm256_or_si256(ymm_, b.ymm_));
    }
    bits256_t operator ^(const bits256_t& b)const noexcept{
        return bits256_t(_mm256_xor_si256(ymm_, b.ymm_));
    }
    bits256_t operator +(const bits256_t& b)const noexcept{
        return bits256_t(ymm_ + b.ymm_);
    }
    bits256_t operator -(const bits256_t& b)const noexcept{
        return bits256_t(ymm_ - b.ymm_);
    }
    bits256_t operator <<(const int i)const noexcept{
        return bits256_t(ymm_ << i);
    }
    bits256_t operator >>(const int i)const noexcept{
        return bits256_t(ymm_ >> i);
    }

    bits256_t& operator |=(const bits256_t& b)noexcept{
        ymm_ = _mm256_or_si256(ymm_, b.ymm_);
        return *this;
    }
    bits256_t& operator &=(const bits256_t::not_type& not_b)noexcept{
        ymm_ = _mm256_andnot_si256(not_b.not_ymm_, ymm_);
        return *this;
    }
    bits256_t& operator &=(const bits256_t& b)noexcept{
        ymm_ = _mm256_and_si256(ymm_, b.ymm_);
        return *this;
    }
    bits256_t& operator ^=(const bits256_t& b)noexcept{
        ymm_ = _mm256_xor_si256(ymm_, b.ymm_);
        return *this;
    }
    bits256_t& operator +=(const bits256_t& b)noexcept{
        ymm_ = ymm_ + b.ymm_;
        return *this;
    }
    bits256_t& operator -=(const bits256_t& b)noexcept{
        ymm_ = ymm_ - b.ymm_;
        return *this;
    }

    bool operator ==(const bits256_t& b)const noexcept{
        __m256i temp = ymm_ - b.ymm_;
        return _mm256_testz_si256(temp, temp);
    }
    bool operator !=(const bits256_t& b)const noexcept{
        return !((*this) == b);
    }

    constexpr bits256_t(): ymm_(){}
    explicit constexpr bits256_t(const __m256i& i): ymm_(i){}
    constexpr bits256_t(const bits256_t& b): ymm_(b.ymm_){}
};
    
bool holds(const bits256_t& b0, const bits256_t& b1)noexcept{
    return _mm256_testc_si256(b0.ymm_, b1.ymm_);
}
bool isExclusive(const bits256_t& b0, const bits256_t& b1)noexcept{
    return _mm256_testz_si256(b0.ymm_, b1.ymm_);
}

#else

class bits128_t{
private:
    std::array<uint64_t, 2> ar_;
public:
    constexpr const std::array<uint64_t, 2>& array()const noexcept{
        return ar_;
    }
    
    bits128_t& clear()noexcept{
        ar_.fill(0);
        return *this;
    }
    static bits128_t zero()noexcept{
        bits128_t tmp;
        tmp.clear();
        return tmp;
    }
    
    constexpr bits128_t(): ar_(){}
    constexpr bits128_t(const bits128_t& b): ar_(b.array()){}
};
    
class bits256_t{
private:
    std::array<uint64_t, 4> ar_;
public:
    constexpr const std::array<uint64_t, 4>& array()const noexcept{
        return ar_;
    }
    
    bits256_t& clear()noexcept{
        ar_.fill(0);
        return *this;
    }
    static bits256_t zero()noexcept{
        bits256_t tmp;
        tmp.clear();
        return tmp;
    }
    
    constexpr bits256_t(): ar_(){}
    constexpr bits256_t(const bits256_t& b): ar_(b.array()){}
};
#endif
    
#endif // UTIL_BIT_OPERATION_HPP_
