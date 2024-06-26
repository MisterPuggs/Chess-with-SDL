//
// Created by cew05 on 21/06/2024.
//

#ifndef CHESS_WITH_SDL_GAMESCREEN_H
#define CHESS_WITH_SDL_GAMESCREEN_H

#include <random>
#include "AppScreen.h"
#include "../../Gameplay/include/Board.h"
#include "../../Gameplay/include/IncludePieces.h"
#include "../../StockfishUtil/StockfishManager.h"

class GameScreen : public AppScreen {
    public:
        enum GameState : int {
            SHOW_PROMO_MENU = LAST_SCREEN_STATE, END_OF_TURN, ALL_TASKS_COMPLETE, CHECKMATE, STALEMATE,
            DRAW_OFFER, RESIGN, BOARD_FLIPPED,
        };

    private:
        enum MenuID : int {
            OPTIONS_MENU, GAME_OVER,
        };
        enum buttonID : int {
            OM_HOME_SCREEN, OM_RESIGN, OM_OFFER_DRAW, OM_NEWGAME, OM_FLIP_BOARD,
        };

        // Pointers to board and pieces
        Board* board = nullptr;
        SelectedPiece* selectedPiece = new SelectedPiece();
        std::vector<Piece*>* allPieces = new std::vector<Piece*>;
        std::vector<Piece*>* whitePieces = new std::vector<Piece*>;
        std::vector<Piece*>* blackPieces = new std::vector<Piece*>;
        std::vector<Piece*>* teamptr = nullptr;
        std::vector<Piece*>* oppptr = nullptr;
        std::vector<Piece*>* userTeamPtr = nullptr;

        // Stockfish
        StockfishManager* sfm = nullptr;

        // Turn management
        bool usersTurn;

    public:
        GameScreen(char _teamID);
        ~GameScreen();

        // Game setup
        void SetUpBoard();
        void SetUpPieces();
        void SetupEngine(bool _limitStrength, int _elo, int _level);

        // Display
        bool CreateTextures() override;
        bool Display() override;
        void ResizeScreen() override;

        // Handle events
        void HandleEvents() override;
        void CheckButtons() override;
        std::string FetchOpponentMove();
        std::string FetchOpponentMoveEngine();
};


#endif //CHESS_WITH_SDL_GAMESCREEN_H
