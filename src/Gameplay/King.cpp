//
// Created by cew05 on 09/05/2024.
//

#include "include/King.h"

King::King(char _colID)
: Piece(_colID) {
    // update pieceinfo with pieceID
    info->name = "King";
    info->pieceID = 'K';

    canCastleQueenside = true;
    canCastleKingside = true;
}

void King::FetchMoves(const std::vector<std::unique_ptr<Piece>> &_teamPieces,
                      const std::vector<std::unique_ptr<Piece>> &_oppPieces,
                      const std::unique_ptr<Board>& _board) {
    /*
     * King moves are in the 8 tiles surrounding him
     * [_][_][_][_][_]
     * [_][a][a][a][_]
     * [_][a][K][a][_]
     * [_][a][a][a][_]
     * [_][_][_][_][_]
     * However, he cannot enter a position where he is being checked (targeted by another piece)
     */

    // only fetch moves if the moves have not been updated, and whilst piece is not captured
    if (updatedMoves || captured) return;

    // ensure valid moves is empty
    validMoves.clear();

    // fetch all moves assuming none result in king being checked
    AvailableMove move;
    std::pair<char, int> movPos = {};

    for (int dc = -1; dc < 2; dc++) {
        for (int dr = -1; dr < 2; dr++) {
            movPos = {char(info->gamepos.first + dc), info->gamepos.second + dr};
            move.SetPosition(movPos);
            move.SetTarget(nullptr);
            if (GetTeamPieceOnPosition(_teamPieces, movPos) != nullptr) continue;

            Piece* target;
            if ((target = GetOppPieceOnPosition(_oppPieces, movPos)) != nullptr) {
                move.SetTarget(target);
            }

            validMoves.push_back(move);
        }
    }

    EnforceBorderOnMoves(_board);

    /*
     * Castling:
     * must be no obstructions between king and rook
     * no spaces travelled by the king may be under check by opponent
     * moves two spaces, and move the rook to the opposite side of the King.
     */

    if (!canCastleKingside && !canCastleQueenside) return;

    // construct castling move
    AvailableMove castleKS {}, castleQS;
    castleKS.SetPosition({char(info->gamepos.first + 2), info->gamepos.second});
    castleQS.SetPosition({char(info->gamepos.first - 2), info->gamepos.second});
    bool castleKingsideNow = true;
    bool castleQueensideNow = true;

    // if king is in check, cannot castle
    if (std::any_of(_oppPieces.begin(), _oppPieces.end(), [](const std::unique_ptr<Piece>& piece){
        return piece->IsCheckingKing();
    })) {
        castleKingsideNow = false;
        castleQueensideNow = false;
    }

    // checking that no space is checked or occupied
    for (int dx = -2; dx < 3; dx++){
        for (const auto& piece: _oppPieces) {
            if (piece->IsTargetingPosition({char(info->gamepos.first + dx), info->gamepos.second})) {
                // piece is targeting one of the spaces, cannot currently castle
                (dx < 0) ? castleQueensideNow = false : castleKingsideNow = false;
            }
        }

        if (GetPieceOnPosition(_teamPieces, _oppPieces, {char(info->gamepos.first + dx), info->gamepos.second}) != nullptr) {
            // Piece is on the position, cannot castle
            (dx < 0) ? castleQueensideNow = false : castleKingsideNow = false;
        }

        // skip over dx = 0 as thats the kings position
        if (dx == -1) dx = 0;
    }

    // test the closest first rook on the Kingside for captured / moved
    if (castleKingsideNow){
        for (char dx = info->gamepos.first; dx < 'i'; dx++) {
            auto piece = GetTeamPieceOnPosition(_teamPieces, {dx, info->gamepos.second});
            if (piece == nullptr) continue;

            // test rook
            auto pi = piece->GetPieceInfoPtr();
            if (pi->pieceID == 'R') {
                if (!piece->HasMoved() && !piece->IsCaptured()) {
                    // set the rook as the target for the move and add to moves list
                    castleKS.SetTarget(piece);
                    validMoves.push_back(castleKS);
                    break;
                }
                else {
                    // The rook has moved, mark unable to castle kingside
                    canCastleKingside = false;
                }
            }
        }
    }

    // test the closest first rook on the Queenside for captured / moved
    if (castleQueensideNow) {
        for (char dx = info->gamepos.first; dx >= 'a'; dx--) {
            auto piece = GetTeamPieceOnPosition(_teamPieces, {dx, info->gamepos.second});
            if (piece == nullptr) continue;

            // test rook
            auto pi = piece->GetPieceInfoPtr();
            if (pi->pieceID == 'R') {
                if (!piece->HasMoved() && !piece->IsCaptured()) {
                    // set the rook as the target for the move and add to moves list
                    castleQS.SetTarget(piece);
                    validMoves.push_back(castleQS);
                    break;
                }
                else {
                    // The rook has moved, mark unable to castle queenside
                    canCastleQueenside = false;
                }
            }
        }
    }

    EnforceBorderOnMoves(_board);
    updatedMoves = true;
}

void King::UpdateCheckerVars() {
    if (hasMoved) {
        canCastleKingside = false;
        canCastleQueenside = false;
    }

    lastMoveDisplayTimer--;
}
