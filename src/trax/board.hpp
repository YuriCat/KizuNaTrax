/*
 board.hpp
 Katsuki Ohto
 */

#ifndef TRAX_BOARD_HPP_
#define TRAX_BOARD_HPP_

#include "eval_params.h"

#include "trax.hpp"
#include "board_elements.hpp"

namespace Trax{
    
    /**************************盤面表現**************************/
    
    class Board{
        // 2次元を基本とした盤面表現
        
    public:
        static constexpr int size()noexcept{ return SIZE; }
        static constexpr int margin()noexcept{ return MARGIN; }
        
        int turn; // 現在の手数
        TileBound bound; // タイルの端の座標
        
        uint64_t hash; // hash value

        int moves; // これまでに置かれたタイルの数(手数と違うので注意)
        
        std::array<LineInfo<SIZE>, N_TURNS> line_; // 線
        int lines;
        
        std::array<MoveInfo, N_TURNS * 4> moveInfo;
        std::array<TurnInfo, N_TURNS> turnInfo;
        std::array<TileCell, SIZE * SIZE> cell_; // タイル情報
        //std::array<LineRef, SIZE * SIZE * 4> lineRef; // 線へのリンク情報
        std::array<EdgeInfo, SIZE * SIZE * 4> edgeInfo_; // エッジの情報
        Straights<SIZE> straights_; // 直線の色情報
        
        // 以下評価のための情報(差分計算される)
        //std::array<std::array<int, 16 * 16>, 2> frontShapeLines; // 各線のエンドの距離型
        
        // 以下評価のための情報
        
        std::array<std::array<AttackInfo, 4>, 2> attackInfo; // アタック情報
        std::array<int, 2> attacks; // 擬アタック数
        std::array<int, 2> colorLines; // 色ごとの線の数
        //std::array<int, 2> corners; // コーナー数
        std::array<int, 2> threats; // 擬スレート数
        //std::array<std::array<int, 3>, 2> end2endLines; // near-nearライン(両端同士の距離が短い線)の数(1-1, 1-2, 2-2)で分ける
        std::array<int, 2> longLines; // ビクトリーラインを伺える線の数
        //std::array<double, 2> sumInvLineEndMD; // 各線のエンド間のマンハッタン距離の逆数
        
        std::array<std::array<int, 16 * 16>, 2> twoLinesFrontShape; // 2線関係
        
        int modifiedLatestLineAge; // 変化させた最も新しい線の世代
        
        // 評価値
        std::array<int, 2> lineShapeScore; // 線割の評価値
        std::array<int, 2> twoLinesFrontShapeScore; // 2線関係の評価値
        
        //std::array<std::array<ThreatInfo, 4>, 2> threatInfo;
        
        void clearEvalInfo()noexcept{
            //colorLines.fill(0);
            //corners.fill(0);
            threats.fill(0);
            //end2endLines.fill({0, 0, 0});
            longLines.fill(0);
            //sumInvLineEndMD.fill(0.0);
            for(int c = 0; c < 2; ++c){
                //end2endLines[c].fill(0);
                //frontShapeLines[c].fill(0);
            }
            for(int c = 0; c < 2; ++c){
                //twoLinesFrontShape[c].fill(0);
            }
            //twoLinesFrontShapeScore.fill(0);
            lineShapeScore.fill(0);
            twoLinesFrontShapeScore.fill(0);
        }
        
        
        Color turnColor()const noexcept{ return toTurnColor(turn); }
        Color lastTurnColor()const noexcept{ return flipColor(turnColor()); }
        
        // ノーテーション -> 座標に変換
        int toBoardX(int nx)const noexcept{ return lx() - 1 + nx; }
        int toBoardY(int ny)const noexcept{ return ly() - 1 + ny; }
        int toBoardZ(int nx, int ny)const noexcept{ return XYtoZ(toBoardX(nx), toBoardY(ny)); }
        
        // 座標 -> ノーテーションに変換
        int toNotationX(int x)const noexcept{ return x - lx() + 1; }
        int toNotationY(int y)const noexcept{ return y - ly()+ 1; }
        
        //int lines(const Color c)const{ return lines_[c]; }
        //int lines()const{ return lines_[0] + lines_[1]; }
 
        std::vector<Move> getPath()const{
            // これまでの着手列を得る
            std::vector<Move> v;
            for(int t = 0; t < turn; ++t){
                int idx = moveIndex(t);
                v.emplace_back(Move(moveInfo[idx].z, moveInfo[idx].tile));
            }
            return v;
        }
        std::vector<RelativeMove> getRelativePath()const{
            // これまでの相対着手列を得る
            std::vector<RelativeMove> v;
            for(int t = 0; t < turn; ++t){
                int idx = moveIndex(t);
                int z = moveInfo[idx].z;
                int x = ZtoX(z), y = ZtoY(z);
                v.emplace_back(RelativeMove(x - turnInfo[t].bound.lx() + 1,
                                            y - turnInfo[t].bound.ly() + 1,
                                            moveInfo[idx].tile));
            }
            return v;
        }
        std::vector<std::string> getPathNotations()const{
            // これまでの着手ノーテーション列を得る
            std::vector<RelativeMove> rpath = getRelativePath();
            std::vector<std::string> v;
            for(const auto& rmv : rpath){
                v.emplace_back(toNotationString(rmv));
            }
            return v;
        }
        std::vector<std::string> getPathExpandedNotations()const{
            // これまでの着手拡張ノーテーション列を得る
            std::vector<RelativeMove> rpath = getRelativePath();
            std::vector<std::string> v;
            for(const auto& rmv : rpath){
                v.emplace_back(toExpandedNotationString(rmv));
            }
            return v;
        }

        Tile whichTile(int z, TileMove tm)const{
            // trax move notation (3 patterns) to tile (6 patterns)
            for(int i = 0; i < 2; ++i){
                Tile tile = toTile(tm, i);
                if(isValidTile(z, tile)){ return tile; }
            }
            return TILE_NONE;
        }
        
        bool isValidTile(int z, Tile tile)const{
            // only color connection
            // isolation is OK in this method
            return tileColorTable[tile].holds(color(z));
        }
        
        // accessors
        int dx()const noexcept{ return bound.dx(); }
        int dy()const noexcept{ return bound.dy(); }
        int d(int dim)const noexcept{ return bound.d(dim); }
        
        int lx()const noexcept{ return bound.lx(); }
        int ly()const noexcept{ return bound.ly(); }
        int l(int dim)const noexcept{ return bound.l(dim); }
        
        int hx()const noexcept{ return bound.hx(); }
        int hy()const noexcept{ return bound.hy(); }
        int h(int dim)const noexcept{ return bound.h(dim); }
        
        TileColor& color(int z){ return cell_[z].tc; }
        const TileColor& color(int z)const{ return cell_[z].tc; }
        
        Tile& tile(int z){ return cell_[z].tile; }
        const Tile& tile(int z)const{ return cell_[z].tile; }
        
        //LineInfo<SIZE>& line(Color c, int l){ return line_[l * 2 + c]; }
        //const LineInfo<SIZE>& line(Color c, int l)const{ return line_[l][c]; }
        
        LineInfo<SIZE>& line(int l){ return line_[l]; }
        const LineInfo<SIZE>& line(int l)const{ return line_[l]; }
        
        EdgeInfo& edgeInfo(int xyd){ return edgeInfo_[xyd]; }
        const EdgeInfo& edgeInfo(int xyd)const{ return edgeInfo_[xyd]; }
        
        int moveIndex(int t)const{ return turnInfo[t].moveIndex; }
        
        template<bool kPseudoLegality = false>
        int makeMove(const Move& mv){
            return makeMove<kPseudoLegality>(mv.z(), mv.tile());
        }
        template<bool kPseudoLegality = false>
        int makeMove(int z, Tile tile){
            return makeMove<kPseudoLegality>(z, tile, tileColorTable[tile]);
        }
        template<bool kPseudoLegality = false>
        int makeMove(int z, Tile tl, TileColor c){
            // makeMoveの前に行う処理
            turnInfo[turn].bound = bound;
            //turnInfo[turn].lineShapeScore = lineShapeScore;
            bound.update(ZtoX(z), ZtoY(z)); // 境界はforcedでは変化しないのでここで良い
            modifiedLatestLineAge = -1;
            int ret = makeMoveSub<kPseudoLegality>(z, tl, c);
            if(ret < 0){ // illegal
                unmakeMoveSub(turn);
            }else{
                // makeMoveの後に行う処理
                ++turn;
                turnInfo[turn].moveIndex = moves;
            }
            return ret;
        }
        
        template<bool kTurnCheck = false>
        void unmakeMove(int t){

            // recover from change to turn |t|
            if(kTurnCheck || (turn > t && t >= 0)){
                turn = t;
                unmakeMoveSub(t);
                turnInfo[t].moveIndex = moves;
            }
        }
        template<bool kTurnCheck = false>
        void unmakeMove(){
            // recover from change on this turn
            unmakeMove<kTurnCheck>(turn - 1);
        }
        
        std::vector<Tile> rowTileVector(int x)const{
            std::vector<Tile> v;
            for(int y = ly() - 1; y <= hy() + 1; ++y){
                Tile t = tile(XYtoZ(x, y));
                //cerr << "tile = " << t << endl;
                v.push_back(t);
            }
            return v;
        }
        
        std::string toRawBoardString()const{
            const int Y = 3;
            const int X = 2;
            std::ostringstream oss;
            oss << "  ";
            for(int y = ly() - 1; y <= hy() + 1; ++y){
                std::string cstr = toColumnString(toNotationY(y));
                oss << " " << cstr << Space(max(0, Y - 1 - int(cstr.size())));
            }oss << endl;
            for(int x = lx() - 1; x <= hx() + 1; ++x){
                int rnx = toNotationX(x);
                auto tiles = rowTileVector(x);
                for(int i = 0; i < X; ++i){
                    if(i == X / 2){
                        std::string rstr = toRowString(rnx);
                        oss << rstr << Space(max(0, 2 - int(rstr.size())));
                    }else{
                        oss << Space(2);
                    }
                    for(int y = ly() - 1; y <= hy() + 1; ++y){
                        int z = XYtoZ(x, y);
                        Tile t = tile(z);
                        if(examTile(t)){
                            if(Y == 3){
                                oss << "\033[40m" << tilePatternString3x2[t][i] << "\033[39m\033[49m";
                            }else if(Y == 5){
                                oss << "\033[40m" << tilePatternString5x3[t][i] << "\033[39m\033[49m";
                            }
                        }else{
                            // search edge of line around here
                            if(Y == 3){
                                if(i == 0){
                                    oss << " ";
                                    if(color(z).any(0)){
                                        oss << char('a' + edgeInfo(z * 4 + 0).lineIndex());
                                    }else{
                                        oss << " ";
                                    }
                                    oss << " ";
                                }else if(i == 1){
                                    for(int d = 1; d < 4; ++d){
                                        if(color(z).any(d)){
                                            oss << char('a' + edgeInfo(z * 4 + d).lineIndex());
                                        }else{
                                            oss << " ";
                                        }
                                    }
                                }
                            }else if(Y == 5){
                                if(i == 0){
                                    oss << Space(Y / 2);
                                    if(color(z).any(0)){
                                        oss << char('a' + edgeInfo(z * 4 + 0).lineIndex());
                                    }else{
                                        oss << " ";
                                    }
                                    oss << Space(Y / 2);
                                }else if(i == 1){
                                    if(color(z).any(1)){
                                        oss << char('a' + edgeInfo(z * 4 + 1).lineIndex());
                                    }else{
                                        oss << " ";
                                    }
                                    oss << Space(Y / 2 + 1);
                                    if(color(z).any(3)){
                                        oss << char('a' + edgeInfo(z * 4 + 3).lineIndex());
                                    }else{
                                        oss << " ";
                                    }
                                }else if(i == 2){
                                    oss << Space(Y / 2);
                                    if(color(z).any(2)){
                                        oss << char('a' + edgeInfo(z * 4 + 2).lineIndex());
                                    }else{
                                        oss << " ";
                                    }
                                    oss << Space(Y / 2);
                                }
                            }
                        }
                    }oss << endl;
                }
            }
            return oss.str();
        }
        
        std::string toBoardString()const{
            std::ostringstream oss;
            oss << bound << endl;
            oss << toRawBoardString();
            return oss.str();
        }
        
        std::string toLineString()const{
            std::ostringstream oss;
            oss << lines << " lines." << endl;
            for(int l = 0; l < lines; ++l){
                oss << char('a' + l) << " " << line(l).toNotationString(lx(), ly()) << " ";
                oss << edgeInfo(line(l).xyd(0)).toString() << "-" << edgeInfo(line(l).xyd(1)).toString() << endl;
            }
            oss << endl;
            return oss.str();
        }
        
        std::string toInfoString()const{
            std::ostringstream oss;
            oss << "turn = " << turn << " tiles = " << moves << endl;
            int pat;
            oss << "abs-key = " << hash << " rel-key = " << calcRepRelativeHash(*this, pat) << endl;
            return oss.str();
        }
        
        std::string toString()const{
            std::ostringstream oss;
            oss << toInfoString();
            oss << toBoardString();
            oss << toLineString();
            return oss.str();
        }
        
        template<int MODE = 0>
        bool equals(const Board& rhs)const{
            if(turn != rhs.turn){
                if(MODE){
                    cerr << "different |turn|" << endl;
                }
                return false;
            }
            if(bound != rhs.bound){
                if(MODE){
                    cerr << "different bound " << bound << " <-> " << rhs.bound << endl;
                }
                return false;
            }
            if(lines != rhs.lines){
                if(MODE){
                    cerr << "different |lines|" << endl;
                }
                return false;
            }else{
                for(int l = 0; l < lines; ++l){
                    if(line(l) != rhs.line(l)){
                        if(MODE){
                            cerr << "different line " << l << endl;
                            cerr << line(l).toNotationString(lx(), ly()) << endl;
                            cerr << rhs.line(l).toNotationString(rhs.lx(), rhs.ly()) << endl;
                        }
                        return false;
                    }
                }
            }
            for(int z = 0; z < SIZE; ++z){
                if(color(z) != rhs.color(z)){
                    if(MODE){
                        cerr << "different color in " << z
                        << " " << color(z) << " <-> " << rhs.color(z) << endl;
                    }
                    return false;
                }
            }
            for(int z = 0; z < SIZE; ++z){
                if(tile(z) != rhs.tile(z)){
                    if(MODE){
                        cerr << "different tile in " << z
                        << " " << tile(z) << " <-> " << tile(z) << endl;
                    }
                    return false;
                }
            }
            for(int zd = 0; zd < SIZE * 4; ++zd){
                if(edgeInfo(zd) != rhs.edgeInfo(zd)){
                    if(MODE){
                        cerr << "different edge info " << zd
                        << " " << edgeInfo(zd) << " <-> " << rhs.edgeInfo(zd) << endl;
                    }
                    return false;
                }
            }
            if(moves != rhs.moves){
                if(MODE){
                    cerr << "different |moves|" << endl;
                }
                return false;
            }else{
                for(int m = 0; m < moves; ++m){
                    if(moveInfo[m] != rhs.moveInfo[m]){
                        if(MODE){
                            cerr << "different move info " << m << endl
                            << moveInfo[m] << " <-> " << endl
                            << rhs.moveInfo[m] << endl;
                        }
                        return false;
                    }
                }
            }
            for(int t = 0; t < turn; ++t){
                if(turnInfo[t] != rhs.turnInfo[t]){
                    if(MODE){
                        cerr << "different turn info " << t << endl
                        << turnInfo[t] << " <-> " << endl
                        << rhs.turnInfo[t] << endl;
                    }
                    return false;
                }
            }
            for(int t = 0; t <= turn; ++t){
                if(moveIndex(t) != rhs.moveIndex(t)){
                    if(MODE){
                        cerr << "different move index " << t << " "
                        << moveIndex(t) << " <-> " << rhs.moveIndex(t) << endl;
                    }
                    return false;
                }
            }
            if(hash != rhs.hash){
                if(MODE){
                    cerr << "different hash value" << endl;
                }
                return false;
            }
            if(!straights_.equals<MODE>(rhs.straights_)){
                if(MODE){
                    cerr << "different straights" << endl;
                }
                return false;
            }
            return true;
        }
        
        bool operator==(const Board& rhs)const{
            return equals<0>(rhs);
        }
        
        bool operator!=(const Board& rhs)const{
            return !((*this) == rhs);
        }
        
        bool exam(bool staticFlag = true)const{
            // turn
            if(turn < 0){
                cerr << "Board::exam() : illegal turn " << turn << endl;
                return false;
            }
            // color
            for(int x = 0; x < SIZE; ++x){
                for(int y = 0; y < SIZE; ++y){
                    int z = XYtoZ(x, y);
                    if(tile(z) >= 0){
                        if(color(z) != tileColorTable[tile(z)]){
                            cerr << "Board::exam() : tile - color inconsistency" << endl;
                            return false;
                        }
                    }
                }
            }
            // line
            if(lines < 0){
                cerr << "Board::exam() : illegal lines " << lines << endl;
                return false;
            }
            for(int l = 0; l < lines; ++l){
                for(int e = 0; e < 2; ++e){
                    int z = line(l).xy(e);
                    int d = line(l).d(e);
                    
                    if(staticFlag){
                        // if static state, no tile on end of each line
                        if(color(z).filled()){
                            cerr << "Board::exam() : tile on end of line ";
                            cerr << XY(z) << endl;
                            return false;
                        }
                    }
                    if(!examColor(line(l).color())){
                        cerr << "Board::exam() : illegal line color " << line(l).color() << endl;
                        return false;
                    }
                    if(line(l).color() != (color(z)[d] & 1)){
                        cerr << "Board::exam() : inconsistent board color <-> line color" << endl;
                        cerr << (color(z)[d] & 1) << " in " << XY(z);
                        cerr << " <-> " << line(l).color() << " in line " << char('a' + l) << endl;
                        
                        return false;
                    }
                }
            }
            // edge info
            for(int x = 0; x < SIZE; ++x){
                for(int y = 0; y < SIZE; ++y){
                    int z = XYtoZ(x, y);
                    if(color(z).any() && !color(z).filled()){ // edge positon
                        for(int d = 0; d < 4; ++d){
                            if(color(z).any(d)){ // edge
                                int zd = z * 4 + d;
                                int l = edgeInfo(zd).lineIndex();
                                int e = edgeInfo(zd).lineEnd();
                                if(line(l).color() != (color(z).at(d) & 1)){
                                    cerr << "Board::exam() : inconsistent line color on ";
                                    cerr << toNotationString(x, y, lx(), ly()) << "(" << d << ")";
                                    cerr << " " << line(l).color() << " <-> " << (color(z).at(d) & 1);
                                    cerr << " in line " << l << endl;
                                    return false;
                                }
                                if(line(l).xyd(e) != zd){
                                    cerr << "Board::exam() : inconsistent line reference ";
                                    //cerr << "line " << l << " " << line(l).toString() << " <-> " << zd << endl;
                                    cerr << toNotationString(x, y, lx(), ly()) << "(" << d << ") ";
                                    cerr << line(l).xyd(e) << " <-> " << zd << endl;
                                    return false;
                                }
                            }
                        }
                    }
                }
            }
            // line-edge
            for(int l = 0; l < lines; ++l){
                // age check
                if(line(l).age() < max(edgeInfo(line(l).xyd(0)).age(),
                                       edgeInfo(line(l).xyd(1)).age())){
                    cerr << "Board::exam() : age of line should not be older than each end edge" << endl;
                    return false;
                }
            }
            
            // move
            if(moves < 0){
                cerr << "Board::exam() : illegal moves " << moves << endl;
                return false;
            }
            for(int m = 0; m < moves; ++m){
                if(moveInfo[m].tile != tile(moveInfo[m].z)){
                    cerr << "Board::exam() : move stack - board tile inconsistency" << endl;
                    cerr << " move " << m << " " << moveInfo[m].toString() << endl;
                    int z = moveInfo[m].z;
                    cerr << " board " << tile(z) << " in " << z << endl;
                }
            }
#ifdef USE_STRAIGHT
            // cell color - strait color
            for(int x = 0; x < SIZE; ++x){
                for(int y = 0; y < SIZE; ++y){
                    int z = XYtoZ(x, y);
                    if(color(z)[0] != straights_.get(0, x, y)){
                        cerr << "Board::exam() : cell color - straight color inconsistency" << endl;
                        cerr << "y-strait " << XY(x, y) << " " << (int64_t)color(z)[0] << " <-> " << (int64_t)straights_.get(0, x, y) << endl;
                        return false;
                    }
                    if(color(z)[2] != straights_.get(0, x + 1, y)){
                        cerr << "Board::exam() : cell color - straight color inconsistency" << endl;
                        cerr << "y-strait " << XY(x, y) << " " << (int64_t)color(z)[2] << " <-> " << (int64_t)straights_.get(0, x + 1, y) << endl;
                        return false;
                    }
                    if(color(z)[1] != straights_.get(1, y, x)){
                        cerr << "Board::exam() : cell color - straight color inconsistency" << endl;
                        cerr << "x-strait " << XY(x, y) << " " << (int64_t)color(z)[1] << " <-> " << (int64_t)straights_.get(1, y, x) << endl;
                        return false;
                    }
                    if(color(z)[3] != straights_.get(1, y + 1, x)){
                        cerr << "Board::exam() : cell color - straight color inconsistency" << endl;
                        cerr << "x-strait " << XY(x, y) << " " << (int64_t)color(z)[3] << " <-> " << (int64_t)straights_.get(1, y + 1, x) << endl;
                        return false;
                    }
                }
            }
#endif
            return true;
        }
        
        void clear()noexcept{
            turn = 0;
            hash = 0ULL;
            bound.set(SIZE / 2, SIZE / 2);
            moveInfo.fill(MoveInfo(0));
            turnInfo.fill(TurnInfo(0));
            lines = 0;
            line_.fill(LineInfo<SIZE>(0));
            cell_.fill(TileCell());
            edgeInfo_.fill(EdgeInfo(0));
            moves = 0;
            clearAttacks();
            for(int c = 0; c < 2; ++c){
                attackInfo[c].fill(AttackInfo());
            }
            straights_.clear();
            clearEvalInfo();
            
            lineShapeScore.fill(0);
            
            twoLinesFrontShapeScore.fill(0);
        }
        
        void checkSetAttacks(){
            clearAttacks();
            for(int l = 0; l < lines; ++l){
                if(!line(l).mate()){
                    if(checkPushLoopAttack(l) <= 0){
                        checkPushSingleVictoryLineAttack(l);
                    }
                }
            }
        }
        
        //template<class params_t>
        //void updateEvalInfo(const params_t& params){
        void updateEvalInfo(){
            // 評価のための諸々の情報を更新する
            // すでに試合終了していないことを前提とする
            const Color myColor = turnColor();
            clearEvalInfo();
            for(int l0 = 0; l0 < lines; ++l0){
                Color c0 = line(l0).color();
                Color oc0 = flipColor(c0);
                
                //colorLines[c0] += 1;
                //frontShapeLines[c0][line(l0).frontShape()] += 1;
                //lineShapes[c0][line(l0).shape()] += 1;
                
                lineShapeScore[myColor] += eval_params[4 + line(l0).shape() * 2 + int(myColor != c0)];
                
                int e0x = line(l0).x(0);
                int e0y = line(l0).y(0);
                int e1x = line(l0).x(1);
                int e1y = line(l0).y(1);
                
                //sumInvLineEndMD[c0] += 1 / double(abs(e1x - e0x) + abs(e1y - e0y));
                
                // コーナー
                bool corner0 = line(l0).is11Corner();
                //if(corner0){ ++corners[c0]; }
                
                // ビクトリーライン候補
                if(max(VICTORY_LINE_LENGTH + 1 - line(l0).dx(),
                       dx() - line(l0).dx()) <= 2){
                    longLines[c0] += 1;
                }
                if(max(VICTORY_LINE_LENGTH + 1 - line(l0).dy(),
                       dy() - line(l0).dy()) <= 2){
                    longLines[c0] += 1;
                }
                
                for(int l1 = 0; l1 < l0; ++l1){
                    Color c1 = line(l1).color();

                    
                    if(c0 == c1){ // 同色
                        
                        // 2線関係
                        unsigned int d[2][2][2];
                        for(int i = 0; i < 2; ++i){
                            for(int j = 0; j < 2; ++j){
                                //= abs(line(l0).x(0) - line(l1).x(0)) + abs(line(l1).y(0) - line(l1).y(0));
                                //d[i][j] = abs(line(l0).x(i) - line(l1).x(i)) + abs(line(l0).y(j) - line(l1).y(j));
                                d[i][j][0] = abs(line(l0).x(i) - line(l1).x(j));
                                d[i][j][1] = abs(line(l0).y(i) - line(l1).y(j));
                            }
                        }
                        
                        uint32_t l2pat0 = (min(d[0][0][0], 3U)
                                           | (min(d[0][0][1], 3U) << 2)
                                           | (min(d[1][1][0], 3U) << 4)
                                           | (min(d[1][1][1], 3U) << 6)
                                           );
                        
                        uint32_t l2pat1 = (min(d[0][1][0], 3U)
                                           | (min(d[0][1][1], 3U) << 2)
                                           | (min(d[1][0][0], 3U) << 4)
                                           | (min(d[1][0][1], 3U) << 6)
                                           );
                        twoLinesFrontShape[c0][l2pat0] += 1;
                        twoLinesFrontShape[c0][l2pat1] += 1;
                        
                        twoLinesFrontShapeScore[myColor] += eval_params[516 + l2pat0 * 2 + int(myColor != c0)];
                        twoLinesFrontShapeScore[myColor] += eval_params[516 + l2pat1 * 2 + int(myColor != c0)];
                        
                        if(corner0 && line(l1).is11Corner()){
                            // 複数コーナー
                            
                            // コーナーの位置が桂馬で、タイルが同じ向きであれば高確率でL字スレート
                            int z0 = line(l0).xy(0) + ar4[line(l0).d(0)];
                            int z1 = line(l1).xy(0) + ar4[line(l1).d(0)];
                            
                            int x0 = ZtoX(z0), y0 = ZtoY(z0);
                            int x1 = ZtoX(z1), y1 = ZtoY(z1);
                            
                            Tile t0 = tile(z0);
                            Tile t1 = tile(z1);
                            
                            static Counter loopThreats("loop_threat");
                            
                            auto Foo = [&]()->void{
                                cerr << toRawBoardString(); getchar();
                            };
                            
                            if(t0 == t1){
                                if(abs((x0 - x1) * (y0 - y1)) == 2){
                                    loopThreats += 1;
                                    threats[c0] += 1;
                                    //Foo();
                                }
                                /*int dir = 0;
                                int axis0 = x0;
                                if(x1 - x0 == 1){
                                    // y方向ストレートを判定
                                    if(y1 > y0){
                                        auto rst = straights_.straight(0, x1).get(y0 + 1, y1 + 1);
                                        if(rst.isAlternate()){
                                            loopThreats += 1;
                                            threats[c0] += 1;
                                            Foo();
                                        }
                                    }else if(y0 > y1){
                                        auto rst = straights_.straight(0, x1).get(y1 + 1, y0 + 1);
                                        if(rst.isAlternate()){
                                            loopThreats += 1;
                                            threats[c0] += 1;
                                            Foo();
                                        }
                                    }
                                }else if(x0 - x1 == 1){
                                    if(y1 > y0){
                                        auto rst = straights_.straight(0, x0).get(y0 + 1, y1 + 1);
                                        if(rst.isAlternate()){
                                            loopThreats += 1;
                                            threats[c0] += 1;
                                            Foo();
                                        }
                                    }else if(y0 > y1){
                                        auto rst = straights_.straight(0, x0).get(y1 + 1, y0 + 1);
                                        if(rst.isAlternate()){
                                            loopThreats += 1;
                                            threats[c0] += 1;
                                            Foo();
                                        }
                                    }
                                }else if(y1 - y0 == 1){
                                    if(x1 > x0){
                                        auto rst = straights_.straight(0, y1).get(x0 + 1, x1 + 1);
                                        if(rst.isAlternate()){
                                            loopThreats += 1;
                                            threats[c0] += 1;
                                            Foo();
                                        }
                                    }else if(x0 > x1){
                                        auto rst = straights_.straight(0, y1).get(x1 + 1, x0 + 1);
                                        if(rst.isAlternate()){
                                            loopThreats += 1;
                                            threats[c0] += 1;
                                            Foo();
                                        }
                                    }
                                }else if(y0 - y1 == 1){
                                    if(x1 > x0){
                                        auto rst = straights_.straight(0, y0).get(x0 + 1, x1 + 1);
                                        if(rst.isAlternate()){
                                            loopThreats += 1;
                                            threats[c0] += 1;
                                            Foo();
                                        }
                                    }else if(x0 > x1){
                                        auto rst = straights_.straight(0, y0).get(x1 + 1, x0 + 1);
                                        if(rst.isAlternate()){
                                            loopThreats += 1;
                                            threats[c0] += 1;
                                            Foo();
                                        }
                                    }
                                }*/
                            }
                            
                            // コーナーの位置が2個離れで、間が二つとも逆の色(ただし同じ線でない)ならエッジスレート
                            static Counter edgeThreats("edge_threat");
                            
                            if(x0 == x1 && abs(y1 - y0) == 3){
                                int aiy0 = (y0 + y1 - 1) / 2;
                                int aiy1 = (y0 + y1 + 1) / 2;
                                if(t0 == toTile(S, c0) && t1 == toTile(B, c0)){
                                    // 上向き
                                    /*auto rst = (y0 < y1) ?
                                    straights_.straight(0, x0 + 1).get(y0 + 1, y1):
                                    straights_.straight(0, x0 + 1).get(y1 + 1, y0);
                                    if(rst.isU2StopAlternate(oc0)){
                                        Foo();
                                    }*/
                                    
                                    TileColor tc0 = color(XYtoZ(x0 - 1, aiy0));
                                    TileColor tc1 = color(XYtoZ(x0 - 1, aiy1));
                                    if((!tc0.filled() && tc0[2] == (oc0 | 2))
                                       && (!tc1.filled() && tc1[2] == (oc0 | 2))){
                                        edgeThreats += 1;
                                        threats[c0] += 1;
                                        DERR << "pattern 0" << toRawBoardString();
                                        //DWAIT;
                                    }
                                }else if(t1 == toTile(S, c0) && t0 == toTile(B, c0)){
                                    // 上向き
                                    TileColor tc0 = color(XYtoZ(x0 - 1, aiy0));
                                    TileColor tc1 = color(XYtoZ(x0 - 1, aiy1));
                                    if((!tc0.filled() && tc0[2] == (oc0 | 2))
                                       && (!tc1.filled() && tc1[2] == (oc0 | 2))){
                                        edgeThreats += 1;
                                        threats[c0] += 1;
                                        DERR << "pattern 1" << toRawBoardString();
                                        //DWAIT;
                                    }
                                }else if(t0 == toTile(S, oc0) && t1 == toTile(B, oc0)){
                                    // 下向き
                                    TileColor tc0 = color(XYtoZ(x0 + 1, aiy0));
                                    TileColor tc1 = color(XYtoZ(x0 + 1, aiy1));
                                    if((!tc0.filled() && tc0[0] == (oc0 | 2))
                                       && (!tc1.filled() && tc1[0] == (oc0 | 2))){
                                        edgeThreats += 1;
                                        threats[c0] += 1;
                                        DERR << "pattern 2" << toRawBoardString();
                                        //DWAIT;
                                    }
                                }else if(t1 == toTile(S, oc0) && t0 == toTile(B, oc0)){
                                    // 下向き
                                    TileColor tc0 = color(XYtoZ(x0 + 1, aiy0));
                                    TileColor tc1 = color(XYtoZ(x0 + 1, aiy1));
                                    if((!tc0.filled() && tc0[0] == (oc0 | 2))
                                       && (!tc1.filled() && tc1[0] == (oc0 | 2))){
                                        edgeThreats += 1;
                                        threats[c0] += 1;
                                        DERR << "pattern 3" << toRawBoardString();
                                        //DWAIT;
                                    }
                                }
                            }else if(y0 == y1 && abs(x1 - x0) == 3){
                                int aix0 = (x0 + x1 - 1) / 2;
                                int aix1 = (x0 + x1 + 1) / 2;
                                if(t0 == toTile(S, c0) && t1 == toTile(B, oc0)){
                                    // 左向き
                                    TileColor tc0 = color(XYtoZ(aix0, y0 - 1));
                                    TileColor tc1 = color(XYtoZ(aix1, y0 - 1));
                                    if((!tc0.filled() && tc0[3] == (oc0 | 2))
                                       && (!tc1.filled() && tc1[3] == (oc0 | 2))){
                                        edgeThreats += 1;
                                        threats[c0] += 1;
                                        DERR << "pattern 4" << toRawBoardString();
                                        //DWAIT;
                                    }
                                }else if(t1 == toTile(S, c0) && t0 == toTile(B, oc0)){
                                    // 左向き
                                    TileColor tc0 = color(XYtoZ(aix0, y0 - 1));
                                    TileColor tc1 = color(XYtoZ(aix1, y0 - 1));
                                    if((!tc0.filled() && tc0[3] == (oc0 | 2))
                                       && (!tc1.filled() && tc1[3] == (oc0 | 2))){
                                        edgeThreats += 1;
                                        threats[c0] += 1;
                                        DERR << "pattern 5" << toRawBoardString();
                                        //DWAIT;
                                    }
                                }else if(t0 == toTile(S, oc0) && t1 == toTile(B, c0)){
                                    // 右向き
                                    TileColor tc0 = color(XYtoZ(aix0, y0 + 1));
                                    TileColor tc1 = color(XYtoZ(aix1, y0 + 1));
                                    if((!tc0.filled() && tc0[1] == (oc0 | 2))
                                       && (!tc1.filled() && tc1[1] == (oc0 | 2))){
                                        edgeThreats += 1;
                                        threats[c0] += 1;
                                        DERR << "pattern 6" << toRawBoardString();
                                        //DWAIT;
                                    }
                                }else if(t1 == toTile(S, oc0) && t0 == toTile(B, c0)){
                                    // 右向き
                                    TileColor tc0 = color(XYtoZ(aix0, y0 + 1));
                                    TileColor tc1 = color(XYtoZ(aix1, y0 + 1));
                                    if((!tc0.filled() && tc0[1] == (oc0 | 2))
                                       && (!tc1.filled() && tc1[1] == (oc0 | 2))){
                                        edgeThreats += 1;
                                        threats[c0] += 1;
                                        DERR << "pattern 7" << toRawBoardString();
                                        //DWAIT;
                                    }
                                }
                            }
                            //if(x0 == x1 && abs(y1 - y0) == 2){
                            //    if()
                            //}
                        }
                        
                        // 両端が近接している線
                        /*int d[2][2];
                        for(int i = 0; i < 2; ++i){
                            for(int j = 0; j < 2; ++j){
                                //= abs(line(l0).x(0) - line(l1).x(0)) + abs(line(l1).y(0) - line(l1).y(0));
                                d[i][j] = abs(line(l0).x(i) - line(l1).x(i)) + abs(line(l0).y(j) - line(l1).y(j));
                            }
                        }
                        if(d[0][0] + d[1][1] <= 4){
                            ASSERT(d[0][0] + d[1][1] >= 2,);
                            end2endLines[c0][d[0][0] + d[1][1] - 2] += 1;
                        }else if(d[0][1] + d[1][0] <= 4){
                            ASSERT(d[0][1] + d[1][0] >= 2,);
                            end2endLines[c0][d[0][1] + d[1][0] - 2] += 1;
                        }*/
                    }
                }
            }
            if(threats[0] > 3){
                //cerr << toRawBoardString();
                //getchar();
            }
        }

        bool hasInevasibleAttacks(const Color c)const{
            // 複数のアタックがあり、回避不可能であるか
            if(attacks[c] < 2){ return false; }
            if(attacks[c] > 2){ return true; }
            // 2つのループアタック同士が 1 turn connectable であれば1手で回避されてしまう
            if(attackInfo[c][0].loop() && attackInfo[c][1].loop()){
                for(int i = 0; i < 2; ++i){
                    for(int j = 0; j < 2; ++j){
                        if(check1TurnConnectable(line(attackInfo[c][0].l).xy(i),
                                                 line(attackInfo[c][1].l).xy(j))){
                            // connectableなのでダメ
                            //cerr << this->toString();
                            //getchar();
                            return false;
                        }
                    }
                }
            }
            return true;
        }
        
        bool isLegalMove(const Move& mv){
            // 真の意味で合法がどうかの確認
            // TODO: 合法性判定に必要な情報だけ書き換える簡単な関数の用意
            int ret = makeMove(mv);
            if(ret < 0){ return false; }
            unmakeMove();
            return true;
        }
        
        template<
        bool NO_OUT_BOARD = false,
        bool NO_DOUBLE = false,
        bool NO_BAD_COLOR = false,
        bool NO_ISOLATED = false,
        bool NO_FIRST_TURN = false>
        bool isPseudoLegalTileColor(const int z, const TileColor tc0)const{
            // とりあえずの合法性判定
            // 連鎖ルールによって最終的に反則になる場合を考慮せず1個置けるかだけを考慮する
            const int x = ZtoX(z);
            const int y = ZtoY(z);
            if(!NO_OUT_BOARD){
                if((x < MARGIN || SIZE - MARGIN <= x)
                   || (y < MARGIN || SIZE - MARGIN <= y)){
                    return false; // 盤外
                }
            }
            if(!NO_FIRST_TURN && turn == 0){
                if(z == Z_FIRST && !(tc0[0] == tc0[3])){
                    return true;
                }else{
                    return false;
                }
            }
            TileColor tc = color(z);
            if(NO_DOUBLE || !tc.filled()){
                if(NO_ISOLATED || tc.any()){
                    if(tc0.holds(tc)){
                        return true;
                    }
                }
            }
            return false;
        }
        
        template<
        bool NO_OUT_BOARD = false,
        bool NO_DOUBLE = false,
        bool NO_BAD_COLOR = false,
        bool NO_ISOLATED = false,
        bool NO_FIRST_TURN = false>
        bool isPseudoLegalMove(const Move& mv)const{
            // とりあえずの合法性判定
            // 連鎖ルールによって最終的に反則になる場合を考慮せず1個置けるかだけを考慮する
            const int z = mv.z();
            const int x = ZtoX(z);
            const int y = ZtoY(z);
            if(!NO_OUT_BOARD){
                if((x < MARGIN || SIZE - MARGIN <= x)
                   || (y < MARGIN || SIZE - MARGIN <= y)){
                    return false; // 盤外
                }
            }
            if(!NO_FIRST_TURN && turn == 0){
                if(z == Z_FIRST && !isBackTile(mv.tile())){
                    return true;
                }else{
                    return false;
                }
            }
            TileColor tc = color(z);
            if(NO_DOUBLE || !tc.filled()){
                if(NO_ISOLATED || tc.any()){
                    if(tileColorTable[mv.tile()].holds(tc)){
                        return true;
                    }
                }
            }
            return false;
        }
        
    protected:
        void setColor(int z, TileColor c){
            // 色が確定した場合に設定する
            // 色を消すことは考えない場合
            /*for(int d = 0; d < 4; ++d){
                color(z + ar4[d]).set(oppositeDirection(d), c[d]); // マスへの設定
            }*/
            color(z) = c;
#ifdef USE_STRAIGHT
            straights_.set(z, c);
#endif
        }
        void assignColor(int z, TileColor c){
            // 色が確定した場合に設定する
            // 色を消す場合もある
            /*for(int d = 0; d < 4; ++d){
                color(z + ar4[d]).assign(oppositeDirection(d), c[d]); // マスへの設定
            }*/
            color(z) = c;
#ifdef USE_STRAIGHT
            straights_.assign(z, c);
#endif
        }
        
        int pushMove(int z, Tile tile, TileColor last){
            // push new move (add 1 tile) to move stack
            // あとから線の情報等を追加するためにインデックスを返す
            int mi = moves;
            moveInfo[mi].z = z;
            moveInfo[mi].last = last;
            moveInfo[mi].tile = tile;
            moves += 1;
            return mi;
        }
        void popMove(){
            // TODO: clear top or not?
            moves -= 1;
        }
        
        void checkToSetVictoryLine(Color c, int lnum, int& ret){
            // victory line check
            // we should do this check after all moves,
            // but it's ok to do it in change
            // because no tiles are forced to be settled out of current bound
            if(dx() >= VICTORY_LINE_LENGTH - 1
               && line(lnum).dx() >= dx() + 2){
                ret |= (Rule::VICTORY_LINE << c);
                line(lnum).setVictoryLine();
            }
            if(dy() >= VICTORY_LINE_LENGTH - 1
               && line(lnum).dy() >= dy() + 2){
                ret |= (Rule::VICTORY_LINE << c);
                line(lnum).setVictoryLine();
            }
        }
        
        void setChangedLineInfo(){
            // 変更された線の情報をまとめる
            // 番号がスワップした線はスワップした後の番号になっているので注意
            //iterate(turn
            //modifiedLatestLineAge = max(modifiedLatestLineAge, int(max(line(lnum).age(), line(lnum1).age()))); // 更新した線の世代の最新
        }
        
        int move(const unsigned int z, const Tile tl, const TileColor last, const TileColor nc){
            // ループやビクトリーラインの判定のために返り値を持つ
            DERR << "moved " << XY(z) << endl;
            
            auto Foo = [&](){
                cerr << "pos = " << XY(z) << " tile = " << tl << " last = " << last << " new = " << nc << endl;
            };
            
            int ret = 0;
            const unsigned int x = ZtoX(z), y = ZtoY(z);
            setColor(z, nc);
            tile(z) = tl;
            hash ^= tileHashTable[z][tl];
            const int mi = pushMove(z, tl, last);
            // bound for victory line judge and reading notation
            // bound.update(x, y);
            // loop check && make line
            for(int ci = 0; ci < 2; ++ci){
                Color c = static_cast<Color>(ci);
                int d0 = tileConnectTable[tl][ci][0];
                int d1 = tileConnectTable[tl][ci][1];
                if(last.any(d0) && last.any(d1)){ // 同じ色が両側元々あった(forced moveであることが確実)
                    unsigned int zd0 = z * 4 + d0;
                    unsigned int zd1 = z * 4 + d1;
                    // line number
                    int lnum0 = edgeInfo(zd0).lineIndex();
                    int lnum1 = edgeInfo(zd1).lineIndex();
                    // color consistency check
                    ASSERT(line(lnum0).color() == c,
                           cerr << "line " << lnum0 << " color = " << line(lnum0).color() << " != " << c << endl; Foo(););
                    ASSERT(line(lnum1).color() == c,
                           cerr << "line " << lnum1 << " color = " << line(lnum1).color() << " != " << c << endl; Foo(););
                    if(lnum0 == lnum1){
                        int lnum = lnum0;
                        // loop
                        ret |= (Rule::LOOP << c);
                        line(lnum0).setLoop();
                        
                        // no vline check for loop line
                    }else{
                        int e0 = edgeInfo(zd0).lineEnd();
                        int e1 = edgeInfo(zd1).lineEnd();
                        
                        // connect
                        // base is smaller number
                        /*if(lnum0 < lnum1){
                         lnum = lnum1;
                         line(lnum0).setEnd(lineRef[zd0][1],
                         line(lnum1).xyd(1 - lineRef[zd1][1]));
                         }else{
                         lnum = lnum0;
                         line(lnum1).setEnd(lineRef[zd1][1],
                         line(lnum0).xyd(1 - lineRef[zd0][1]));
                         }*/
                        int lnum = lnum0;
                        int swappedlnum = lnum1;
                        int oxyd1 = line(swappedlnum).xyd(1 - e1);
                        line(lnum).assignEnd(e0, oxyd1);
                        moveInfo[mi].lineAge[c][0] = line(lnum).age(); // 線の世代保存
                        moveInfo[mi].lineAge[c][1] = line(lnum1).age(); // 線の世代保存
                        // 評価関数の差分計算
                        //lineShapeScore[c]            -= eval_params[4 + line(lnum0).shape() * 2 + 0];
                        //lineShapeScore[flipColor(c)] -= eval_params[4 + line(lnum0).shape() * 2 + 1];
                        //lineShapeScore[c]            -= eval_params[4 + line(lnum1).shape() * 2 + 0];
                        //lineShapeScore[flipColor(c)] -= eval_params[4 + line(lnum1).shape() * 2 + 1];
                        
                        modifiedLatestLineAge = max(modifiedLatestLineAge, int(max(line(lnum).age(), line(lnum1).age()))); // 更新した線の世代の最新
                        //turnInfo[turn].setChangedLine(lnum);
                        line(lnum).assignAge(turn); // 線の世代更新(このときエッジ世代は古いまま)
                        line(lnum).setShape(); // エンド型設定
                        // 評価関数の差分計算
                        //lineShapeScore[c]            += eval_params[4 + line(lnum).shape() * 2 + 0];
                        //lineShapeScore[flipColor(c)] += eval_params[4 + line(lnum).shape() * 2 + 1];
                        
                        edgeInfo(oxyd1).setLine(lnum, e0);
                        
                        --lines;
                        
                        if(swappedlnum != lines){ // not last line
                            for(int e = 0; e < 2; ++e){
                                edgeInfo(line(lines).xyd(e)).setLineIndex(swappedlnum);
                            }
                            line(swappedlnum) = line(lines);
                            
                            DERR << "line " << lines << " swapped to " << swappedlnum << " by connection!" << endl;
                            
                            //moveInfo[mi].swappedLine[c] = swappedlnum;
                        }else{
                            DERR << "line " << lines << " not swap" << endl;
                            //moveInfo[mi].swappedLine[c] = 0; // line 0 can't swap
                        }
                        line(lines).clear();
                        
                        checkToSetVictoryLine(c, lnum, ret);
                    }
                }else if(last.any(d0)){
                    unsigned int zd0 = z * 4 + d0;
                    int lnum = edgeInfo(zd0).lineIndex();
                    // color consistency check
                    ASSERT(line(lnum).color() == c,
                           cerr << "line " << lnum << " color = " << line(lnum).color() << " != " << c << endl; Foo(););
                    int tz = z + ar4[d1];
                    int od = oppositeDirection(d1);
                    color(tz).setRawColor(od, c);
                    int tzd = tz * 4 + od;
                    int e = edgeInfo(zd0).lineEnd();
                    //cerr << line(lnum).toString() << endl;
                    line(lnum).assignEnd(e, tzd);
                    moveInfo[mi].lineAge[c][0] = line(lnum).age(); // 線の世代保存
                    modifiedLatestLineAge = max(modifiedLatestLineAge, int(line(lnum).age())); // 更新した線の世代の最新
                    line(lnum).assignAge(turn); // 線の世代更新
                    //lineShapeScore[c]            -= eval_params[4 + line(lnum).shape() * 2 + 0]; // 評価関数の差分計算
                    //lineShapeScore[flipColor(c)] -= eval_params[4 + line(lnum).shape() * 2 + 1]; // 評価関数の差分計算
                    line(lnum).setShape(); // エンド型設定
                    //lineShapeScore[c] += eval_params[4 + line(lnum).shape() * 2 + 0]; // 評価関数の差分計算
                    //lineShapeScore[flipColor(c)] += eval_params[4 + line(lnum).shape() * 2 + 1]; // 評価関数の差分計算
                    edgeInfo(tzd).setLine(lnum, e);
                    edgeInfo(tzd).setAge(turn); // エッジの世代設定
                    
                    checkToSetVictoryLine(c, lnum, ret);
                }else if(last.any(d1)){
                    unsigned int zd1 = z * 4 + d1;
                    int lnum = edgeInfo(zd1).lineIndex();
                    // color consistency check
                    ASSERT(line(lnum).color() == c,
                           cerr << "line " << lnum << " color = " << line(lnum).color() << " != " << c << endl; Foo(););
                    int tz = z + ar4[d0];
                    int od = oppositeDirection(d0);
                    color(tz).setRawColor(od, c);
                    int tzd = tz * 4 + od;
                    int e = edgeInfo(zd1).lineEnd();
                    //cerr << line(lnum).toString() << endl;
                    line(lnum).assignEnd(e, tzd);
                    moveInfo[mi].lineAge[c][0] = line(lnum).age(); // 線の世代保存
                    modifiedLatestLineAge = max(modifiedLatestLineAge, int(line(lnum).age())); // 更新した線の世代の最新
                    line(lnum).assignAge(turn); // 線の世代更新
                    //lineShapeScore[c]            -= eval_params[4 + line(lnum).shape() * 2 + 0]; // 評価関数の差分計算
                    //lineShapeScore[flipColor(c)] -= eval_params[4 + line(lnum).shape() * 2 + 1]; // 評価関数の差分計算
                    line(lnum).setShape(); // エンド型設定
                    //lineShapeScore[c]            += eval_params[4 + line(lnum).shape() * 2 + 0]; // 評価関数の差分計算
                    //lineShapeScore[flipColor(c)] += eval_params[4 + line(lnum).shape() * 2 + 1]; // 評価関数の差分計算
                    edgeInfo(tzd).setLine(lnum, e);
                    edgeInfo(tzd).setAge(turn); // エッジの世代設定
                    
                    checkToSetVictoryLine(c, lnum, ret);
                }else{ // new line
                    int lnum = lines;
                    // first end is smaller zd
                    int tz0 = z + ar4[d0];
                    int tz1 = z + ar4[d1];
                    int od0 = oppositeDirection(d0);
                    int od1 = oppositeDirection(d1);
                    color(tz0).setRawColor(od0, c);
                    color(tz1).setRawColor(od1, c);
                    int tzd0 = tz0 * 4 + od0;
                    int tzd1 = tz1 * 4 + od1;
                    line(lnum).setColor(c);
                    if(tzd0 < tzd1){
                        line(lnum).assignEnd(0, tzd0);
                        line(lnum).assignEnd(1, tzd1);
                        edgeInfo(tzd0).setLine(lnum, 0);
                        edgeInfo(tzd1).setLine(lnum, 1);
                    }else{
                        line(lnum).assignEnd(0, tzd1);
                        line(lnum).assignEnd(1, tzd0);
                        edgeInfo(tzd0).setLine(lnum, 1);
                        edgeInfo(tzd1).setLine(lnum, 0);
                    }
                    line(lnum).assignAge(turn); // 線の世代設定
                    //line(lnum).setNewShape(isPlusTile(tl)); // エンド型設定
                    line(lnum).setShape(); // エンド型設定
                    //lineShapeScore[c]            += eval_params[4 + line(lnum).shape() * 2 + 0]; // 評価関数の差分計算
                    //lineShapeScore[flipColor(c)] += eval_params[4 + line(lnum).shape() * 2 + 1]; // 評価関数の差分計算
                    edgeInfo(tzd0).setAge(turn); // エッジの世代設定
                    edgeInfo(tzd1).setAge(turn); // エッジの世代設定
                    ++lines;
                }
            }
            // エッジの繋がりを調べる
            /*auto tileBits = last.tileBits(); // タイルがある場所のビット集合(4ビット)
            switch(tileBits){
                case 0:{ // isolated(初手のみ)
                    
                }break;
            }*/
            
            return ret;
        }
        void unmove(const int z, const Tile tl, const TileColor last){
            const int mi = moves;
            for(int c = 2 - 1; c >= 0; --c){ // moveが 0 ~ 1 の順なので逆に行う必要がある
                int d0 = tileConnectTable[tl][c][0];
                int d1 = tileConnectTable[tl][c][1];
                if(last.any(d0) && last.any(d1)){
                    unsigned int zd0 = z * 4 + d0;
                    unsigned int zd1 = z * 4 + d1;
                    // line number
                    int lnum0 = edgeInfo(zd0).lineIndex();
                    int lnum1 = edgeInfo(zd1).lineIndex();
                    
                    if(lnum0 == lnum1){
                        // loop
                        // set line number to clear flags
                        int lnum = lnum0;
                        line(lnum).clearFlags();
                    }else{
                        int e0 = edgeInfo(zd0).lineEnd();
                        int e1 = edgeInfo(zd1).lineEnd();
                        
                        DERR << lines << endl;
                        
                        // unswap line
                        int swappedlnum = lnum1;
                        if(swappedlnum != lines){ // swapped by this move
                            line(lines) = line(swappedlnum);
                            for(int e = 0; e < 2; ++e){
                                edgeInfo(line(swappedlnum).xyd(e)).setLineIndex(lines);
                            }
                            DERR << "line " << swappedlnum << " swapped to " << lines << " by cut!" << endl;
                        }
                        ++lines;
                        
                        // set line(lnum1)
                        line(lnum1).clear();
                        line(lnum1).setEnd(e1, zd1);
                        int zd1opp = line(lnum0).xyd(e0);
                        line(lnum1).setEnd(1 - e1, zd1opp);
                        line(lnum1).setColor(c);
                        edgeInfo(zd1opp).setLine(lnum1, 1 - e1);
                        //line(lnum1).assignAge(max(edgeInfo(zd1).age(),
                        //                       edgeInfo(zd1opp).age())); // 線の世代再設定
                        line(lnum1).setAge(moveInfo[moves - 1].lineAge[c][1]);
                        line(lnum1).setShape(); // エンド型設定
                        // set line(lnum0)
                        line(lnum0).assignEnd(e0, zd0);
                        line(lnum0).clearFlags();
                        //line(lnum0).assignAge(max(edgeInfo(zd0).age(),
                        //                   edgeInfo(line(lnum0).xyd(1 - e0)).age())); // 線の世代再設定
                        line(lnum0).assignAge(moveInfo[moves - 1].lineAge[c][0]);
                        line(lnum0).setShape(); // エンド型設定
                    }
                }else if(last.any(d0)){
                    unsigned int zd0 = z * 4 + d0;
                    int lnum = edgeInfo(zd0).lineIndex();
                    int e = edgeInfo(zd0).lineEnd();
                    int tz = z + ar4[d1];
                    int od = oppositeDirection(d1);
                    color(tz).clear(od);
                    int tzd = tz * 4 + od;
                    edgeInfo(tzd).clear();
                    line(lnum).assignEnd(e, zd0);
                    //line(lnum).assignAge(max(edgeInfo(zd0).age(),
                    //                      edgeInfo(line(lnum).xyd(1 - e)).age())); // 線の世代再設定
                    line(lnum).assignAge(moveInfo[moves - 1].lineAge[c][0]);
                    line(lnum).setShape(); // エンド型設定
                    line(lnum).clearFlags();
                }else if(last.any(d1)){
                    unsigned int zd1 = z * 4 + d1;
                    int lnum = edgeInfo(zd1).lineIndex();
                    int e = edgeInfo(zd1).lineEnd();
                    int tz = z + ar4[d0];
                    int od = oppositeDirection(d0);
                    color(tz).clear(od);
                    int tzd = tz * 4 + od;
                    edgeInfo(tzd).clear();
                    line(lnum).assignEnd(e, zd1);
                    //line(lnum).assignAge(max(edgeInfo(zd1).age(),
                    //                      edgeInfo(line(lnum).xyd(1 - e)).age())); // 線の世代再設定
                    line(lnum).assignAge(moveInfo[moves - 1].lineAge[c][0]);
                    line(lnum).setShape(); // エンド型設定
                    line(lnum).clearFlags();
                }else{ // new line
                    --lines;
                    int lnum = lines;
                    // first end is smaller zd
                    int tz0 = z + ar4[d0];
                    int tz1 = z + ar4[d1];
                    int od0 = oppositeDirection(d0);
                    int od1 = oppositeDirection(d1);
                    color(tz0).clear(od0);
                    color(tz1).clear(od1);
                    int tzd0 = tz0 * 4 + od0;
                    int tzd1 = tz1 * 4 + od1;
                    edgeInfo(tzd0).clear();
                    edgeInfo(tzd1).clear();
                    line(lnum).clear();
                }
            }
            popMove();
            hash ^= tileHashTable[z][tl];
            tile(z) = TILE_NONE;
            assignColor(z, last);
        }
        
        template<bool kPseudoLegality = false>
        int makeMoveSub(int z, Tile tl, TileColor c){
            
            /*if(moves > 200){
                cerr << toString();
            }*/
            
            int x = ZtoX(z), y = ZtoY(z);
            // TODO: forcedで初めて盤外に出ることはないので pseudoLegality
            if(!kPseudoLegality){
                if((x < MARGIN || SIZE - MARGIN <= x)
                   || (y < MARGIN || SIZE - MARGIN <= y)){
                    return OUT_BOARD; // 盤外
                }
            }
            TileColor last = color(z); // record for UNDO
            if(!kPseudoLegality){
                // 擬合法性が判定されていない場合は判定する
                if(isTile(tile(z))){ return DOUBLE; } // 2重置き
                if(!c.holds(last)){ return BAD_COLOR; } // 色矛盾
                if(!last.any() && turn != 0){ return ISOLATED; } // isolated move
                if(turn == 0){
                    if((z != XYtoZ(hx(), hy())) || (tl != PW && tl != SW)){
                        return FIRST_RESTRICTION; // first move is restricted CW or PLW
                    }
                }
            }
            
            // 少なくともここに1枚置けることは確定(forcedが続いた結果非合法にはなるかも)
            int ret = move(z, tl, last, c);
            ASSERT(ret >= 0, cerr << ret << endl;);
            // 4近傍がforcedになるかチェック
            for(int d = 0; d < 4; ++d){
                if(/*!last.any(d) && */tile(z + ar4[d]) == TILE_NONE){ // there is no tile
                    int tz = z + ar4[d];
                    TileColor tc = color(tz);
                    Tile ftile = static_cast<Tile>(forcedTileTable[tc]);
                    //TileColor forced = forcedTable[tc];
                    //if(!forced.any()){ // illegal color
                    //    return BADCOLOR;
                    //}
                    if(ftile < 0){
                        // forcedによって色矛盾がおきた
                        ret = FORCED_BAD_COLOR; break;
                    }
                    //if(forced.filled()){ // forced move
                    if(ftile < N_TILES){
                        TileColor forced = tileColorTable[int(ftile)];
                        int tret = makeMoveSub<true>(tz, ftile, forced);
                        if(tret < 0){ // illegal
                            ret = tret; break;
                        }else{
                            ret |= tret;
                        }
                    }
                }
            }
            return ret;
        }
        void unmakeMoveSub(int t){
            // unmake until turn |t|
            for(int i = moves - 1, ilast = moveIndex(t); i >= ilast ; --i){
                const MoveInfo& mv = moveInfo[i];
                unmove(mv.z, mv.tile, mv.last);
            }
            bound = turnInfo[turn].bound;
            //lineShapeScore = turnInfo[turn].lineShapeScore;
        }
        
        void pushAttack(Color c, int l, int type){
            attackInfo[c][attacks[c]].set(l, type);
            attacks[c] += 1;
        }
        
        int check1TurnConnectable(const unsigned int z0,
                                  const unsigned int z1)const{
            // 1手で接続可能な2点エッジかどうかチェックする
            // アタックの判定とダブルアタックの回避可能性判定に使える
            int x0 = ZtoX(z0), y0 = ZtoY(z0);
            int x1 = ZtoX(z1), y1 = ZtoY(z1);
            
            int dx = x1 - x0;
            int dy = y1 - y0;
            
            int adx = abs(dx);
            int ady = abs(dy);
            
            ASSERT(adx + ady > 0,); // staticでない
            if(adx + ady > 2){
                return 0;
            }
            if(adx + ady == 1){ // 確実に2マスconnectable
                return 2;
            }
            if(adx + ady == 2){ // ３マスconnectableの可能性
                if(!(adx & 1)){ // 直線型
                    int mx = (x0 + x1) / 2;
                    int my = (y0 + y1) / 2;
                    int mz = XYtoZ(mx, my);
                    
                    if(!color(mz).filled() && color(mz).any()){
                        return 3;
                    }
                }else{ // カド型
                    int z = XYtoZ(x0, y1);
                    if(!color(z).filled() && color(z).any()){
                        return 3;
                    }
                    z = XYtoZ(x1, y0);
                    if(!color(z).filled() && color(z).any()){
                        return 3;
                    }
                }
            }
            return 0;
        }
        
        bool checkSingleVictoryLineAttackSubX(const Color c, const int l, int e0 , int e1){
            if(line(l).x(e0) + 1 == lx() && isPseudoLegalTileColor<true, true, false, true,true>(line(l).xy(e1),
                                                                                                 endForcedTileColorTable[c][line(l).d(e1)][2])){
                return true;
            }
            if(line(l).x(e1) - 1 == hx() && isPseudoLegalTileColor<true, true, false, true,true>(line(l).xy(e0),
                                                                                                 endForcedTileColorTable[c][line(l).d(e0)][0])){
                return true;
            }
            return false;
        }
        bool checkSingleVictoryLineAttackSubY(const Color c, const int l, int e0 , int e1){
            if(line(l).y(e0) + 1 == ly() && isPseudoLegalTileColor<true, true, false, true,true>(line(l).xy(e1),
                                                                                                 endForcedTileColorTable[c][line(l).d(e1)][3])){
                return true;
            }
            if(line(l).y(e1) - 1 == hy() && isPseudoLegalTileColor<true, true, false, true,true>(line(l).xy(e0),
                                                                                                 endForcedTileColorTable[c][line(l).d(e0)][1])){
                return true;
            }
            return false;
        }
        
        int checkPushSingleVictoryLineAttack(const int l){
            const Color c = line(l).color();
            // 単体のビクトリーラインアタックであるか判定する
            int ldx = line(l).dx();
            if(ldx >= VICTORY_LINE_LENGTH){ // 長さ VICTORY_LINE_LENGTH - 1 以上
                if(ldx == dx() + 2){ // この場合は必ず置けるので pseudo legal が確定
                    pushAttack(c, l, VICTORY_LINE_LENGTH - 1);
                    return VICTORY_LINE_LENGTH - 1;
                }else if(ldx == dx() + 1){ // 1つ置ければライン完成
                    if(line(l).x(0) < line(l).x(1)){
                        if(checkSingleVictoryLineAttackSubX(c, l, 0, 1)){
                            pushAttack(c, l, VICTORY_LINE_LENGTH - 1);
                            return VICTORY_LINE_LENGTH - 1;
                        }
                    }else{
                        if(checkSingleVictoryLineAttackSubX(c, l, 1, 0)){
                            pushAttack(c, l, VICTORY_LINE_LENGTH - 1);
                            return VICTORY_LINE_LENGTH - 1;
                        }
                    }
                }
            }
            int ldy = line(l).dy();
            if(ldy >= VICTORY_LINE_LENGTH){ // 長さ VICTORY_LINE_LENGTH - 1 以上
                if(ldy == dy() + 2){ // この場合は必ず置けるので pseudo legal が確定
                    pushAttack(c, l, VICTORY_LINE_LENGTH - 1);
                    return VICTORY_LINE_LENGTH - 1;
                }else if(ldy == dy() + 1){ // 1つ置ければライン完成
                    if(line(l).y(0) < line(l).y(1)){
                        if(checkSingleVictoryLineAttackSubY(c, l, 0, 1)){
                            pushAttack(c, l, VICTORY_LINE_LENGTH - 1);
                            return VICTORY_LINE_LENGTH - 1;
                        }
                    }else{
                        if(checkSingleVictoryLineAttackSubY(c, l, 1, 0)){
                            pushAttack(c, l, VICTORY_LINE_LENGTH - 1);
                            return VICTORY_LINE_LENGTH - 1;
                        }
                    }
                }
            }
            return -1;
        }
        
        int checkPushLoopAttack(const int l){
            // アタックかどうか判定し追加する
            // staticな局面であることを仮定する
            // すでにループやビクトリーラインが完成していないことを前提とする
            DERR << "attack check for line " << char('a' + l) << " -> ";
            
            ASSERT(!line(l).mate(),);
            
            // ループスレート判定
            int x0 = line(l).x(0);
            int y0 = line(l).y(0);
            int x1 = line(l).x(1);
            int y1 = line(l).y(1);
            
            int sdx = x1 - x0;
            int sdy = y1 - y0;
            
            int adx = abs(sdx);
            int ady = abs(sdy);
            
            ASSERT(adx + ady > 0, cerr << line(l).toNotationString(lx(), ly()) << endl;); // staticでない
            
            if(adx + ady > 2){
                DERR << "much distance" << endl;
                return -1;
            } // ループアタックではない
            
            const Color c = line(l).color();
            
            if(adx + ady == 1){
                // 確実に2マスアタック
                // TODO: 合法性は大丈夫?
                DERR << "2d attack" << endl;
                pushAttack(line(l).color(), l, 2);
                return 2;
            }
            if(adx + ady == 2){
                // ３マスアタックの可能性あり
                if(!(adx & 1)){
                    // 直線型
                    DERR << "3d-straight attack candidate ";
                    
                    int mx = (x0 + x1) / 2;
                    int my = (y0 + y1) / 2;
                    int mz = XYtoZ(mx, my);
                    
                    if(!color(mz).filled() && color(mz).any()){
                        DERR << "OK" << endl;
                        pushAttack(c, l, 3);
                        return 3;
                    }
                }else{
                    // 凹カド型
                    DERR << "3d-corner attack candidate ";
                    
                    /*if(isCornerLine<true>(l)){
                        // コーナーなので基本的にアタックではない
                        // TODO: もしカドに置けるならばアタックの可能性があるかも?チェックすべき
                        return -1; //
                    }*/
                    
                    int z = XYtoZ(x0, y1);
                    /*if(!color(z).filled() && color(z).any()){
                        DERR << "OK" << endl;
                        pushAttack(c, l, 3);
                        return 3;
                    }*/
                    if(isPseudoLegalMove(z)){
                        DERR << "OK" << endl;
                        pushAttack(c, l, 3);
                        return 3;
                    }
                    
                    z = XYtoZ(x1, y0);
                    /*if(!color(z).filled() && color(z).any()){
                        DERR << "OK" << endl;
                        pushAttack(c, l, 3);
                        return 3;
                    }*/
                    if(isPseudoLegalMove(z)){
                        DERR << "OK" << endl;
                        pushAttack(c, l, 3);
                        return 3;
                    }
                    
                }DERR << " NG" << endl;
                return -1;
            }
            
            // ビクトリーライン判定
            // 片側が盤端であり，もう片側を盤端より向こう側に置ければ勝ち
            if(dx() >= VICTORY_LINE_LENGTH - 1 - 1){
                
            }
            if(dy() >= VICTORY_LINE_LENGTH - 1 - 1){
                
            }
            
            return -1;
        }
        
        void clearAttacks()noexcept{
            attacks[0] = attacks[1] = 0;
        }
    };
    
    std::string toComparedString(const Board& bd0, const Board& bd1){
        return lineUp(bd0.toString(), bd1.toString(), 1);
    }
    
    template<bool NO_FIRST_TURN = false, class move_t>
    int generateMoves(move_t *const pmv0, const Board& bd){
        // no forced-illegality check
        if(!NO_FIRST_TURN && bd.turn == 0){ // first move
            pmv0->set(Z_FIRST, PW);
            (pmv0 + 1)->set(Z_FIRST, SW);
            return 2;
        }
        move_t *pmv = pmv0;
        for(int l = 0; l < bd.lines; ++l){
            for(int e = 0; e < 2; ++e){
                int xy = bd.line(l).xy(e);
                TileColor c = bd.color(xy);
                BitSet8 moveBits = tileMoveBitTable[c];
                iterate(moveBits, [&pmv, xy](int tm){
                    //cerr << XY(xy) << " " << tm << endl;
                    pmv->set(xy, tm);
                    ++pmv;
                });
            }
        }
        return pmv - pmv0;
    }
    
    template<bool NO_FIRST_TURN = false, class move_t>
    int generateNewerLineMoves(move_t *const pmv0, const Board& bd){
        // 線の新しい順に生成
        if(!NO_FIRST_TURN && bd.turn == 0){ // first move
            pmv0->set(Z_FIRST, PW);
            (pmv0 + 1)->set(Z_FIRST, SW);
            return 2;
        }
        std::array<int, N_TURNS> lineIndex;
        for(int l = 0; l < bd.lines; ++l){
            lineIndex[l] = l;
        }
        std::sort(lineIndex.begin(), lineIndex.begin() + bd.lines);
        move_t *pmv = pmv0;
        for(int l = bd.lines - 1; l >= 0; --l){
            for(int e = 0; e < 2; ++e){
                int xy = bd.line(lineIndex[l]).xy(e);
                TileColor c = bd.color(xy);
                BitSet8 moveBits = tileMoveBitTable[c];
                iterate(moveBits, [&pmv, xy](int tm){
                    //cerr << XY(xy) << " " << tm << endl;
                    pmv->set(xy, tm);
                    ++pmv;
                });
            }
        }
        return pmv - pmv0;
    }
    
    template<class move_t = Move, bool kRemoveIllegalMoves = false>
    std::vector<move_t> generateMoveVector(Board& bd){
        std::vector<move_t> v;
        if(bd.turn == 0){ // first move
            v.push_back(move_t(Move(Z_FIRST, PW)));
            v.push_back(move_t(Move(Z_FIRST, SW)));
            return v;
        }
        for(int l = 0; l < bd.lines; ++l){
            for(int e = 0; e < 2; ++e){
                int xy = bd.line(l).xy(e);
                TileColor c = bd.color(xy);
                BitSet8 moveBits = tileMoveBitTable[c];
                iterate(moveBits, [&bd, &v, xy](int tm){
                    //cerr << XY(xy) << " " << tm << endl;
                    Move mv(xy, tm);
                    if(kRemoveIllegalMoves){
                        if(bd.isLegalMove(mv)){
                            v.push_back(move_t(mv));
                        }
                    }else{
                        v.push_back(move_t(mv));
                    }
                });
            }
        }
        return v;
    }
    
    template<class dice_t>
    Move playRandomly(const Board& bd, dice_t *const pdice){
        Move move[1024];
        int moves = generateMoves(move, bd);
        return move[pdice->rand() % moves];
    }
}

#endif // TRAX_BOARD_HPP_