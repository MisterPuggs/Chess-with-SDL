//
// Created by cew05 on 28/04/2024.
//

#include "include/SelectedPiece.h"

SelectedPiece::SelectedPiece() = default;

void SelectedPiece::CheckForPieceClicked(std::vector<Piece*>* _teamptr) {
    // Has the user clicked within the window boundaries
    if (!mouse.UnheldClick(window.currentRect)) return;

    if (!std::any_of(_teamptr->begin(), _teamptr->end(), [&](Piece* piece)
    {
        if (piece->CheckClicked()) {
            // piece was clicked, so update the selected piece
            ChangeSelectedPiece(piece);
            return true;
        }
        return false;
    })) {
        // no piece was clicked
        ChangeSelectedPiece(nullptr);
    }
}

void SelectedPiece::ChangeSelectedPiece(Piece *_newSelected) {
    // clear selectedPiece pointers
    if (selectedPiece != nullptr) {
        selectedPiece->UpdateSelected();
        selectedPiece = nullptr;
        selectedPieceInfo = nullptr;
    }

    // set new selectedPiece
    if ((selectedPiece = _newSelected) != nullptr) {
        selectedPiece->UpdateSelected();
        selectedPieceInfo = selectedPiece->GetPieceInfoPtr();
    }
}

bool SelectedPiece::CheckForMoveClicked(const Board* _board) {
    /*
     * Assigns selectedMove if a move rect has been clicked on by the user. Exit early if no piece has been selected
     * yet. revert selectedMove to nullptr if no move has been selected. Returns true when move made.
     */
    if (selectedPiece == nullptr) return false;

    auto movesList = *selectedPiece->GetAvailableMovesPtr();
    if (movesList.empty()) {
        return false;
    }

    selectedMove = nullptr;
    for (auto &move: *selectedPiece->GetAvailableMovesPtr()) {
        // Fetch the rect bounds of the move and fit to within the 80% dimensions
        SDL_Rect moveRect;
        _board->GetBorderedRectFromPosition(moveRect, move.GetPosition());

        // test if mouse click is inside the rect
        if (mouse.UnheldClick(moveRect)) {
            selectedMove = &move;
            return true;
        }
    }

    // no move selected
    return false;
}

void SelectedPiece::CreateACNstring(std::vector<Piece*>* _teamptr) {
    /*
     * Returns an ACN string of the selectedMove, and stores the ACN string as the lastMoveACN var.
     * First deals with disambiguation of moving piece, then if it is a capture, then the destination, then checking
     * or checkmate.
     * Special moves for promotion, castling.
     */

    /*
     * STANDARD MOVES
     */

    // Clear vars
    lastMoveACN = "";

    // Add pieceID
    if (lastMove.GetTarget() != nullptr || lastMovedInfo.name != "Pawn") {
        lastMoveACN = selectedPieceInfo->pieceID;
    }

    // Are two pieces able to move to the same pos? Differentiate by file first
    if (std::any_of(_teamptr->begin(), _teamptr->end(), [&](Piece* piece) {
        // only check same piece types
        if (piece == lastMovedPiece) return false;
        if (piece->GetPieceInfoPtr()->pieceID != lastMovedInfo.pieceID) return false;

        Position<char, int> movePos{}, testMovePos{};
        lastMove.GetPosition(&movePos);
        for (auto move: *piece->GetAvailableMovesPtr()) {
            move.GetPosition(&testMovePos);

            // if both pieces can do the same move, ensure they are on different columns
            if (testMovePos.x == movePos.x && testMovePos.y == movePos.y) {
                if (piece->GetPieceInfoPtr()->gamepos.x != lastMovedInfo.gamepos.x) {
                    return true;
                }
            }
        }

        return false;
    })) {
        // requires a column indicator
        lastMoveACN += lastMovedInfo.gamepos.x;
    }

    // Now check by rank
    if (std::any_of(_teamptr->begin(), _teamptr->end(), [&](Piece* piece){
        // only check same piece types
        if (piece == lastMovedPiece) return false;
        if (piece->GetPieceInfoPtr()->pieceID != lastMovedInfo.pieceID) return false;

        Position<char, int> movePos{}, testMovePos{};
        lastMove.GetPosition(&movePos);
        for (auto move: *piece->GetAvailableMovesPtr()) {
            move.GetPosition(&testMovePos);

            // if both pieces can do the same move, ensure they are on different rows and column is same
            if (testMovePos.x == movePos.x && testMovePos.y == movePos.y) {
                if (piece->GetPieceInfoPtr()->gamepos.y != lastMovedInfo.gamepos.y &&
                piece->GetPieceInfoPtr()->gamepos.x == lastMovedInfo.gamepos.x) {
                    return true;
                }
            }
        }

        return false;
    })) {
        // requires a row indicator
        lastMoveACN += std::to_string(lastMovedInfo.gamepos.y);
    }

    // Is it a capture?
    Piece* target = lastMove.GetTarget();
    if (target != nullptr) {
        if (target->GetPieceInfoPtr()->colID != lastMovedInfo.colID) lastMoveACN += "x";
    }

    // Now add destination
    lastMoveACN += lastMove.GetPosition().x + std::to_string(lastMove.GetPosition().y);

    /*
     * SPECIAL MOVES
     */

    // castling results in lastMoveACN being overwritten
    if (target != nullptr) {
        // assume basic King-Side castle
        if (target->GetPieceInfoPtr()->colID == lastMovedInfo.colID) {
            lastMoveACN = "O-O";
            // if Rook is to the left of King, then it is Queen-Side castle
            if (target->GetPieceInfoPtr()->gamepos.x < lastMovedInfo.gamepos.x) lastMoveACN += "-O";
        }

    }

    // Promotion
    if (lastMovedPiece->IsCaptured()) {
        auto promotedPiece = lastMovedPiece->GetPromotedTo();
        lastMoveACN += "=";
        lastMoveACN += (char)promotedPiece->GetPieceInfoPtr()->pieceID;

        // is the promotion a check?
        if (promotedPiece->IsCheckingKing()) lastMoveACN += "+";
    }

    /*
     * CHECKS (checkmate is added at the end of the game)
     */

    // is it a check
    if (lastMovedPiece->IsCheckingKing()) lastMoveACN += "+";

    // add to move list
    moveList.push_back(lastMoveACN);
}

void SelectedPiece::GetACNMoveString(std::string &_move) {
    _move = lastMoveACN;
}

void SelectedPiece::MakeMove(Board* _board) {
    // Moves the selected Piece

    // Store pieceInfo into vars before moving piece
    lastMovedPiece = selectedPiece;
    lastMovedInfo = *selectedPiece->GetPieceInfoPtr();
    if (selectedMove->GetTarget() != nullptr) lastMovedTargetInfo = *selectedMove->GetTarget()->GetPieceInfoPtr();
    lastMove = *selectedMove;

    // if pawn, update id to match current file
    if (selectedPiece->GetPieceInfoPtr()->name == "Pawn") {
        selectedPiece->GetPieceInfoPtr()->pieceID = selectedPiece->GetPieceInfoPtr()->gamepos.x;
    }

    // move selected piece
    selectedPiece->MoveTo(selectedMove->GetPosition(), _board);
    selectedPiece->ClearMoves();

    // Castling moves and Capturing moves both have a target piece to handle
    Piece* target;
    if ((target = selectedMove->GetTarget()) != nullptr) {
        // if the move target is on the same team, then its a castling move
        if (target->GetPieceInfoPtr()->colID == selectedPiece->GetPieceInfoPtr()->colID) {
            auto pi = selectedPiece->GetPieceInfoPtr();

            // rook must now swap to the opposite side of the king
            int dx = (target->GetPieceInfoPtr()->gamepos.x < pi->gamepos.x) ? 1 : -1;
            target->MoveTo({char(pi->gamepos.x + dx), pi->gamepos.y}, _board);
        } else {
            //move is to capture a target, mark target as captured
            selectedMove->GetTarget()->Captured(true);
        }

    }

    // unselect selected piece and set to lastmoved Piece
    selectedPiece->UnselectPiece();
    selectedPiece = nullptr;
}
