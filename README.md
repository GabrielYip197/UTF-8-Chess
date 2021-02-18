# UTF-8-Chess
A simple, Windows-terminal based chess game that implements basic two-player controls and graphical colored UTF-8 glyphs in lieu of simple ASCII characters.


## Controls

WASD/Arrow Keys - Move
Spacebar/Enter - Select
Backspace - Undo move
Esc - Back/Quit
F5 key - Export the current game's move history to a Notepad file

"Player 1" (White) is controlled by WASD/Spacebar
"Player 2" (Black) is controlled by Arrow Keys/Enter

### Options
Toggle Turn Lockout:
OFF (default) - Does not restrict Player 1's controls from moving Player 2's pieces and vice versa, good for testing and debugging
ON - Locks out each Player's controls to prevent each other from moving during the other's turn

Pawn Glyph Width:
+0 (default) - Pawn glyphs appear normally spaced, at least on my end
+1 - I had a friend to playtest and he noticed that his pawn glyphs were only half the width they were supposed to be (1 character length, instead of 2), so this fixes that unknown issue.

#
I put this project together over summer, intending to replicate a similar experience to Windows 7's chess, using UTF-8 glyphs instead of the common ASCII characters. I had been fiddling around the Windows console, testing with ANSI escape codes to modify colors in the console. I decided to leave the color scheme as the default 16; though I could modify it, I experienced some issues with the console settings not properly resetting, interfering with normal terminal usage. 

## Implemented Features
- [x] Two player controls
- [x] Unlimited undo moves 
- [x] Colored and graphical interface with chessboard and pieces
- [x] Showcase possible moves for selected pieces
- [x] Highlight moves that capture in red
- [x] Showcase move history using algebraic notation
- [x] Ability to export the current game's move history as a .txt file
- [ ] Check detection
- [ ] Preventing the King from moving into a move that would put him in check
- [ ] Checkmate and endgame detection
- [ ] Stalemate detection
- [ ] Castling
- [ ] En passant capture
- [ ] Pawn promotion
- [ ] Custom key binding
- [ ] AI implementation for single-player
