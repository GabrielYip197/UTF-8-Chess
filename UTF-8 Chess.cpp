#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <string>
#include <algorithm>
#include <ctime>
#include <conio.h>
#include <Windows.h>



class ChessGame
{
	// Technical data 
	HANDLE hConsole;
	
	enum KeyControls { MOVE_UP, MOVE_LEFT, MOVE_DOWN, MOVE_RIGHT, SELECT, UNDO, EXIT, INCR_FONT, DECR_FONT, SAVE_MOVES, NUM_CONTROLS };
	std::pair<char, char> KeyBinds[10] = { {'W', VK_UP}, {'A', VK_LEFT}, {'S', VK_DOWN}, {'D', VK_RIGHT}, {' ', VK_RETURN}, {VK_BACK, VK_DELETE}, {VK_ESCAPE, VK_ESCAPE}, {VK_PRIOR, VK_PRIOR}, {VK_NEXT, VK_NEXT}, {VK_F5, '@'}, };

	enum PieceID {
		EMPTY = 0, KING = 1, QUEEN = 2, BISHOP_L = 3, BISHOP_R = 4, KNIGHT_L = 5, KNIGHT_R = 6, ROOK_L = 7, ROOK_R = 8,
		PAWN_A = 9, PAWN_B = 10, PAWN_C = 11, PAWN_D = 12, PAWN_E = 13, PAWN_F = 14, PAWN_G = 15, PAWN_H = 16,
	};

	enum GlyphName { aTILE, aKING, aQUEEN, aROOK, aBISHOP, aKNIGHT, aPAWN, Horizontal_Bar, Vertical_Bar, TopLeftCorner, TopRightCorner, BottomLeftCorner, BottomRightCorner, NUM_GLYPHS };
	std::array<const char*, NUM_GLYPHS> Glyph = { u8"\u2588", u8"\u265A", u8"\u265B", u8"\u265C", u8"\u265D", u8"\u265E", u8"\u265F",		u8"\u2550",		u8"\u2551",		u8"\u2554",		u8"\u2557",		u8"\u255A",		u8"\u255D", };


	// Display data
	int fontSize = 64;

	enum PlayerNum { BLACK_PLAYER, WHITE_PLAYER, NUM_PLAYERS };
	enum Colors { BLACK_ON_LIGHT = 0x70, BLACK_ON_DARK = 0x80, WHITE_ON_LIGHT = 0x7F, WHITE_ON_DARK = 0x8F, LIGHT = 0x07, DARK = 0x08, BLACK_ON_GREEN = 0x20, WHITE_ON_GREEN = 0x2F, GREEN = 0x02, BLACK_ON_BLUE = 0x90, WHITE_ON_BLUE = 0x9F, BLUE = 0x09, BLACK_ON_CYAN = 0xB0, WHITE_ON_CYAN = 0xBF, CYAN = 0x0B, BLACK_ON_RED = 0XC0, WHITE_ON_RED = 0xCF, RED = 0x0C, NUM_COLORS };
	
	// Structs
	struct PieceData
	{
		bool capturedYet = false;
		int pieceID = 0;
		std::pair<int, int> position{ -1, -1 };
	};

	struct MoveHistoryData
	{
		std::string moveNotation = "";
		int pieceID;
		bool isWhitePiece = false;
		std::pair<int, int> startPos;
		std::pair<int, int> finishPos;

		bool aCaptureMove = false;
		int capturedPieceID = 0;
	};


	// Game variables
	char userInput;

	// 
	static const int BOARD_DIM = 8;
	int Chessboard[BOARD_DIM][BOARD_DIM] = { 0 };

	bool stillPlaying = true, updateScreen = false;
	bool isWhiteTurn = true, isWhitePiece = false;	// Black = 0, White = 1  --  use for the array
	bool isPieceSelected = false, updatedPosMoves = false;	// Have we selected a piece to view its moves; and boolean to check if we need to rerun calculations
	int selectedPiece, pieceIDIndex;
	std::pair<int, int> highlightedTile{ 3, 6 };		// (x, y) coordinate
	std::pair<int, int> selectedTile = { -1, -1 };	// (-1, -1) is position when isPieceSelected is false
	std::vector<std::pair<int, int>> possibleTiles;
	bool turnLockout = false;
	bool isCheck, isCheckmate;
	bool isPawnOneCharWidth = false;

	std::vector< PieceData> PlayerPieces[NUM_PLAYERS];
	std::vector<std::pair<MoveHistoryData, MoveHistoryData>> allMoveHistory;


	// Private functions
	void clearscreen()
	{
		HANDLE hOut;
		COORD Position;

		hOut = GetStdHandle(STD_OUTPUT_HANDLE);

		Position.X = 0;
		Position.Y = 0;
		SetConsoleCursorPosition(hOut, Position);
	}

	void ShowConsoleCursor(bool showFlag)
	{
		CONSOLE_CURSOR_INFO     cursorInfo;

		GetConsoleCursorInfo(hConsole, &cursorInfo);
		cursorInfo.bVisible = showFlag; // set the cursor visibility
		SetConsoleCursorInfo(hConsole, &cursorInfo);
	}

	void setFontSize(const int& FontSize)
	{
		CONSOLE_FONT_INFOEX info = { 0 };
		info.cbSize = sizeof(info);
		info.dwFontSize.Y = FontSize; // leave X as zero
		info.FontWeight = FW_NORMAL;
		wcscpy_s(info.FaceName, L"MS Gothic");
		SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), NULL, &info);
	}

	bool isPositive(const int& value)
	{
		if (value > 0)
			return true;
		else
			return false;
	}

	void DisplayTitle(const std::string& title)
	{
		const char Decor = '=';
		const int WfromS = 6;
		// To center it, make within the box constraints, the title always has 11 empty spaces on either side
		// Therefore the BAR_WIDTH calculation (length of the outer border) will be:
		// title.length() + 11*2 + 2
		int BAR_WIDTH = title.length() + (WfromS * 2);

		std::cout << " ";
		std::cout << Glyph[TopLeftCorner];
		for (int i = 0; i < BAR_WIDTH; i++) {
			std::cout << Glyph[Horizontal_Bar];
		}
		std::cout << Glyph[TopRightCorner] << "                               \n";

		std::cout << " ";
		std::cout << Glyph[Vertical_Bar];
		for (int j = 0; j < WfromS; j++) {
			if (j > 0 && j < 4) {
				std::cout << Decor;
			}
			else {
				std::cout << " ";
			}
		}
		std::cout << title;
		for (int j = 0; j < WfromS; j++) {
			if (j > WfromS - 5 && j < WfromS - 1) {
				std::cout << Decor;
			}
			else {
				std::cout << " ";
			}
		}

		std::cout << Glyph[Vertical_Bar] << "                               \n";

		std::cout << " ";
		std::cout << Glyph[BottomLeftCorner];
		for (int i = 0; i < BAR_WIDTH; i++) {
			std::cout << Glyph[Horizontal_Bar];
		}
		std::cout << Glyph[BottomRightCorner] << "                               \n";

		//std::cout << "\n";
	}

	bool isKeyBindPressed(const int& userInput)
	{
		// Find out if the key pressed is a double byte

		// If first bind is pressed
		if (((GetAsyncKeyState(KeyBinds[userInput].first)) & 0x8000) && !(turnLockout && !isWhiteTurn)) {
			// If it's a double byte
			if ((KeyBinds[userInput].first > 0x20 && KeyBinds[userInput].first < 0x30) || (KeyBinds[userInput].first >= 0x70 && KeyBinds[userInput].first <= 0x87))
				_getch();
		}
		else if (((GetAsyncKeyState(KeyBinds[userInput].second)) & 0x8000) && !(turnLockout && isWhiteTurn)) {
			// If it's a double byte
			if ((KeyBinds[userInput].second > 0x20 && KeyBinds[userInput].second < 0x30) || (KeyBinds[userInput].second >= 0x70 && KeyBinds[userInput].second <= 0x87))
				_getch();
		}


		if (((((GetAsyncKeyState(KeyBinds[userInput].first)) & 0x8000) && !(turnLockout && !isWhiteTurn)) || ((GetAsyncKeyState(KeyBinds[userInput].second) & 0x8000)) && !(turnLockout && isWhiteTurn)))
			return true;
		else
			return false;
	}

	int PrintMenu(std::vector<std::string>& MenuOptions, int& selectedOption, bool& actuallySelected)
	{
		bool leaveFunc = false;
		bool turnLockout = false;
		bool isWhiteTurn = false;

		SetConsoleTextAttribute(hConsole, 0x0F);

		for (int i = 0; i < MenuOptions.size(); i++)
		{
			if (i == selectedOption) {
				SetConsoleTextAttribute(hConsole, 0xF0);
				std::cout << "  > ";
				std::cout << MenuOptions[i];
				for (int j = 0; j < 25 - MenuOptions[i].length(); j++)
					std::cout << " ";
			}
			else {
				SetConsoleTextAttribute(hConsole, 0x0F);
				std::cout << "   ";
				std::cout << MenuOptions[i];
				for (int j = 0; j < 26 - MenuOptions[i].length(); j++)
					std::cout << " ";
			}

			SetConsoleTextAttribute(hConsole, 0x0F);
			std::cout << "\n";
		}

		std::cout << "\n\n [Created by Gabriel Yip]\n";

		for (int k = 0; k < 10; k++)
			std::cout << "                         \n";

		// Wait for user input
		_getch();

		if (isKeyBindPressed(SELECT)) {
			actuallySelected = true;
			return selectedOption;
		}
		else if (isKeyBindPressed(MOVE_UP))
		{
			if (selectedOption > 0)
				selectedOption--;
			else
				selectedOption = MenuOptions.size() - 1;
		}
		else if (isKeyBindPressed(MOVE_DOWN))
		{
			if (selectedOption < MenuOptions.size() - 1)
				selectedOption++;
			else
				selectedOption = 0;
		}

	}

	void OptionsMenu()
	{
		std::vector<std::string> OptionsMenu_Options = { "Turn Lockout", "Pawn Glyph Width", "Back" };
		int selectedOption = 0;


		while (true)
		{
			bool actuallySelected = false;

			clearscreen();

			DisplayTitle("Options");
			std::cout << "\n";

			if (turnLockout)
				OptionsMenu_Options[0] = "Toggle Turn Lockout - ON ";
			else
				OptionsMenu_Options[0] = "Toggle Turn Lockout - OFF";

			if (isPawnOneCharWidth)
				OptionsMenu_Options[1] = "Pawn Glyph Width - +1";
			else
				OptionsMenu_Options[1] = "Pawn Glyph Width - +0";


			switch (PrintMenu(OptionsMenu_Options, selectedOption, actuallySelected))
			{
				// Turn lockout toggle
			case 0:
				if (actuallySelected)
					turnLockout = !turnLockout;
				break;

				// Pawn char width toggle
			case 1:
				if (actuallySelected)
					isPawnOneCharWidth = !isPawnOneCharWidth;
				break;

				// Go back
			case 2:
				if (actuallySelected)
					return;
				break;
			}
		}
	}

	int getPieceIDIndex(int& selectedPiece)
	{
		if (selectedPiece < 0) {
			for (int i = 0; i < PlayerPieces[BLACK_PLAYER].size(); i++)
			{
				if (PlayerPieces[BLACK_PLAYER][i].pieceID == abs(selectedPiece))
					return i;
			}
		}
		else if (selectedPiece > 0) {
			for (int i = 0; i < PlayerPieces[WHITE_PLAYER].size(); i++)
			{
				if (PlayerPieces[WHITE_PLAYER][i].pieceID == abs(selectedPiece))
					return i;
			}
		}

		return -1;
	}

	std::string getFullNotation(int& selectedPiece, std::pair<int, int>& startPos, std::pair<int, int>& finishPos, bool isCapture, bool isCheck, bool isCheckmate)
	{
		const std::string files[] = { "a", "b", "c", "d", "e", "f", "g", "h" };
		std::string notation;

		// Find out what the piece being moved is
		switch (abs(selectedPiece)) {

		case ROOK_L: case ROOK_R:
			notation.append("R");
			break;

		case KNIGHT_L: case KNIGHT_R:
			notation.append("N");
			break;

		case BISHOP_L: case BISHOP_R:
			notation.append("B");
			break;

		case QUEEN:
			notation.append("Q");
			break;

		case KING:
			notation.append("K");
			break;

			// Pawns don't have a letter assigned to them
		case PAWN_A: case PAWN_B: case PAWN_C: case PAWN_D: case PAWN_E: case PAWN_F: case PAWN_G: case PAWN_H:
		default:
			break;
		}

		// Then find out which the starting tile
		notation.append((files[startPos.first]) + std::to_string(startPos.second + 1));

		// If it's a capture, append 'x', but if just a normal move, append '-' hyphen
		if (isCapture)
			notation.append("x");
		else
			notation.append("-");

		// Add finishing tile
		notation.append((files[finishPos.first]) + std::to_string(finishPos.second + 1));


		// Check for checkmate/check
		if (isCheckmate)
			notation.append("#");
		else if (isCheck)
			notation.append("+");

		return notation;
	}

	bool isPieceWithinPos(std::pair<int, int>& highlightedTile, std::vector<std::pair<int, int>>& possibleTiles)
	{
		for (int i = 0; i < possibleTiles.size(); i++) {
			if (highlightedTile == possibleTiles[i])
				return true;
		}
		return false;
	}

	bool ExportMoveHistory()
	{
		time_t result = time(NULL);
		char str[30];
		ctime_s(str, sizeof str, &result);

		std::string date = str;

		std::replace(date.begin(), date.end(), ':', '-'); // replace all ':' to '-'

		std::string filename = "Chess game - ";
		filename.append(date).pop_back();
		filename.append(".txt");

		std::ofstream myFile;

		myFile.open(filename);
		if (myFile.is_open())
		{
			myFile << "Move History:\t(w/b)\n";
			for (int i = 0; i < allMoveHistory.size(); i++)
				myFile << i + 1 << ")  " << allMoveHistory[i].first.moveNotation << "\t" << allMoveHistory[i].second.moveNotation << "\n";

			filename;

			myFile.close();
			return true;
		}
		else
			return false;
	}

	// Functions
public:

	ChessGame(HANDLE& hConsole, const int fontSize = 64)
	{
		this->hConsole = hConsole;
		setFontSize(fontSize);
		ShowConsoleCursor(false);
	}

	bool MainMenu();
	
	void InitializeBoard();
	
	void UpdateBoard();

	void DrawScreen();

	bool ReadInput();

	void CalcMoves();
};


int main()
{
	// Set the output to UTF-8, so we can draw the full amount of symbols in the console
	SetConsoleOutputCP(CP_UTF8);

	// Allow us to change color of text
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	// Force to fullscreen by default
	SetConsoleDisplayMode(hConsole, CONSOLE_FULLSCREEN_MODE, 0);

	// Create instance of game
	ChessGame Game(hConsole);

	bool stillPlaying = true;

	// main menu
	stillPlaying = Game.MainMenu();

	Game.InitializeBoard();

	while(stillPlaying)
	{
		// Update board pieces
		Game.UpdateBoard();

		// Draw function
		Game.DrawScreen();

		// Waits for player input, and once it receives, updates the game
		// 
		stillPlaying = Game.ReadInput();

		// All game logic inside here
		Game.CalcMoves();
	}

	SetConsoleTextAttribute(hConsole, 0x0F);
	std::cout << "> Game was quit.\n\n";
	SetConsoleTextAttribute(hConsole, 0x00);

	return 0;
}

bool ChessGame::MainMenu()
{	
	// List of all options
	std::vector<std::string> MainMenu_Options = { "Play Chess", "Options", "Quit" };
	int selectedOption = 0;

	while (true)
	{
		bool actuallySelected = false;

		clearscreen();

		DisplayTitle("  Main Menu  ");
		std::cout << "\n";

		// PrintMenu returns an int corresponding to which option was chosen
		switch (PrintMenu(MainMenu_Options, selectedOption, actuallySelected))
		{
			// Play Chess
		case 0:
			if (actuallySelected) {
				return true;
			}
			break;

			// Options
		case 1:
			if (actuallySelected)
				OptionsMenu();
			break;

			// Quit
		case 2:
			if (actuallySelected) {
				std::cout << "\n";
				stillPlaying = false;
				return false;
			}
			break;
		}


	}
}

void ChessGame::InitializeBoard()
{
	PlayerPieces[BLACK_PLAYER].clear();
	PlayerPieces[WHITE_PLAYER].clear();

	const int StartingBoard[BOARD_DIM][BOARD_DIM] =
	{
		{-ROOK_L, -KNIGHT_L, -BISHOP_L, -QUEEN, -KING, -BISHOP_R, -KNIGHT_R, -ROOK_R},
		{-PAWN_A, -PAWN_B, -PAWN_C, -PAWN_D, -PAWN_E, -PAWN_F, -PAWN_G, -PAWN_H		},
		{ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
		{ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
		{ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
		{ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
		{ PAWN_A, PAWN_B, PAWN_C, PAWN_D, PAWN_E, PAWN_F, PAWN_G, PAWN_H			},
		{ ROOK_L, KNIGHT_L, BISHOP_L, QUEEN, KING, BISHOP_R, KNIGHT_R, ROOK_R		}
	};


	for (int y = 0; y < BOARD_DIM; y++)
	{
		for (int x = 0; x < BOARD_DIM; x++)
		{
			// If black piece, fill up black player's std::vector
			if (StartingBoard[y][x] < 0)
				PlayerPieces[BLACK_PLAYER].push_back(PieceData{ false, abs(StartingBoard[y][x]), std::make_pair(x, y) });
			// Else if white piece, fill up white player's std::vector
			else if (StartingBoard[y][x] > 0)
				PlayerPieces[WHITE_PLAYER].push_back(PieceData{ false, abs(StartingBoard[y][x]), std::make_pair(x, y) });
		}
	}
}

void ChessGame::UpdateBoard()
{
	// Reset chessboard; all tiles must overwritten and marked as empty
	for (int y = 0; y < BOARD_DIM; y++)
	{
		for (int x = 0; x < BOARD_DIM; x++)
			Chessboard[y][x] = EMPTY;
	}

	// Update black pieces (overwrite empty tiles)
	for (int i = 0; i < PlayerPieces[BLACK_PLAYER].size(); i++) {
		// For all pieces not captured yet, place each black piece onto the chessboard's position as indicated by the piece's position, and make the value negative to indicate it is a black piece
		if (!PlayerPieces[BLACK_PLAYER][i].capturedYet)
			Chessboard[PlayerPieces[BLACK_PLAYER][i].position.second][PlayerPieces[BLACK_PLAYER][i].position.first] = -PlayerPieces[BLACK_PLAYER][i].pieceID;
	}

	// Update white pieces
	for (int i = 0; i < PlayerPieces[WHITE_PLAYER].size(); i++) {
		// Place each uncaptured white piece onto the chessboard's position likewise
		if (!PlayerPieces[WHITE_PLAYER][i].capturedYet)
			Chessboard[PlayerPieces[WHITE_PLAYER][i].position.second][PlayerPieces[WHITE_PLAYER][i].position.first] = PlayerPieces[WHITE_PLAYER][i].pieceID;
	}
}

void ChessGame::DrawScreen()
{
	bool foundInPos = false;

	clearscreen();

	SetConsoleTextAttribute(hConsole, 0x0F);
	DisplayTitle("Chess");

	isWhiteTurn ? std::cout << "  Turn: WHITE \n                              \n" : std::cout << "  Turn: BLACK \n                              \n";

	// Print out 
	SetConsoleTextAttribute(hConsole, 0x0F);
	std::cout << "   A B C D E F G H    Move History (w/b)\n";
	//std::cout << "   A0B1C2D3E4F5G6H7   Move History (w/b)\n";
	for (int y = 0; y < BOARD_DIM; y++)
	{
		// Print out 
		SetConsoleTextAttribute(hConsole, 0x0F);
		//std::cout << " " << 8 - y << " ";
		std::cout << " " << y << " ";

		for (int x = 0; x < BOARD_DIM; x++)
		{
			// Determine the color and character glyph at this part of the chessboard

			// Color check:
			// If both X and Y are even/odd (X & Y same sign), then light tile
			// Else if X and Y are different signs, then dark tile (background color)
			// If sign of Chessboard piece is negative, then black piece. Else if positive, white piece

			// COLOR CHECKING; reset
			foundInPos = false;

			// Determine if black piece or white piece
			// BLACK PIECE
			if (Chessboard[y][x] < 0)
			{
				// If current tile is highlighted tile
				if (highlightedTile == std::make_pair(x, y))
					SetConsoleTextAttribute(hConsole, BLACK_ON_GREEN);
				else {

					// Check if current tile is one of the possible tiles to move to
					if (isPieceSelected) {
						for (int i = 0; i < possibleTiles.size(); i++) {
							if (possibleTiles[i] == std::make_pair(x, y)) {
								SetConsoleTextAttribute(hConsole, BLACK_ON_RED);
								foundInPos = true;
							}
						}
					}

					if (!foundInPos) {

						if (selectedTile == std::make_pair(x, y))
							SetConsoleTextAttribute(hConsole, BLACK_ON_BLUE);
						// Else, normal tile
						else {
							// If LIGHT tile
							if (x % 2 == y % 2)
								SetConsoleTextAttribute(hConsole, BLACK_ON_LIGHT);
							// Else if DARK tile
							else
								SetConsoleTextAttribute(hConsole, BLACK_ON_DARK);
						}
					}
				}
			}
			// WHITE PIECE
			else
			{
				// If current tile is highlighted tile
				if (highlightedTile == std::make_pair(x, y))
					SetConsoleTextAttribute(hConsole, WHITE_ON_GREEN);
				else {

					// Check if current tile is one of the possible tiles to move to
					if (isPieceSelected) {
						for (int i = 0; i < possibleTiles.size(); i++) {
							if (possibleTiles[i] == std::make_pair(x, y)) {
								SetConsoleTextAttribute(hConsole, WHITE_ON_RED);
								foundInPos = true;
							}
						}
					}

					if (!foundInPos) {

						if (selectedTile == std::make_pair(x, y))
							SetConsoleTextAttribute(hConsole, WHITE_ON_BLUE);
						// Else, normal tile
						else {
							// If LIGHT tile
							if (x % 2 == y % 2)
								SetConsoleTextAttribute(hConsole, WHITE_ON_LIGHT);
							// Else if DARK tile
							else
								SetConsoleTextAttribute(hConsole, WHITE_ON_DARK);
						}
					}
				}
			}

			// Actually print out the character
			switch (abs(Chessboard[y][x]))
			{
				// If it's just an empty tile
			case EMPTY:

				// If this current empty tile is highlighted, then make it green
				if (highlightedTile == std::make_pair(x, y))
					SetConsoleTextAttribute(hConsole, GREEN);
				// For logic's sake, redundant:
				else if (selectedTile == std::make_pair(x, y))
					SetConsoleTextAttribute(hConsole, BLUE);

				// ELSE, if it's a normal tile to draw
				else
				{
					// If empty tile is odd/even, make it light/dark
					if (x % 2 == y % 2)
						SetConsoleTextAttribute(hConsole, LIGHT);
					else
						SetConsoleTextAttribute(hConsole, DARK);
				}

				// If piece selected, show the possible moves
				if (isPieceSelected) {
					// Check every possible move
					for (int i = 0; i < possibleTiles.size(); i++) {
						// If the empty tile's a possible tile, make cyan
						if (highlightedTile == std::make_pair(x, y))
							SetConsoleTextAttribute(hConsole, GREEN);
						// But, if it turns out to be a highlighted tile, then paint it green
						else if (possibleTiles[i] == std::make_pair(x, y))
							SetConsoleTextAttribute(hConsole, CYAN);
					}
				}
				std::cout << Glyph[aTILE] << Glyph[aTILE];
				break;

				// Else... check every piece type
			case PAWN_A: case PAWN_B: case PAWN_C: case PAWN_D: case PAWN_E: case PAWN_F: case PAWN_G: case PAWN_H:
				std::cout << Glyph[aPAWN] << (isPawnOneCharWidth ? " " : "");
				break;

			case KING:
				std::cout << Glyph[aKING] << " ";
				break;

			case QUEEN:
				std::cout << Glyph[aQUEEN] << " ";
				break;

			case BISHOP_L: case BISHOP_R:
				std::cout << Glyph[aBISHOP] << " ";
				break;

			case KNIGHT_L: case KNIGHT_R:
				std::cout << Glyph[aKNIGHT] << " ";
				break;

			case ROOK_L: case ROOK_R:
				std::cout << Glyph[aROOK] << " ";
				break;
			}

		}
		// Print out MoveHistory, up to 8 previous moves (one for each row)
		SetConsoleTextAttribute(hConsole, 0x0F);
		std::cout << "   ";

		// Print out MoveHistory

		// If there's more than 8 entries within the move history
		if (allMoveHistory.size() > BOARD_DIM)
		{
			std::cout << allMoveHistory.size() - (7 - y) << ")  " << allMoveHistory[allMoveHistory.size() - 1 - (7 - y)].first.moveNotation << " \t";
			if (allMoveHistory[allMoveHistory.size() - 1 - (7 - y)].second.moveNotation.size() > 0)
				std::cout << allMoveHistory[allMoveHistory.size() - 1 - (7 - y)].second.moveNotation;
			else
				std::cout << "           ";
		}
		// Else if there are fewer than or equal to 8 entries within move history (so scrolling is not a factor)
		else if (allMoveHistory.size() <= BOARD_DIM && y < allMoveHistory.size())
		{
			std::cout << y + 1 << ")  " << allMoveHistory[y].first.moveNotation << " \t";
			if (allMoveHistory[y].second.moveNotation.size() > 0)
				std::cout << allMoveHistory[y].second.moveNotation;
			else
				std::cout << "           ";
		}
		else
			std::cout << "                  ";


		// Reset color to black at the end of a row
		SetConsoleTextAttribute(hConsole, 0x00);
		std::cout << ".\n";
	}
	std::cout << "\n";
}

bool ChessGame::ReadInput()
{
	_getch();

	if (isKeyBindPressed(MOVE_UP))
	{
		highlightedTile.second > 0 ? highlightedTile.second-- : highlightedTile.second = BOARD_DIM - 1;
	}

	if (isKeyBindPressed(MOVE_DOWN))
	{
		highlightedTile.second < BOARD_DIM - 1 ? highlightedTile.second++ : highlightedTile.second = 0;
	}

	if (isKeyBindPressed(MOVE_LEFT))
	{
		highlightedTile.first > 0 ? highlightedTile.first-- : highlightedTile.first = BOARD_DIM - 1;
	}

	if (isKeyBindPressed(MOVE_RIGHT))
	{
		highlightedTile.first < BOARD_DIM - 1 ? highlightedTile.first++ : highlightedTile.first = 0;
	}


	if (isKeyBindPressed(SELECT))
	{
		// Find the index of that piece		
		pieceIDIndex = getPieceIDIndex(selectedPiece);

		// If we're just moving around highlighting pieces, because one isn't selected to view its possible moves
		if (!isPieceSelected)
		{
			// If the highlighted tile has a piece on it (not empty tile),
			// AND it's the correct color (a piece of the respective player's turn)
			if (Chessboard[highlightedTile.second][highlightedTile.first] != EMPTY
				&& isWhiteTurn == isPositive(Chessboard[highlightedTile.second][highlightedTile.first])
				) {
				// Set the selectedTile coordinates to the highlighted then
				selectedTile = highlightedTile;

				// Toggle the state to say that we have a piece selected
				isPieceSelected = true;			// COMMENT OUT THIS LINE TO VIEW SQUARE'S HIGHLIGHTED COLORS WHEN DEBUGGING

				// Set the selected piece from the one on the chessboard
				selectedPiece = Chessboard[highlightedTile.second][highlightedTile.first];

				// We change boolean so only when the board is updated WHEN we select a possible tile, we then run the calculation of possible moves
				updatedPosMoves = true;
			}
		}
		// Else, if we have selected a piece and we're viewing its moves...
		else
		{
			// If highlightedTile is over the selectedTile (aka DESELECTING the piece we chose)
			if (highlightedTile == selectedTile)
			{
				// Then the selected tile to Out Of Bounds (so it can't be shown on screen and selects no piece)
				selectedTile = { -1, -1 };

				// Toggle state back to false, we deselected
				isPieceSelected = false;
			}
			// Else, if NOT deselecting the selectedTile... must be a different tile... so we run checks
			else {
				// If we're hovering over a square that represents a POSSIBLE MOVE...
				if (isPieceWithinPos(highlightedTile, possibleTiles))
				{
					// If it's white turn, then add a new pair of moves to the MoveHistory vector - else, we would just use the existing one that was created on white's previous turn
					if (isWhiteTurn)
						allMoveHistory.push_back(std::make_pair(MoveHistoryData(), MoveHistoryData()));

					// Then check to see if it's  an empty space, or whether it's an ENEMY PIECE
					// If empty space, then we move our selectedTile to it!
					if (Chessboard[highlightedTile.second][highlightedTile.first] == EMPTY)
					{
						// Depending on black or white piece, depends on which pair we fit it in
						// Black
						if (selectedPiece < 0) {
							allMoveHistory.back().second.aCaptureMove = false;
							allMoveHistory.back().second.pieceID = (selectedPiece);
							allMoveHistory.back().second.isWhitePiece = false;
							allMoveHistory.back().second.startPos = PlayerPieces[BLACK_PLAYER][pieceIDIndex].position;
							PlayerPieces[BLACK_PLAYER][pieceIDIndex].position = highlightedTile;
							allMoveHistory.back().second.finishPos = PlayerPieces[BLACK_PLAYER][pieceIDIndex].position;
							allMoveHistory.back().second.moveNotation = getFullNotation(selectedPiece, allMoveHistory.back().second.startPos, allMoveHistory.back().second.finishPos, false, false, false);
						}
						// White
						else if (selectedPiece > 0) {
							allMoveHistory.back().first.aCaptureMove = false;
							allMoveHistory.back().first.pieceID = (selectedPiece);
							allMoveHistory.back().first.isWhitePiece = true;
							allMoveHistory.back().first.startPos = PlayerPieces[WHITE_PLAYER][pieceIDIndex].position;
							PlayerPieces[WHITE_PLAYER][pieceIDIndex].position = highlightedTile;
							allMoveHistory.back().first.finishPos = PlayerPieces[WHITE_PLAYER][pieceIDIndex].position;
							allMoveHistory.back().first.moveNotation = getFullNotation(selectedPiece, allMoveHistory.back().first.startPos, allMoveHistory.back().first.finishPos, false, false, false);
						}

					}
					// Else, if it's an enemy piece we are capturing
					else
					{
						// Depending on black or white piece, depends on which pair we fit it in
						// Black
						if (selectedPiece < 0) {
							allMoveHistory.back().second.aCaptureMove = true;
							allMoveHistory.back().second.capturedPieceID = Chessboard[highlightedTile.second][highlightedTile.first];
							PlayerPieces[WHITE_PLAYER][getPieceIDIndex(allMoveHistory.back().second.capturedPieceID)].capturedYet = true;
							allMoveHistory.back().second.pieceID = (selectedPiece);
							allMoveHistory.back().second.isWhitePiece = false;
							allMoveHistory.back().second.startPos = PlayerPieces[BLACK_PLAYER][pieceIDIndex].position;
							PlayerPieces[BLACK_PLAYER][pieceIDIndex].position = highlightedTile;
							allMoveHistory.back().second.finishPos = PlayerPieces[BLACK_PLAYER][pieceIDIndex].position;
							allMoveHistory.back().second.moveNotation = getFullNotation(selectedPiece, allMoveHistory.back().second.startPos, allMoveHistory.back().second.finishPos, true, false, false);
						}
						// White
						else if (selectedPiece > 0) {
							allMoveHistory.back().first.aCaptureMove = true;
							allMoveHistory.back().first.capturedPieceID = Chessboard[highlightedTile.second][highlightedTile.first];
							PlayerPieces[BLACK_PLAYER][getPieceIDIndex(allMoveHistory.back().first.capturedPieceID)].capturedYet = true;
							allMoveHistory.back().first.pieceID = (selectedPiece);
							allMoveHistory.back().first.isWhitePiece = true;
							allMoveHistory.back().first.startPos = PlayerPieces[WHITE_PLAYER][pieceIDIndex].position;
							PlayerPieces[WHITE_PLAYER][pieceIDIndex].position = highlightedTile;
							allMoveHistory.back().first.finishPos = PlayerPieces[WHITE_PLAYER][pieceIDIndex].position;
							allMoveHistory.back().first.moveNotation = getFullNotation(selectedPiece, allMoveHistory.back().first.startPos, allMoveHistory.back().first.finishPos, true, false, false);
						}

					}

					// Reset selectedTile
					selectedTile = { -1, -1 };
					isPieceSelected = false;

					// Toggle the turn
					isWhiteTurn = !isWhiteTurn;

				}


			}

		}
	}

	// If Undo key pressed
	if (isKeyBindPressed(UNDO) && !allMoveHistory.empty())
	{
		// Undoing requires popping the allMoveHistory back to remove the most recent move, then we set everything to the data in the newest "backmost" cell of the vector,
		// Reset PlayerPieces to the previous cell, and then UpdateBoard will fill out the Chessboard 2D array to the previous one

		// Find who's turn it is
		if (isWhiteTurn)
		{
			// Now move the black piece back
			PlayerPieces[BLACK_PLAYER][getPieceIDIndex(allMoveHistory.back().second.pieceID)].position = allMoveHistory.back().second.startPos;

			// Check to see if it was a capture move we're undoing, in that case, we have to spawn
			if (allMoveHistory.back().second.aCaptureMove)
				PlayerPieces[WHITE_PLAYER][getPieceIDIndex(allMoveHistory.back().second.capturedPieceID)].capturedYet = false;

			// Reset that black piece move as if it never happened
			allMoveHistory.back().second = MoveHistoryData();
		}
		else
		{
			PlayerPieces[WHITE_PLAYER][getPieceIDIndex(allMoveHistory.back().first.pieceID)].position = allMoveHistory.back().first.startPos;

			if (allMoveHistory.back().first.aCaptureMove)
				PlayerPieces[BLACK_PLAYER][getPieceIDIndex(allMoveHistory.back().first.capturedPieceID)].capturedYet = false;
			allMoveHistory.pop_back();
		}

		// Toggle turn backwards
		isWhiteTurn = !isWhiteTurn;
	}


	// Increase/decrease font size
	if (isKeyBindPressed(INCR_FONT) && fontSize < 128)
		setFontSize(fontSize += 4);

	if (isKeyBindPressed(DECR_FONT) && fontSize > 4)
		setFontSize(fontSize -= 4);

	// Save move history
	if (isKeyBindPressed(SAVE_MOVES))
		stillPlaying = ExportMoveHistory();

	// Quit game
	if (isKeyBindPressed(EXIT))
		return false;
	else
		return true;
}

void ChessGame::CalcMoves()
{
	// Only calculated if a piece is selected, and we haven't calculated the moves for that piece yet... 
	if (isPieceSelected && updatedPosMoves && selectedPiece != EMPTY)
	{
		// Check if current piece 
		bool isWhitePiece = selectedPiece < 0 ? false : true;
		int currentTilePiece;
		/*if (selectedPiece < 0)
			isWhiteTile = false;
		else if(selectedPiece > 0)*/

		// Clear possibleTiles, we're going to add new possible tiles
		possibleTiles.clear();

		// Check if a PAWN
		if (abs(selectedPiece) >= PAWN_A && abs(selectedPiece) <= PAWN_H) {
			// If so, determine if black or white piece throughout			
			// Go to the selectedTile (the coordinates of selectedPiece) and calculate if it can move 
				// Since it's a black or white pawn; see if can move down/up 1 space (unobstructed, right below/above it MUST be an empty space)
			if (Chessboard[selectedTile.second + (isWhitePiece ? -1 : 1)][selectedTile.first] == EMPTY) {

				possibleTiles.push_back({ selectedTile.first, selectedTile.second + (isWhitePiece ? -1 : 1) });

				// Check if it's in its original spot. (Row 1 for black.) If it is unmoved, then check again if you can move another spot down/up (first move 2 tiles)
				if (selectedTile.second == (isWhitePiece ? 6 : 1) && abs(Chessboard[selectedTile.second + (isWhitePiece ? -2 : 2)][selectedTile.first]) == EMPTY)
					possibleTiles.push_back({ selectedTile.first, selectedTile.second + (isWhitePiece ? -2 : 2) });
			}

			// Check if can capture (both sides diagonal down/up)
			// Diagonally down/up + LEFT, check if not on the LEFTMOST column of board, and piece must be of opposite color
			if (selectedTile.first > 0 && abs((Chessboard[selectedTile.second + (isWhitePiece ? -1 : 1)][selectedTile.first - 1])) != EMPTY
				&& (isWhitePiece != isPositive((Chessboard[selectedTile.second + (isWhitePiece ? -1 : 1)][selectedTile.first - 1]))))
				possibleTiles.push_back({ selectedTile.first - 1, selectedTile.second + (isWhitePiece ? -1 : 1) });


			// Diagonally down/up + RIGHT, check if not on the RIGHTMOST column of board, and piece must be of opposite color
			if (selectedTile.first < BOARD_DIM - 1 && abs((Chessboard[selectedTile.second + (isWhitePiece ? -1 : 1)][selectedTile.first + 1])) != EMPTY
				&& (isWhitePiece != isPositive((Chessboard[selectedTile.second + (isWhitePiece ? -1 : 1)][selectedTile.first + 1]))))
				possibleTiles.push_back({ selectedTile.first + 1, selectedTile.second + (isWhitePiece ? -1 : 1) });




		}
		// Check if a ROOK
		else if (abs(selectedPiece) == ROOK_L || abs(selectedPiece) == ROOK_R) {

			// Rooks can move any number of spaces up, down, left, or right, stopping at the edge of the board, or at the capture of another piece
			// Check all spaces UP first
			for (int y = selectedTile.second - 1; y >= 0; y--) {
				// If there's another consecutive vacant space above it, add it to the list of possible moves
				if (abs(Chessboard[y][selectedTile.first]) == EMPTY)
					possibleTiles.push_back({ selectedTile.first, y });
				// Else, add it ONLY if it's an enemy piece; and then stop checking and move on, we've been blocked by a piece or the end of the board
				else {
					if (isWhitePiece != isPositive(Chessboard[y][selectedTile.first]))
						possibleTiles.push_back({ selectedTile.first, y });
					break;
				}
			}

			// Check all spaces DOWN
			for (int y = selectedTile.second + 1; y <= BOARD_DIM - 1; y++) {
				// If there's another consecutive vacant space below it, add it to the list of possible moves
				if (abs(Chessboard[y][selectedTile.first]) == EMPTY)
					possibleTiles.push_back({ selectedTile.first, y });
				// Else, add it ONLY if it's an enemy piece; and then stop checking and move on, we've been blocked by a piece or the end of the board
				else {
					if (isWhitePiece != isPositive(Chessboard[y][selectedTile.first]))
						possibleTiles.push_back({ selectedTile.first, y });
					break;
				}
			}

			// Check all spaces LEFT
			for (int x = selectedTile.first - 1; x >= 0; x--) {
				// If there's another consecutive vacant space above it, add it to the list of possible moves
				if (abs(Chessboard[selectedTile.second][x]) == EMPTY)
					possibleTiles.push_back({ x, selectedTile.second });
				// Else, add it ONLY if it's an enemy piece; and then stop checking and move on, we've been blocked by a piece or the end of the board
				else {
					if (isWhitePiece != isPositive(Chessboard[selectedTile.second][x]))
						possibleTiles.push_back({ x, selectedTile.second });
					break;
				}
			}

			// Check all spaces RIGHT
			for (int x = selectedTile.first + 1; x <= BOARD_DIM - 1; x++) {
				// If there's another consecutive vacant space below it, add it to the list of possible moves
				if (abs(Chessboard[selectedTile.second][x]) == EMPTY)
					possibleTiles.push_back({ x, selectedTile.second });
				// Else, add it ONLY if it's an enemy piece; and then stop checking and move on, we've been blocked by a piece or the end of the board
				else {
					if (isWhitePiece != isPositive(Chessboard[selectedTile.second][x]))
						possibleTiles.push_back({ x, selectedTile.second });
					break;
				}
			}

		}
		// Check if KNIGHT
		else if (abs(selectedPiece) == KNIGHT_L || abs(selectedPiece) == KNIGHT_R) {

			// Knights can move 2 spaces in a direction, then 1 space perpendicular to it, on an empty or enemy piece (to capture)

			// 2 right, 1 up/down
			if (selectedTile.first <= BOARD_DIM - 3 && selectedTile.second >= 1)
			{
				currentTilePiece = Chessboard[selectedTile.second - 1][selectedTile.first + 2];
				if (currentTilePiece == EMPTY || isWhiteTurn != isPositive(currentTilePiece)) {
					possibleTiles.push_back({ selectedTile.first + 2, selectedTile.second - 1 });
				}
			}
			if (selectedTile.first <= BOARD_DIM - 3 && selectedTile.second < BOARD_DIM - 1)
			{
				currentTilePiece = Chessboard[selectedTile.second + 1][selectedTile.first + 2];
				if (currentTilePiece == EMPTY || isWhiteTurn != isPositive(currentTilePiece)) {
					possibleTiles.push_back({ selectedTile.first + 2, selectedTile.second + 1 });
				}
			}


			// 2 left, 1 up/down
			if (selectedTile.first >= 2 && selectedTile.second >= 1)
			{
				currentTilePiece = Chessboard[selectedTile.second - 1][selectedTile.first - 2];
				if (currentTilePiece == EMPTY || isWhiteTurn != isPositive(currentTilePiece)) {
					possibleTiles.push_back({ selectedTile.first - 2, selectedTile.second - 1 });
				}
			}
			if (selectedTile.first >= 2 && selectedTile.second <= BOARD_DIM - 2)
			{
				currentTilePiece = Chessboard[selectedTile.second + 1][selectedTile.first - 2];
				if (currentTilePiece == EMPTY || isWhiteTurn != isPositive(currentTilePiece)) {
					possibleTiles.push_back({ selectedTile.first - 2, selectedTile.second + 1 });
				}
			}

			// 2 up, 1 left/right
			if (selectedTile.second >= 2 && selectedTile.first >= 1)
			{
				currentTilePiece = Chessboard[selectedTile.second - 2][selectedTile.first - 1];
				if (currentTilePiece == EMPTY || isWhiteTurn != isPositive(currentTilePiece)) {
					possibleTiles.push_back({ selectedTile.first - 1, selectedTile.second - 2 });
				}
			}
			if (selectedTile.second >= 2 && selectedTile.first <= BOARD_DIM - 2)
			{
				currentTilePiece = Chessboard[selectedTile.second - 2][selectedTile.first + 1];
				if (currentTilePiece == EMPTY || isWhiteTurn != isPositive(currentTilePiece)) {
					possibleTiles.push_back({ selectedTile.first + 1, selectedTile.second - 2 });
				}
			}

			// 2 down, 1 left/right
			if (selectedTile.second <= BOARD_DIM - 3 && selectedTile.first >= 1)
			{
				currentTilePiece = Chessboard[selectedTile.second + 2][selectedTile.first - 1];
				if (currentTilePiece == EMPTY || isWhiteTurn != isPositive(currentTilePiece)) {
					possibleTiles.push_back({ selectedTile.first - 1, selectedTile.second + 2 });
				}
			}
			if (selectedTile.second <= BOARD_DIM - 3 && selectedTile.first <= BOARD_DIM - 2)
			{
				currentTilePiece = Chessboard[selectedTile.second + 2][selectedTile.first + 1];
				if (currentTilePiece == EMPTY || isWhiteTurn != isPositive(currentTilePiece)) {
					possibleTiles.push_back({ selectedTile.first + 1, selectedTile.second + 2 });
				}
			}


		}
		// Check if BISHOP
		else if (abs(selectedPiece) == BISHOP_L || abs(selectedPiece) == BISHOP_R) {

			// Bishops move diagonally, so we check all four diagonal directions til the edge of the board, or the first enemy piece to capture
			// Up and right
			for (int i = 1; i < min(BOARD_DIM - selectedTile.first, selectedTile.second + 1); i++) {
				currentTilePiece = Chessboard[selectedTile.second - i][selectedTile.first + i];

				// If we encounter an empty tile, we allow it
				if (currentTilePiece == EMPTY)
					possibleTiles.push_back({ selectedTile.first + i, selectedTile.second - i });
				// Otherwise, if not empty, then check too see if it's an enemy piece we can capture. Then break loop, regardless
				else {
					if (isWhiteTurn != isPositive(currentTilePiece))
						possibleTiles.push_back({ selectedTile.first + i, selectedTile.second - i });
					break;
				}
			}

			// Up and left
			for (int i = 1; i < min(selectedTile.first + 1, selectedTile.second + 1); i++) {
				currentTilePiece = Chessboard[selectedTile.second - i][selectedTile.first - i];

				// If we encounter an empty tile, we allow it
				if (currentTilePiece == EMPTY)
					possibleTiles.push_back({ selectedTile.first - i, selectedTile.second - i });
				// Otherwise, if not empty, then check too see if it's an enemy piece we can capture. Then break loop, regardless
				else {
					if (isWhiteTurn != isPositive(currentTilePiece))
						possibleTiles.push_back({ selectedTile.first - i, selectedTile.second - i });
					break;
				}
			}

			// Down and left
			for (int i = 1; i < min(selectedTile.first + 1, BOARD_DIM - selectedTile.second); i++) {
				currentTilePiece = Chessboard[selectedTile.second + i][selectedTile.first - i];

				// If we encounter an empty tile, we allow it
				if (currentTilePiece == EMPTY)
					possibleTiles.push_back({ selectedTile.first - i, selectedTile.second + i });
				// Otherwise, if not empty, then check too see if it's an enemy piece we can capture. Then break loop, regardless
				else {
					if (isWhiteTurn != isPositive(currentTilePiece))
						possibleTiles.push_back({ selectedTile.first - i, selectedTile.second + i });
					break;
				}
			}

			// Down and right 
			for (int i = 1; i < min(BOARD_DIM - selectedTile.first, BOARD_DIM - selectedTile.second); i++) {
				currentTilePiece = Chessboard[selectedTile.second + i][selectedTile.first + i];

				// If we encounter an empty tile, we allow it
				if (currentTilePiece == EMPTY)
					possibleTiles.push_back({ selectedTile.first + i, selectedTile.second + i });
				// Otherwise, if not empty, then check too see if it's an enemy piece we can capture. Then break loop, regardless
				else {
					if (isWhiteTurn != isPositive(currentTilePiece))
						possibleTiles.push_back({ selectedTile.first + i, selectedTile.second + i });
					break;
				}
			}

		}
		// Check if QUEEN
		else if (abs(selectedPiece) == QUEEN) {

			// Queens can move as both rooks and bishops
			// Check all spaces UP first
			for (int y = selectedTile.second - 1; y >= 0; y--) {
				// If there's another consecutive vacant space above it, add it to the list of possible moves
				if (abs(Chessboard[y][selectedTile.first]) == EMPTY)
					possibleTiles.push_back({ selectedTile.first, y });
				// Else, add it ONLY if it's an enemy piece; and then stop checking and move on, we've been blocked by a piece or the end of the board
				else {
					if (isWhitePiece != isPositive(Chessboard[y][selectedTile.first]))
						possibleTiles.push_back({ selectedTile.first, y });
					break;
				}
			}

			// Check all spaces DOWN
			for (int y = selectedTile.second + 1; y <= BOARD_DIM - 1; y++) {
				// If there's another consecutive vacant space below it, add it to the list of possible moves
				if (abs(Chessboard[y][selectedTile.first]) == EMPTY)
					possibleTiles.push_back({ selectedTile.first, y });
				// Else, add it ONLY if it's an enemy piece; and then stop checking and move on, we've been blocked by a piece or the end of the board
				else {
					if (isWhitePiece != isPositive(Chessboard[y][selectedTile.first]))
						possibleTiles.push_back({ selectedTile.first, y });
					break;
				}
			}

			// Check all spaces LEFT
			for (int x = selectedTile.first - 1; x >= 0; x--) {
				// If there's another consecutive vacant space above it, add it to the list of possible moves
				if (abs(Chessboard[selectedTile.second][x]) == EMPTY)
					possibleTiles.push_back({ x, selectedTile.second });
				// Else, add it ONLY if it's an enemy piece; and then stop checking and move on, we've been blocked by a piece or the end of the board
				else {
					if (isWhitePiece != isPositive(Chessboard[selectedTile.second][x]))
						possibleTiles.push_back({ x, selectedTile.second });
					break;
				}
			}

			// Check all spaces RIGHT
			for (int x = selectedTile.first + 1; x <= BOARD_DIM - 1; x++) {
				// If there's another consecutive vacant space below it, add it to the list of possible moves
				if (abs(Chessboard[selectedTile.second][x]) == EMPTY)
					possibleTiles.push_back({ x, selectedTile.second });
				// Else, add it ONLY if it's an enemy piece; and then stop checking and move on, we've been blocked by a piece or the end of the board
				else {
					if (isWhitePiece != isPositive(Chessboard[selectedTile.second][x]))
						possibleTiles.push_back({ x, selectedTile.second });
					break;
				}
			}

			// Up and right
			for (int i = 1; i < min(BOARD_DIM - selectedTile.first, selectedTile.second + 1); i++) {
				currentTilePiece = Chessboard[selectedTile.second - i][selectedTile.first + i];

				// If we encounter an empty tile, we allow it
				if (currentTilePiece == EMPTY)
					possibleTiles.push_back({ selectedTile.first + i, selectedTile.second - i });
				// Otherwise, if not empty, then check too see if it's an enemy piece we can capture. Then break loop, regardless
				else {
					if (isWhiteTurn != isPositive(currentTilePiece))
						possibleTiles.push_back({ selectedTile.first + i, selectedTile.second - i });
					break;
				}
			}

			// Up and left
			for (int i = 1; i < min(selectedTile.first + 1, selectedTile.second + 1); i++) {
				currentTilePiece = Chessboard[selectedTile.second - i][selectedTile.first - i];

				// If we encounter an empty tile, we allow it
				if (currentTilePiece == EMPTY)
					possibleTiles.push_back({ selectedTile.first - i, selectedTile.second - i });
				// Otherwise, if not empty, then check too see if it's an enemy piece we can capture. Then break loop, regardless
				else {
					if (isWhiteTurn != isPositive(currentTilePiece))
						possibleTiles.push_back({ selectedTile.first + i, selectedTile.second - i });
					break;
				}
			}

			// Down and left
			for (int i = 1; i < min(selectedTile.first + 1, BOARD_DIM - selectedTile.second); i++) {
				currentTilePiece = Chessboard[selectedTile.second + i][selectedTile.first - i];

				// If we encounter an empty tile, we allow it
				if (currentTilePiece == EMPTY)
					possibleTiles.push_back({ selectedTile.first - i, selectedTile.second + i });
				// Otherwise, if not empty, then check too see if it's an enemy piece we can capture. Then break loop, regardless
				else {
					if (isWhiteTurn != isPositive(currentTilePiece))
						possibleTiles.push_back({ selectedTile.first - i, selectedTile.second + i });
					break;
				}
			}

			// Down and right
			for (int i = 1; i < min(BOARD_DIM - selectedTile.first, BOARD_DIM - selectedTile.second); i++) {
				currentTilePiece = Chessboard[selectedTile.second + i][selectedTile.first + i];

				// If we encounter an empty tile, we allow it
				if (currentTilePiece == EMPTY)
					possibleTiles.push_back({ selectedTile.first + i, selectedTile.second + i });
				// Otherwise, if not empty, then check too see if it's an enemy piece we can capture. Then break loop, regardless
				else {
					if (isWhiteTurn != isPositive(currentTilePiece))
						possibleTiles.push_back({ selectedTile.first + i, selectedTile.second + i });
					break;
				}
			}



		}
		// Check if KING
		else if (abs(selectedPiece) == KING) {

			// A king can move 1 step in any direction
			// Move up
			if (selectedTile.second > 0 && (Chessboard[selectedTile.second - 1][selectedTile.first] == EMPTY || isWhiteTurn != isPositive(Chessboard[selectedTile.second - 1][selectedTile.first])))
				possibleTiles.push_back({ selectedTile.first, selectedTile.second - 1 });

			// Move up & right
			if (selectedTile.second > 0 && selectedTile.first < BOARD_DIM - 1 && (Chessboard[selectedTile.second - 1][selectedTile.first + 1] == EMPTY || isWhiteTurn != isPositive(Chessboard[selectedTile.second - 1][selectedTile.first + 1])))
				possibleTiles.push_back({ selectedTile.first + 1, selectedTile.second - 1 });

			// Move right
			if (selectedTile.first < BOARD_DIM - 1 && (Chessboard[selectedTile.second][selectedTile.first + 1] == EMPTY || isWhiteTurn != isPositive(Chessboard[selectedTile.second][selectedTile.first + 1])))
				possibleTiles.push_back({ selectedTile.first + 1, selectedTile.second });

			// Move down & right
			if (selectedTile.second < BOARD_DIM - 1 && selectedTile.first < BOARD_DIM - 1 && (Chessboard[selectedTile.second + 1][selectedTile.first + 1] == EMPTY || isWhiteTurn != isPositive(Chessboard[selectedTile.second + 1][selectedTile.first + 1])))
				possibleTiles.push_back({ selectedTile.first + 1, selectedTile.second + 1 });

			// Move down
			if (selectedTile.second < BOARD_DIM - 1 && (Chessboard[selectedTile.second + 1][selectedTile.first] == EMPTY || isWhiteTurn != isPositive(Chessboard[selectedTile.second + 1][selectedTile.first])))
				possibleTiles.push_back({ selectedTile.first, selectedTile.second + 1 });

			// Move down & left
			if (selectedTile.second < BOARD_DIM - 1 && selectedTile.first > 0 && (Chessboard[selectedTile.second + 1][selectedTile.first - 1] == EMPTY || isWhiteTurn != isPositive(Chessboard[selectedTile.second + 1][selectedTile.first - 1])))
				possibleTiles.push_back({ selectedTile.first - 1, selectedTile.second + 1 });

			// Move left
			if (selectedTile.first > 0 && (Chessboard[selectedTile.second][selectedTile.first - 1] == EMPTY || isWhiteTurn != isPositive(Chessboard[selectedTile.second][selectedTile.first - 1])))
				possibleTiles.push_back({ selectedTile.first - 1, selectedTile.second });

			// Move up & left
			if (selectedTile.second > 0 && selectedTile.first > 0 && (Chessboard[selectedTile.second - 1][selectedTile.first - +1] == EMPTY || isWhiteTurn != isPositive(Chessboard[selectedTile.second - 1][selectedTile.first - +1])))
				possibleTiles.push_back({ selectedTile.first - 1, selectedTile.second - 1 });

			// I should probably not allow the King to put himself in check...
		}

		// Reset updatedPosMoves
		updatedPosMoves = false;
	}
}
