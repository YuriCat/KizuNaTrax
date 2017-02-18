/*
 board_elements.hpp
 Katsuki Ohto
 */

#ifndef TRAX_BOARDELEMENTS_HPP_
#define TRAX_BOARDELEMENTS_HPP_

#include "trax.hpp"

// 盤面表現に用いるパーツ

namespace Trax{
    
    /**************************線へのリンク**************************/
    
    /*class LineRef{
     public:
     void set(unsigned int l, unsigned int e){
     v = std::make_tuple(l, e);
     }
     void setIndex(unsigned int l)noexcept{
     std::get<0>(v) = l;
     }
     void setEnd(unsigned int e)noexcept{
     std::get<1>(v) = e;
     }
     int index()const noexcept{
     return std::get<0>(v);
     }
     int end()const noexcept{
     return std::get<1>(v);
     }
     void clear()noexcept{
     v = std::make_tuple(0, 0);
     }
     
     std::string toString()const{
     std::stringstream oss;
     oss << index() << "-" << end();
     return oss.str();
     }
     
     LineRef():
     v(){}
     
     LineRef(unsigned int al, unsigned int ae):
     v(std::make_tuple(al, ae)){}
     
     bool operator==(const LineRef& rhs)const noexcept{
     return index() == rhs.index() || end()== rhs.end();
     }
     bool operator!=(const LineRef& rhs)const noexcept{
     return !((*this) == rhs);
     }
     
     private:
     std::tuple<unsigned int, unsigned int> v;
     };
     
     std::ostream& operator<<(std::ostream& ost, const LineRef& lr){
     ost << lr.toString();
     return ost;
     }*/
    
    /**************************エッジの情報**************************/
    
    struct EdgeInfo{
        //LineRef lRef; // このエッジを端点とする線の情報
        short lineIndex_; // 線のインデックス
        uint8_t lineEnd_; // 線のどちらの端点か
        uint8_t age_; // このエッジが出来たターン
        int last_, next_; // 前と次のエッジインデックス
        EdgeConnection connection_; // 次のエッジとの接続関係
        
    public:
        int lineIndex()const noexcept{
            return lineIndex_;
        }
        uint8_t lineEnd()const noexcept{
            return lineEnd_;
        }
        EdgeConnection connection()const noexcept{
            return connection_;
        }
        uint8_t age()const noexcept{
            return age_;
        }
        int last()const noexcept{
            return last_;
        }
        int next()const noexcept{
            return next_;
        }
        
        
        void setLine(unsigned int l, unsigned int e){
            setLineIndex(l);
            setLineEnd(e);
        }
        void setLineIndex(unsigned int l)noexcept{
            lineIndex_ = l;
        }
        void setLineEnd(unsigned int e)noexcept{
            lineEnd_ = e;
        }
        void clearLine()noexcept{
            lineIndex_ = 0;
            lineEnd_ = 0;
        }
        void setLast(int le)noexcept{
            last_ = le;
        }
        void setNext(int ne)noexcept{
            next_ = ne;
        }
        void setAge(uint8_t a)noexcept{
            age_ = a;
        }
        void setConnection(EdgeConnection c)noexcept{
            connection_ = c;
        }
        
        void clear(){
            lineIndex_ = 0;
            lineEnd_ = 0;
            age_ = 0;
            last_ = next_ = 0;
            connection_ = EDGECONNECTION_NONE;
        }
        
        bool operator==(const EdgeInfo& rhs)const noexcept{
            return lineIndex() == rhs.lineIndex() || lineEnd()== rhs.lineEnd();
        }
        bool operator!=(const EdgeInfo& rhs)const noexcept{
            return !((*this) == rhs);
        }
        
        std::string toString()const{
            std::stringstream oss;
            oss << "L:" << char('a' + (lineIndex() % 26)) << "(" << (int)lineEnd() << ")" << "[" << (int)age() << "]";
            return oss.str();
        }
        
        EdgeInfo(){}
        EdgeInfo(int v):
        lineIndex_(0), lineEnd_(0),
        age_(0), last_(0), next_(0),
        connection_(EDGECONNECTION_NONE){}
        
        //EdgeInfo(unsigned int al, unsigned int ae):
        //v(std::make_tuple(al, ae)){}
        
        /*bool operator==(const EdgeInfo& rhs)const noexcept{
         return index() == rhs.index() || end()== rhs.end();
         }
         bool operator!=(const EdgeInfo& rhs)const noexcept{
         return !((*this) == rhs);
         }*/
    };
    
    std::ostream& operator<<(std::ostream& ost, const EdgeInfo& ei){
        ost << ei.toString();
        return ost;
    }
    
    /**************************線の情報**************************/
    
    namespace LineFlag{
        enum{
            AGE = 32,
            
            COLOR = 40,
            
            // エンド間のマンハッタン距離(0, 1, 2, 3以上)
            FRONT_END_DISTANCE_X = 44,
            FRONT_END_DISTANCE_Y = 46,
            BACK_END_DISTANCE_X = 48,
            BACK_END_DISTANCE_Y = 50,
            
            CORNER_DIRECTION = 54, // どちらの角を向いているか
            
            FLAG_MIN = 58,
            
            ATTACK_LOOP = 58,
            ATTACK_VLINE_SINGLE = 59,
            
            LOOP = 62,
            VLINE = 63,
        };
        
        constexpr uint64_t DISTANCE_1_1_CORNER =
        (1ULL << FRONT_END_DISTANCE_X)
        | (1ULL << FRONT_END_DISTANCE_Y);
    }
    
    template<int kSize>
    class LineInfo : public BitArray64<1>{
        // 端点座標 16 bits * 2
        // 世代 8 bits
        // 色(必要?) 1 bit
        // ループフラグ 1 bit
        // ビクトリーラインフラグ 1 bit
    public:
        /*void setEnd(unsigned int e, unsigned int z, unsigned int d){
            assign(e * 16, (x * kSize + y) * 4 + d, 16);
        }*/
        
        
        void setEnd(unsigned int e, unsigned int zd){
            set(e * 16, zd);
        }
        void assignEnd(unsigned int e, unsigned int zd){
            assign(e * 16, zd, 16);
        }
        
        unsigned int xyd(unsigned int e)const noexcept{
            return at(e * 16, 16);
        }
        
        unsigned int xy(unsigned int e)const noexcept{
            return xyd(e) / 4;
        }
        int x(unsigned int e)const{
            return xyd(e) / (4 * kSize);
        }
        int y(unsigned int e)const{
            return (xyd(e) / 4) % kSize;
        }
        int d(unsigned int e)const{
            return xyd(e) % 4;
        }
        
        int bx(unsigned int e)const{
            return x(e) + ar4_2d[d(e)][0];
        }
        int by(unsigned int e)const{
            return y(e) + ar4_2d[d(e)][1];
        }
        
        unsigned int dx()const{
            return abs(x(1) - x(0));
        }
        unsigned int dy()const{
            return abs(y(1) - y(0));
        }
        
        unsigned int dbx()const{
            return abs(bx(1) - bx(0));
        }
        unsigned int dby()const{
            return abs(by(1) - by(0));
        }
        
        void setColor(uint64_t c)noexcept{
            // color won't change
            set(LineFlag::COLOR, c);
        }
        Color color()const noexcept{
            return static_cast<Color>(at(LineFlag::COLOR));
        }
        uint8_t age()const noexcept{
            return uint8_t((*this) >> LineFlag::AGE);
        }
        /*void setAge(uint8_t a)noexcept{
         (*this) |= (uint64_t(a) << 48);
         }*/
        void setAge(uint64_t a)noexcept{ // age would change?
            set(LineFlag::AGE, a);
        }
        void assignAge(uint64_t a)noexcept{ // age would change?
            assign(LineFlag::AGE, a, 8);
        }
        
        void setLoop()noexcept{
            set(LineFlag::LOOP, 1);
        }
        void setVictoryLine()noexcept{
            set(LineFlag::VLINE, 1);
        }
        
        void setFrontEndDistanceX(unsigned int dis)noexcept{
            assign(LineFlag::FRONT_END_DISTANCE_X, min(3U, dis), 2);
        }
        void setFrontEndDistanceY(unsigned int dis)noexcept{
            assign(LineFlag::FRONT_END_DISTANCE_Y, min(3U, dis), 2);
        }
        void setBackEndDistanceX(unsigned int dis)noexcept{
            assign(LineFlag::BACK_END_DISTANCE_X, min(3U, dis), 2);
        }
        void setBackEndDistanceY(unsigned int dis)noexcept{
            assign(LineFlag::BACK_END_DISTANCE_Y, min(3U, dis), 2);
        }
        
        void setShape(){
            //setFrontEndDistanceX(dx());
            //setFrontEndDistanceY(dy());
            //setBackEndDistanceX(dbx());
            //setBackEndDistanceY(dby());
            assign(LineFlag::FRONT_END_DISTANCE_X,
                   (min(3U, dx()) << 0) | (min(3U, dy()) << 2) | (min(3U, dbx()) << 4) | (min(3U, dby()) << 6),
                   8);
        }
        /*void setNewShape(Tile tile){
            //setFrontEndDistanceX(dx());
            //setFrontEndDistanceY(dy());
            //setBackEndDistanceX(dbx());
            //setBackEndDistanceY(dby());
            if(!isPlusTile(tile)){
                assign(LineFlag::FRONT_END_DISTANCE_X, (1 << 0) | (1 << 2) | (0 << 4) | (0 << 6), 8);
            }else if(tile == Tile::PW){
                assign(LineFlag::FRONT_END_DISTANCE_X, (1 << 0) | (1 << 2) | (0 << 4) | (0 << 6), 8);
            }else{
                assign(LineFlag::FRONT_END_DISTANCE_X, (1 << 0) | (1 << 2) | (0 << 4) | (0 << 6), 8);
            }
        }*/
        
        uint8_t frontShape()const noexcept{
            return at(LineFlag::FRONT_END_DISTANCE_X, 4);
        }
        uint8_t backShape()const noexcept{
            return at(LineFlag::BACK_END_DISTANCE_X, 4);
        }
        uint8_t shape()const noexcept{
            return at(LineFlag::FRONT_END_DISTANCE_X, 8);
        }
        
        void clearFlags()noexcept{
            (*this) &= (1ULL << LineFlag::FLAG_MIN) - 1ULL;
        }
        
        uint64_t loop()const noexcept{
            return any(LineFlag::LOOP);
        }
        uint64_t victoryLine()const noexcept{
            return any(LineFlag::VLINE);
        }
        uint64_t mate()const noexcept{
            return get_part(LineFlag::LOOP, 2);
        }
        
        bool is11Corner()const noexcept{
            return get_part(LineFlag::FRONT_END_DISTANCE_X, 8) == LineFlag::DISTANCE_1_1_CORNER;
        }
        
        LineInfo():
        BitArray64<1>(){}
        
        LineInfo(uint64_t ai):
        BitArray64<1>(ai){}
        
        std::string toString()const{
            std::ostringstream oss;
            oss << colorChar[color()]
            <<"(" << x(0) << ", " << y(0) << ", " << d(0) << ")-("
            << x(1) << ", " << y(1) << ", " << d(1) << ")";
            return oss.str();
        }
        std::string toShapeString()const{
            std::ostringstream oss;
            oss << BitArray8<2>(at(LineFlag::FRONT_END_DISTANCE_X, 8));
            return oss.str();
        }
        std::string toNotationString(int lx, int ly)const{
            std::ostringstream oss;
            oss << "[" << colorChar[color()] << "," << (int)age() << "] ";
            for(int e = 0; e < 2; ++e){
                oss << Trax::toNotationString(x(e), y(e), lx, ly) << "(" << d(e) << ")";
                if(e == 0){ oss << "-"; }
            }
            
            return oss.str() + " " + toShapeString() + toInfoString();
        }
        std::string toInfoString()const{
            std::ostringstream oss;
            if(loop()){
                oss << " -loop";
            }
            if(victoryLine()){
                oss << " -vline";
            }
            return oss.str();
        }
    };
    
    /**************************タイルを置く際の情報**************************/
    
    struct MoveInfo{
        // タイルを1つ置くときに保存するデータ
        // 不可逆的に変化する盤面情報を保存しておく必要がある
        int z; // 置いた座標
        TileColor last; // 置いたマスの元々のTileColor
        Tile tile; // 新しいマスのTileColor
        std::array<std::array<uint8_t, 2>, 2> lineAge; // 更新された線の元の世代(不可逆変化のためここに記録)
        
        MoveInfo(){}
        
        MoveInfo(int i):
        z(i), last(0), tile(TILE_NONE){
            //lineAge.fill({0, 0});
        }
        
        bool operator==(const MoveInfo& rhs)const noexcept{
            return z == rhs.z
            && last == rhs.last
            && tile == rhs.tile;
        }
        bool operator!=(const MoveInfo& rhs)const noexcept{
            return !((*this) == rhs);
        }
        
        std::string toString()const{
            std::stringstream oss;
            oss << tile << " in " << z;
            return oss.str();
        }
    };
    
    std::ostream& operator<<(std::ostream& ost, const MoveInfo& mi){
        ost << mi.toString();
        return ost;
    }
    
    struct TurnInfo{
        // 1手(連鎖ルールにより複数タイルが置かれることもある)の情報を保存するデータ
        
        int moveIndex; // この手のmoveの最初のインデックス
        int lineIndex; // この世代のlineの最初のインデックス
        
        //int attacks; // 作ったアタックの数
        TileBound bound; // タイルのある範囲(forced moveによって変化することはないのでターンごとの記録でよい)
        //std::array<BitSet64, (N_TURNS + 63) / 64> changedLineSet; // 変化のあった線のビットセット
        //std::array<int, 2> lineShapeScore; // 線割
        
        bool operator==(const TurnInfo& rhs)const noexcept{
            return moveIndex == rhs.moveIndex
            && bound == rhs.bound
            && lineIndex == rhs.lineIndex;
            //&& lineShapeScore == rhs.lineShapeScore;
        }
        bool operator!=(const TurnInfo& rhs)const noexcept{
            return !((*this) == rhs);
        }
        
        /*void setChangedLine(int l){
            changedLineSet[l / 64].set(l % 64);
        }*/
        
        template<class board_t>
        void setInfoBeforeMake(const board_t& bd)noexcept{
            // makeMoveの前に保存しておく情報のセットå
            bound = bd.bound;
        }
        template<class board_t>
        void setInfoAfterMake(const board_t& bd)noexcept{
            // makeMoveの後に保存しておく情報のセット
            moveIndex = bd.moves;
            lineIndex = bd.lines;
        }
        template<class board_t>
        void loadInfoBeforeMake(board_t *const pbd)noexcept{
            // makeMoveの前に保存した情報やらを復帰させる
            pbd->moves = moveIndex;
            //pbd->lines = lineIndex;
            pbd->bound = bound;
        }
        
        
        TurnInfo(){}
        
        TurnInfo(int i):
        moveIndex(0), lineIndex(0), bound(0){}
        
        std::string toString()const{
            std::stringstream oss;
            oss << "mi = " << moveIndex;
            return oss.str();
        }
    };
    
    std::ostream& operator<<(std::ostream& ost, const TurnInfo& mi){
        ost << mi.toString();
        return ost;
    }
    
    struct TileCell{
        TileColor tc;
        Tile tile;
        
        TileCell():
        tc(0), tile(TILE_NONE){}
    };
    
    struct AttackInfo{
        int l; // 線番号
        int type; // 種類
        void set(int al, int atype)noexcept{
            l = al;
            type = atype;
        }
        bool loop()const noexcept{
            return type <= 3;
        }
        
        AttackInfo():
        l(-1), type(0){}
    };
    
    /*struct ThreatInfo{
     int type; // 種類
     void set(int al, int atype)noexcept{
     l = al;
     type = atype;
     }
     bool loop()const noexcept{
     return type <= 3;
     }
     
     ThreatInfo():
     l(-1), type(0){}
     };*/
    
    /**************************ループが狙えそうな角型の線の情報**************************/
    
    struct LoopThreatLine{
        BitSet8 direction; // アタックを形成しうる方向(4ビット)
        std::array<uint8_t, 4> distance; // 距離
        int lineIndex;
        
        template<class lineInfo_t>
        void setLine(int l, const lineInfo_t& lineInfo){
            lineIndex = l;
            // アタックを形成しうる方向を調べる
            
        }
    };
    
    /**************************複数の線同士の情報**************************/
    
    struct TwoLinesInfo{
        int endDistance[2][2]; // エンド間のマンハッタン距離

    };
    
    /**************************直線の色の並び情報**************************/
    
    struct RangedStraight : public BitArray64<2>{
        
        uint64_t TILE_MASK = 0xaaaaaaaaaaaaaaaa;
        uint64_t COLOR_MASK = 0x5555555555555555;
        
        // 判定
        bool isAlternate()const noexcept{
            // 赤白が交互に並んでいるかどうか
            // ビット列を1マス分ずらして and をとって(端以外)0になればOK
            //cerr << *this << endl;
            uint64_t b = data() ^ (data() << 2);
            return !(b & COLOR_MASK);
        }
        
        bool isU2StopAlternate(Color color)const{
            // 途中で色 color のダブりがあってそれ以外交互に並んでいるかどうか
            // ダブりは2個までが限界
            cerr << toString() << endl;
            uint64_t b = data() ^ (data() << 2);
            uint64_t c = b & COLOR_MASK;
            size_t pos = bsf64(c); // 最初のダブりのビット位置
            uint64_t s = c >> pos;
            if(s == color){
                return true;
            }
            if((s & 1) == color){
                if(!(s ^ (s << 2))){
                    return true;
                }
            }
            return false;
        }
        
        std::string toString()const{
            std::ostringstream oss;
            iterateAny(*this, [&oss](uint8_t data)->void{
                oss << toColorString(static_cast<Color>(data & 1));
            });
            return oss.str();
        }
        
        constexpr RangedStraight()
        :BitArray64<2>(){}
        
        constexpr RangedStraight(uint64_t adata)
        :BitArray64<2>(adata){}
    };
    
    template<int N> // 盤面サイズ
    struct Straight{
        // 各エッジに 2 bits ずつ与える
        static constexpr size_t INT_SIZE = 64;
        
        std::array<BitArray64<2>, (N * 2 + INT_SIZE - 1) / INT_SIZE> bits_; // 端数の分も含める
        
        RangedStraight getByBitIndex(size_t m, size_t n)const noexcept{
            // 64ビットに収まることを前提としている
            size_t mDataIndex = m / INT_SIZE;
            size_t mBitIndex = m % INT_SIZE;
            
            size_t nDataIndex = (n - 1) / INT_SIZE;
            size_t nBitIndex = n % INT_SIZE;
            
            DERR << "from " << mDataIndex << " " << mBitIndex;
            DERR << " to " << nDataIndex << " " << nBitIndex << endl;
            
            BitSet64 ans = (bits_[mDataIndex] >> mBitIndex);
            if(nDataIndex == mDataIndex){
                ans &= (1ULL << (n - m)) - 1ULL; // 上位ビットを外す
            }else{
                // 上位ビットを外す必要はない
                ans |= (bits_[nDataIndex] & ((1ULL << nBitIndex) - 1ULL)) << (INT_SIZE - mDataIndex);
            }
            return RangedStraight(ans.data());
        }
        
        RangedStraight get(size_t m, size_t n)const noexcept{
            return getByBitIndex(m * 2, n * 2);
        }
        
        void set(size_t index, uint8_t cb){
            DERR << "set straight " << index << " " << (int)cb << endl;
            index *= 2;
            size_t dataIndex = index / INT_SIZE;
            size_t bitIndex = index % INT_SIZE;
            bits_[dataIndex].set_by_bit_index(bitIndex, cb);
        }
        void assign(size_t index, uint8_t cb){
            DERR << "assign straight " << index << " " << (int)cb << endl;
            index *= 2;
            size_t dataIndex = index / INT_SIZE;
            size_t bitIndex = index % INT_SIZE;
            bits_[dataIndex].assign_by_bit_index(bitIndex, cb);
        }
        
        void clear(){
            bits_.fill(0);
        }
        
        template<int MODE = 0>
        bool equals(const Straight& rhs)const{
            return bits_ == rhs.bits_;
        }
        
        bool operator==(const Straight& rhs)const noexcept{
            return equals<0>(rhs);
        }
        bool operator!=(const Straight& rhs)const noexcept{
            return !((*this) == rhs);
        }
    };
    
    template<int N>
    struct Straights{
        // x, y それぞれに対して直線情報を用意
        // y軸方向が0, x軸方向が1
        std::array<std::array<Straight<N>, N + 1>, 2> st_;
        
        void clear(){
            for(int i = 0; i < 2; ++i){
                for(int j = 0; j < N + 1; ++j){
                    st_[i][j].clear();
                }
            }
        }
        
        void set(int z, TileColor tc){
            // 4点を同時に設定(設定済みの場合もあるが...)
            const int x = ZtoX(z), y = ZtoY(z);
            st_[0][x].set(y, tc[0]);
            st_[0][x + 1].set(y, tc[2]);
            st_[1][y].set(x, tc[1]);
            st_[1][y + 1].set(x, tc[3]);
        }
        void assign(int z, TileColor tc){
            // 4点を同時に設定(設定済みの場合もあるが...)
            const int x = ZtoX(z), y = ZtoY(z);
            st_[0][x].assign(y, tc[0]);
            st_[0][x + 1].assign(y, tc[2]);
            st_[1][y].assign(x, tc[1]);
            st_[1][y + 1].assign(x, tc[3]);
        }
        
        const Straight<N>& straight(int i, int axis0)const{
            return st_[i][axis0];
        }
        Straight<N>& straight(int i, int axis0){
            return st_[i][axis0];
        }
        
        RangedStraight get(int i, int axis0, int axis1)const{
            return straight(i, axis0).get(axis1, axis1 + 1);
        }
        
        template<int MODE = 0>
        bool equals(const Straights& rhs)const{
            for(int j = 0; j < N + 1; ++j){
                if(!st_[0][j].template equals<MODE>(rhs.st_[0][j])){
                    if(MODE){
                        cerr << "different y-straight " << j;
                    }
                    return false;
                }
            }
            for(int j = 0; j < N + 1; ++j){
                if(!st_[1][j].template equals<MODE>(rhs.st_[1][j])){
                    if(MODE){
                        cerr << "different x-straight " << j;
                    }
                    return false;
                }
            }
            return true;
        }
        
        bool operator==(const Straights& rhs)const noexcept{
            return equals<0>(rhs.st_);
        }
        bool operator!=(const Straights& rhs)const noexcept{
            return !((*this) == rhs);
        }
    };
}

#endif // TRAX_BOARDELEMENTS_HPP_