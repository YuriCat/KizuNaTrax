/*
 analyzer.hpp
 Katsuki Ohto
 */

#ifndef UTIL_ANALYZER_HPP_
#define UTIL_ANALYZER_HPP_

#include <iostream>
#include <array>

#include "../defines.h"

// メソッドアナライザー
// メソッドのcpu時間や使用回数など諸々を調べる

// part...どれかが単体で動くことを想定
// phase...一度の処理の中で連続して存在することを想定

// 簡単に使うため、アナライザクラスには解析可能性のあるデータについて全て実装しておく
// 最適化で邪魔な部分が消えてくれない限り、相当なメモリの無駄だが
// 本番用では全体をオフにするので不問とする

// std:atomicでスレッドセーフにしましたが少し遅くなっていると思います

namespace Analysis{
    // アナライザーの表示タイプ
    
    enum{
        TYPE_PLANE = 0, // 回数、時間計測、失敗回数のみ
        TYPE_SEARCH, // 探索結果
        TYPE_PLAYOUT, // 未実装
    };
    
}

// アナライザと呼ぶほどもでない静的カウンタ、時間計測器

// デストラクタで回数を表示するカウンタ
struct Counter{
#ifdef USE_ANALYZER
    uint64_t c_;
    std::string nm_; // 表示名
    Counter() : c_(0), nm_("COUNTER"){}
    Counter(uint64_t ac) : c_(ac), nm_("COUNTER"){}
    Counter(const std::string& nm) : c_(0), nm_(nm){}
    Counter(uint64_t ac, const std::string& nm) : c_(ac), nm_(nm){}
    ~Counter(){ std::cerr << nm_ << " : " << c_ << std::endl; }
    operator uint64_t()const noexcept{ return c_; }
    Counter& operator=(uint64_t n)noexcept{ c_ = n; return *this; }
    Counter& operator++()noexcept{ c_ += 1; return *this; }
    Counter& operator+=(uint64_t n)noexcept{ c_ += n; return *this; }
#else
    Counter(){}
    Counter(uint64_t ac){}
    Counter(const std::string& nm){}
    Counter(uint64_t ac, const std::string& nm){}
    ~Counter(){}
    operator uint64_t()const noexcept{ return 0ULL; }
    Counter& operator=(uint64_t n)noexcept{ return *this; }
    Counter& operator++()noexcept{ return *this; }
    Counter& operator+=(uint64_t n)noexcept{ return *this; }
#endif
};

#ifdef USE_ANALYZER
std::ostream& operator<<(std::ostream& ost, const Counter& c){
    ost << c.c_;
    return ost;
}
#else
std::ostream& operator<<(std::ostream& ost, const Counter& c){
    ost << "none";
    return ost;
}
#endif

// アトミック演算でより正確なカウント
struct AtomicCounter{
#ifdef USE_ANALYZER
    std::atomic<uint64_t> c_;
    std::string nm_; // 表示名
    AtomicCounter() : c_(0), nm_("COUNTER"){}
    AtomicCounter(uint64_t ac) : c_(ac), nm_("COUNTER"){}
    AtomicCounter(const std::string& nm) : c_(0), nm_(nm){}
    AtomicCounter(uint64_t ac, const std::string& nm) : c_(ac), nm_(nm){}
    ~AtomicCounter(){ std::cerr << nm_ << " : " << c_ << std::endl; }
    uint64_t count()const noexcept{ return c_; }
    void add(uint64_t n = 1)noexcept{ c_ += n; }
    AtomicCounter& operator=(uint64_t n)noexcept{ c_ = n; return *this; }
    AtomicCounter& operator++()noexcept{ c_ += 1; return *this; }
    AtomicCounter& operator+=(uint64_t n)noexcept{ c_ += n; return *this; }
#else
    AtomicCounter(){}
    AtomicCounter(uint64_t ac){}
    AtomicCounter(const std::string& nm){}
    AtomicCounter(uint64_t ac, const std::string& nm){}
    ~AtomicCounter(){}
    static uint64_t count()noexcept{ return 0ULL; }
    void add(uint64_t n = 1)const noexcept{}
    AtomicCounter& operator=(uint64_t n)noexcept{ return *this; }
    AtomicCounter& operator++()noexcept{ return *this; }
    AtomicCounter& operator+=(uint64_t n)noexcept{ return *this; }
#endif
};

struct StaticClock{
#ifdef USE_ANALYZER
    Clock cl;
    std::atomic<uint64_t> c_, t_;
    StaticClock() : c_(0ULL), t_(0ULL){}
    StaticClock(uint64_t ac, uint64_t at) : c_(ac), t_(at){}
    ~StaticClock(){ report(); }
    void start()noexcept{ cl.start(); }
    void stop()noexcept{
        uint64_t tmp = cl.stop();
        ++c_;
        t_ += tmp;
    }
    void report()const{
        std::cerr << "CLOCK : " << t_ << " clock ( " << c_ << " times)";
        if (c_ > 0){
            uint64_t avg = t_ / c_;
            std::cerr << " " << avg << " per trial.";
        }
        std::cerr << std::endl;
    }
#else
    StaticClock(){}
    StaticClock(uint64_t ac, uint64_t at){}
    ~StaticClock(){}
    void start()const noexcept{}
    void stop()const noexcept{}
    void report()const noexcept{}
#endif
};

template<int PART_NUM = 1, int PHASE_NUM = 1, int MODE = 0,
typename base_integer_t = uint64_t, typename integer_t = uint64_t>
struct BaseAnalyzer{
#ifdef USE_ANALYZER
    Clock clock;
    std::string name;
    std::array<integer_t, PART_NUM> trials;
    std::array<std::array<integer_t, PHASE_NUM>, PART_NUM> time; // 時間
    std::array<integer_t, PART_NUM> failures; // 失敗
    // 探索解析
    std::array<integer_t, PART_NUM> nodes;
    std::array<integer_t, PART_NUM> childs;
    // プレイアウト解析
    std::array<integer_t, PART_NUM> turns;
    
    void startClock()noexcept{ clock.start(); }
    base_integer_t stopClock()noexcept{ return clock.stop(); }
    base_integer_t restartClock(){ return clock.restart(); }
    void addTrial(const int pa = 0){ ++trials[pa]; }
    void addTime(const uint64_t argTime, const int pa = 0, const int ph = 0){
        time[pa][ph] += argTime;
    }
    void addFailure(const int pa = 0){ ++failures[pa]; }
    void addNodes(const int argNodes, const int pa = 0){ nodes[pa] += argNodes; }
    void addChilds(const int argChilds, const int pa = 0){ childs[pa] += argChilds; }
    void addTurns(const int argTurns, const int pa = 0){ turns[pa] += argTurns; }
    void init()noexcept{
        for(int i = 0; i < PART_NUM; ++i){
            trials[i] = 0;
            failures[i] = 0;
            nodes[i] = 0;
            childs[i] = 0;
            turns[i] = 0;
            for(int j = 0; j < PHASE_NUM; ++j){
                time[i][j] = 0;
            }
        }
    }
    BaseAnalyzer(){ init(); }
    BaseAnalyzer(const std::string& argName) : name(argName){ init(); }
    ~BaseAnalyzer(){ std::cerr << toString() << std::endl; }
#else
    void startClock()noexcept{}
    base_integer_t stopClock()noexcept{ return 0; }
    base_integer_t restartClock(){ return 0; }
    void addTrial(const int pa = 0){}
    void addTime(const uint64_t argTime, const int pa = 0, const int ph = 0){}
    void addFailure(const int pa = 0){}
    void addNodes(const int argNodes, const int pa = 0){}
    void addChilds(const int argChilds, const int pa = 0){}
    void addTurns(const int argTurns, const int pa = 0){}
    void init()noexcept{}
    BaseAnalyzer(){}
    BaseAnalyzer(const std::string& argName){}
    ~BaseAnalyzer(){}
#endif
    void start()noexcept{ startClock(); }
    void restart(uint64_t argTime, int pa, int ph){ // 外部クロックの場合
        addTime(argTime, pa, ph);
    }
    void restart(int pa = 0, int ph = 0){
        addTime(stopClock(), pa, ph);
        startClock();
    }
    void end(uint64_t argTime, int pa, int ph){ // 外部クロックの場合
        addTime(argTime, pa, ph);
        addTrial(pa);
    }
    void end(int pa = 0, int ph = 0){
        end(stopClock(), pa, ph);
    }
    std::string toString()const{
#ifdef USE_ANALYZER
        std::ostringstream oss;
        
        uint64_t time_all_sum = 0ULL;
        uint64_t time_part_sum[PART_NUM] = { 0ULL };
        uint64_t time_per_trial_all = 0ULL;
        uint64_t time_per_trial_part[PART_NUM] = { 0ULL };
        uint64_t time_per_trial_phase[PART_NUM][PHASE_NUM] = { 0ULL };
        
        uint64_t trial_sum = 0U;
        uint64_t failure_sum = 0U;
        
        // sum
        for(auto i = 0; i < PART_NUM; ++i){
            if(trials[i]){
                trial_sum += trials[i];
                failure_sum += failures[i];
                for(int j = 0; j < PHASE_NUM; ++j){
                    time_part_sum[i] += time[i][j];
                    time_per_trial_phase[i][j] = time[i][j] / trials[i];
                }
                time_all_sum += time_part_sum[i];
                time_per_trial_part[i] = time_part_sum[i] / trials[i];
            }
        }
        if(trial_sum){
            time_per_trial_all = time_all_sum / trial_sum;
        }
        
        oss << "****** Analysis of " << name << " ******" << std::endl;
        
        oss << " < Normal Analysis >" << std::endl;
        
        oss << " " << trial_sum << " trials.  " << time_all_sum << " clock ( " << time_per_trial_all << " clock/trial).  " << failure_sum << " failures." << std::endl;
        if(PART_NUM > 1 || PHASE_NUM > 1){
            for(auto i = 0; i < PART_NUM; ++i){
                oss << "   Part " << i << " : " << trials[i] << " trials.  " << time_part_sum[i] << " clock ( " << time_per_trial_part[i] << " clock/trial).  " << failures[i] << " failures." << std::endl;
                if(PHASE_NUM > 1){
                    for(auto j = 0; j < PHASE_NUM; ++j){
                        oss << "      Phase " << j << " : " << time[i][j] << " clock ( " << time_per_trial_phase[i][j] << " clock/trial)." << std::endl;
                    }
                }
            }
        }
        
        if(MODE == Analysis::TYPE_SEARCH){
            uint64_t node_sum = 0U;
            uint64_t child_sum = 0U;
            
            uint64_t node_per_trial_part[PART_NUM] = { 0U };
            uint64_t node_per_trial_all = 0U;
            
            uint64_t time_per_node_part[PART_NUM] = { 0ULL };
            uint64_t time_per_node_all = 0ULL;
            
            uint64_t child_per_trial_part[PART_NUM] = { 0U };
            uint64_t child_per_trial_all = 0U;
            
            uint64_t time_per_child_part[PART_NUM] = { 0ULL };
            uint64_t time_per_child_all = 0ULL;
            
            for(auto i = 0; i < PART_NUM; ++i){
                if(trials[i]){
                    node_sum += nodes[i];
                    child_sum += childs[i];
                    
                    if(nodes[i]){
                        node_per_trial_part[i] = nodes[i] / trials[i];
                        time_per_node_part[i] = time_part_sum[i] / nodes[i];
                    }
                    
                    if(childs[i]){
                        child_per_trial_part[i] = childs[i] / trials[i];
                        time_per_child_part[i] = time_part_sum[i] / childs[i];
                    }
                }
            }
            
            if(trial_sum){
                if(node_sum){
                    node_per_trial_all = node_sum / trial_sum;
                    time_per_node_all = time_all_sum / node_sum;
                }
                if(child_sum){
                    child_per_trial_all = child_sum / trial_sum;
                    time_per_child_all = time_all_sum / child_sum;
                }
            }
            
            
            oss << " < Search Analysis >" << std::endl;
            oss << " " << node_sum << " nodes ( " << node_per_trial_all << " nodes/trial ; " << time_per_node_all << " clock/node).  ";
            oss << " " << child_sum << " childs ( " << child_per_trial_all << " childs/trial ; " << time_per_child_all << " clock/child).  " << std::endl;
            if (PART_NUM > 1){
                for (auto i = 0; i < PART_NUM; ++i){
                    oss << "   Part " << i << " : " << nodes[i] << " nodes ( " << node_per_trial_part[i] << " nodes/trial ; " << time_per_node_part[i] << " clock/node).  ";
                    oss << childs[i] << " childs ( " << child_per_trial_part[i] << " childs/trial ; " << time_per_child_part[i] << " clock/child)." << std::endl;
                }
            }
        }
        return oss.str();
#endif
        return "none";
    }
};

template<int PART_NUM = 1, int PHASE_NUM = 1, int MODE = 0,
typename base_integer_t = uint64_t>
struct Analyzer : public BaseAnalyzer<PART_NUM, PHASE_NUM, MODE,
base_integer_t, base_integer_t>{
    // アナライザの基本形
    Analyzer() : BaseAnalyzer<PART_NUM, PHASE_NUM, MODE,
    base_integer_t, base_integer_t>(){}
    Analyzer(const std::string& nm) : BaseAnalyzer<PART_NUM, PHASE_NUM, MODE,
    base_integer_t, base_integer_t>(nm){}
};

template<int PART_NUM = 1, int PHASE_NUM = 1, int MODE = 0,
typename base_integer_t = uint64_t>
struct AtomicAnalyzer : public BaseAnalyzer<PART_NUM, PHASE_NUM, MODE,
base_integer_t, std::atomic<base_integer_t>>{
    // アトミック更新でマルチスレッドの場合にも正確な値を出すが遅くなる
    AtomicAnalyzer() : BaseAnalyzer<PART_NUM, PHASE_NUM, MODE,
    base_integer_t, std::atomic<base_integer_t>>(){}
    AtomicAnalyzer(const std::string& nm) : BaseAnalyzer<PART_NUM, PHASE_NUM, MODE,
    base_integer_t, std::atomic<base_integer_t>>(nm){}
};

struct HashBookAnalyzer{
    // 置換表の解析用
    
#ifdef USE_ANALYZER
    const std::string name;
    const uint64_t entry;
    const uint64_t memory;
    
    // state
    std::atomic<uint64_t> filled;
    std::atomic<uint64_t> deleted;
    
    // read
    std::atomic<uint64_t> hit;
    std::atomic<uint64_t> unfounded;
    std::atomic<uint64_t> white;
    
    // regist
    std::atomic<uint64_t> registration;
    std::atomic<uint64_t> registrationFailure;
    
    void init()noexcept{
        filled = 0; deleted = 0;
        hit = 0; white = 0; unfounded = 0;
        registration = 0; registrationFailure = 0;
    }
    
    void addFilled()noexcept{ ++filled; }
    void addDeleted()noexcept{ ++deleted; }
    
    void addHit()noexcept{ ++hit; }
    void addWhite()noexcept{ ++white; }
    void addUnfounded()noexcept{ ++unfounded; }
    
    void addRegistration()noexcept{ ++registration; }
    void addRegistrationFailure()noexcept{ ++registrationFailure; }
    
    void report()const{
        using std::cerr; using std::endl;
        double realRate = filled / (double)entry;
        double idealRate = 1.0 - pow((1.0 - 1.0 / (double)entry), registration);
        cerr << "****** Analysis(HashBook) of " << name;
        cerr << " (" << entry << " entries, " << memory << " bytes) ******" << endl;
        cerr << "State  : filled = " << realRate << " ( ideally... " << idealRate << " )" << endl;
        cerr << "Regist : success = " << registration << "  failure = " << registrationFailure << endl;
        cerr << "Read   : hit = " << hit << "  unfounded = " << unfounded << "  white = " << white << endl;
    }
    
#else
    
    void init()const noexcept{}
    
    void addFilled()const noexcept{}
    void addDeleted()const noexcept{}
    
    void addHit()const noexcept{}
    void addWhite()const noexcept{}
    void addUnfounded()const noexcept{}
    
    void addRegistration()const noexcept{}
    void addRegistrationFailure()const noexcept{}
    
    void report()const noexcept{}
    
#endif
    
    HashBookAnalyzer()
#ifdef USE_ANALYZER
    :name("none"), entry(0), memory(0)
#endif
    { init(); }
    
    HashBookAnalyzer(const std::string& argName, const uint64_t aent, const uint64_t amem)
#ifdef USE_ANALYZER
    :name(argName), entry(aent), memory(amem)
#endif
    {
        init();
    }
    
    ~HashBookAnalyzer(){
        report();
    }
};


#endif // UTIL_ANALYZER_HPP_
