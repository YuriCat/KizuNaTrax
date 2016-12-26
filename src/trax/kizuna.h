/*
 kizuna.h
 Katsuki Ohto
 */

#ifndef TRAX_KIZUNA_H_
#define TRAX_KIZUNA_H_

#include "trax.hpp"
#include "board.hpp"

#include "node.hpp"

//#include "book.hpp"
#include "hash.hpp"

using Position = Trax::Board;
using Node = Trax::TraxNode<Trax::Board>;

namespace Trax{
    
        
    // ルート着手クラス
    // 「技巧」より
    class RootMove {
    public:
        RootMove(Move m):move(m), pv{m}{}
        RootMove(const RootMove& rm)
        :move(rm.move),
        score(rm.score),
        previousScore(rm.previousScore),
        nodes(rm.nodes),
        pv(rm.pv){}
        
        // 評価値順ソートのための比較
        bool operator<(const RootMove& rm) const { return score < rm.score; }
        bool operator>(const RootMove& rm) const { return score > rm.score; }
        
        // 同一性
        bool operator==(Move m) const { return move == m; }
        
        Move move;
        Score score = -kScoreInfinite;
        Score previousScore = -kScoreInfinite; // 反復深化探索における、１イテレーション前の評価値
        uint64_t nodes = 0; // その手以下を探索したノード数
        std::vector<Move> pv; // その手のPV
    };
    
    namespace KizuNa{
        
        // 探索クラス
        // 「技巧」より
        class Search{
        public:
            enum NodeType {
                kRootNode, kPvNode, kNonPvNode,
            };
            
            struct StackData{
                std::array<Move, 2> killers; // キラー手
                Move hashMove; // ハッシュ手
                Move currentMove;
                Move excludedMove;
                Depth reduction;
                Score staticScore; // 静的評価点
                bool skipNullMove;
            };
            
            template<class board_t, class moveIterator_t>
            static std::tuple<Move, Score> searchRawMate(const Depth depth,
                                                         board_t& bd,
                                                         moveIterator_t *const bufferIterator);
            
            template<NodeType kNodeType = kNonPvNode, class board_t, class moveIterator_t>
            MoveScore search(board_t& bd,
                             Score alpha, Score beta,
                             Depth depth,
                             const int ply,
                             moveIterator_t *const bufferIterator);
            
            template<class board_t, class moves_t>
            MoveScore searchRoot(board_t& bd, moves_t& moves, Score alpha, Score beta, Depth depth);
            
            template<class board_t>
            MoveScoreDepth iterativeDeepening(board_t& bd);
            
            /*template<class board_t>
            static std::vector<RootMove> CreateRootMoves(board_t& root_position, // for 合法性判定
                                                         const std::vector<Move>& searchmoves,
                                                         const std::vector<Move>& ignoremoves){
                // ルート着手を生成
                std::vector<RootMove> root_moves;
                
                if(!searchmoves.empty()){
                    // searchmovesオプションの指定がある場合
                    for(Move move : searchmoves){
                        if(root_position.MoveIsLegal(move)){
                            root_moves.emplace_back(move);
                        }
                    }
                }else{
                    for(Move move : generateMoveVector<true>(root_position)){ // 完全な合法性判定付きで生成
                        root_moves.emplace_back(move);
                    }
                    // ignoremovesオプションの指定がある場合
                    for (Move move : ignoremoves) {
                        root_moves.erase(std::remove(root_moves.begin(), root_moves.end(), move),
                                         root_moves.end());
                    }
                }
                
                return root_moves;
            }
            
            std::vector<Move> GetPv() const {
                assert(!rootMoves_.empty());
                return std::max_element(rootMoves_.begin(), rootMoves_.end())->pv;
            }
            
            const RootMove& GetBestRootMove() const {
                assert(!rootMoves_.empty());
                return *std::max_element(rootMoves_.begin(), rootMoves_.end());
            }
            
            void set_root_moves(const std::vector<RootMove>& root_moves){
                // ルート着手をセット(コピー)
                rootMoves_ = root_moves;
            }*/
            
            void clearSearchStack(){
                std::memset(stack_.begin(), 0, 5 * sizeof(StackData));
            }
            
            void prepareSearch(){
                clearSearchStack();
                for(int c = 0; c < 2; ++c){
                    //historyStats_[c].clear();
                }
            }
            
            bool isMasterThread()const{
                return threadIndex_ == 0;
            }
            
            StackData* search_stack_at_ply(int ply) {
                assert(0 <= ply && ply <= kMaxPly);
                return stack_.begin() + 2 + ply; // stack_at_ply(0) - 2 の参照を可能にするため
            }
            
            void PrepareForNextSearch();
            Search(size_t threadIndex):
            threadIndex_(threadIndex){}
            
        private:
            static constexpr int kStackSize = kMaxPly + 6;
            
            //std::array<Score, 2> drawScores_{kScoreDraw, kScoreDraw};
            uint64_t numNodesSearched;
            int max_reach_ply_ = 0;
            int multipv_ = 1, pvIndex_ = 0;
            bool learning_mode_ = false;
            std::array<StackData, kStackSize> stack_; // 探索スタック
            std::array<MoveScore, 16384> buffer_; // 着手生成用バッファ
            Stats<Score> historyStats_[2]; // ヒストリー
            
            //PvTable pv_table_;
            
            //HistoryStats history_;
            //MovesStats countermoves_;
            //MovesStats followupmoves_;
            //GainsStats gains_;
            //std::vector<RootMove> rootMoves_;
            const size_t threadIndex_;
        };
        
        // 探索スレッド
        // 「技巧」より
        class SearchThread {
        public:
            SearchThread(size_t thread_id, Node& node//, SharedData& shared_data,
            //ThreadManager& thread_manager
            );
            ~SearchThread();
            void IdleLoop();
            void SetRootNode(const Node& node);
            void StartSearching();
            void WaitUntilSearchIsFinished();
            
            Search search_;
        private:
            //friend class ThreadManager;
            //ThreadManager& thread_manager_;
            Node& root_node_; // 重いので参照にする
            std::mutex mutex_;
            std::condition_variable sleep_condition_;
            std::atomic_bool searching_, exit_;
            std::thread native_thread_;
        };
        
        // スレッド管理
        // 「技巧」より
        class ThreadManager {
        public:
            ThreadManager(//SharedData& shared_data, TimeManager& time_manager
            );
            //TimeManager& time_manager() {
            //    return time_manager_;
            //}
            void SetNumSearchThreads(size_t num_threads);
            uint64_t CountNodesSearchedByWorkerThreads() const;
            uint64_t CountNodesUnder(Move move) const;
            //RootMove
            //SearchResult
            
            MoveScoreDepth ParallelSearch(Node& node, //Score draw_score,
                                    //const UsiGoOptions& go_options,
                                    const std::vector<Move> searchmoves,
                                    const std::vector<Move> ignoremoves,
                                    int multipv);
        //private:
            //SharedData& shared_data_;
            //TimeManager& time_manager_;
            std::vector<std::unique_ptr<SearchThread>> worker_threads_;
        };
        
        struct SearchResult{
            std::vector<MoveScore> moves;
            int iterations; // イテレーション回数
        };
    }
}

namespace Trax{
    namespace Global{ // Grobal Data
        XorShift64 dice;
        Color rootColor;
        std::vector<std::string> record;
        
        KizuNa::ThreadManager manager; // マルチスレッド管理
        HashTable tt; // 置換表
        std::atomic<uint64_t> signals;
        std::array<MoveScore, 16384> buffer; // 着手生成用バッファ(スレッドの準備をせずに使う用)
        Node node[N_THREADS]; // 各スレッド用の盤面表現 重いのでグローバルに置いておく
        Board rootBoard; // ルート用盤面
        ClockMS clock;
        //Book book; // 定跡
        bool pondering = true; // 相手手番中の先読みを行うか
        //std::vector<int> evalParams; // 評価関数パラメータ
        //CounterMoveStats counterMoveStats[2]; // 最近見つけた良い応手
        
        constexpr uint64_t SIGNAL_STOP = 1ULL << 63;
        constexpr uint64_t SIGNAL_THREAD_MASK = (1ULL << N_THREADS) - 1ULL;
        
        // stats(MINIMUMが付くと集計を止める)
        Counter hashCut("hashCut");
        Counter myMate("myMate");
        Counter oppMate("oppMate");
        Counter oppAttack("oppAtack");
        Counter myDoubleAttacks("doubleAttacks");
        Counter nodes("nodes");
        
        void initStats(){
            hashCut = 0;
            myMate = 0;
            oppMate = 0;
            oppAttack = 0;
            myDoubleAttacks = 0;
            nodes = 0;
        }
        
        std::string toLineStatsString(){
            std::ostringstream oss;
            oss << "nodes = " << nodes << " hashcut = " << hashCut << " hashfull = " << tt.hashfull() << endl;
            return oss.str();
        }
        std::string toFullStatsString(){
            std::ostringstream oss;
            oss << "mate = " << myMate << " omate = " << oppMate
            << " oattack = " << oppAttack << " dattacks = " << myDoubleAttacks << endl;
            return oss.str();
        }
    }
    
    namespace KizuNa{
        void ThreadManager::SetNumSearchThreads(size_t num_search_threads) {
            // 必要なワーカースレッドの数を求める（１を引いているのは、マスタースレッドの分。）
            size_t num_worker_threads = num_search_threads - 1;
            
            // ワーカースレッドを増やす場合
            while (num_worker_threads > worker_threads_.size()) {
                size_t thread_id = worker_threads_.size() + 1; // ワーカースレッドのIDは1から始める
                worker_threads_.emplace_back(new SearchThread(thread_id, Global::node[thread_id]/*, shared_data_, *this*/));
            }
            
            // ワーカースレッドを減らす場合
            while (num_worker_threads < worker_threads_.size()) {
                worker_threads_.pop_back();
            }
        }
    }
}

#include "thread.hpp"


namespace Trax{
    namespace KizuNa{
        //SearchResult
        MoveScoreDepth ThreadManager::ParallelSearch(Node& node,// const Score draw_score,
                                                     const std::vector<Move> searchmoves,
                                                     const std::vector<Move> ignoremoves,
                                                     const int multipv) {
            // goコマンドのsearchmovesとignoremovesオプションから、
            // ルート局面で探索すべき手を列挙する
            //const std::vector<RootMove> root_moves = Search::CreateRootMoves(
            //                                                                 node, searchmoves, ignoremoves);
            
            Global::initStats(); // スタッツ初期化
            
            // ワーカースレッドの探索を開始する
            for (std::unique_ptr<SearchThread>& worker : worker_threads_) {
                //worker->SetRootNode(node);
                //worker->search_.set_draw_scores(node.side_to_move(), draw_score);
                //worker->search_.set_root_moves(root_moves);
                //worker->search_.set_multipv(multipv);
                //worker->search_.PrepareForNextSearch();
                worker->StartSearching();
            }
            
            // マスタースレッドの探索を開始する
            Search master_search(0/*shared_data_*/);
            //master_search.set_draw_scores(node.side_to_move(), draw_score);
            //master_search.set_root_moves(root_moves);
            //master_search.set_multipv(multipv);
            //master_search.PrepareForNextSearch();
            MoveScoreDepth best = master_search.iterativeDeepening(node);
            
            // ワーカースレッドの終了を待つ
            for (std::unique_ptr<SearchThread>& worker : worker_threads_) {
                worker->WaitUntilSearchIsFinished();
            }
            
            // 最善手と、相手の予想手を取得する
            //const RootMove& best_root_move = master_search.GetBestRootMove();
            //return best_root_move;
            
            //RootMove rm(Move(best));
            //rm.score = best.score;
            
            return std::move(best);
        }
    }
}

#endif // TRAX_KIZUNA_H_