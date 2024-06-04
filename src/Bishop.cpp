//
// Created by cew05 on 09/05/2024.
//

#include "src_headers/Bishop.h"

Bishop::Bishop(const std::string &_name, const std::string &_color, Position<char, int> _gamepos)
: Piece(_name, _color,_gamepos) {
    info->pieceID = 'B';
}

void Bishop::FetchMoves(const std::vector<Piece *> &_teamPieces, const std::vector<Piece *> &_oppPieces,
                        const Board &_board) {
    /* Fetches diagonal empty squares in all 4 directions until either:
     * Exceeding board bounds
     * Reaching tile occupied by teammate
     * Reaching tile occupied by enemy piece (capturable)
     */

    // only fetch moves if the moves have not been updated, and whilst piece is not captured
    if (updatedMoves || captured) return;

    // ensure valid moves is empty
    validMoves.clear();

    // delta rows or change in rows. increments by +-1 to move in diagonals
    int maxRows, maxCols, dr = 0, dc = 0;
    Board::GetTileRowsColumns(maxRows, maxCols);

    // repeat 4 times for each of the diagonal directions (order RT -> RB -> LB -> LT)
    for (int direction = 0; direction < 4; direction++) {
        // determine row and column change using direction (directionRow, directionColumn)
        dr = (direction == 0 || direction == 3) ? 1 : -1;
        dc = (direction == 0 || direction == 1) ? 1 : -1;

        // set move to next tile in direction
        AvailableMove move;
        Position<char, int> movPos = {char(info->gamepos.x + dc), info->gamepos.y + dr};
        move.SetPosition(movPos);

        while ((0 < movPos.y && movPos.y <= maxRows) && ('a' <= movPos.x && movPos.x <= char('a' + maxCols - 1))) {
            // if the position is occupied by a teammember, do not add move, and change direction
            if (GetTeamPieceOnPosition(_teamPieces, movPos) != nullptr) {
                break;
            }

            // if the position is occupied by an enemy, mark move as a capture, add to list and change direction
            Piece* target;
            if ((target = GetOppPieceOnPosition(_oppPieces, movPos)) != nullptr) {
                move.SetTarget(target);
                validMoves.push_back(move);
                break;
            }

            // add move to valid moves list
            validMoves.push_back(move);

            // next tile in direction
            movPos = {char(movPos.x+dc), movPos.y+dr};
            move.SetPosition(movPos);
        }
    }

    EnforceBorderOnMoves();
    updatedMoves = true;
}
