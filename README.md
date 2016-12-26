# KizuNa

unlimited trax engine written in C++14

2nd place in **The 2nd RECONF/CPSY/ARC/GI Trax Design Competition** (2016)

http://trax-fit2016.github.io/contest/ (Japanese page)

## How to build

`make release`

or

`make release -j4`

## Usage

./out/release/kizuna_engine

trax engine with stdin, stdout (HUMAN vs. CPU)

./out/release/kizuna_client

trax engine with TCP-IP communication (mainly CPU vs. CPU)

### commands before game

**-W**

game start (CPU is 1st player)

**-B**

game start (CPU is 2nd player)

**-M (Trax Notation)**

do 1 move

**-U**

undo 1 move

**-I**

initialize board

**-R (Trax Notations) -F**

do moves

examples : `-R @0+ B1+ -F -W`

then game will starts after "@0+ B1+"

**-E**

exit program

**-T**

send team code (competition protocol)

### commands in game

**(Trax Notation)**

send your move (when CPU vs. CPU, opponent's move)

**-U**

undo 2 moves

**-E**

exit from game
