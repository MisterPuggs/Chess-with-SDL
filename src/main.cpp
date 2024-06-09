//
// Created by cew05 on 19/04/2024.
//

#define SDL_MAIN_HANDLED
#include <iostream>
#include <SDL.h>
#include <SDL_ttf.h>
#include <fstream>

#include "src_headers/GlobalSource.h"
#include "src_headers/GlobalVars.h"
#include "src_headers/Board.h"
#include "src_headers/Piece.h"
#include "src_headers/SelectedPiece.h"
#include "src_headers/Knight.h"
#include "src_headers/Bishop.h"
#include "src_headers/Rook.h"
#include "src_headers/Queen.h"
#include "src_headers/King.h"

int EnsureWindowSize(Board* _board) {
    // Fetch rect of current window properties
    SDL_Rect rect_current = {0, 0};
    SDL_GetWindowSize(window.window, &rect_current.w, &rect_current.h);
    //SDL_GetWindowPosition(Window.window, &rect_current.a, &rect_current.b);

    // fetch min size boundaries
    int minW, minH;
    _board->GetMinDimensions(minW, minH);

    // detect if window has changed size
    int changed = 0;
    if(rect_current.h != window.currentRect.h || rect_current.w != window.currentRect.w) { // changed (generic)
        changed = 1;
        window.currentRect = rect_current;
    }
    if (rect_current.w < minW) { // below min
        changed = -1;
        SDL_SetWindowSize(window.window, minW, rect_current.h);
    }
    if (rect_current.h < minH) { // below min
        changed = -1;
        SDL_SetWindowSize(window.window, rect_current.w, minH);
    }

    return changed;
}

int main(int argc, char** argv) {
    // Init SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        LogError("Issue initialising SDL", SDL_GetError(), true);
        return 0;
    }

    // Init TTF
    if (TTF_Init() != 0) {
        LogError("TTF failed to init", SDL_GetError(), true);
        return 0;
    }

    // produce window
    window.currentRect = {0, 0, 500, 500};
    window.minRect = window.currentRect;
    window.window = SDL_CreateWindow("ChesSDL", 0, 0, 500, 500, SDL_WINDOW_RESIZABLE);
    if (!window.window) {
        LogError("Failed to create window", SDL_GetError(), true);
        return 0;
    }

    // Create renderer
    window.renderer = SDL_CreateRenderer(window.window, 0, SDL_RENDERER_ACCELERATED);
    if (!window.renderer) {
        LogError("Failed to create renderer", SDL_GetError(), true);
        return 0;
    }

    // fetch border sizes and apply to position
    int b_top, b_left, b_right, b_bottom;
    if (SDL_GetWindowBordersSize(window.window, &b_top, &b_left, &b_bottom, &b_right) != 0) {
        LogError("Failed to retrive display borders size", SDL_GetError(), false);
    }

    // Set Window Background
    window.background = IMG_LoadTexture(window.renderer, "../Resources/GameBoard/Green_Background.png");

//    // fetch device screen dimensions and apply border sizes to w, h
//    if(SDL_GetDisplayUsableBounds(0, &window.currentRect) != 0) {
//        LogError("Failure to obtain display usable bounds", SDL_GetError(), false);
//    }
//
//    // Apply borders to position and dimensions of window
//    window.currentRect.w -= b_right + b_left;
//    window.currentRect.h -= b_top;

    SDL_SetWindowPosition(window.window, b_left, b_top);
    SDL_SetWindowSize(window.window, window.currentRect.w, window.currentRect.h);

    // clean up any other errors which may appear before deliberate error checking
    SDL_ClearError();

    /*
     * CONFIG FILES
     */

    if (!ConfigExists()) {
        return -1;
    }

    /*
     *  CONSTRUCT GAME ELEMENTS
     */

    // Construct Board
    Board board;
    board.CreateBoardTexture();

    // Ensure GameData directory exists else end program
    if (!board.GameDataDirectoryExists()) {
        return 0;
    }
    board.ClearExcessGameFiles();
    // Create game data files in dir
    if (!board.CreateGameFiles()) {
        printf("Error creating game files.\n");
        return 0;
    }

    /*
     * CONSTRUCT WHITE PIECES
     */

    std::vector<Piece*> all_pieces;
    std::vector<Piece*> white_pieces;
    std::vector<Piece*> black_pieces;

    // Read standard board from file
    std::fstream boardStandardFile("../RequiredFiles/BasicSetup.csv");
    std::string pieceString;
    std::vector<std::string> pieceElements;
    char div = ',';
    uint64_t splitpos;

    while (std::getline(boardStandardFile, pieceString)) {
        pieceElements.clear();

        while ((splitpos = pieceString.find(div)) != std::string::npos) {
            pieceElements.push_back(pieceString.substr(0, splitpos));
            pieceString.erase(0, splitpos + 1);
        }
        pieceElements.push_back(pieceString);

        // construct Piece from elements taken from file
        Piece* newPiece = nullptr;
        if (pieceElements[1] == "Pawn") {
            newPiece = new Piece("Pawn", pieceElements[0], {(char)pieceElements[2][0], std::stoi(pieceElements[3])});
        }
        if (pieceElements[1] == "Knight") {
            newPiece = new Knight("Knight", pieceElements[0], {(char)pieceElements[2][0], std::stoi(pieceElements[3])});
        }
        if (pieceElements[1] == "Bishop") {
            newPiece = new Bishop("Bishop", pieceElements[0], {(char)pieceElements[2][0], std::stoi(pieceElements[3])});
        }
        if (pieceElements[1] == "Rook") {
            newPiece = new Rook("Rook", pieceElements[0], {(char)pieceElements[2][0], std::stoi(pieceElements[3])});
        }
        if (pieceElements[1] == "King") {
            newPiece = new King("King", pieceElements[0], {(char)pieceElements[2][0], std::stoi(pieceElements[3])});
        }
        if (pieceElements[1] == "Queen") {
            newPiece = new Queen("Queen", pieceElements[0], {(char)pieceElements[2][0], std::stoi(pieceElements[3])});
        }

        if (newPiece != nullptr) {
            newPiece->CreateTextures();
            newPiece->GetRectOfBoardPosition(board);
            (pieceElements[0][0] == 'W') ? white_pieces.push_back(newPiece) : black_pieces.push_back(newPiece);
            all_pieces.push_back(newPiece);
        }
    }
    boardStandardFile.close();

    printf("CONSTRUCTED %zu WHITE PIECES, %zu BLACK PIECES, %zu TOTAL PIECES\n",
           white_pieces.size(), black_pieces.size(), all_pieces.size());


    board.WriteStartPositionsToFile(all_pieces);

    /*
     * CONSTRUCT ADDITIONAL
     */

    auto teamptr = &white_pieces;
    auto oppptr = &black_pieces;
    SelectedPiece selectedPiece(teamptr, oppptr, &board);

    bool running = true;
    bool eot = false;
    while (running) {
        // Clear screen
        SDL_RenderClear(window.renderer);

        /*
         *  DRAW TO SCREEN
         */

        // Display Board and Background
        SDL_RenderCopy(window.renderer, window.background, nullptr, &window.currentRect);
        board.DisplayGameBoard();

        // Display Pieces and fetch their moves
        for (Piece* piece : all_pieces) {
            piece->FetchMoves(*teamptr, *oppptr, board);
            piece->PreventMoveIntoCheck(*teamptr, *oppptr, board);

            piece->DisplayPiece();
            piece->DisplayMoves(board);
        }

        /*
         * CHECKMATE + STALEMATE CHECKING
         * this is done after moves are fetched and confirmed
         */

        // Check if team pieces have any available moves
        bool canMove = std::any_of(teamptr->begin(), teamptr->end(), [](Piece *piece) {
            return !piece->GetAvailableMovesPtr()->empty();
        });

        // if there are no moves available, check if King is being checked by opp
        if (!canMove && std::any_of(oppptr->begin(), oppptr->end(), [](Piece *piece) {
            return piece->IsCheckingKing();
        })) {
            // conditions met: checkmate
            printf("CHECKMATE! 1:0");
            running = false;
        } else if (!canMove) {
            // conditions met: stalemate
            printf("STALEMATE! 0.5:0.5");
            running = false;
        }

        /*
         *  FETCH USER INPUT FROM EVENTS
         */

        // User input events
        SDL_Event event;
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                mouse.UpdateState(true);
            }
            if (event.type == SDL_MOUSEBUTTONUP) {
                mouse.UpdateState(false);
            }
        }

        /*
         * EVENT MANAGEMENT
         */

        // check if user has made a move
        if (selectedPiece.MadeMove(oppptr, board)) {
            eot = true;
        }

        // check if user clicks on a piece
        if (!eot) selectedPiece.CheckForClicked(teamptr);

        /*
         *  END OF TURN MANAGEMENT
         */

        // Do promotion if pawn has reached the end tile
        if (eot) {
            for (auto piece : *teamptr) {
                if (piece->ReadyToPromote()) {
                    // remove piece from board
                    piece->Captured();

                    // fetch input
                    std::string promoteInput;
                    printf("%c pawn can promote\n", piece->GetPieceInfoPtr()->gamepos.x);
                    printf("What will you promote to? Q R N B : ");
                    std::getline(std::cin, promoteInput);

                    // Create new piece on position
                    Position<char, int> position = piece->GetPieceInfoPtr()->gamepos;
                    Piece* promotedPiece;
                    if (promoteInput == "N") {
                        promotedPiece = new Knight("Knight", piece->GetPieceInfoPtr()->color, position);
                    }
                    else if (promoteInput == "B") {
                        promotedPiece = new Bishop("Bishop", piece->GetPieceInfoPtr()->color, position);
                    }
                    else if (promoteInput == "R") {
                        promotedPiece = new Rook("Rook", piece->GetPieceInfoPtr()->color, position);
                    }
                    else {
                        promotedPiece = new Queen("Queen", piece->GetPieceInfoPtr()->color, position);
                    }

                    // Set up piece texture
                    promotedPiece->CreateTextures();
                    promotedPiece->GetRectOfBoardPosition(board);

                    // add to piece lists
                    teamptr->push_back(promotedPiece);
                    all_pieces.push_back(promotedPiece);
                }
            }
        }

        if (eot) {
            // remove moves from this turn
            for (auto piece : all_pieces) {
                piece->ClearMoves();
                piece->ClearNextMoves();
                piece->UpdateCheckerVars();
            }

            selectedPiece.SwapPieceSetPointers();
            std::swap(teamptr, oppptr);

            std::string lastMove;
            selectedPiece.GetMove(lastMove);
            board.WriteMoveToFile(lastMove);
            board.IncrementTurn();

            eot = false;
        }

        /*
         *  RECREATE TEXTURES IF REQUIRED
         */

        int winSizeChanged = EnsureWindowSize(&board);
        if (winSizeChanged == 1) {
            // Update board size
            int w, h;
            SDL_GetWindowSize(window.window, &w, &h);
            board.FillToBounds(w, h);
        }
        if (winSizeChanged != 0){
            // board
            board.CreateBoardTexture();

            // white pieces
            for (Piece* piece : white_pieces) {
                piece->GetRectOfBoardPosition(board);
            }

            // black pieces
            for (Piece* piece : black_pieces) {
                piece->GetRectOfBoardPosition(board);
            }
        }


        /*
         *  UPDATE SCREEN
         */

        SDL_RenderPresent(window.renderer);
    }

    SDL_Quit();
    TTF_Quit();

    return 1;
}