//
// Created by cew05 on 09/05/2024.
//

#include "src_headers/Rook.h"

Rook::Rook(const std::string &_name, const std::string &_color, Position<char, int> _gamepos)
: Piece(_name, _color,_gamepos) {
    info->pieceID = 'R';
}

void Rook::FetchMoves(const std::vector<Piece *> &_teamPieces, const std::vector<Piece *> &_oppPieces,
                        const Board &_board) {
    /* Fetches vertical/horizontal empty squares in all 4 directions until either:
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

    // repeat 4 times for each of the diagonal directions (order T -> R -> B -> L)
    for (int direction = 0; direction < 4; direction++) {
        // flip direction to negative value on B, L directions
        int dir = (direction > 1) ? -1 : 1;

        // determine row and column change using direction (directionRow, directionColumn) and dir value
        dr = (direction == 0 || direction == 2) ? dir : 0;
        dc = (direction == 1 || direction == 3) ? dir : 0;

        // set move to next tile in direction
        AvailableMove move = {char(info->gamepos.x + dc), info->gamepos.y + dr};

        while ((0 < move.position.y && move.position.y <= maxRows) && ('a' <= move.position.x && move.position.x <= char('a' + maxCols - 1))) {

            // if the position is occupied by a teammember, do not add move, and change direction
            if (PositionOccupied(_teamPieces, _oppPieces, move.position) == -1) {
                break;
            }

            // if the position is occupied by an enemy, mark move as a capture, add to list and change direction
            Piece* target;
            if ((target = GetOpponentOnPosition(_oppPieces, move.position)) != nullptr) {
                move.capture = true;
                move.target = target;
                validMoves.push_back(move);
                break;
            }

            // add move to valid moves list
            validMoves.push_back(move);

            // next tile in direction
            move.position = {char(move.position.x+dc), move.position.y+dr};
        }
    }

    EnforceBorderOnMoves();

    updatedMoves = true;
}