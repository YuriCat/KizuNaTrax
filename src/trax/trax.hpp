/*
 trax.hpp
 Katsuki Ohto
 */

#ifndef TRAX_TRAX_HPP_
#define TRAX_TRAX_HPP_

#include <cstring>
#include <unistd.h>
#include <sys/time.h>
#include <ctime>

#include <cmath>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cassert>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <array>
#include <vector>
#include <queue>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <random>
#include <algorithm>
#include <string>
#include <bitset>
#include <numeric>

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>

#else

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

#endif

// 設定

#if !defined(MINIMUM)
#define USE_ANALYZER
#endif

// 相手手番中の先読み
#define PONDER
// ストレートデータ構造を使う
//#define USE_STRAIGHT

constexpr std::size_t N_THREADS = 8;

// 基本的定義、ユーティリティ
// 使いそうなものは全て読み込んでおく

// 出力
#include "../common/util/io.hpp"

// 文字列処理
#include "../common/util/string.hpp"

// 乱数
#include "../common/util/xorShift.hpp"

// 確率分布
//#include "../../common/util/random.hpp"

// ビット演算
#include "../common/util/bitOperation.hpp"

// 包括ユーティリティ
//#include "../../common/util/container.hpp"
//#include "../../common/util/arrays.h"

// クラスユーティリティ
#include "../common/util/bitSet.hpp"
#include "../common/util/bitArray.hpp"
#include "../common/util/longBitSet.hpp"

// 応用ユーティリティ
//#include "../../common/util/bitPartition.hpp"

// 解析用
#include "../common/util/math.hpp"
//#include "../../common/util/pd.hpp"

// 探索
//#include "../../common/util/search.hpp"

// ハッシュ
//#include "../../common/hash/HashFunc.hpp"
//#include "../../common/hash/HashBook.hpp"


// 並列化
#include "../common/util/lock.hpp"

// 解析
#include "../common/analyze/analyzer.hpp"

using std::size_t;

namespace Trax{
    
    /**************************型の定義**************************/
    
    using Key32 = uint32_t;
    using Key64 = uint64_t;
    
    constexpr Key32 ToKey32(Key64 key_)noexcept{
        return static_cast<Key32>(static_cast<uint64_t>(key_) >> 32);
    }
    
    enum Depth : int{
        kDepthZero = 0,
        kOnePly    = 64,
        kDepthNone = -127 * kOnePly,
    };
    
    ENABLE_ARITHMETIC_OPERATORS(Depth)
    
    enum Bound {
        kBoundNone  = 0,
        kBoundUpper = 1,
        kBoundLower = 1 << 1,
        kBoundExact = kBoundUpper | kBoundLower
    };
    
    /**************************基本的定義**************************/
    
    constexpr std::array<int, 2> kNullPosition = {-1, -1};
    
    // 手番, タイルの線の色
    enum Color : int{
        COLOR_NONE = -1,
        WHITE = 0, // 白線
        RED = 1, // 赤線
    };
    
    const char* colorChar = "WR";
    
    static constexpr Color flipColor(Color c)noexcept{
        return static_cast<Color>(1 - c);
    }
    static constexpr bool examColor(Color c)noexcept{
        return !(c & ~1);
    }
    static constexpr Color toTurnColor(unsigned int t)noexcept{
        return static_cast<Color>(t & 1);
    }
    
    Color readColor(char cchar){
        if(cchar == colorChar[WHITE]){
            return WHITE;
        }else if(cchar == colorChar[RED]){
            return RED;
        }
        return COLOR_NONE;
    }
    char toColorChar(Color c){
        if(!examColor(c)){ return -1; }
        return colorChar[c];
    }
    std::string toColorString(Color c){
        return std::string(1, toColorChar(c));
    }
    
    // maximum turns
    constexpr int N_TURNS = 256;
    
    // minimum length of victory lines
    constexpr int VICTORY_LINE_LENGTH = 8;
    
    // 方向
    constexpr unsigned int oppositeDirection(unsigned int d)noexcept{
        return (d + 2) % 4;
    }
    constexpr unsigned int areaToDirection(int x, int y)noexcept{
        // (x, y) 座標から方向への変換
        return ((unsigned int)(x + y > 0) << 1) + (unsigned int)bool(y);
    }
    
    // 対称性
    template<class callback_t>
    void iterateSymmetries(int x, int y, int mx, int my, const callback_t& callback){
        callback(0,      x,      y);
        callback(1,      x, my - y);
        callback(2, mx - x,      y);
        callback(3, mx - x, my - y);
        callback(4,      y,      x);
        callback(5,      y, mx - x);
        callback(6, my - y,      x);
        callback(7, my - y, mx - x);
    }
    
    // 対称型の逆変換テーブル 定跡に戻す時に用いる
    // 5 と 6 だけ逆関数が同じでない
    constexpr int invSymmetryTable[8] = {
        0, 1, 2, 3, 4, 6, 5, 7,
    };
    
    // 着手タイル状態(向きは盤面状態によって一意に定めるので区別しない)
    enum TileMove : int8_t{
        TILEMOVE_NONE = -1,
        P = 0, // 横 '+'
        S = 1, // 左上/右下 '/'
        B = 2, // 右上/左下 '\'
    };
    const char* tileChar = "+/\\";
    constexpr int TILEMOVE_MIN = P;
    constexpr int TILEMOVE_MAX = B;
    
    static constexpr bool examTileMove(TileMove tm)noexcept{ // 完全なチェック
        return (TILEMOVE_MIN <= tm && tm <= TILEMOVE_MAX);
    }
    
    // 回転状態も含めたタイル状態
    enum Tile : int8_t{
        TILE_NONE = -1,
        PW = 0, // 白が縦 '+'
        PR = 1, // 赤が横 '+'
        SW = 2, // 白が左上 '/'
        SR = 3, // 赤が左上 '/'
        BW = 4, // 白が右上 '\'
        BR = 5, // 赤が右上 '\'
    };
    
    constexpr int N_TILES = 6;
    
    // 以下 定義が変わったときのためにできるだけこれらを使う
    constexpr int TILE_MIN = PW;
    constexpr int TILE_MAX = BR;
    
    // チェック関数
    static constexpr bool examTile(Tile t)noexcept{ // 完全なチェック
        return (TILE_MIN <= t && t <= TILE_MAX);
    }
    static constexpr bool isTile(Tile t)noexcept{ // 実用上のチェック
        return (TILE_MIN <= t);
    }
    static constexpr bool isPlusTile(Tile t)noexcept{ // P型かS,B型か判別
        return (t <= PR);
    }
    static constexpr bool isBackTile(Tile t)noexcept{ // B型かP,S型か判別
        return (BW <= t);
    }
    
    // タイルから色情報を抜く(置くときには確定しているから)
    static constexpr TileMove toTileMove(Tile tile)noexcept{
        return static_cast<TileMove>(static_cast<uint8_t>(tile) >> 1);
    }
    static constexpr Tile toTile(TileMove tm, int ci)noexcept{
        return static_cast<Tile>((static_cast<uint8_t>(tm) << 1) + ci);
    }
    static constexpr Color toTopColor(Tile tile)noexcept{ // 上部の色を得る
        return static_cast<Color>(static_cast<uint8_t>(tile) & 1);
    }
    
    constexpr BitSet8 tileColorBitsTable[TILE_MAX + 1] = {
        // 0 上, 1 左, 2 下, 3 右
        // 赤の位置にビットを立てる
        BitSet8(1, 3),
        BitSet8(0, 2),
        BitSet8(2, 3),
        BitSet8(0, 1),
        BitSet8(1, 2),
        BitSet8(0, 3),
    };
    constexpr uint8_t tileOppositeTable[TILE_MAX + 1][4] = {
        // 各エンドと線が繋がっている反対側のエンド
        {2, 3, 0, 1},
        {2, 3, 0, 1},
        {1, 0, 3, 2},
        {1, 0, 3, 2},
        {3, 2, 1, 0},
        {3, 2, 1, 0},
    };
    constexpr std::array<uint8_t, 2> tileConnectTable[TILE_MAX + 1][2] = {
        // タイルごとの各色のエンド
        // {end_0, end_1}[tile][color]
        {{0, 2}, {1, 3}},
        {{1, 3}, {0, 2}},
        {{0, 1}, {2, 3}},
        {{2, 3}, {0, 1}},
        {{0, 3}, {1, 2}},
        {{1, 2}, {0, 3}},
    };
    constexpr int8_t tileSymmetryTable[TILE_MAX + 1][8] = {
        // タイルを反転回転
        {PW, PW, PW, PW, PR, PR, PR, PR},
        {PR, PR, PR, PR, PW, PW, PW, PW},
        {SW, BW, BR, SR, SW, BW, BR, SR},
        {SR, BR, BW, SW, SR, BR, BW, SW},
        {BW, SW, SR, BR, BR, SR, SW, BW},
        {BR, SR, SW, BW, BW, SW, SR, BR},
    };
    
    Color getEndColor(Tile t, int d){
        // タイルの端点の色を知る
        return static_cast<Color>(tileColorBitsTable[t].get(d));
    }
    Color getEndColor2D(Tile t, int dx, int dy){
        return getEndColor(t, areaToDirection(dx, dy));
    }
    int getOpposite(Tile t, int d){
        // 反対側の端点を返す
        return tileOppositeTable[t][d];
    }
    Tile getSymmetryTile(Tile t, int s){
        // タイルを反転や回転させる
        return static_cast<Tile>(tileSymmetryTable[t][s]);
    }
    
    // 着手タイル表現から文字列へ
    TileMove readTileMove(const char c){
        if(c == tileChar[TileMove::P]){
            return TileMove::P;
        }else if(c == tileChar[TileMove::S]){
            return TileMove::S;
        }else if(c == tileChar[TileMove::B]){
            return TileMove::B;
        }
        return TILEMOVE_NONE;
    }
    char toTileChar(Tile t){
        //cerr << t << endl;
        if(!examTile(t)){ return -1; }
        return tileChar[toTileMove(t)];
    }
    std::string toTileString(Tile t){
        //cerr << "tileChar = " << toTileChar(t) << endl;
        //cerr << "tileString = " << std::string(1, toTileChar(t)) << endl;
        return std::string(1, toTileChar(t));
    }
    
    // 座標
    int toRowInt(const std::string& r){
        //cerr<< r << endl;
        if(r.size() <= 0){ return -1; }
        int ret = 0;
        for(int i = 0; i < r.size(); ++i){
            if(r[i] < '0' || '9' < r[i]){
                return -1;
            }
            ret *= 10;
            ret += int(r[i] - '0');
        }
        return ret;
    }
    std::string toRowString(const int r){
        if(r < 0){ return ""; }
        std::ostringstream oss;
        oss << r;
        return oss.str();
    }
    
    // 以下、エクセル列番号問題を解く
    // 参考サイト
    // http://qiita.com/rana_kualu/items/6b8c74328233b8827a8b
    
    int toColumnInt(const std::string& c){
        if(c == ""){ return -1; }
        if(c == "@"){ return 0; }
        int ret = 0;
        for(int i = 0; i < c.size(); ++i){
            if(c[i] < 'A' || 'Z' < c[i]){
                return -1;
            }
            ret *= 26;
            ret += int(c[i] - 'A');
            ret += 1;
        }
        return ret;
    }
    
    std::string toColumnString(const int c){
        if(c < 0){
            return "";
        }else if(c == 0){
            return "@";
        }else{
            std::string ret = "";
            int t = c;
            do{
                int r = t % 26;
                t = t / 26;
                if(r == 0){
                    --t;
                    r = 26;
                }
                ret += char('A' - 1 + r);
            }while(t);
            return std::string(ret.rbegin(), ret.rend());
        }
    }
    
    std::string toNotationString(int x, int y, int lx, int ly){
        int nx = x - lx + 1;
        int ny = y - ly + 1;
        return toColumnString(ny) + toRowString(nx);
    }
    
    std::array<int, 2> readNotation(const std::string& str){
        if(str.size() < 2){ return kNullPosition; }
        // search number
        int rlast = 0;
        for(int i = 0; i < str.size(); ++i){
            if('0' <= str[i] && str[i] <= '9'){ // a number character
                rlast = i;
                break;
            }
        }
        //cerr << "rlast = " << rlast << endl;
        
        // column notation
        int column = toColumnInt(str.substr(0, rlast));
        //cerr << "column = " << column << endl;
        if(column < 0){ return kNullPosition; }
        
        // row notation
        int row = toRowInt(str.substr(rlast, str.size() - 1));
        //cerr << "row = " << row << endl;
        if(row < 0){ return kNullPosition; }
        
        std::array<int, 2> ret = {row, column};
        return ret;
    }
    
    /**************************エッジの接続状態**************************/
    
    enum EdgeConnection : uint8_t{
        FLAT = 0,
        OU = 1,
        TOTSU = 2,
        EDGECONNECTION_NONE = 255,
    };
    
    const char* edgeConnectionChar = "-v^";
    
    /**************************タイルのコンソール出力**************************/
    
    const std::string tilePatternString3x2[TILE_MAX + 1][2] = {
        { // Tile 0
            "\033[35m_\033[37m|\033[35m_",
            "\033[37m | ",
        },
        { // Tile 1
            "\033[37m_\033[35m|\033[37m_",
            "\033[35m | ",
        },
        { // Tile 2
            "\033[37m_/ ",
            "\033[35m  /",
        },
        { // Tile 3
            "\033[35m_/ ",
            "\033[37m  /",
        },
        { // Tile 4
            "\033[35m_\033[37m \\",
            "\033[35m \\ ",
        },
        { // Tile 5
            "\033[37m_\033[35m \\",
            "\033[37m \\ ",
        },
    };
    
    const std::string tilePatternString5x3[TILE_MAX + 1][3] = {
        { // Tile 0
            "\033[37m  |  ",
            "\033[35m--\033[37m|\033[35m--",
            "\033[37m  |  ",
        },
        { // Tile 1
            "\033[35m  |  ",
            "\033[37m--\033[35m|\033[37m--",
            "\033[35m  |  ",
        },
        { // Tile 2
            "\033[37m  |  ",
            "\033[37m-/ \033[35m/-",
            "\033[35m  |  ",
        },
        { // Tile 3
            "\033[35m  |  ",
            "\033[35m-/ \033[37m/-",
            "\033[37m  |  ",
        },
        { // Tile 4
            "\033[37m  |  ",
            "\033[35m-\\ \033[37m\\-",
            "\033[35m  |  ",
        },
        { // Tile 5
            "\033[35m  |  ",
            "\033[37m-\\ \033[35m\\-",
            "\033[37m  |  ",
        },
    };
    
    std::string drawTile(const Tile t){
        std::ostringstream oss;
        // タイルの模様を 2 x 3 で表示
        if(examTile(t)){
            for(int i = 0; i < 2; ++i){
                oss << "\x1b[1m\033[40m"
                << tilePatternString3x2[t][i]
                << "\033[39m\033[49m\x1b[0m" // fat line
                << endl;
            }
        }
        return oss.str();
    }
    
    
    std::string drawTiles(const std::vector<Tile>& tiles, const std::string& rstr){
        std::ostringstream oss;
        // タイルの模様を 2 x 3 で表示
        for(int i = 0; i < 2; ++i){
            if(i == 0){
                oss << rstr << Space(max(0, 2 - int(rstr.size())));
            }else{
                oss << "  ";
            }
            for(auto t : tiles){
                if(examTile(t)){
                    oss << "\x1b[1m\033[40m"
                    << tilePatternString3x2[t][i]
                    << "\033[39m\033[49m\x1b[0m"; // fat line
                    //oss << "\033[40m" << tilePatternString3x2[t][i] << "\033[39m\033[49m";
                }else{
                    oss << "   ";
                }
            }oss << endl;
        }
        return oss.str();
    }
    
    /**************************Evaluation**************************/
    
    constexpr int kMaxPly = 100;
    constexpr int kMaxTiles = N_TURNS * 3;
    
    // by 技巧
    enum Score{
        kScoreFoul      = -32400,
        kScoreZero      =      0,
        kScoreDraw      =      0,
        kScoreMaxEval   =  28000,
        kScoreAlmostWin =  30000,
        kScoreSuperior  =  30500,
        kScoreKnownWin  =  31000,
        kScoreMate      =  32000,
        kScoreInfinite  =  32600,
        kScoreNone      =  32601,
        kScoreMateInMaxPly  = +kScoreMate - kMaxPly,
        kScoreMatedInMaxPly = -kScoreMate + kMaxPly,
    };
    
    ENABLE_ARITHMETIC_OPERATORS(Score)
    
    /**************************着手の結果**************************/
    
    enum Rule{
        OUT_BOARD = -1,
        DOUBLE = -2,
        BAD_COLOR = -3,
        ISOLATED = -4,
        FIRST_RESTRICTION = -5,
        FORCED_BAD_COLOR = -6,
        
        // 勝利フラグは色(0, 1)分ビットシフトして使う
        LOOP = (1 << 0),
        VICTORY_LINE = (1 << 2),
        WON = LOOP | VICTORY_LINE,
    };
    
    Color whichWon(int result, Color lastTurnColor)noexcept{
        if(result > 0){
            if(result & (Rule::WON << lastTurnColor)){
                return lastTurnColor;
            }else{
                return flipColor(lastTurnColor);
            }
        }
        return COLOR_NONE;
    }
    
    std::string resultDescription(int result){
        if(result < 0){
            return "violation";
        }else if(result > 0){
            std::string ret = "";
            for(int c = 0; c < 2; ++c){
                std::string cstr;
                cstr = colorChar[c];
                if(result & (Rule::LOOP << c)){
                    ret += cstr + "-loop";
                }
                if(result & (Rule::VICTORY_LINE << c)){
                    ret += cstr + "-vline";
                }
            }
            return ret;
        }else{
            return "none";
        }
    }
    
    /**************************着手表現**************************/
    
    struct Move{
        // 32ビット
        BitArray32<8, 4> m_;
        
        void assignTile(int tl)noexcept{
            m_.assign(0, tl);
        }
        void assignZ(int z)noexcept{
            m_.assign(1, z);
        }
        
        void set(int az, int atile)noexcept{
            m_ = (atile | (az << 8));
        }
        void set(const Move& mv)noexcept{
            m_ = mv.m_;
        }
        
        std::string toString()const{
            std::ostringstream oss;
            oss << tile() << "(" << z() << ")";
            return oss.str();
        }
        
        constexpr bool operator==(const Move& rhs)const noexcept{
            return m_ == rhs.m_;
        }
        constexpr bool operator!=(const Move& rhs)const noexcept{
            return m_ != rhs.m_;
        }
        
        constexpr Tile tile()const noexcept{ return static_cast<Tile>(m_[0]); }
        int z()const noexcept{ return m_.at(1, 3); }
        
        constexpr bool is_real_move()const noexcept; // ルール上存在する着手である
        
        bool IsOk()const noexcept{ return true; }
        
        constexpr Move(): m_(){}
        constexpr Move(uint32_t a): m_(a){}
        constexpr Move(uint32_t az, uint8_t atile):
        m_(atile | (az << 8)){}
    };
    
    std::ostream& operator<<(std::ostream& ost, const Move& mv){
        ost << mv.toString();
        return ost;
    }
    
    constexpr Move kMoveNone = Move(0, 0); // 存在しない無効着手
    constexpr Move kMoveNull = Move(0, TILE_MAX + 1); // パスの代わり
    
    constexpr bool Move::is_real_move()const noexcept{
        return (*this) != kMoveNone && (*this) != kMoveNull;
    }
    
    struct MoveScore : public Move{
        // 着手のオーダリング用
        int32_t score;
        
        // 評価値順ソートのための比較
        bool operator<(const MoveScore& m) const { return score < m.score; }
        bool operator>(const MoveScore& m) const { return score > m.score; }
        
        // 同一性
        bool operator==(const MoveScore& m) const { return Move(*this) == m; }
        
        constexpr MoveScore(): Move(), score(){}
        constexpr MoveScore(Move m): Move(m), score(){}
        constexpr MoveScore(Move m, Score s): Move(m), score(s){}
    };
    struct MoveScoreDepth : public MoveScore{
        int depth;
    };
    
    struct RelativeMove{
        // 境界に対しての数え方が違うので Move と分ける
        // ノーテーションと同じ位置にしておく
        int x_, y_;
        Tile tile_;
        
        constexpr int x()const noexcept{ return x_; }
        constexpr int y()const noexcept{ return y_; }
        constexpr Tile tile()const noexcept{ return tile_; }
        
        std::string toString()const{
            return toNotationString(x(), y(), 1, 1) + toTileChar(tile());
        }
        
        // 同一性
        bool operator==(const RelativeMove& rhs)const noexcept{
            return (x_ == rhs.x_) && (y_ == rhs.y_) && (tile_ == rhs.tile_);
        }
        bool operator!=(const RelativeMove& rhs)const noexcept{
            return !((*this) == rhs);
        }
        
        constexpr RelativeMove(): x_(), y_(), tile_(){}
        constexpr RelativeMove(int ax, int ay, Tile at): x_(ax), y_(ay), tile_(at){}
    };
    
    constexpr RelativeMove kRelativeMoveNone = RelativeMove(0, 0, TILE_NONE); // 存在しない無効着手
    
    struct RelativeMoveScore : public RelativeMove{
        int score;
        
        constexpr RelativeMoveScore(): RelativeMove(), score(){}
        constexpr RelativeMoveScore(RelativeMove m, int s): RelativeMove(m), score(s){}
    };
    
    struct RelativeMoveBound : public RelativeMove{
        // 着手と盤面の最大座標情報(回転の際に必要)
        int mx_, my_;
        
        constexpr int mx()const noexcept{ return mx_; }
        constexpr int my()const noexcept{ return my_; }
        
        constexpr RelativeMoveBound(): RelativeMove(), mx_(), my_(){}
        constexpr RelativeMoveBound(const RelativeMove& armv, int amx, int amy):
        RelativeMove(armv), mx_(amx), my_(amy){}
        constexpr RelativeMoveBound(int ax, int ay, int amx, int amy, Tile at):
        RelativeMove(ax, ay, at), mx_(amx), my_(amy){}
        
    };
    
    /**************************タイルの周辺色状態**************************/
    
    using TileColorMove = BitArray32<8, 4>;
    
    class TileColor : public BitArray8<2, 4>{
    public:
        constexpr bool filled()const noexcept{
            return ((data() & 0b10101010) == 0b10101010);
        }
        bool holds(TileColor c)const noexcept{
            ASSERT(filled(), cerr << "only filled pattern will use this method." << endl;);
            std::uint8_t mask = c.data() & 0b10101010;
            mask >>= 1;
            return ((data() & mask) == (c.data() & mask));
        }
        
        void setRawColor(size_t d, Color c)noexcept{
            set(d, 2 | c);
        }
        
        // TODO: ここらの計算の簡略化のために4ビットで情報を分ける形式にして大丈夫かチェック
        BitSet8 tileBits()const noexcept{
            uint8_t d = data();
            return ((d >> 1) & 1) | ((d >> 2) & 2) | ((d >> 3) & 4) | ((d >> 4) & 8);
        }
        BitSet8 colorBits()const noexcept{
            uint8_t d = data();
            return ((d >> 0) & 1) | ((d >> 1) & 2) | ((d >> 2) & 4) | ((d >> 3) & 8);
        }
        
        constexpr TileColor():
        BitArray8<2, 4>(){}
        
        constexpr TileColor(std::uint8_t a):
        BitArray8<2, 4>(a){}
    };
    
    constexpr TileColor tileColorTable[TILE_MAX + 1] = {
        std::uint8_t(0b11101110U),
        std::uint8_t(0b10111011U),
        std::uint8_t(0b11111010U),
        std::uint8_t(0b10101111U),
        std::uint8_t(0b10111110U),
        std::uint8_t(0b11101011U),
    };
    
    Tile colorTileTable[256]; // TileColor -> Tile
    Tile forcedTileTable[256]; // TileColor -> Tile (forced)
    TileColor forcedTable[256]; // TileColor -> TileColor (forced)
    TileColorMove moveTable[256]; // TileColor -> puttable TileColor Array (x4)
    BitArray32<4, 4> tileMoveTable[256]; // TileColor -> puttable Tile Array(x4)
    BitSet8 tileMoveBitTable[256]; // TileColor -> puttable Tile-Number Bits
    TileColor endForcedTileColorTable[2][4][4]; // both ends -> forced Tile Color
    Tile endForcedTileTable[2][4][4]; // both ends -> forced Tile Color
    //char colorTileTable[256];
    
    void initTileColorTable(){
        // initialize tables
        for(int i = 0; i < 256; ++i){
            
            colorTileTable[i] = TILE_NONE;
            forcedTileTable[i] = TILE_NONE;
            forcedTable[i] = 0;
            moveTable[i] = 0;
            tileMoveTable[i].clear();
            tileMoveBitTable[i].reset();
            
            TileColor c = TileColor(i);
            
            //cerr << i << " " << c << endl;
            
            int white = 0;
            int red = 0;
            for(int d = 0; d < 4; ++d){
                if(c[d] & 2){
                    if(c[d] & 1){
                        red += 1;
                    }else{
                        white += 1;
                    }
                }else if(c[d] & 1){ // impossible
                    continue;
                }
            }
            if(red == 0 && white == 0){ // isolated
                forcedTileTable[i] = TILE_NONE;
                forcedTable[i] = 0;
                // set first moves (only 2 moves are legal)
                moveTable[i].assign(0, tileColorTable[0]);
                moveTable[i].assign(1, tileColorTable[2]);
                
                tileMoveBitTable[i].set(0);
                tileMoveBitTable[i].set(1);
                
            }else if(red >= 3 || white >= 3){ // illegal
                forcedTileTable[i] = TILE_NONE;
                forcedTable[i] = 0;
            }else{
                int validTiles = 0;
                TileColor forced = 0;
                Tile forcedTile;
                for(int t = TILE_MIN; t <= TILE_MAX; ++t){
                    if(tileColorTable[t].holds(c)){
                        moveTable[i].assign(validTiles, tileColorTable[t]);
                        tileMoveBitTable[i].set(t);
                        validTiles += 1;
                        forced = tileColorTable[t];
                        forcedTile = static_cast<Tile>(t);
                    }
                }
                if(validTiles == 1){
                    forcedTileTable[i] = forcedTile;
                    forcedTable[i] = forced;
                }else{ // several valid tiles
                    forcedTileTable[i] = static_cast<Tile>(N_TILES);
                    forcedTable[i] = c;
                }
            }
        }
        
        // initialize endForcedTileTable, endForcedTileColorTable
        for(int c = 0; c < 2; ++c){
            for(int i = 0; i < 4; ++i){
                for(int j = 0; j < 4; ++j){
                    endForcedTileTable[c][i][j] = TILE_NONE;
                    endForcedTileColorTable[c][i][j] = TileColor(0);
                }
            }
        }
        for(int t = TILE_MIN; t <= TILE_MAX; ++t){
            TileColor tc = tileColorTable[t];
            for(int c = 0; c < 2; ++c){
                int e0 = tileConnectTable[t][c][0];
                int e1 = tileConnectTable[t][c][1];
                
                endForcedTileTable[c][e0][e1] = static_cast<Tile>(t);
                endForcedTileTable[c][e1][e0] = static_cast<Tile>(t);
                
                endForcedTileColorTable[c][e0][e1] = tc;
                endForcedTileColorTable[c][e1][e0] = tc;
            }
        }
    }
    
    /**************************タイルの座標の上端,下端表現(ノーテーション扱いとビクトリーライン判定のため)**************************/
    
    struct TileBound{
        std::array<int, 4> b_;
        
        int lx()const noexcept{ return std::get<0>(b_); }
        int ly()const noexcept{ return std::get<1>(b_); }
        int hx()const noexcept{ return std::get<2>(b_); }
        int hy()const noexcept{ return std::get<3>(b_); }
        
        int& lx()noexcept{ return std::get<0>(b_); }
        int& ly()noexcept{ return std::get<1>(b_); }
        int& hx()noexcept{ return std::get<2>(b_); }
        int& hy()noexcept{ return std::get<3>(b_); }
        
        int l(int dim)const noexcept{ return b_[dim]; }
        int h(int dim)const noexcept{ return b_[2 + dim]; }
        
        void updateLX()noexcept{ std::get<0>(b_) -= 1; }
        void updateLY()noexcept{ std::get<1>(b_) -= 1; }
        void updateHX()noexcept{ std::get<2>(b_) += 1; }
        void updateHY()noexcept{ std::get<3>(b_) += 1; }
        
        void update(unsigned int x, unsigned int y){
            if(x < lx()){
                lx() = x;
            }else if(x > hx()){
                hx() = x;
            }
            if(y < ly()){
                ly() = y;
            }else if(y > hy()){
                hy() = y;
            }
        }
        
        /*void update(unsigned int x, unsigned int y){
         if(x < lx()){
         updateLX(x);
         }else if(x > hx()){
         updateHX(x);
         }
         if(y < ly()){
         updateLY(y);
         }else if(y > hy()){
         updateHY(y);
         }
         }*/
        
        int dx()const noexcept{ return hx() - lx(); }
        int dy()const noexcept{ return hy() - ly(); }
        
        int d(int dim)const noexcept{ return h(dim) - l(dim); }
        
        bool operator==(const TileBound& rhs)const noexcept{
            return b_ == rhs.b_;
        }
        bool operator!=(const TileBound& rhs)const noexcept{
            return !((*this) == rhs);
        }
        void set(int bx, int by){
            std::get<0>(b_) = bx;
            std::get<1>(b_) = by;
            std::get<2>(b_) = bx - 1;
            std::get<3>(b_) = by - 1;
        }
        
        std::string toString()const{
            std::stringstream oss;
            oss << "x = [" << lx() << ", " << hx() << "] y = [" << ly() << ", " << hy() << "]";
            return oss.str();
        }
        
        TileBound(){}
        TileBound(int v){
            std::get<0>(b_) = v;
            std::get<1>(b_) = v;
            std::get<2>(b_) = v;
            std::get<3>(b_) = v;
        }
        TileBound(int bx, int by){
            set(bx, by);
        }
    };
    
    std::ostream& operator<<(std::ostream& ost, const TileBound& rhs){
        ost << rhs.toString();
        return ost;
    }
    
    /**************************盤面表現定数**************************/
    
    constexpr int SIZE = 128;
    constexpr int MARGIN = 2;
    
    constexpr int ar4[4] = { -SIZE, -1, +SIZE, +1, };
    constexpr int ar4_2d[4][2] = { {-1, 0}, {0, -1}, {+1, 0}, {0, +1} };
    
    constexpr int ZtoX(unsigned int z)noexcept{ return z / SIZE; }
    constexpr int ZtoY(unsigned int z)noexcept{ return z % SIZE; }
    constexpr int XYtoZ(unsigned int x, unsigned int y)noexcept{ return x * SIZE + y; }
    
    constexpr int Z_FIRST = XYtoZ(SIZE / 2 - 1, SIZE / 2 - 1);
    
    struct XY{
        int z;
        XY(int az):
        z(az){}
        XY(int ax, int ay):
        z(XYtoZ(ax, ay)){}
        XY(const std::array<int, 2>& a):
        z(XYtoZ(a[0], a[1])){}
    };
    
    std::ostream& operator<<(std::ostream& ost, const XY& xy){
        ost << "(" << ZtoX(xy.z) << ", " << ZtoY(xy.z) << ")";
        return ost;
    }
    
    uint64_t tileHashTable[SIZE * SIZE][TILE_MAX + 1];
    
    void initHashTable(){
        //XorShift64 dice(71);
        std::mt19937 dice(71);
        for(int i = 0; i < SIZE * SIZE; ++i){
            for(int j = 0; j <= TILE_MAX; ++j){
                //tileHashTable[i][j] = dice.rand();
                tileHashTable[i][j] = (uint64_t(dice()) << 32) | dice(); // mt19937が32ビット...
            }
        }
    }
    
    int connectedEnd(int baseZ, int dstZ)noexcept{
        // baseZ と dstZ が近接関係にあるとき
        // dstZ が baseZ のどのエンドと接続するか
        switch(dstZ - baseZ){
            case -SIZE: return 0; break;
            case -1: return 1; break;
            case +1: return 2; break;
            case +SIZE: return 3; break;
            default: UNREACHABLE; break;
        }
        UNREACHABLE;
        return -1;
    }
    
    /**************************相対着手<->絶対着手変換**************************/
    
    template<class board_t>
    Move toMove(const RelativeMove& rmv, const board_t& bd){
        return Move(XYtoZ(bd.lx() - 1 + rmv.x(), bd.ly() - 1 + rmv.y()), rmv.tile());
    }
    
    template<class board_t>
    RelativeMove toRelativeMove(const Move& mv, const board_t& bd){
        return RelativeMove(ZtoX(mv.z()) - bd.lx() + 1, ZtoY(mv.z()) - bd.ly() + 1, mv.tile());
    }
    
    /**************************対称性の考慮**************************/
    
    template<class board_t>
    uint64_t calcRepRelativeHash(const board_t& bd, const Color color, int& pattern){
        // 定跡のため、平行移動と回転を考慮したハッシュ値を計算
        // どちらの手番かの情報は一応後でも入れられるようにしておく
        // TODO: タイルの回転処理にテーブル参照が入るので他の方法の方がいいかも?
        uint64_t rhash[8] = {0};
        const int dx = bd.dx(), dy = bd.dy();
        for(int i = 0; i <= dx; ++i){
            for(int j = 0; j <= dy; ++j){
                int x = bd.lx() + i;
                int y = bd.ly() + j;
                Tile tile = bd.tile(XYtoZ(x, y));
                if(isTile(tile)){ // 空でないときのみ
                    // ノーテーションの位置と合わせて対称型をとる
                    iterateSymmetries(i + 1, j + 1, dx + 2, dy + 2,
                                      [tile, &rhash](int s, int u, int v)->void{
                                          // sが対称型のタイプ
                                          // u, vが新しい座標
                                          DERR << s << " (" << u << ", " << v << ") ";
                                          // 盤面の最大サイズによってハッシュ値がかわらないように一次元化
                                          int index = cartesian(u, v);
                                          // 反転回転させたタイル
                                          Tile t = getSymmetryTile(tile, s);
                                          DERR << t << endl;
                                          rhash[s] ^= tileHashTable[index][t];
                                      });
                }
            }
        }
        for(int s = 0; s < 8; ++s){
            DERR << rhash[s] << " ";
        }DERR << endl;
        
        // 最小のハッシュ値を代表として採用する
        // このときどの対称パターンが採用されたか返す(定跡データから戻すときに必要)
        pattern = 0;
        uint64_t repHash = rhash[0];
        
        for(int s = 1; s < 8; ++s){
            if(rhash[s] < repHash){
                repHash = rhash[s];
                pattern = s;
            }
        }
        return (repHash & (~1ULL)) | bd.turnColor(); // 色情報をまぜる
    }
    template<class board_t>
    uint64_t calcRepRelativeHash(const board_t& bd, int& pattern){
        return calcRepRelativeHash(bd, bd.turnColor(), pattern);
    }
    
    RelativeMoveBound symmetryTransform(const RelativeMoveBound& mv, const int pattern){
        // 対称変換
        ASSERT(0 <= pattern && pattern < 8, cerr << pattern << endl;);
        Tile st = getSymmetryTile(mv.tile(), pattern);
        int x = mv.x(), y = mv.y(), mx = mv.mx(), my = mv.my();
        switch(pattern){
            case 0: return RelativeMoveBound(     x,      y, mx, my, st); break;
            case 1: return RelativeMoveBound(     x, my - y, mx, my, st); break;
            case 2: return RelativeMoveBound(mx - x,      y, mx, my, st); break;
            case 3: return RelativeMoveBound(mx - x, my - y, mx, my, st); break;
            case 4: return RelativeMoveBound(     y,      x, my, mx, st); break;
            case 5: return RelativeMoveBound(     y, mx - x, my, mx, st); break;
            case 6: return RelativeMoveBound(my - y,      x, my, mx, st); break;
            case 7: return RelativeMoveBound(my - y, mx - x, my, mx, st); break;
            default: ASSERT(pattern, cerr << pattern << endl;); break;
        }
        return RelativeMoveBound(kRelativeMoveNone, 0, 0);
    }
    RelativeMoveBound invSymmetryTransform(const RelativeMoveBound& mv, const int pattern){
        // 対称変換の逆変換
        ASSERT(0 <= pattern && pattern < 8, cerr << pattern << endl;);
        return symmetryTransform(mv, invSymmetryTable[pattern]);
    }
    
    /**************************ノーテーション読み**************************/
    
    RelativeMove readRelativeMoveNotation(const std::string& str){
        // expanded trax notation の形で読む
        if(str.size() < 4){ return kRelativeMoveNone; }
        // search number
        int rlast = 0;
        for(int i = 0; i < str.size(); ++i){
            if('0' <= str[i] && str[i] <= '9'){ // a number character
                rlast = i;
                break;
            }
        }
        // column notation
        int column = toColumnInt(str.substr(0, rlast - 0));
        if(column < 0){ return kRelativeMoveNone; }
        // row notation
        int row = toRowInt(str.substr(rlast, str.size() - 2 - rlast));
        if(row < 0){ return kRelativeMoveNone; }
        // tilemove notation
        TileMove tm = readTileMove(str[str.size() - 2]);
        if(!examTileMove(tm)){ return kRelativeMoveNone; }
        // color notation
        Color c = readColor(str[str.size() - 1]);
        if(!examColor(c)){ return kRelativeMoveNone; }
        
        return RelativeMove(row, column, toTile(tm, c));
    }
    
    template<class board_t>
    Move readMoveNotation(const std::string& str, const board_t& bd){
        if(str.size() < 3){ return kMoveNone; }
        // search number
        int rlast = 0;
        for(int i = 0; i < str.size(); ++i){
            if('0' <= str[i] && str[i] <= '9'){ // a number character
                rlast = i;
                break;
            }
        }
        //cerr << "rlast = " << rlast << endl;
        
        // column notation
        int column = toColumnInt(str.substr(0, rlast - 0));
        //cerr << str.substr(0, rlast) << endl;
        //cerr << "column = " << column << endl;
        if(column < 0){ return kMoveNone; }
        
        // row notation
        //cerr << str << str.size() << endl;
        int row = toRowInt(str.substr(rlast, str.size() - 1 - rlast));
        //cerr << "row = " << row << endl;
        if(row < 0){ return kMoveNone; }
        
        int z = bd.toBoardZ(row, column);
        
        // tile notation
        TileMove tm = readTileMove(str[str.size() - 1]);
        //cerr << "tm = " << tm << endl;
        if(!examTileMove(tm)){ return kMoveNone; }
        
        return Move(z, bd.whichTile(z, tm));
    }
    
    template<class board_t>
    std::string toNotationString(const Move& mv, const board_t& bd){
        return toNotationString(ZtoX(mv.z()), ZtoY(mv.z()), bd.lx(), bd.ly()) + toTileString(mv.tile());
    }
    std::string toNotationString(const RelativeMove& mv){
        return toNotationString(mv.x(), mv.y(), 1, 1) + toTileString(mv.tile());
    }
    
    // 拡張ノーテーション(タイルの色付き)への変換
    template<class board_t>
    std::string toExpandedNotationString(const Move& mv, const board_t& bd){
        return toNotationString(ZtoX(mv.z()), ZtoY(mv.z()), bd.lx(), bd.ly()) + toTileString(mv.tile()) + toColorString(toTopColor(mv.tile()));
    }
    std::string toExpandedNotationString(const RelativeMove& mv){
        return toNotationString(mv.x(), mv.y() , 1, 1) + toTileString(mv.tile()) + toColorString(toTopColor(mv.tile()));
    }
    
    template<class moves_t, class board_t>
    std::vector<std::string> toNotationStrings(const moves_t& moves, board_t& bd){
        // moveの列をノーテーション列に変換
        // TODO: 途中で非合法になった場合の表示はどうするの
        std::vector<std::string> v;
        const int originalTurn = bd.turn;
        for(const auto& mv : moves){
            v.emplace_back(toNotationString(mv, bd));
            if(bd.makeMove(mv) < 0){
                break;
            }
        }
        bd.unmakeMove(originalTurn); // 元のターンまで戻す
        return v;
    }
    
    /**************************スタッツ**************************/
    
    template<typename T>
    struct Stats{
        
        // tableの要素の値を取り出す
        const T* operator[](int z)const{
            return table[z];
        }
        T* operator[](int z){
            return table[z];
        }
        
        // tableのclear
        void clear() { memset(table, 0, sizeof(table)); }
        
        T get(Move move)const{
            return table[move.z()][move.tile()];
        }
   
        void update(Move move, Score v){
            v = max((Score)-324, v);
            v = min((Score)+324, v);
            int z = move.z();
            Tile tile = move.tile();
            table[z][tile] -= table[z][tile] * abs(int(v)) / 324;
            table[z][tile] += int(v) * 32;
        }
    private:
        T table[SIZE * SIZE][TILE_MAX + 1]; // 座標 x 置くタイル
    };
    
    struct CounterMoveStats{
        
        // tableのclear
        void clear(){ memset(table, 0, sizeof(table)); }
        
        void update(Move oppMove, Move mv){
            table[oppMove.z()][oppMove.tile()] = mv;
        }
        
        Move get(Move oppMove)const{
            return table[oppMove.z()][oppMove.tile()];
        }
        
        CounterMoveStats(){
            clear();
        }
        
    private:
        Move table[SIZE * SIZE][TILE_MAX + 1]; // 座標 x 置くタイル
    };
    
    /**************************Initialization**************************/
    
    void initTrax()noexcept{
        initTileColorTable();
        initHashTable();
    }
}

#endif // TRAX_TRAX_HPP_