//
// Created by cew05 on 09/05/2024.
//

#ifndef CHESS_WITH_SDL_ROOK_H
#define CHESS_WITH_SDL_ROOK_H

#include "Piece.h"

class Rook : public Piece{
    private:
        bool canCastle = true;

    public:
        Rook(const std::string& _name, char _colID, std::pair<char, int> _gamepos);
        void FetchMoves(const std::vector<Piece*> &_teamPieces, const std::vector<Piece*> &_oppPieces, const Board& _board) override;
        void UpdateCheckerVars() override;
};


#endif //CHESS_WITH_SDL_ROOK_H
