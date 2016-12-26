/*
 search.hpp
 Katsuki Ohto
 */

#ifndef TRAX_SEARCH_HPP_
#define TRAX_SEARCH_HPP_

#include "trax.hpp"
#include "kizuna.h"

namespace Trax{
    namespace KizuNa{
        
        // TODO: アタックが擬合法だが合法ではないとき
        
        // -R @0+ B1+ C1+ @1+ D0+ D0/ D0/ C2+ E4/ F2+ D0/ D6\ F0/ G4/ -F
        
        // -R @0+ B1+ C1+ @1+ D0+ D0/ B2/ C4\ -F
        
        void Search::PrepareForNextSearch() {
            // 探索情報をリセットする
            /*num_nodes_searched_ = 0;
             max_reach_ply_ = 0;
             pv_table_.Clear();
             history_.Clear();
             countermoves_.Clear();
             followupmoves_.Clear();
             gains_.Clear();*/
            
            // マスタースレッドの場合は、スレッド間で共有する置換表の世代を更新する
            if (isMasterThread()) {
                Global::tt.NextAge();
            }
        }
        
        template<bool kIsPv>
        inline bool HashCutOk(Bound bound, Score hash_score, Score beta) {
            if (kIsPv) {
                return bound == kBoundExact;
            } else {
                return hash_score >= beta ? (bound & kBoundLower) : (bound & kBoundUpper);
            }
        }
        
        constexpr size_t halfDensityTableSize = 20;
        const std::vector<int> halfDensityTable[halfDensityTableSize] = {
            // lazy smpで先細りな割り当てを行うためのテーブル
            {0, 1},
            {1, 0},
            {0, 0, 1, 1},
            {0, 1, 1, 0},
            {1, 1, 0, 0},
            {1, 0, 0, 1},
            {0, 0, 0, 1, 1, 1},
            {0, 0, 1, 1, 1, 0},
            {0, 1, 1, 1, 0, 0},
            {1, 1, 1, 0, 0, 0},
            {1, 1, 0, 0, 0, 1},
            {1, 0, 0, 0, 1, 1},
            {0, 0, 0, 0, 1, 1, 1, 1},
            {0, 0, 0, 1, 1, 1, 1, 0},
            {0, 0, 1, 1, 1, 1, 0 ,0},
            {0, 1, 1, 1, 1, 0, 0 ,0},
            {1, 1, 1, 1, 0, 0, 0 ,0},
            {1, 1, 1, 0, 0, 0, 0 ,1},
            {1, 1, 0, 0, 0, 0, 1 ,1},
            {1, 0, 0, 0, 0, 1, 1 ,1},
        };
        
        template<class board_t, class moveIterator_t>
        std::tuple<Move, Score> Search::searchRawMate(const Depth depth,
                                                      board_t& bd,
                                                      moveIterator_t *const bufferIterator){
            // 高速化を掛けずに詰みを探索
            const Color myColor = bd.turnColor();
            const Color oppColor = flipColor(myColor);
            const int moves = generateMoves(bufferIterator, bd);
            Score bestScore = -kScoreMate;
            Move bestMove = *bufferIterator;
            for(int m = 0; m < moves; ++m){
                int ret = bd.template makeMove<true>(bufferIterator[m]);
                if(ret < 0){ // illegal move
                    continue;
                }
                
                Score score;
                if(ret & (Rule::WON << myColor)){ // my mate
                    score = kScoreMate;
                }else if(ret & (Rule::WON << oppColor)){ // opponent mate
                    score = -kScoreMate;
                }else{
                    if(depth > kDepthZero){
                        auto ms = searchRawMate(depth - 1, bd, bufferIterator + moves);
                        score = -std::get<1>(ms);
                    }else{
                        score = kScoreZero;
                    }
                }
                bd.template unmakeMove<true>();
                if(score >= kScoreKnownWin){
                    return std::make_tuple(bufferIterator[m], score);
                }
                if(score > bestScore){
                    bestScore = score;
                    bestMove = bufferIterator[m];
                }
                ASSERT(bd.exam(),);
            }
            return std::make_tuple(bestMove, bestScore);
        }
        
        template<Search::NodeType kNodeType, class board_t, class moveIterator_t>
        MoveScore Search::search(board_t& bd,
                                 Score alpha, Score beta,
                                 const Depth depth,
                                 const int ply,
                                 moveIterator_t *const bufferIterator){
            
            constexpr bool kIsRoot = kNodeType == kRootNode;
            constexpr bool kIsPv   = kNodeType == kPvNode || kNodeType == kRootNode;
            
            assert(-kScoreInfinite <= alpha && alpha < beta && beta <= kScoreInfinite);
            
            DERR << alpha << " - " << beta << endl;
            
            const Color myColor = bd.turnColor();
            const Color oppColor = flipColor(myColor);
            int moves;
            
            // ノードを初期化する
            StackData* const ss = search_stack_at_ply(ply);
            const bool in_check = bd.attacks[oppColor] > 0;
            bool mate3_tried = false;
            
            Move bestMove;
            Score bestScore = -kScoreInfinite;
            ss->currentMove = ss->hashMove = (ss + 1)->excludedMove = bestMove = kMoveNone;
            (ss + 1)->skipNullMove = false;
            (ss + 1)->reduction = kDepthZero;
            // 2手先のkillerの初期化
            (ss + 2)->killers[0] = (ss + 2)->killers[1] = kMoveNone;
            
            // 評価値計算
            //DERR << "corners : " << bd.countCorners(myColor) << " " << bd.countCorners(oppColor) << endl;
            
            
            
            /*if(depth <= kDepthZero && !bd.attacks[oppColor]){
             // ここで探索終了の場合
             return staticScore;
             }*/
            
            // 置換表を参照する
            //Move excluded_move = ss->excluded_move;
            //Key64 pos_key = excluded_move != kMoveNone ? node.exclusion_key() : node.key();
            int pat;
            Key64 positionKey = (bd.moves < 8) ? static_cast<Key64>(calcRepRelativeHash(bd, pat)) : bd.key();
            //Key64 positionKey = bd.key();
            const HashEntry* entry = Global::tt.LookUp(positionKey);
            
            Score hashScore = entry ? entry->score() : kScoreNone;
            Move hashMove = entry ? entry->move() : kMoveNone;
            //ss->hash_move = hash_move;
            
            // Hash Cut
            if (1
                && !kIsPv
                //&& !learning_mode_
                && entry != nullptr
                && entry->depth() >= depth
                && hashScore != kScoreNone // Only in case of TT access race
                && hashMove != kMoveNone
                && HashCutOk<kIsPv>(entry->bound(), hashScore, beta)
                ) {
                ss->currentMove = hashMove; // hash_move == kMoveNone になりうる
                /*if (   hash_score >= beta
                 && hash_move != kMoveNone
                 && hash_move.is_quiet()
                 && !in_check) {
                 UpdateStats(ss, hash_move, depth, nullptr, 0);
                 }*/
                
                //cerr << "hash cut!" << endl;
                
                if (   hashScore >= beta
                    && hashMove != kMoveNone
                    && !in_check){
                    
                    // キラー手の更新
                    if(!kIsRoot){
                        if(ss->killers[0] != hashMove){
                            ss->killers[1] = ss->killers[0];
                            ss->killers[0] = hashMove;
                        }
                    }
                    // ヒストリーの更新
                    //Score bonus = Score((depth / kOnePly) * int(depth / kOnePly) + 2 * depth / kOnePly - 2);
                    //historyStats_[myColor].update(bestMove, bonus);
                }
                
                Global::hashCut += 1;
                
                //return hashScore;
                return MoveScore(hashMove, hashScore);
            }
            
            // 相手の色のスレートがある場合、回避手のみ生成
            
            // 相手のアタックが無く、自分のスレートがある場合勝ち
            
            // まずキラー手を試す
            if(!kIsRoot){ // ルートノードは関係なし
                for(int k = 0; k < 2; ++k){
                    Move move = ss->killers[k];
                    if(move != kMoveNone
                       && bd.isPseudoLegalMove(move)){ // 全くもって非合法な場合もある
                        int ret = bd.template makeMove<true>(move);
                        ss->currentMove = move;
                        
                        Global::nodes += 1;
                        
                        if(ret < 0){ // illegal move
                            ASSERT(bd.exam(),);
                            continue;
                        }
                        // 簡単な判定はここでかける
                        Score score;
                        if(ret & (Rule::WON << myColor)){ // my mate
                            score = +kScoreMate - static_cast<Score>(bd.turn);
                            Global::myMate += 1;
                        }else if(ret & (Rule::WON << oppColor)){ // opponent mate
                            score = -kScoreMate + static_cast<Score>(bd.turn);
                            Global::oppMate += 1;
                        }else{
                            bd.checkSetAttacks(); // アタック情報を更新
                            if(depth < kOnePly * 8
                               && bd.attacks[oppColor]){ // 相手の色のアタックが有ったら負け
                                score = -kScoreMate + static_cast<Score>(bd.turn + 1);
                                Global::oppAttack += 1;
                            }else if(depth < kOnePly * 2
                                     && bd.hasInevasibleAttacks(myColor)){
                                // 相手の色のアタックが無く、自分の色のアタックが回避不能であれば勝ち
                                // 現在不正確なので完全な詰みよりも点を低くする
                                //cerr << bd.toString();
                                score = +kScoreAlmostWin - static_cast<Score>(bd.turn + 2);
                                Global::myDoubleAttacks += 1;
                            }else{
                                // 通常の評価に入る
                                if(depth <= kDepthZero && (!bd.attacks[myColor] || bd.moves > kMaxTiles)){
                                    score = -bd.evaluate(oppColor);
                                    score += static_cast<Score>((Global::dice.rand() % 20) - 10); // random score
                                }else{
                                    MoveScore ms;
                                    
                                    Depth nextDepth = depth - kOnePly;
                                    // 王手延長
                                    if(bd.attacks[myColor]){
                                        nextDepth += kOnePly / 2;
                                    }
                                    
                                    if(kIsRoot){
                                        ms = search<kPvNode>(bd, -beta, -alpha, nextDepth, ply + 1, bufferIterator);
                                    }else{
                                        ms = search<kNonPvNode>(bd, -beta, -alpha, nextDepth, ply + 1, bufferIterator);
                                    }
                                    score = static_cast<Score>(-ms.score);
                                }
                            }
                        }
                        bd.template unmakeMove<true>();
                        
                        if (Global::signals.load() & Global::SIGNAL_STOP) {
                            return MoveScore(kMoveNone, kScoreZero);
                        }
                        
                        if(score > beta){
                            bestScore = score;
                            bestMove = move;
                            goto search_end;
                        }else{
                            alpha = max(alpha, score);
                            if(score > bestScore){
                                bestScore = score;
                                bestMove = move;
                            }
                        }
                    }
                }
            }
            
            // 着手生成
            // ルート以外では初手の可能性はない
            moves = kIsRoot ? generateMoves(bufferIterator, bd) : generateNewerLineMoves(bufferIterator, bd);//generateMoves<true>(bufferIterator, bd);
            
            if(0 && threadIndex_ == 0 && ply == 0){
                cerr << bd.toString();
                for(int m = 0; m < moves; ++m){
                    cerr << toNotationString(Move(bufferIterator[m]), bd) << " ";
                }
                cerr << endl;
            }
            
            DERR << "my color = " <<  myColor << " " << colorChar[myColor] << endl;
            
            // 着手のオーダリング
            // アタック加点, それ以外は評価値で
            /*if(depth > kOnePly * 2){
             for(int m = 0; m < moves; ++m){
             Move move = Move(bufferIterator[m]);
             // 着手のスコア計算
             Score sc;
             int tret = bd.makeMove<true>(Move(bufferIterator[m]));
             if(tret < 0){
             sc = kScoreFoul;
             }else{
             if(tret & (Rule::WON << myColor)){ // my mate
             sc = +kScoreMate - static_cast<Score>(bd.turn);
             //Global::myMate += 1;
             }else if(tret & (Rule::WON << oppColor)){ // opponent mate
             sc = -kScoreMate + static_cast<Score>(bd.turn);
             //Global::oppMate += 1;
             }else{
             bd.checkSetAttacks(); // アタック情報を更新
             sc = bd.evaluateMove(myColor);
             }
             bd.template unmakeMove<true>();
             }
             
             // ヒストリーテーブルによるスコア計算
             Score sc = m * 4 - historyStats_[myColor].get(move);
             bufferIterator[m].score = sc;
             }
             std::sort(bufferIterator, bufferIterator + moves);
             }*/
            
            for(int m = 0; m < moves && alpha < beta; ++m){
                
                const bool is_pv_move = kIsPv && m == 0;
                
                // 手を進める前の枝刈り
                //CERR << bd.toString();
                Move move = Move(bufferIterator[m]);
                
                // キラー手はもう探索したのでやらない
                if(!kIsRoot){
                    if(move == ss->killers[0] || move == ss->killers[1]){
                        continue;
                    }
                }
                
                // 相手のカウンター最善は見ない
                /*if(move != hashMove && ply >= 2 && depth < 2 * kOnePly){
                 if(Global::counterMoveStats[oppColor].get((ss - 1)->currentMove) == move){
                 continue;
                 }
                 }*/
                
                if(threadIndex_ == 3){
                    //cerr << std::string(ply * 4, ' ') << toNotationString(move, bd) << endl;
                }
                
                // ヒストリー枝刈り
                //if(Global::historyStats[move.z()][move.tile()] < )
                
                //ASSERT(bd.isPseudoLegalMove(move), cerr << move << endl;);
                int ret = bd.template makeMove<true>(move);
                ss->currentMove = move;
                
                Global::nodes += 1;
                
                if(ret < 0){ // illegal move
                    ASSERT(bd.exam(),);
                    continue;
                }
                
                // 簡単な判定はここでかける
                Score score;
                if(ret & (Rule::WON << myColor)){ // my mate
                    score = +kScoreMate - static_cast<Score>(bd.turn);
                    Global::myMate += 1;
                }else if(ret & (Rule::WON << oppColor)){ // opponent mate
                    DERR << fat("opponent mate") << endl;
                    score = -kScoreMate + static_cast<Score>(bd.turn);
                    Global::oppMate += 1;
                }else{
                    bd.checkSetAttacks(); // アタック情報を更新
                    if(depth < kOnePly * 8
                       && bd.attacks[oppColor]){ // 相手の色のアタックが有ったら負け
                        score = -kScoreMate + static_cast<Score>(bd.turn + 1);
                        Global::oppAttack += 1;
                    }else if(depth < kOnePly * 2
                             && bd.hasInevasibleAttacks(myColor)){
                        // 相手の色のアタックが無く、自分の色のアタックが回避不能であれば勝ち
                        // 現在不正確なので完全な詰みよりも点を低くする
                        //cerr << bd.toString();
                        score = +kScoreAlmostWin - static_cast<Score>(bd.turn + 2);
                        Global::myDoubleAttacks += 1;
                    }else{
                        // 通常の評価に入る
                        if(bd.moves < kMaxTiles
                           && (depth > kDepthZero
                               || bd.attacks[myColor]
                               //|| (in_check && depth > -kOnePly * 4)
                               //|| bd.countThreats(myColor)
                               //|| bd.countThreats(oppColor)
                               )){
                               
                               Depth nextDepth = depth - kOnePly;
                               // old line reduction
                               if(bd.modifiedLatestLineAge < bd.turn - 2){
                                   DERR << "old line reduction!" << endl;
                                   nextDepth -= kOnePly * min(4, bd.turn - 2 - bd.modifiedLatestLineAge) / 4;
                               }
                               // 王手延長
                               if(bd.attacks[myColor]){
                                   nextDepth += kOnePly / 2;
                               }
                               // ハッシュ手延長, カウンター延長, リダクション
                               /*if(hashMove == move){
                                nextDepth += kOnePly / 3;
                                }else if(ply >= 2 && depth < 2 * kOnePly){
                                Move counter = Global::counterMoveStats[myColor].get((ss - 1)->currentMove);
                                if(counter != kMoveNone){
                                if(counter != move){
                                nextDepth -= kOnePly / 4;
                                }else{
                                //cerr << (ss - 1)->currentMove << " -> " << move << endl; getchar();
                                nextDepth += kOnePly / 4;
                                }
                                }
                                }*/
                               
                               // hash move extension
                               /*if(hashMove == move){
                                nextDepth += kOnePly / 3;
                                }
                                
                                // move count reduction
                                if(m > 4){
                                nextDepth -= kOnePly / 8;
                                }else if(m > 16){
                                nextDepth -= kOnePly / 4;
                                }*/
                               
                               // razoring
                               //if(hashMove != kMoveNone)
                               
                               // futility pruning
                               if(!bd.attacks[myColor]
                                  && depth < kOnePly
                                  && hashMove != kMoveNone
                                  && hashScore > beta + 256){
                                   // betaカットとしておく
                                   score = hashScore - 256;
                               }else if(!bd.attacks[myColor]
                                        && depth < kOnePly
                                        && hashMove != kMoveNone
                                        && hashScore < alpha - 256){
                                   // 読まない
                                   score = hashScore + 256;
                               }else{
                                   
                                   MoveScore ms;
                                   if(kIsRoot){
                                       ms = search<kPvNode>(bd, -beta, -alpha, nextDepth , ply + 1, bufferIterator + moves);
                                   }else{
                                       ms = search<kNonPvNode>(bd, -beta, -alpha, nextDepth, ply + 1, bufferIterator + moves);
                                   }
                                   score = static_cast<Score>(-ms.score);
                               }
                           }else{
                               score = -bd.evaluate(oppColor);
                               //score += static_cast<Score>((Global::dice.rand() % 20) - 10); // random score
                           }
                    }
                }
                bd.template unmakeMove<true>();
                
                //bufferIterator[m].score = score;
                
                if (Global::signals.load() & Global::SIGNAL_STOP) {
                    return MoveScore(kMoveNone, kScoreZero);
                }else if(Global::clock.stop() > LIMIT_TIME){ // 時間管理
                    Global::signals |= Global::SIGNAL_STOP;
                    return MoveScore(kMoveNone, kScoreZero);
                }
                
                if (kIsRoot) {
                    //auto& rootMoves = rootMoves_;
                    //RootMove& rm = *std::find(rootMoves.begin(), rootMoves.end(), move);
                    
                    // PV及び評価値を更新する
                    /*if (is_pv_move || score > alpha) {
                     rm.score = score;
                     rm.pv.resize(1);
                     pv_table_.CopyPv(move, ply);
                     for (size_t i = 1; i < pv_table_.size(); ++i) {
                     rm.pv.push_back(pv_table_[i]);
                     }
                     } else {
                     rm.score = -kScoreInfinite;
                     }*/
                    
                    // 時間管理用の情報を記録する
                    //shared_.signals.first_move_completed = true; // 最善手の探索を終えた
                    //rm.nodes += (num_nodes_searched_ - nodes_before_search); // この手の探索に用いたノード数
                }
                
                /*if (score > bestScore) {
                 bestScore = score;
                 
                 if (score > alpha) {
                 bestMove = move;
                 
                 if (kIsPv && !kIsRoot) {
                 //pvTable_.CopyPv(move, ply);
                 }
                 if(kIsPv && score < beta) { // 常に alpha < beta にする
                 alpha = score;
                 }else{
                 assert(score >= beta); // fail high
                 // 統計データを更新
                 //if (true) {
                 //    g_sum_move_counts += searched_move_count;
                 //    g_num_beta_cuts += 1;
                 //    g_cuts_by_move[std::min(63, searched_move_count)] += 1;
                 //}
                 break;
                 }
                 }
                 }*/
                
                if(score > beta){
                    bestScore = score;
                    bestMove = move;
                    goto search_end;
                }else{
                    alpha = max(alpha, score);
                    if(score > bestScore){
                        bestScore = score;
                        bestMove = move;
                    }
                    
                }
                
                //search_next_move:;
                ASSERT(bd.exam(),);
            }
            
        search_end:
            // 置換表に新しいデータを保存
            Global::tt.Save(positionKey, bestMove, bestScore, depth,
                            bestScore >= beta              ? kBoundLower :
                            kIsPv && bestMove != kMoveNone ? kBoundExact : kBoundUpper,
                            //ss->static_score,
                            bestScore,
                            false);
            // ヒストリーの更新
            //Score bonus = Score((depth / kOnePly) * int(depth / kOnePly) + 2 * depth / kOnePly - 2);
            //Global::historyStats.update(bestMove, (myColor == WHITE) ? bestScore : -bestScore);
            //historyStats_[myColor].update(bestMove, bonus);
            // 応手の更新
            /*if(!kIsRoot){
             Global::counterMoveStats[myColor].update((ss - 1)->currentMove, bestMove);
             }*/
            // キラー手の更新
            if(!kIsRoot){
                if(ss->killers[0] != bestMove){
                    ss->killers[1] = ss->killers[0];
                    ss->killers[0] = bestMove;
                }
            }
            
            return MoveScore(bestMove, bestScore);
        }
        
        template<class board_t, class moves_t>
        MoveScore Search::searchRoot(board_t& bd, moves_t& moves, Score alpha, Score beta, Depth depth){
            const Color myColor = bd.turnColor();
            const Color oppColor = flipColor(myColor);
            Move bestMove = kMoveNone;
            Score bestScore = -kScoreInfinite;
            MoveScore ms;
            Score previousBestScore = static_cast<Score>(moves[0].score);
            for(size_t m = 0; m < moves.size(); ++m){
                
                Move move = Move(moves[m]);
                int ret = bd.template makeMove<true>(move);
                Global::nodes += 1;
                
                //ms = MoveScore(kMoveNull);
                
                Score score;
                if(ret < 0){
                    continue; // ここでcontinueしないと大変なことに
                }else if(ret & (Rule::WON << myColor)){ // my mate
                    score = +kScoreMate - static_cast<Score>(bd.turn);
                    Global::myMate += 1;
                }else if(ret & (Rule::WON << oppColor)){ // opponent mate
                    score = -kScoreMate + static_cast<Score>(bd.turn);
                    Global::oppMate += 1;
                }else{
                    bd.checkSetAttacks(); // アタック情報を更新
                    if(depth < kOnePly * 8
                       && bd.attacks[oppColor]){ // 相手の色のアタックが有ったら負け
                        score = -kScoreMate + static_cast<Score>(bd.turn + 1);
                        Global::oppAttack += 1;
                    }else if(depth < kOnePly * 2
                             && bd.hasInevasibleAttacks(myColor)){
                        // 相手の色のアタックが無く、自分の色のアタックが回避不能であれば勝ち
                        //cerr << bd.toString();
                        score = +kScoreAlmostWin - static_cast<Score>(bd.turn + 2);
                        Global::myDoubleAttacks += 1;
                    }else{
                        if(depth <= kDepthZero && !bd.attacks[myColor]){
                            score = -bd.evaluate(oppColor);
                        }else{
                            
                            Depth nextDepth = depth - kOnePly;
                            
                            // 王手延長
                            if(bd.attacks[myColor]){
                                nextDepth += kOnePly / 2;
                            }
                            
                            if(m > 0){
                                // ルートでの評価値差によるリダクション
                                // イテレーションが増えるほど信頼出来るはずなので効果を大きくする
                                //nextDepth -= Depth(int(sqrt(previousBestScore - moves[m].score)) * (double)kOnePly / (28 / sqrt((double)depth / double(kOnePly))));
                                nextDepth -= Depth(int(sqrt(previousBestScore - moves[m].score)) * kOnePly / 17);
                                // ルートでの順位によるリダクション
                                nextDepth = Depth((double)nextDepth * (2.8 / (4 + m) + 0.3));
                            }else{
                                // 最善延長
                                //nextDepth += kOnePly / 2;
                            }
                            
                            
                            ms = search<kPvNode>(bd, -beta, -alpha, nextDepth, 1, buffer_.begin());
                            score = -static_cast<Score>(ms.score);
                            //cerr << "search score = " << score << endl;
                        }
                    }
                }
                // 評価点保存
                //cerr << score << endl;
                
                bd.template unmakeMove<true>();
                
                //if(Move(ms) != kMoveNone){
                    moves[m].score = score;
                    
                    if(score > beta){
                        bestScore = score;
                        bestMove = move;
                        break;
                    }else{
                        alpha = max(alpha, score);
                        if(score > bestScore){
                            bestScore = score;
                            bestMove = move;
                        }
                    }
               // }
                
                if(Global::signals.load() & Global::SIGNAL_STOP){
                    return MoveScore(kMoveNone, kScoreZero);
                }
            }
            return MoveScore(bestMove, bestScore);
        }
        
        template<class board_t>
        //SearchResult
        
        MoveScoreDepth Search::iterativeDeepening(board_t& bd){
            
            Global::signals |= 1ULL << threadIndex_; // 探索中のスレッドフラグをつける
            
            prepareSearch(); // 探索スタック等初期化
            ClockMS localClock; // ponderスレッドがいつまでも生き残らないようにローカルの時計でも終了判定する
            localClock.start();
            
            MoveScoreDepth best;
            Score score = kScoreZero;
            Score previousScore;
            
            // ルート着手を生成
            SearchResult result;
            result.moves = generateMoveVector<MoveScore>(bd);
            
            MoveScore ms;
            
            // 反復深化
            for(int iteration = 0; iteration < kMaxPly; ++iteration){
                
                // Lazy SMP
                // ワーカースレッドは、平均して２回に１回、スキップする
                if (!(isMasterThread() || ((threadIndex_ == 1)  && bd.turnColor() != Global::rootColor))){
                    const auto& halfDensity = halfDensityTable[(threadIndex_ - 1) % halfDensityTableSize];
                    if (halfDensity[(iteration + bd.turn) % halfDensity.size()]) {
                        continue;
                    }
                }
                
                // αβウィンドウをセットする
                Score alpha = -kScoreInfinite, beta = kScoreInfinite;
                
                // Aspiration Windows
                // 前回探索結果の近くに探索結果が収まると仮定して設定
                //Score halfWindow = Score(64);
                //Score halfWindow = Score(256);
                Score halfWindow = Score(65536);
                if(iteration >= 5){
                    previousScore = static_cast<Score>(best.score);
                    //Score previousScore = rootMoves_.at(pvIndex_).previousScore;
                    alpha = std::max(previousScore - halfWindow, -kScoreInfinite);
                    beta = std::min(previousScore + halfWindow, kScoreInfinite);
                }
                
                while(true){
                    // 探索を行う
                    //Depth depth = Depth(iteration * double(kOnePly) * 0.65);
                    Depth depth = iteration * kOnePly;
                    
                    //score = search<kRootNode>(bd, alpha, beta, depth, 0, buffer_.begin());
                    //rootMoves_.at(pvIndex_).score = score;
                    
                    ms = searchRoot(bd, result.moves, alpha, beta, depth);
                    
                    //ms = search<kRootNode>(bd, alpha, beta, depth, 0, buffer_.begin());
                    
                    // 置換表からPVが消える場合があるので、置換表にPVを保存しておく
                    //CERR << rootMoves_.size() << endl;
                    /*for(int i = 0; i <= pvIndex_; ++i) {
                     Global::tt.InsertMoves(bd, rootMoves_.at(i).pv);
                     }*/
                    // 停止命令が来ていたら終了
                    if(Global::signals.load() & Global::SIGNAL_STOP){
                        break;
                    }else if(localClock.stop() > LIMIT_TIME){ // 時間管理
                        Global::signals |= Global::SIGNAL_STOP;
                        break;
                    }
                    
                    // αβウィンドウを再設定する
                    if(score <= alpha){
                        // fail-low
                        alpha = std::max(alpha - halfWindow, -kScoreInfinite);
                        beta = (alpha + beta) / 2;
                    }else if(score >= beta){
                        // fail-high
                        alpha = (alpha + beta) / 2;
                        beta = std::min(beta + halfWindow, kScoreInfinite);
                    }else{
                        break;
                    }
                    // ウィンドウを指数関数的に増加させる
                    halfWindow += halfWindow / 2;
                }
                
                // 現在までに探索した指し手をソートする
                std::stable_sort(result.moves.begin(), result.moves.end(), std::greater<MoveScore>());
                
                // 変な手でないか調べる
                /*if(bd.isPseudoLegalMove(result.moves[0])){
                    best.set(Move(result.moves[0]));
                    best.score = result.moves[0].score;
                    best.depth = iteration + 1;
                    score = static_cast<Score>(best.score);
                }*/
                if(bd.isPseudoLegalMove(Move(ms))){
                    best.set(Move(ms));
                    best.score = ms.score;
                    best.depth = iteration + 1;
                    score = static_cast<Score>(best.score);
                }
                
                /*if(isMasterThread()){
                 for(auto m : result.moves){
                 cerr << Move(m) << " " << m.score << endl;
                 }
                 }*/
                
                assert(score != kScoreNone);
                
                // スタッツ表示
                if(isMasterThread()
                   || (Global::rootColor != bd.turnColor() && threadIndex_ == 1)){ // ponder時は1番スレッド
                    CERR << "iteration = " << (iteration + 1) << " time = " << Global::clock.stop();
                    CERR << " move = " << toNotationString(Move(best), bd) << " score = " << best.score;
                    CERR << " " << Global::toLineStatsString();
                    
                    /*if(abs(best.score) >= kScoreMate - (iteration + 1 + bd.turn)){
                     // 勝ち or 負け
                     Global::signals |= Global::SIGNAL_STOP; // stopを出さないとおかしくなるかも
                     break;
                     }*/
                }
                
                if(Global::signals.load() & Global::SIGNAL_STOP){
                    break;
                }else if(localClock.stop() > LIMIT_TIME){ // 時間管理
                    Global::signals |= Global::SIGNAL_STOP;
                    break;
                }
            } // イテレーションのループ
            if(isMasterThread()){
                CERR << Global::toFullStatsString();
            }
            
            //return std::move(result);
            
            Global::signals &= ~(1ULL << threadIndex_); // 探索中のスレッドフラグを消す
            
            return best;
        }
    }
}

#endif // TRAX_SEARCH_HPP_