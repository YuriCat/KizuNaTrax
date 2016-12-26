# KizuNaTrax
unlimited trax engine

## How to build

`make release`
or
`make release -j4`

in root directory.

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

**-E**
exit program

### commands in game

**(Trax Notation)**
send your (or opponent's) move

**-U**
undo 2 moves

**-E**
exit from game
