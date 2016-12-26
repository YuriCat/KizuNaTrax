/*
 trax_test.h
 Katsuki Ohto
 */

#ifndef TRAX_TRAXTEST_H_
#define TRAX_TRAXTEST_H_

// Trax Header of Unit Test
#include <string>
#include <vector>

#include "trax.hpp"

// sample record
const std::string wonByLineStr = "@0+ B1+ C1\\ C2\\ D2/ C3/ E2\\ E3\\ D4\\ F2/ E1\\ D0/ G4+ F5\\ C6/ B5\\ A5\\ F1/ F0/ H4+";
const std::string longest60Str = "@0/ @1/ A2+ @2+ B0/ A2+ A0/ @3\\ B0+ C1/ D1+ @3/ @1+ C0/ G2\\ F7/ E0/ F1/ F0/ G1/ H1+ G0+ I2/ H6+ I6/ J5\\ I1/ I0/ I8+ J9/ C2\\ E1+ I0+ J3+ K4+ K8/ L6\\ L10+ I11\\ H12+ E13\\ @8+ D11+ G14/ L3/ N3+ L1/ M2+ C3+ C2\\ C1\\ O6\\ O7+ O9\\ N10+ P6+ P9+ P3/ O1+ Q3\\";
const std::string longest148Str = "@0/ @1/ B2/ C2/ C3/ D3/ D4/ E4/ E5/ F5/ F6/ G6/ G7/ H7/ H8/ I8/ I9/ J9/ J10/ J11/ A0/ J13/ K13+ L13+ L11/ M13+ L14/ N11+ O12/ I14/ K15+ K16/ I11+ H12+ H10+ G13/ H14+ F13/ O10/ P10/ J9/ L9+ M9+ N8\\ I15/ G15\\ H16+ H17/ H18+ H19/ G10/ H20+ I8+ I7\\ H6/ J7+ M7+ O8/ N7+ P9+ N6+ O6+ N5\\ P6/ Q7/ N4+ Q10/ R9\\ R8+ H21+ F14+ I19\\ E12+ D12\\ L8+ S7+ S9\\ S6/ T7\\ P11\\ M14+ H22/ K17/ I20\\ G21/ M15\\ F16+ E17\\ G18+ F19+ D16/ E14+ E10+ C17\\ K18+ J21+ I22+ R5+ M4+ L4/ K6+ K5+ M3/ O4+ J4+ J3\\ M2/ N1\\ H5+ C11/ D14/ B18+ E20+ E22+ C21+ D22/ E23+ A21+ @20+ B18+ O14+ Q14+ S11+ U8+ V9/ V7+ S4/ T3\\ H23+ F24/ C22+ G25/ O16+ P17+ M19+ K23/ F25+ T12+ R15+ T15+ U12/ V12/ U10+ W6/ U5+ W9+ Q19\\ W8+";
const std::string nobashi = "@0+ A2+ B2+ C2/ B1/ A0+ @3\\ @2+ E4\\ F3/ F2+ G3\\ G4\\ F1/ C0/ B5\\ D6\\ G6+ F7+ C0\\ F9+ F10+ F11+ F12+ F13+ F14+ F15+ F16+ F17+ F18+ F19+ F20+ F21+ F22+ F23+ F24+ F25+ F26+ F27+ F28+ F29+ F30+ F31+ F32+ F33+ F34+ F35+ F36+";
const std::string vsPeryaudoFirstWon = "@0/ A0/ A0/ @1/ B0+ @1/ B0+ B0/ @2\\ A1\\ A0\\ @1\\ D0+ @5/ C6+ G8\\ F9/ @2\\ A1\\ D0+ F10+ G3\\ @5+ F0/ G1/ G0/ H2/ I2/ H1/ E1\\ A1+ I4+";
const std::string vsPeryaudoLose41 = "@0/ @1/ B0+ @1/ C3/ D3/ D4/ E4/ D5+ F5/ B0+ F7\\ G7\\ E8/ D8/ D9/ C6/ B8+ A8/ G10\\ H8+ D11/ C11/ B6\\ A6\\ @8/ @8/ @6+ A5\\ A4+ @6/ A4+ @4\\ @4+ C3+ M12\\ N10\\ O9/ E12/ I1+ D11/"; // search bug? KizuNa said MATE in turn 28 ~ 36

const std::string vsPeryaudoWonSnake = "@0/ A0/ A0/ @1/ B0+ @1/ B0+ @1/ A0/ @1/ A0/ @2+ G7/ G8/ H8/ H9/ I9/ I10/ J10/ J11/ K11/ L11/ L12/ M12/ M13/ N13/ N14/ O13+ O15/ P15/ A0/ @1/ P17+ R17/ A0/ R19\\ S19\\ Q20/ P20/ P21/ O20/ S22\\ T21+ O22/ P23+ N21\\"; // snake game

// @0/ A0+ B1/ C1/ D1/ @1\ D0+ @2/ B4\ F3/ A4/ B5\ B6\ G3\ G4\ D5/ D7+ E6\ H3+
const std::string version17Violation = "@0/ A0+ B1/ C1/ D1/ @1\\ D0+ @2/ B4\\ F3/ A4/ B5\\ B6\\ G3\\ G4\\ D5/ D7+ E6\\ H3+"; // KizuNa ver 17 lost by violation F4/

std::vector<std::vector<std::string>> sample; // vector of record notation

struct TraxTestInitializer{
    TraxTestInitializer(){
        sample.push_back(split(wonByLineStr, ' '));
        sample.push_back(split(longest60Str, ' '));
        sample.push_back(split(longest148Str, ' '));
        sample.push_back(split(nobashi, ' '));
        sample.push_back(split(vsPeryaudoFirstWon, ' '));
        sample.push_back(split(vsPeryaudoLose41, ' '));
        sample.push_back(split(vsPeryaudoWonSnake, ' '));
        sample.push_back(split(version17Violation, ' '));
    }
};

TraxTestInitializer _traxTestInitializer;

#endif // TRAX_TRAXTEST_H_