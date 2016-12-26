/*
 trax_test.cc
 Katsuki Ohto
 */

// Trax Unit Test
#include "trax_test.h"

#include "trax.hpp"
#include "board.hpp"

using namespace std;
using namespace Trax;

int testTile(){
    // basic calculation about tiles
    
    // tile raversal, rotation test
    /*for(Tile t = TILE_MIN; t <= TILE_MAX; ++t){
        for(int s = 0; s < 8; ++s){
            Tile rt = getSymmetryTile(t, s);
            
        }
    }*/
    
    return 0;
}

int testNotation(){
    // reading / writing notations is a little bit difficult in TRAX.
    
    vector<string> columnNotation = {"@", "A", "AZ", "ZZ", "AAA", "AAZA", "_", "AA-", "C%Q", ""};
    vector<int> column = {0, 1, 26 + 26, (1 + 26) * 26, (1 + 26) * 26 + 1, (1 + 26) * 26 * 26 + 1 + 26 * 26, -1, -1, -1, -1};
    
    vector<string> rowNotation = {"0", "12", "100", "999", "+", "1L", "#0", ""};
    vector<int> row = {0, 12, 100, 999, -1, -1, -1, -1};
    
    for(int i = 0; i < 10; ++i){
        string cstr = toColumnString(column[i]);
        if(column[i] >= 0 && cstr != columnNotation[i]){
            cerr << "failed column integer -> notation. "
            << column[i] << " -> " << cstr << " != " << columnNotation[i] << endl;
            return -1;
        }
        int ci = toColumnInt(columnNotation[i]);
        if(ci != column[i]){
            cerr << "failed column notation -> integer. "
            << columnNotation[i] << " -> " << ci << " != " << column[i] << endl;
            return -1;
        }
    }
    for(int j = 0; j < 8; ++j){
        string rstr = toRowString(row[j]);
        if(row[j] >= 0 && rstr != rowNotation[j]){
            cerr << "failed row integer -> notation. "
            << row[j] << " -> " << rstr << " != " << rowNotation[j] << endl;
            return -1;
        }
        int ri = toRowInt(rowNotation[j]);
        if(ri != row[j]){
            cerr << "failed row notation -> integer. "
            << rowNotation[j] << " -> " << ri << " != " << row[j] << endl;
            return -1;
        }
    }
    for(int i = 0; i < 4; ++i){
        for(int j = 0; j < 4; ++j){
            // no neccesarity to check (int, int) -> string because it's only addition
            string pstr = columnNotation[i] + rowNotation[j];
            auto ni = readNotation(pstr);
            if(get<0>(ni) != row[j] || get<1>(ni) != column[i]){ // row (x) is first
                cerr << "failed position notation -> integer. "
                << pstr << " -> (" << get<0>(ni) << ", " << get<1>(ni) << ")"
                << " != (" << row[j] << ", " << column[i] << ")" << endl;
                return -1;
            }
        }
    }
    return 0;
}

template<class board_t>
int testSymmetry(){
    // symmetry check
    board_t *pbd = new board_t();
    board_t& bd = *pbd;
    int pat;
    
    uint64_t hash[TILE_MAX + 1];
    for(int t = TILE_MIN; t <= TILE_MAX; ++t){
        bd.clear();
        bd.template makeMove<true>(Move(Z_FIRST, t)); // force put
        uint64_t thash = calcRepRelativeHash(bd, WHITE, pat);
        hash[t] = thash;
        
        cerr << drawTile(static_cast<Tile>(t));
        cerr << "hash value = " << hash[t] << endl;
    }
    for(int ti0 = TILE_MIN; ti0 <= TILE_MAX; ++ti0){
       for(int ti1 = TILE_MIN; ti1 <= TILE_MAX; ++ti1){
           Tile t0 = static_cast<Tile>(ti0);
           Tile t1 = static_cast<Tile>(ti1);
           
           if(bool(isPlusTile(t0) == isPlusTile(t1))
              != bool(hash[t0] == hash[t1])){
               cerr << "failed 1 tile symmery-hash test." << endl;
               
               cerr << drawTile(t0);
               cerr << hash[t0] << endl;
               cerr << drawTile(t1);
               cerr << hash[t1] << endl;
               
               return -1;
           }
       }
    }
    
    std::vector<const char*> sampleRecord = {
        "@0+ A0/", "@0+ A0\\",
        "@0+ A2/", "@0+ A2\\",
        "@0/ A0+", "@0/ @1+",
        "@0\\ A0+", "@0\\ B1+",
    };
    
    std::vector<uint64_t> hashes;
    for(const auto& r : sampleRecord){
        bd.clear();
        std::vector<std::string> v = split(r, ' ');
        for(const auto& nt : v){
            Move mv = readMoveNotation(nt, bd);
            bd.template makeMove<true>(mv); // force put
        }
        uint64_t hash = calcRepRelativeHash(bd, WHITE, pat);
        hashes.push_back(hash);
        
        cerr << bd.toRawBoardString();
        cerr << "hash value = " << hash << endl;
    }
    // check if all hash values are same
    for(size_t i = 0; i < hashes.size(); ++i){
        if(hashes[0] != hashes[i]){
            cerr << "failed 2 tiles symmetry-hash test." << endl;
            
            return -1;
        }
    }
    delete(pbd);
    
    return 0;
}

template<class board_t, class move_t>
int testMakeUnmakeConsistency(const board_t& bd, const move_t& mv){
    // is makeMove() unmakeMove() consistent?
    board_t *pbd = new board_t();
    board_t& tbd = *pbd;
    tbd = bd;
    tbd.makeMove(mv);
    tbd.unmakeMove();
    int err = 0;
    if(!tbd.exam(false)){ cerr << "failed validation" << endl; err = 1; }
    if(!bd.template equals<1>(tbd) || err){
        cerr << " *** ORIGINAL BOARD *** " << endl;
        cerr << bd.toString();
        board_t *pbd1 = new board_t();
        board_t& tbd1 = *pbd1;
        tbd1 = bd;
        tbd1.makeMove(mv);
        cerr << " *** MOVED BOARD *** (" << toNotationString(mv, bd) << ")" << endl;
        cerr << tbd1.toString();
        cerr << " *** MOVED - UNMOVED BOARD *** " << endl;
        //cerr << toComparedString(bd, tbd);
        cerr << tbd.toString();
        delete(pbd1);
        return -1;
    }
    delete(pbd);
    return 0;
}

template<class board_t, class record_t>
int testLongUnmakeConsistency(const board_t& bd, const record_t& r){
    // is makeMove()... unmakeMove(t) consistent?
    for(int l = bd.turn; l < r.size(); ++l){
        board_t *pbd = new board_t();
        board_t& tbd = *pbd;
        tbd = bd;
        for(int i = bd.turn; i < l; ++i){
            tbd.makeMove(readMoveNotation(r[i], tbd));
        }
        board_t *pbd1 = new board_t();
        board_t& tbd1 = *pbd1;
        tbd1 = tbd;
        tbd.unmakeMove(bd.turn); // unmake to turn |bd.turn|
        int err = 0;
        if(!tbd.exam(false)){ cerr << "failed validation" << endl; err = 1; }
        if(!bd.template equals<1>(tbd) || err){
            cerr << " *** ORIGINAL BOARD *** " << endl;
            cerr << bd.toString();
            cerr << " *** MOVED BOARD *** (" << toString(r, " ") << ")" << endl;
            cerr << tbd1.toString();
            cerr << " *** MOVED - UNMOVED BOARD *** " << endl;
            //cerr << toComparedString(bd, tbd);
            cerr << tbd.toString();
            return -1;
        }
        delete(pbd);
        delete(pbd1);
    }
    return 0;
}

template<class board_t>
int testLegalityCheckConsistency(const board_t& bd){
    // is isLegalMove() saves board state?
    board_t *pbd = new board_t();
    board_t& tbd = *pbd;
    tbd = bd;
    auto moves = generateMoveVector(tbd);
    for(Move mv : moves){
        bool legality = tbd.isLegalMove(mv);
        if(!bd.template equals<1>(tbd)){
            cerr << " *** ORIGINAL BOARD *** " << endl;
            cerr << bd.toString();
            cerr << " *** CHECKED BOARD *** (" << toNotationString(mv, bd) << ", ";
            cerr << (legality ? "true" : "false") << ")" << endl;
            cerr << tbd.toString();
            return -1;
        }
    }
    delete(pbd);
    return 0;
}

template<class board_t>
int testPseudoLegality(const board_t& bd){
    // pseudoLegality() should holds legality()
    board_t *pbd = new board_t();
    board_t& tbd = *pbd;
    tbd = bd;
    auto moves = generateMoveVector(tbd);
    for(Move mv : moves){
        bool legality = tbd.isLegalMove(mv);
        bool pseudoLegality = tbd.isPseudoLegalMove(mv);
        if(legality && !pseudoLegality){
            // legal but not pseudo-legal
            cerr << "move " << mv << " is judged legal but not pseudo-legal." << endl;
            cerr << bd.toString();
            return -1;
        }
    }
    delete(pbd);
    return 0;
}

template<class board_t, class move_t>
int testInvalidMakeConsistency(const board_t& bd, const move_t& mv){
    // check whether
    cerr << bd.turn << endl;
    board_t *pbd = new board_t();
    board_t& tbd = *pbd;
    tbd = bd;
    Move badMove = mv;
    badMove.assignTile((mv.tile() + 1 + bd.turn) % N_TILES);
    cerr << mv << " -> " << badMove << endl;
    if(tbd.makeMove(badMove) < 0){
        cerr << "illegal!" << endl;
        // board must not be change if illegal move
        if(!bd.template equals<1>(tbd)){
            cerr << bd.toString() << endl;
            return -1;
        }
    }
    delete(pbd);
    return 0;
}

template<class board_t>
int testLoading(const vector<string>& v){
    
    cerr << toString(v, " ") << endl;

    board_t *pbd = new board_t();
    board_t& bd = *pbd;
    bd.clear();
    int err = 0;
    for(int i = 0; i < v.size(); ++i){
        Move mv = readMoveNotation(v[i], bd);
        cerr << "move = " << mv << "in notation " << v[i] << endl;
        int ret = bd.makeMove(mv);
        cerr << bd.toString() << endl;
        cerr << "result desciption : " << resultDescription(ret) << endl;
        if(ret < 0){ cerr << "invalid makemove?" << endl; err = 1; }
        if(ret > 0 && i != v.size() - 1){ cerr << "should continue but " << (ret - 1) << " won?" << endl; err = 1; }
        //if(ret == 0 && i == v.size() - 1){ cerr << "should finish but continue?" << endl; err = 1; }
        if(!bd.exam(ret == 0)){ cerr << "failed validation" << endl; err = 1; }
        if(err){
            return -1;
        }
    }
    delete(pbd);
    return 0;
}

int testLines(){
    // line data validity
    return 0;
}

int testVictoryLine(){
    // judgement of victory line
    return 0;
}

template<class board_t>
int testAttacks(board_t& bd){
    // attack data test
    
    Move buffer[1024];
    const int moves = generateMoves(buffer, bd);
    cerr << bd.toString();
    
    bd.checkSetAttacks();
    
    for(int c = 0; c < 2; ++c){
        
        if(bd.attacks[c]){
            // check if there is winning move
            
            for(int m = 0; m < moves; ++m){
                board_t *pbd = new board_t();
                board_t& tbd = *pbd;
                tbd = bd;
                int ret = tbd.makeMove(buffer[m]);
                if(ret >= 0 && (ret & (Rule::WON << c))){
                    goto OK;
                }
                delete(pbd);
            }
            
            // has attacks but no mate move
            cerr << "has attacks but no mate move. (color = " << colorChar[c] << ")" << endl;
            return -1;
        }else{
            // check that there is no move
            
            for(int m = 0; m < moves; ++m){
                board_t tbd = bd;
                int ret = tbd.makeMove(buffer[m]);
                if(ret >= 0 && (ret & (Rule::WON << c))){
                    cerr << "no attack but mate move. (color = " << colorChar[c] << ")" << endl;
                    return -1;
                }
            }
        }
        
    OK:;
    }
    return 0;
}

template<class board_t>
int testBoard(){
    // loading test
    for(int i = 0; i < sample.size(); ++i){
        if(testLoading<board_t>(sample[i]) < 0){
            cerr << "failed loading test with sample " << i << "." << endl;
            return -1;
        }
    }
    cerr << "passed loading test." << endl;
    
    // make-unmake test
    for(int i = 0; i < sample.size(); ++i){
        board_t *pbd = new board_t();
        board_t& bd = *pbd;
        bd.clear();
        for(int j = 0; j < sample[i].size(); ++j){
            Move mv = readMoveNotation(sample[i][j], bd);
            if(testMakeUnmakeConsistency(bd, mv)){
                cerr << "failed make - unmake consistency test." << endl;
                return -1;
            }
            bd.makeMove(mv);
        }
        delete(pbd);
    }
    cerr << "passed make - unmake consistency test." << endl;
    
    // long make-unmake test
    for(int i = 0; i < sample.size(); ++i){
        board_t *pbd = new board_t();
        board_t& bd = *pbd;
        bd.clear();
        for(int j = 0; j < sample[i].size(); ++j){
            Move mv = readMoveNotation(sample[i][j], bd);
            if(testLongUnmakeConsistency(bd, sample[i])){
                cerr << "failed long make - unmake consistency test." << endl;
                return -1;
            }
            bd.makeMove(mv);
        }
        delete(pbd);
    }
    cerr << "passed long make - unmake consistency test." << endl;
    
    // legality check test
    for(int i = 0; i < sample.size(); ++i){
        board_t *pbd = new board_t();
        board_t& bd = *pbd;
        bd.clear();
        for(int j = 0; j < sample[i].size(); ++j){
            Move mv = readMoveNotation(sample[i][j], bd);
            if(testLegalityCheckConsistency(bd)){
                cerr << "failed legality check consistency test." << endl;
                return -1;
            }
            bd.makeMove(mv);
        }
        delete(pbd);
    }
    cerr << "passed legality check consistency test." << endl;
    
    // invalid make test
    for(int i = 0; i < sample.size(); ++i){
        board_t *pbd = new board_t();
        board_t& bd = *pbd;
        bd.clear();
        for(int j = 0; j < sample[i].size(); ++j){
            Move mv = readMoveNotation(sample[i][j], bd);
            if(testInvalidMakeConsistency(bd, mv)){
                cerr << "failed invalid make consistency test." << endl;
                return -1;
            }
            bd.makeMove(mv);
        }
        delete(pbd);
    }
    cerr << "passed invalid make consistency test." << endl;
    
    // pseudo-legality test
    for(int i = 0; i < sample.size(); ++i){
        board_t *pbd = new board_t();
        board_t& bd = *pbd;
        bd.clear();
        for(int j = 0; j < sample[i].size(); ++j){
            Move mv = readMoveNotation(sample[i][j], bd);
            if(testPseudoLegality(bd)){
                cerr << "failed pseudo-legality test." << endl;
                return -1;
            }
            bd.makeMove(mv);
        }
        delete(pbd);
    }
    cerr << "passed pseudo-legality test." << endl;
    
    // attack test
    for(int i = 0; i < sample.size(); ++i){
        board_t *pbd = new board_t();
        board_t& bd = *pbd;
        bd.clear();
        for(int j = 0; j < sample[i].size(); ++j){
            Move mv = readMoveNotation(sample[i][j], bd);
            if(testAttacks(bd)){
                cerr << "failed attack test." << endl;
                return -1;
            }
            bd.makeMove(mv);
        }
        delete(pbd);
    }
    cerr << "passed attack test." << endl;
    
    return 0;
}

template<class board_t>
int moveGeneratorTest(){
    // check whether exactly same moves were generated
    return 0;
}

int main(int argc, char* argv[]){
    
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    
    // initialization
    initTrax();
    
    int err = 0;
    
    // notation test
    if(testNotation() < 0){
        cerr << "failed notation test." << endl;
        return -1;
    }
    
    // board implementation test
    if(testBoard<Board>() < 0){
        return -1;
    }
    
    // symmetry hash value test
    if(testSymmetry<Board>() < 0){
        cerr << "failed symmetry-hash test." << endl;
        return -1;
    }
    
    return 0;
}