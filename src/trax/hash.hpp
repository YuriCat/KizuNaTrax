/*
 hash.hpp
 */

#ifndef TRAX_HASH_HPP_
#define TRAX_HASH_HPP_

#include "trax.hpp"

using namespace Trax;

struct HashEntry{

    enum Flag{
        /** 何もフラグが立てられていないことを示します. */
        kFlagNone  = 0x00,
        
        /** ３手詰め関数をスキップすべきことを知らせるフラグです. */
        kSkipMate3 = 0x10,
    };
    
    // flag_メンバ変数には、Boundも保存されるので、それとビットが重ならないようにする
    static_assert((kSkipMate3 & kBoundExact) == 0, "");

    bool empty()const noexcept{ return key32_ == 0; }
    Key32 key32()const  noexcept{ return key32_; }
    Move move()const noexcept{ return move_; }
    Score score()const noexcept{ return static_cast<Score>(score_); }
    Score eval()const noexcept{ return static_cast<Score>(eval_ ); }
    Depth depth()const noexcept{ return static_cast<Depth>(depth_); }
    Bound bound()const noexcept{ return static_cast<Bound>(flags_ & kBoundExact); }
    uint8_t age()const noexcept{ return age_; }
    
    bool skip_mate3()const noexcept{ return flags_ & kSkipMate3; }
    
    void set_age(uint8_t new_age)noexcept{ age_ = new_age; }
    
    /**
     * エントリに探索によって得られたデータを保存します.
     */
    void Save(Key64 key64,
              Score score,
              Bound bound,
              Depth depth,
              Move move,
              Score eval,
              Flag flag,
              uint8_t age) {
        key32_ = ToKey32(key64);
        move_  = move;
        score_ = static_cast<int16_t>(score);
        eval_  = static_cast<int16_t>(eval);
        depth_ = static_cast<int16_t>(depth);
        flags_ = static_cast<uint8_t>(bound | flag);
        age_   = age;
    }
    
    Key32   key32_;
    Move    move_;
    int16_t score_, eval_, depth_;
    uint8_t flags_, age_;
};

// TTEntryがぴったり１６バイトになっているかチェックする
static_assert(sizeof(HashEntry) == 16, "");

class HashTable{
public:
    
    /**
     * ハッシュテーブルから、特定の局面の情報を参照します.
     * @param key64 情報を取得したい局面のハッシュ値（64ビット）
     * @return 局面に関する情報が記録されているポインタ
     */
    HashEntry* LookUp(Key64 key64)const{
        const Key32 key32 = ToKey32(key64);
        for (HashEntry& tte : table_[key64 & key_mask_]) {
            if (tte.key32() == key32) {
                tte.set_age(age_); // Refresh
                return &tte;
            }
        }
        return nullptr;
    }
    
    /**
     * 特定の局面に関する情報を保存する.
     */
    void Save(Key64 key64, Move move, Score score, Depth depth, Bound bound,
              Score eval, bool skip_mate3){
        const Key32 key32 = ToKey32(key64);
        HashEntry::Flag flag = skip_mate3 ? HashEntry::kSkipMate3 : HashEntry::kFlagNone;
        
        // 1. 保存先を探す
        Bucket& bucket = table_[key64 & key_mask_];
        HashEntry* replace = bucket.begin();
        for (HashEntry& tte : bucket) {
            // a. 空きエントリや完全一致エントリが見つかった場合
            if (tte.empty() || tte.key32() == key32) {
                // すでにあるハッシュ手はそのまま残す
                if (move == kMoveNone) {
                    move = tte.move();
                }
                
                // ３手詰みをスキップ可能であるとのフラグがすでに存在するときは、そのフラグをそのまま残す
                if (skip_mate3 == false) {
                    flag = static_cast<HashEntry::Flag>(tte.flags_ & HashEntry::kSkipMate3);
                }
                
                replace = &tte;
                hashfull_ += tte.empty(); // エントリが空の場合は、ハッシュテーブルの使用率が上がる
                break;
            }
            
            // b. 置き換える場合
            if (  (tte.age() == age_ || tte.bound() == kBoundExact)
                - (replace->age() == age_)
                - (tte.depth() < replace->depth()) < 0) {
                replace = &tte;
            }
        }
        
        // 2. メモリに保存する
        replace->Save(key64, score, bound, depth, move, eval, flag, age_);
    }
    /**
     * 指定されたキーに対応するエントリのプリフェッチを行います.
     */
    void Prefetch(Key64 key) const {
        __builtin_prefetch(&table_[key & key_mask_]);
    }
    
    /**
     * 新規に探索を行う場合に呼んでください.
     */
    void NextAge() {
        ++age_;
    }
    
    /**
     * ハッシュテーブルに保存されている情報を物理的にクリアします.
     */
    void Clear(){
        std::memset(table_.get(), 0, size_ * sizeof(Bucket));
        age_ = 0;
        hashfull_ = 0;
    }
    
    /**
     * ハッシュテーブルの大きさを変更します.
     * @param megabytes メモリ上に確保したいハッシュテーブルの大きさ（メガバイト単位で指定）
     */
    void SetSize(size_t megabytes){
        size_t bytes = megabytes * 1024 * 1024;
        age_  = 0;
        size_ = (static_cast<size_t>(1) << bsr<uint64_t>(bytes)) / sizeof(Bucket);
        key_mask_ = size_ - 1;
        hashfull_ = 0;
        table_.reset(new Bucket[size_]);
        // テーブルのゼロ初期化を行う（省略不可）
        // Moveクラスのデフォルトコンストラクタにはゼロ初期化処理がないので、ここでゼロ初期化を行わないと、
        // ハッシュムーブがおかしな手になってしまい、最悪セグメンテーションフォールトを引き起こす。
        std::memset(table_.get(), 0, sizeof(Bucket) * size_);
    }
    
    /**
     * ハッシュテーブルの使用率をパーミル（千分率）で返します.
     * USIのinfoコマンドのhashfullにそのまま使うと便利です。
     */
    int hashfull() const {
        return (UINT64_C(1000) * hashfull_) / (kBucketSize * size_);
    }
    
private:
    /** バケツ１個あたりに保存する、エントリの数. */
    static constexpr size_t kBucketSize = 4;
    
    /**
     * エントリを保存するためのバケツです.
     * kBucketSizeは４なので、バケツ１個につき４個のエントリを保存できます。
     */
    typedef std::array<HashEntry, kBucketSize> Bucket;
    
    /** ハッシュテーブルのポインタ */
    std::unique_ptr<Bucket[]> table_;
    
    /** ハッシュテーブルの要素数 */
    size_t size_;
    
    /** ハッシュキーから、テーブルのインデックスを求めるためのビットマスク */
    size_t key_mask_;
    
    /** 使用済みのエントリの数 */
    size_t hashfull_;
    
    /** ハッシュテーブルに入っている情報の古さ */
    uint8_t age_;
};

#endif // TRAX_HASH_HPP_
