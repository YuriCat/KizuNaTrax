/*
 node.hpp
 Katsuki Ohto
 */

#ifndef TRAX_NODE_HPP_
#define TRAX_NODE_HPP_

#include "trax.hpp"
#include "board.hpp"
//#include "ring_board.hpp"

// 盤面表現クラスに探索用の情報を付加し
// 技巧のルーチンへの対応を行ったもの

namespace Trax{
    
    template<class board_t>
    class TraxNode : public board_t{
        public:
        bool MoveIsLegal(const Move& mv){ return board_t::isLegalMove(mv); }
        int MakeMove(const Move& mv){ return board_t::makeMove(mv); }
        Key64 key()const noexcept{ return static_cast<Key64>((board_t::hash & (~1ULL)) | board_t::turnColor()); }
        
        template<typename score_t>
        bool DetectRepetition(score_t *const psc){ return false; } // 千日手なし
        
        Score evaluate(const Color myColor){
            // ヌルムーブ枝刈りのために相手の色でも呼べるように色を引数に持つ
            // 手番を持つ(次にプレーする)プレーヤーをmyColorとする
            const int *const params = eval_params;
            //board_t::updateEvalInfo(params);
            board_t::updateEvalInfo();
            
            const Color oppColor = flipColor(myColor);
            /*return static_cast<Score>(48
                                      + board_t::threats[myColor] * 678
                                      + board_t::threats[oppColor] * 14
                                      
                                      + board_t::longLines[myColor] * 133
                                      + board_t::longLines[oppColor] * -89
                                      
                                      + board_t::end2endLines[myColor][0] * 40
                                      + board_t::end2endLines[oppColor][0] * -32
                                      
                                      + board_t::end2endLines[myColor][1] * 4
                                      + board_t::end2endLines[oppColor][1] * 20
                                      
                                      + board_t::end2endLines[myColor][2] * -90
                                      + board_t::end2endLines[oppColor][2] * 40
                                      
                                      + board_t::corners[myColor] * 38
                                      + board_t::corners[oppColor] * -48
                                      
                                      + board_t::colorLines[myColor] * -67
                                      + board_t::colorLines[oppColor] * 96
                                      
                                      + board_t::sumInvLineEndMD[myColor] * 262
                                      + board_t::sumInvLineEndMD[oppColor] * -301
                                      
                                      );*/
            
            int s = params[1028];
            s += board_t::threats[myColor] * params[0];
            s += board_t::threats[oppColor] * params[1];
            
            s += board_t::longLines[myColor] * params[2];
            s += board_t::longLines[oppColor] * params[3];
            
            /*for(int i = 0; i < 3; ++i){
                s += board_t::end2endLines[myColor][i] * params[4 + i * 2];
                s += board_t::end2endLines[oppColor][i] * params[4 + i * 2 + 1];
            }*/
            
            /*for(int i = 0; i < 16 * 16; ++i){
                s += board_t::frontShapeLines[myColor][i] * params[4 + i * 2];
                s += board_t::frontShapeLines[oppColor][i] * params[4 + i * 2 + 1];
            }*/
            
            s += board_t::lineShapeScore[myColor];
            s += board_t::twoLinesFrontShapeScore[myColor];
            
            /*for(int i = 0; i < 16 * 16; ++i){
                s += board_t::twoLinesFrontShape[myColor][i] * params[516 + i * 2];
                s += board_t::twoLinesFrontShape[oppColor][i] * params[516 + i * 2 + 1];
                //s += board_t::twoLinesFrontShape[2][i] * params[36 + i * 3 + 2];
            }*/
            
            return static_cast<Score>(s);
        }
        std::vector<double> getFeatureVector(){
            
            board_t::updateEvalInfo();
            // 特徴ベクトルを返す
            std::vector<double> v;
            
            const Color myColor = board_t::turnColor();
            const Color oppColor = flipColor(myColor);
            
            //v.push_back(board_t::attacks[myColor]);
            //v.push_back(board_t::attacks[oppColor]);
            
            v.push_back(board_t::threats[myColor]);
            v.push_back(board_t::threats[oppColor]);
            
            v.push_back(board_t::longLines[myColor]);
            v.push_back(board_t::longLines[oppColor]);
            
            //v.push_back(board_t::end2endLines[myColor][0]);
            //v.push_back(board_t::end2endLines[oppColor][0]);
            
            //v.push_back(board_t::end2endLines[myColor][1]);
            //v.push_back(board_t::end2endLines[oppColor][1]);
            
            //v.push_back(board_t::end2endLines[myColor][2]);
            //v.push_back(board_t::end2endLines[oppColor][2]);
            
            //v.push_back(board_t::corners[myColor]);
            //v.push_back(board_t::corners[oppColor]);
            
            //v.push_back(board_t::colorLines[myColor]);
            //v.push_back(board_t::colorLines[oppColor]);
            
            //v.push_back(board_t::sumInvLineEndMD[myColor]);
            //v.push_back(board_t::sumInvLineEndMD[oppColor]);
            
            /*for(int i = 0; i < 16 * 16; ++i){
                v.push_back(board_t::frontShapeLines[myColor][i]);
                v.push_back(board_t::frontShapeLines[oppColor][i]);
            }
            
            for(int i = 0; i < 16 * 16; ++i){
                v.push_back(board_t::twoLinesFrontShape[myColor][i]);
                v.push_back(board_t::twoLinesFrontShape[oppColor][i]);
                //v.push_back(board_t::twoLinesFrontShape[2][i]);
            }*/
            
            return v;
        }
        
        Score evaluateMove(const Color myColor)const{
            // ヌルムーブ枝刈りのために相手の色でも呼べるように
            // 今プレーしたプレーヤーをmyColorとする
            Score tmp = static_cast<Score>(0
                                           + board_t::attacks[myColor] * 2000);
            return tmp - evaluate(flipColor(myColor));
        }
        
        /*void set(const board_t& bd){
            (*this) = bd;
        }*/
        
        TraxNode(const board_t& abd): board_t(abd){}
        TraxNode(): board_t(){
            board_t::clear();
        }
    };
    
    //template<class board_t, class move_t>
    //move_t8 GenerateMoves<kGt>(
    
}

#endif // TRAX_NODE_HPP_