/*
 client.cc
 Katsuki Ohto
 */

#include "trax.hpp"
#include "board.hpp"
#include "kizuna.h"

uint64_t LIMIT_TIME = 750;

#include "search.hpp"

//#if defined(CERR)
//#undef CERR
//#endif

const std::string MY_DEFAULT_CODE = "KZ";
const std::string MY_NAME = "KizuNa";
const std::string MY_VERSION = "160907";

void outputCommunicationLog(const std::string& str){
    std::ofstream ofs;
    ofs.open("./kizuna_communication.log", std::ios::app);
    if(!ofs){
        cerr << "failed to open log file." << endl;
    }
    ofs << getpid() << " " << str; // プロセスID付きでログを残す
    ofs.close();
}

void outputErrorLog(const std::string& str){
    std::ofstream ofs;
    ofs.open("./kizuna.log", std::ios::app);
    if(!ofs){
        cerr << "failed to open log file." << endl;
    }
    ofs << getpid() << " " << str; // プロセスID付きでログを残す
    ofs.close();
}

bool recvMessage(std::string *const pstr){
    if(!(std::cin >> *pstr)){
        return false;
    }
    outputCommunicationLog(">> " + *pstr + "\n");
    return true;
}

bool sendMessage(const std::string& str){
    std::cout << str << std::endl;
    outputCommunicationLog("<< " + str + "\n");
    return true;
}

Trax::Move think(Trax::Board& bd){
    
    CERR << " *** Thinking Phase ***" << endl;

    //Trax::Easy::moves = 0;
    //auto mvsc = Trax::Easy::searchRoot(max(4, 8 - bd.turn / 2), bd);
    
    for(int th = 0; th < N_THREADS; ++th){
        Global::node[th].unmakeMove(0); // 初期局面まで戻す
    }
    for(const auto& nt : Global::record){
        Move mv = readMoveNotation(nt, Global::node[0]);
        //cerr << mv << endl;
        for(int th = 0; th < N_THREADS; ++th){
            Global::node[th].makeMove(mv);
        }
    }
    
    Node& node = Global::node[0];
    
    Global::manager.SetNumSearchThreads(N_THREADS);
    Global::signals = 0;
    auto bestMove = Global::manager.ParallelSearch(node, {}, {}, 1);
    
    //CERR << "best move = " << bestMove << " " << toNotationString(bestMove.pv, bd) << endl;
    //CERR << "best score = " << std::get<1>(mvsc) << endl;
    
    // 評価点とpvを表示
    //CERR << " best = " << bestMove.score << " " << toString(toNotationStrings(bestMove.pv, bd), " ") << endl;
    CERR << "search best = " << bestMove.score << " " << toNotationString(Move(bestMove), bd) << " depth = " << bestMove.depth << endl;
    
    // 探索が終了したので定跡を見る
    /*int pat = 0;
    uint64_t rhash = calcRepRelativeHash(bd, pat); // 対称性を考慮した局面ハッシュ値を取得(同時に回転方向も得る)
    auto *pBookEntry = Global::book.read(rhash);
    if(pBookEntry != nullptr){ // 定跡から発見
        RelativeMoveScore bestBookMove = RelativeMoveScore(kRelativeMoveNone, 0);
        if(bd.turn <= 2){
            bestBookMove = pBookEntry->bestMove();
        }else{
            //int depth = pBookEntry->depth;
            //if(depth >= bestMove.depth){
            //    // より深い探索がなされていた場合には定跡データで着手を決定
            //    bestBookMove = pBookEntry->softmaxMove(800 / double(sqrt(bd.turn + 1)), &Global::dice);
            //}
        }
        if(bestBookMove != kRelativeMoveNone){
            // 対称変換を元に戻す(実際の局面でのタイルバウンド情報が必要)
            RelativeMoveBound rmv = symmetryTransform(invSymmetryTransform(RelativeMoveBound(bestBookMove, bd.dx() + 2, bd.dy() + 2),
                                                                           pBookEntry->pattern),
                                                      pat);
            
            Move mv = toMove(rmv, bd);
            
            //CERR << "book move = " << toNotationString(mv, bd) << endl;
            
            // 合法性がチェックされればこの着手で決定
            if(1 &&
               bd.isLegalMove(mv)){
                CERR << "book best = " << bestBookMove.score << " " << toNotationString(mv, bd) << " depth = " << pBookEntry->depth << " chosen" << endl;
                return mv;
            }
        }
    }*/
    
    //return std::get<0>(mvsc);
    return Move(bestMove);
    
    //return Trax::playRandomly(bd, &Trax::Global::dice);
}

void startPondering(){
    // 先読みを開始する
    
    CERR << " *** Pondering Phase ***" << endl;
    
    for(int th = 0; th < N_THREADS; ++th){
        Global::node[th].unmakeMove(0); // 初期局面まで戻す
    }
    for(const auto& nt : Global::record){
        Move mv = readMoveNotation(nt, Global::node[0]);
        //cerr << mv << endl;
        for(int th = 0; th < N_THREADS; ++th){
            Global::node[th].makeMove(mv);
        }
    }
    
    Global::initStats(); // スタッツ初期化
    Global::manager.SetNumSearchThreads(N_THREADS);
    Global::signals = 0;
    
    // ワーカースレッドの探索を開始する
    for (auto& worker : Global::manager.worker_threads_) {
        worker->StartSearching();
    }
}

void finishPondering(){
    Global::signals |= Global::SIGNAL_STOP; // stop signal
    for (auto& worker : Global::manager.worker_threads_) {
        worker->WaitUntilSearchIsFinished();
    }
}

int gameLoop(Trax::Board& bd, const Trax::Color myColor){ // main loop to proceed game
    using namespace Trax;
    while(1){
        
        const Color turnColor = bd.turnColor();
        
        int ret = 0;
        if(turnColor == myColor){ // my turn
            
            // 先手初手はランダム
            Trax::Move move;
            if(bd.turn == 0){
                if(Global::dice.rand() % 2 == 0){
                    move = readMoveNotation("@0+", bd);
                }else{
                    move = readMoveNotation("@0/", bd);
                }
            }else{
                move = think(bd); // decide my move
            }
            
            CERR << " *** finished Thinking Move ***" << endl;
            
            std::string notationString = toNotationString(move, bd);
            if(!sendMessage(notationString)){ // send move
                CERR << "failed to send my move." << endl;
                break;
            }
            Global::clock.start(); // start clock for pondering
            Global::record.push_back(notationString);
            ret = bd.makeMove(move);
            if(ret < 0){
                CERR << "my violation?" << endl; break;
            }
        }else{ // opponent turn
            
#ifdef PONDER
            if(Global::pondering){ // pondering
                startPondering();
            }
#endif
            
            std::string oppNotationString;
            
            if(!recvMessage(&oppNotationString)){
                CERR << "failed to receive opponent's move." << endl;
                break;
            }
            // got message
            ClockMS tmpClock(0);
            
#ifdef PONDER
            if(Global::pondering){
                finishPondering();
                CERR << " *** finished Pondering ***" << endl;
            }
#endif
            Global::clock = tmpClock; // copy clock
            
            // check if it is notation
            if(oppNotationString == "-U"){ // undo 2 moves
                bd.unmakeMove();
                Global::record.pop_back();
                bd.unmakeMove();
                Global::record.pop_back();
                Global::clock.start(); // start clock for thinking
            }else if(oppNotationString == "-E"){ // exit
                break;
            }else{
                
                CERR << " *** received Oppoent Move ***" << endl;
                
                Move oppMove = readMoveNotation(oppNotationString, bd);
                //CERR << oppNotationString << endl;
                //CERR << oppMove << endl;
                if(oppMove == kMoveNone){
                    CERR << "opponent move was unrecognized." << endl;
                    continue;
                }
                CERR << "opponent move = " << oppMove << " in notation " << toNotationString(oppMove, bd) << endl;
                Global::record.push_back(oppNotationString);
                ret = bd.makeMove(oppMove);
                if(ret < 0){
                    CERR << "opponent violation!" << endl;
#ifdef ENGINE
                    continue;
#else
                    break;
#endif
                }
            }
        }
        CERR << bd.toString();
        if(!bd.exam(ret == 0)){
            return -1;
        }
        if(ret > 0){
            if(Trax::whichWon(ret, turnColor) == myColor){
                CERR << "KizuNa won!!!" << endl;
            }else{
                CERR << "opponent won..." << endl;
            }
            break;
        }
    }
    CERR << "game record = ";
    CERR << toString(Global::record, " ") << endl;
    
    return 0;
}

int main(int argc, char* argv[]){
    
    using namespace Trax;
    CERR << "opened " << MY_NAME << " " << MY_VERSION << endl;
    
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    
    std::string host = "127.0.0.1";
    int port = 10001;
    std::string myCode = MY_DEFAULT_CODE;
    std::string bookFilePath = "./data/book.txt";
    std::string evalParamFilePath = "./data/eval_params.dat";
    int numThreads = N_THREADS;
    
    // receive arguments
    for(int c = 1; c < argc; ++c){
        if(!strcmp(argv[c], "-h")){
            host = std::string(argv[c + 1]);
        }else if(!strcmp(argv[c], "-p")){
            port = atoi(argv[c + 1]);
        }else if(!strcmp(argv[c], "-c")){
            myCode = std::string(argv[c + 1]);
        }else if(!strcmp(argv[c], "-b")){
            bookFilePath = std::string(argv[c + 1]);
        }else if(!strcmp(argv[c], "-np")){
            Global::pondering = false;
        }else if(!strcmp(argv[c], "-pi")){
            //evalParamFilePath = std::string(argv[c + 1]);
        }else if(!strcmp(argv[c], "-th")){
            numThreads = atoi(argv[c + 1]);
        }
    }
    
    // initialization
    Trax::initTrax();
    Trax::Global::dice.srand((unsigned int)time(NULL));
    Global::rootColor = RED;
    Global::tt.Clear();
    Global::tt.SetSize(1024);
    //Global::book.fin(bookFilePath);
    Global::manager.SetNumSearchThreads(numThreads);
    /*{
        std::ifstream ifs(evalParamFilePath);
        while(ifs){
            double val;
            ifs >> val;
            Global::evalParams.push_back(int(val));
        }
    }*/
    
    Node& bd = Global::node[0];
    bd.clear();
    int rv = 0; // return value of latest makemove
    
    CERR << "finished initialization." << endl;
    
    // connection
#if !defined(ENGINE)
#if defined(_WIN32)
    // Windows 独自の設定
    WSADATA data;
    if(SOCKET_ERROR == WSAStartup(MAKEWORD(2, 0), &data)){
        CERR << "failed to initialize WSA-data." << endl;
        exit(1);
    }
#endif // _WIN32
    
    // open socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0){
        CERR << "failed to open socket." << endl;
        exit(1);
    }
    //sockaddr_in 構造体のセット
    struct sockaddr_in addr;
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(host.c_str());
    // 接続
     if(connect(sock, (struct sockaddr*)&addr, sizeof(addr))){ // ソケット接続失敗
        CERR << "failed to connect to server." << endl;
        exit(1);
    }
    // 標準入出力に付け替える
    dup2(sock, STDIN_FILENO);
    dup2(sock, STDOUT_FILENO);
#endif // !ENGINE
    
    std::string command;
    
    while(recvMessage(&command)){
        if(command == "-T"){
            sendMessage(myCode); // send my code
        }else if(command == "-N"){
            sendMessage(MY_NAME + MY_VERSION); // send my real name
        }else if(command == "-B"){
            Global::rootColor = RED; // game start (I'm red)
            Global::clock.start(); // start clock for thinking
            gameLoop(bd, Global::rootColor); break;
        }else if(command == "-W"){
            Global::rootColor = WHITE; // game start (I'm white)
            Global::clock.start(); // start clock for thinking
            gameLoop(bd, Global::rootColor); break;
        }else if(command == "-M"){ // do 1 move
            std::string notationString;
            recvMessage(&notationString);
            Move move = readMoveNotation(notationString, bd);
            if(move == kMoveNone){ CERR << "unrecognized move." << endl; break; }
            rv = bd.makeMove(move);
            Global::record.push_back(notationString);
            CERR << bd.toString();
        }else if(command == "-U"){ // undo 1 move
            bd.unmakeMove();
            rv = 0;
            Global::record.pop_back();
            CERR << bd.toString();
        }else if(command == "-R"){ // record
            // record reading mode
            std::string notationString;
            while(recvMessage(&notationString)){
                if(notationString == "-F"){ // finish to read record
                    break;
                }
                Move move = readMoveNotation(notationString, bd);
                if(move == kMoveNone){ CERR << "unrecognized move." << endl; break; }
                rv = bd.makeMove(move);
                Global::record.push_back(notationString);
                CERR << bd.toString();
            }
        }else if(command == "-I"){ // initialize board
            bd.clear();
            rv = 0;
            Global::record.clear();
            CERR << bd.toString();
        }else if(command == "-J"){ // judge game result
            std::ostringstream oss;
            oss << rv;
            sendMessage(oss.str());
        }else if(command == "-E"){ // exit program
            break;
        }else if(command == "-S"){ // unlimited search
            LIMIT_TIME = -1;
            Global::clock.start(); // start clock for thinking
            Global::rootColor = bd.turnColor();
            think(bd);
            break;
        }
    }
    
#if !defined(ENGINE)
#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif // _WIN32
#endif // !ENGINE
    
    return 0;
}
