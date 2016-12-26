/*
 defines.h
 Katsuki Ohto
 */

#ifndef DEFINES_H_
#define DEFINES_H_

// ゲーム以前の定義とか
// 時間計測もすぐに使えるようにここに入れておく
// 設定ファイルの内容を使うので、
// 設定ファイルはこれより先にインクルードする
// ただし設定ファイルのインクルード無しでもコンパイルエラーにならないようにする

// インクルード
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cassert>
#include <vector>
#include <string>
#include <algorithm>
#include <sys/time.h>

#if defined (HAVE_BMI2)
#include <immintrin.h>
#endif

#if defined (HAVE_SSE4)
#include <smmintrin.h>
#elif defined (HAVE_SSE2)
#include <emmintrin.h>
#endif

// by Apery
#if !defined(NDEBUG)
#define UNREACHABLE assert(0)
#elif defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#define UNREACHABLE __assume(0)
#elif defined(__INTEL_COMPILER)
#define UNREACHABLE __assume(0)
#elif defined(__GNUC__) && (4 < __GNUC__ || (__GNUC__ == 4 && 4 < __GNUC_MINOR__))
#define UNREACHABLE __builtin_unreachable()
#else
#define UNREACHABLE assert(0)
#endif

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#define FORCE_INLINE __forceinline
#elif defined(__INTEL_COMPILER)
#define FORCE_INLINE inline
#elif defined(__GNUC__)
#define FORCE_INLINE __attribute__((always_inline)) inline
#else
#define FORCE_INLINE inline
#endif

// Unroller
template <int N> struct Unroller {
    template <typename T> FORCE_INLINE void operator () (T t) {
        Unroller<N-1>()(t);
        t(N-1);
    }
};
template <> struct Unroller<0> {
    template <typename T> FORCE_INLINE void operator () (T t) {}
};

// オペレータの一括オーバーロード
// by 技巧

// 足し算引き算系だけ
#define ENABLE_ADD_SUB_OPERATORS(T)                                    \
constexpr T operator-(T lhs) { return T(-int(lhs)); }                  \
constexpr T operator+(T lhs, T rhs) { return T(int(lhs) + int(rhs)); } \
constexpr T operator-(T lhs, T rhs) { return T(int(lhs) - int(rhs)); } \
constexpr T operator+(T lhs, int rhs) { return T(int(lhs) + rhs); }    \
constexpr T operator-(T lhs, int rhs) { return T(int(lhs) - rhs); }    \
constexpr T operator+(int lhs, T rhs) { return T(lhs + int(rhs)); }    \
constexpr T operator-(int lhs, T rhs) { return T(lhs - int(rhs)); }    \
inline T& operator+=(T& lhs, T rhs) { return lhs = lhs + rhs; }        \
inline T& operator-=(T& lhs, T rhs) { lhs = lhs - rhs; return lhs; }   \
inline T& operator+=(T& lhs, int rhs) { lhs = lhs + rhs; return lhs; } \
inline T& operator-=(T& lhs, int rhs) { lhs = lhs - rhs; return lhs; } \
inline T& operator++(T& lhs) { lhs = lhs + 1; return lhs; }            \
inline T& operator--(T& lhs) { lhs = lhs - 1; return lhs; }            \
inline T operator++(T& lhs, int) { T t = lhs; lhs += 1; return t; }    \
inline T operator--(T& lhs, int) { T t = lhs; lhs -= 1; return t; }

// 掛け算割り算も含める
#define ENABLE_ARITHMETIC_OPERATORS(T)                                 \
ENABLE_ADD_SUB_OPERATORS(T)                                            \
constexpr T operator*(T lhs, int rhs) { return T(int(lhs) * rhs); }    \
constexpr T operator*(int lhs, T rhs) { return T(lhs * int(rhs)); }    \
constexpr T operator/(T lhs, int rhs) { return T(int(lhs) / rhs); }    \
inline T& operator*=(T& lhs, int rhs) { lhs = lhs * rhs; return lhs; } \
inline T& operator/=(T& lhs, int rhs) { lhs = lhs / rhs; return lhs; }

// 整数定義
using std::int8_t;
using std::uint8_t;
using std::int16_t;
using std::uint16_t;
using std::int32_t;
using std::uint32_t;
using std::int64_t;
using std::uint64_t;

// クロック計測関数(64ビットで値を返す)
static uint64_t cputime(){
    unsigned int ax, dx;
    asm volatile("rdtsc\nmovl %%eax,%0\nmovl %%edx,%1":"=g"(ax),"=g"(dx): :"eax","edx");
    return ((unsigned long long int)(dx) << 32) + (unsigned long long int)(ax);
}

// タイムスタンプ読み込みにかかる時間
// 環境依存しそうなので0にしておく
constexpr uint64_t CLOCK_LATENCY = 0ULL;

// デバッグ用

uint64_t CLOCK_START;

static void tick()noexcept{
    CLOCK_START = cputime();
}

static void tock(){
    uint64_t CLOCK_END = cputime();
    printf("clock=%lld\n", (unsigned long long int)(CLOCK_END - CLOCK_START - CLOCK_LATENCY));
}

timeval CLOCKMICS_START;

static void tickMicS()noexcept{
    gettimeofday(&CLOCKMICS_START, NULL);
}

static void tockMicS(){
    timeval CLOCKMICS_END;
    gettimeofday(&CLOCKMICS_END, NULL);
    long t = (CLOCKMICS_END.tv_sec - CLOCKMICS_START.tv_sec) * 1000000 + CLOCKMICS_END.tv_usec - CLOCKMICS_START.tv_usec;
    printf("%ld micsec.\n", t);
}

//腕時計クラス


class Clock{
private:
    uint64_t c_start;
public:
    void start()noexcept{ c_start = cputime(); }
    uint64_t stop()const noexcept{ return cputime() - c_start - CLOCK_LATENCY; }
    uint64_t restart()noexcept{ // 結果を返し、0から再スタート
        uint64_t tmp = cputime();
        uint64_t old = c_start;
        c_start = tmp;
        return tmp - old - CLOCK_LATENCY;
    }
    
    constexpr Clock(): c_start(){}
};

class ClockMicS{
    // microsec単位
private:
    timeval t_start;
public:
    void start()noexcept{ gettimeofday(&t_start, NULL); }
    long stop()const noexcept{
        timeval t_end;
        gettimeofday(&t_end, NULL);
        long t = (t_end.tv_sec - t_start.tv_sec) * 1000000 + t_end.tv_usec - t_start.tv_usec;
        return t;
    }
    long restart()noexcept{ // 結果を返し、0から再スタート
        timeval t_end;
        gettimeofday(&t_end,NULL);
        long t = (t_end.tv_sec - t_start.tv_sec) * 1000000 + t_end.tv_usec - t_start.tv_usec;
        t_start = t_end;
        return t;
    }
    static long now()noexcept{
        timeval t_now;
        gettimeofday(&t_now, NULL);
        long t = t_now.tv_sec * 1000000 + t_now.tv_usec;
        return t;
    }
    ClockMicS(){}
    ClockMicS(int m){ start(); }
};

class ClockMS{
    // millisec単位
private:
    timeval t_start;
public:
    void start()noexcept{ gettimeofday(&t_start, NULL); }
    long stop()const noexcept{
        timeval t_end;
        gettimeofday(&t_end, NULL);
        long t = (t_end.tv_sec - t_start.tv_sec) * 1000 + (t_end.tv_usec - t_start.tv_usec) / 1000;
        return t;
    }
    long restart()noexcept{ // 結果を返し、0から再スタート
        timeval t_end;
        gettimeofday(&t_end, NULL);
        long t = (t_end.tv_sec - t_start.tv_sec) * 1000 + (t_end.tv_usec - t_start.tv_usec) / 1000;
        t_start = t_end;
        return t;
    }
    static long now()noexcept{
        timeval t_now;
        gettimeofday(&t_now, NULL);
        long t = t_now.tv_sec * 1000 + t_now.tv_usec / 1000;
        return t;
    }
    const timeval& get_t_start()const noexcept{
        return t_start;
    }
    ClockMS(){}
    ClockMS(int m){ start(); }
    ClockMS(const ClockMS& a):t_start(a.get_t_start()){}
};

// 出力
struct Space{
    int n;
    constexpr Space():n(0){}
    constexpr Space(const int arg):n(arg){}
};

static std::ostream& operator<<(std::ostream& os, const Space& sp){
    for(int i = 0; i < sp.n; ++i){ os << ' '; }
    return os;
}

template<typename T = int>
struct OutXY{
    T x, y;
    constexpr OutXY(T ax, T ay):x(ax), y(ay){}
};

template<typename T>
static std::ostream& operator<<(std::ostream& os, const OutXY<T>& arg){
    os << '(' << arg.x << ", " << arg.y << ')';
    return os;
}

template<typename T = int>
struct OutXYZ{
    T x, y, z;
    constexpr OutXYZ(T ax, T ay, T az):x(ax), y(ay), z(az){}
};

template<typename T>
static std::ostream& operator<<(std::ostream& os, const OutXYZ<T>& arg){
    os << '(' << arg.x << ", " << arg.y << ", " << arg.z << ')';
    return os;
}

// 3値定数
constexpr int _YES = 1;
constexpr int _NO = 0;
constexpr int _BOTH = 2;

// constexprのmin,max
template<typename T>
constexpr static T cmax(const T& a, const T& b){
    return a < b ? b : a;
}

template<typename T>
constexpr static T cmin(const T& a, const T& b){
    return a > b ? b : a;
}

// 出力等可能なアサーション

// アサーション用関数
void reportError( const char *functionName,
                 const char *fileName, int lineNumber,
                 std::string message );

// 条件x、命令等y
#ifdef NDEBUG
#define ASSERT(X, Y)
#define FASSERT(f, o)
#define FEQUALS(f0, f1, o)
#else
#define ASSERT(X, Y)  if(!(X)){ Y; assert(0); }
// 浮動小数点がまともな値を取っているかどうかのアサーション
#define FASSERT(f, o) if(!(!std::isinf(f) && !std::isnan(f))){ cerr << (f) << endl; {o}\
assert(!std::isinf(f) && !std::isnan(f)); assert(0); };
// 浮動小数点が「ほぼ」同じ値を取っているかのチェック && FASSERT
#define FEQUALS(f0, f1, o) { if(!(!std::isinf(f0) && !std::isnan(f0))){ cerr << (f0) << endl; {o}\
assert(!std::isinf(f0) && !std::isnan(f0)); assert(0); };\
if(!(!std::isinf(f1) && !std::isnan(f1))){ cerr << (f1) << endl; {o}};\
assert(!std::isinf(f1) && !std::isnan(f1)); assert(0); };\
if(!(abs((f0) - (f1)) <= 0.00001)){ cerr << (f0) << " <-> " << (f1) << endl; {o}\
assert(abs((f0) - (f1)) <= 0.00001); assert(0); }
#endif // NDEBUG

// 標準ライブラリ使用
using std::cout;
using std::cerr;
using std::setw;
using std::endl;
using std::ostream;
using std::stringstream;
using std::max;
using std::min;

using std::size_t;

// 出力
#ifdef DEBUG
#define DOUT std::cout
#define DERR std::cerr
#define DWAIT getchar()
#else
#define DERR 0 && std::cerr
#define DOUT 0 && std::cout
#define DWAIT
#endif

#ifndef MINIMUM
#define COUT cout
#define CERR cerr
#define CWAIT getchar()
#else
#define CERR 0 && cerr
#define COUT 0 && cout
#define CWAIT
#endif


// デバッグ用のブレークポイント
#define BREAKPOINT {volatile int a = 1; std::random_device seed_gen; std::mt19937 engine(seed_gen()); if(engine() % 2){ std::cerr << "break" << endl; }}

// 環境依存にはまらないように色々

#ifndef M_PI
#define M_PI 3.14159265358979343846
#endif

#endif // DEFINES_H_
