//
// Created by cew05 on 19/04/2024.
//

#include "include/Piece.h"

Piece::Piece(const std::string& _name, char _colID, std::pair<char, int> _gamepos) {
    // set piece info values
    info = new Piece_Info{_name, (char)tolower(_gamepos.first), _colID, _gamepos};
    if (_name != "Pawn") info->pieceID = _name[0];
    if (_name == "Knight") info->pieceID = 'N';

    // Load TextureID for piece
    TextureID t;
    switch (info->pieceID) {
        case 'K': t = WHITE_KING; break;
        case 'Q': t = WHITE_QUEEN; break;
        case 'R': t = WHITE_ROOK; break;
        case 'B': t = WHITE_BISHOP; break;
        case 'N': t = WHITE_KNIGHT; break;
        default: t = WHITE_PAWN; break;
    }
    info->textureID = TextureID(t + (info->colID == 'W' ? 0 : 1) + PIECE_STYLE.second);
    tm->OpenTexture(info->textureID);

    // Load texture for move display
    tm->OpenTexture(MOVE);
    tm->OpenTexture(CAPTURE);
    tm->OpenTexture(SELECTED);

    // Create rects
    rm->NewResource({0, 0, 0, 0}, RectID::PIECE_RECT);
    rm->NewResource({0, 0, 0, 0}, RectID::BOARDPOS_RECT);

    // Pawn info, direction of movement, and canPromote status
    dir = (info->colID == 'W') ? 1 : -1;
    canPromote = (info->name == "Pawn");
}

/*
 *  DISPLAY
 */

// Creating Textures

int Piece::CreateTextures() {
    /*
     * Create the textures for move highlights, the piece model
     */

    return 0;
}

void Piece::SetRects(Board *_board) {
    SDL_Rect rect;
    _board->GetBorderedRectFromPosition(rect, info->gamepos);
    rm->ChangeResource(rect, RectID::BOARDPOS_RECT);

    _board->GetBorderedRectFromPosition(rect, info->gamepos);
    rm->ChangeResource(rect, RectID::PIECE_RECT);
}

void Piece::GetRectOfBoardPosition(Board* _board) {
    SDL_Rect rect;
    _board->GetBorderedRectFromPosition(rect, info->gamepos);
    rm->ChangeResource(rect, RectID::BOARDPOS_RECT);
}

// Displaying Piece / Moves

void Piece::DisplayPiece(Board* _board) {
    // Don't display piece if captured
    if (captured) return;

    // temp vars
    SDL_Texture* texture;
    SDL_Rect rect;

    if(selected) {
        texture = tm->AccessTexture(SELECTED);
        rm->FetchResource(rect, RectID::BOARDPOS_RECT);
        SDL_RenderCopy(window.renderer, texture, nullptr, &rect);
    }

    // Show the move made by the piece if it just moved
    if (lastMoveDisplayTimer > 0) {
        // last position
        _board->GetTileRectFromPosition(rect, info->lastpos);
        SDL_SetRenderDrawColor(window.renderer, 0, 0, 0, 25);
        SDL_RenderFillRect(window.renderer, &rect);
        SDL_SetRenderDrawColor(window.renderer, 0, 0, 0, 0);

        // new position
        _board->GetTileRectFromPosition(rect, info->gamepos);
        SDL_SetRenderDrawColor(window.renderer, 0, 0, 0, 50);
        SDL_RenderFillRect(window.renderer, &rect);
        SDL_SetRenderDrawColor(window.renderer, 0, 0, 0, 0);
    }

    // Change the PIECE_RECT values over time to produce animation of movement to the new position
    if (frameTick.currTick < animStartTick + animLenTicks) {
        SDL_Rect oldPos, newPos, pieceRect;
        _board->GetBorderedRectFromPosition(oldPos, info->lastpos);
        _board->GetBorderedRectFromPosition(newPos, info->gamepos);

        double dx = double(newPos.x - oldPos.x) / animLenTicks;
        double dy = double(newPos.y - oldPos.y) / animLenTicks;

        pieceRect = oldPos;
        pieceRect.x += int(dx * int(frameTick.currTick - animStartTick));
        pieceRect.y += int(dy * int(frameTick.currTick - animStartTick));

        rm->ChangeResource(pieceRect, PIECE_RECT);
    } else {
        SetRects(_board);
    }

    rm->FetchResource(rect, RectID::PIECE_RECT);

    // Check if mouse dragging piece
    if (heldClick) {
        SDL_Rect boardRect;
        rm->FetchResource(boardRect, RectID::BOARDPOS_RECT);
        rm->FetchResource(rect, RectID::PIECE_RECT);
        rect.w = boardRect.w * 4/3;
        rect.h = boardRect.h * 4/3;
        rect.x = mouse.GetMousePosition().first - rect.w/2;
        rect.y = mouse.GetMousePosition().second - rect.h/2;
        rm->ChangeResource(rect, RectID::PIECE_RECT);
    }
    else {
        SetRects(_board);
    }

    texture = tm->AccessTexture(info->textureID);
    SDL_RenderCopy(window.renderer, texture, nullptr, &rect);
}

void Piece::DisplayMoves(Board* _board) {
    /*
     * When the piece is selected, will display the valid moves listed in the validMoves vector
     */

    if (!selected || captured) return;

    for (AvailableMove move : validMoves) {
        SDL_Rect moveRect = {0, 0};
        _board->GetBorderedRectFromPosition(moveRect, move.GetPosition());

        // if the move is a capture (except if capturing same team rook as this is a castling move) then use diff icon
        SDL_Texture* moveIcon = tm->AccessTexture(MOVE);
        if (move.GetTarget() != nullptr) {
            if (move.GetTarget()->GetPieceInfoPtr()->colID != info->colID) {
                moveIcon = tm->AccessTexture(CAPTURE);
            }
        }

        SDL_RenderCopy(window.renderer, moveIcon, nullptr, &moveRect);
    }
}

/*
 * FETCHING AND TESTING MOVES
 */

// Fetching and testing Moves

void Piece::FetchMoves(const std::vector<Piece*> &_teamPieces, const std::vector<Piece*> &_oppPieces, const Board& _board) {
    /*
     * These moves apply to the Pawn piece. This function is overwritten by other pieces. Move Summary:
     * Forward by 1 always unless blocked
     * Check for takes on side
     * Check for en passant
     * Move 2 if not yet moved
     */

    // only fetch moves if the moves have not been updated, and whilst piece is not captured
    if (updatedMoves || captured) return;

    // ensure valid moves is empty
    validMoves.clear();

    // normal moves
    // check that no pieces are blocking the next tile in direction pawn is moving
    if(GetPieceOnPosition(_teamPieces, _oppPieces, {info->gamepos.first, info->gamepos.second + dir}) == nullptr){
        validMoves.emplace_back();
        validMoves.back().SetPosition({info->gamepos.first, info->gamepos.second + dir});

        // check for moving forwards 2 spaces
        if (!hasMoved) {
            // check that no pieces are blocking the second next tile in direction pawn is moving
            if(GetPieceOnPosition(_teamPieces, _oppPieces, {info->gamepos.first, info->gamepos.second + (2 * dir)}) == nullptr){
                validMoves.emplace_back();
                validMoves.back().SetPosition({info->gamepos.first, info->gamepos.second + (2 * dir)});
            }
        }
    }

    // check for capture moves
    for (Piece* piece : _oppPieces) {
        // ignore captured pieces
        if (piece->captured) continue;

        // check left and right side
        if (piece->info->gamepos.first == info->gamepos.first - 1 || piece->info->gamepos.first == info->gamepos.first + 1){
            // check in front
            if (piece->info->gamepos.second == info->gamepos.second + dir) {
                validMoves.emplace_back();
                validMoves.back().SetPosition(piece->info->gamepos);
                validMoves.back().SetTarget(piece);
            }

            // check passant
            if (piece->info->gamepos.second == info->gamepos.second && piece->canPassant) {
                validMoves.emplace_back();
                validMoves.back().SetPosition({piece->info->gamepos.first, piece->info->gamepos.second + dir});
                validMoves.back().SetTarget(piece);
            }
        }
    }

    EnforceBorderOnMoves(_board);

    updatedMoves = true;
}

void Piece::EnforceBorderOnMoves(const Board& _board) {
    // Removes any moves from the valid moves list that go past the border.
    int rows, cols;
   _board.GetRowsColumns(rows, cols);

    if (validMoves.empty()) return;

    for (auto iter = validMoves.begin(); iter != validMoves.end();)  {
        std::pair<char, int> movPos {};
        iter->GetPosition(&movPos);

        // check if move exceeds b positions then check a positions
        if ((movPos.second < 1 || movPos.second > rows) || (movPos.first < 'a' || movPos.first > char('a' + cols - 1))){
            iter = validMoves.erase(iter);
        }
        else {
            iter++;
        }
    }
}

bool Piece::MoveLeadsToCheck(const std::vector<Piece *> &_teamPieces, const std::vector<Piece *> &_oppPieces,
                             const Board &_board, AvailableMove* _move, Piece* _movingPiece) {
    // store move data locally, as info at pointer can be changed and idk how to fix that
    AvailableMove move = *_move;

    // move piece temporarily and calculate if king is being checked after the move
    _movingPiece->TempMoveTo(&move);

    bool checking = false;
    for (auto oppPiece : _oppPieces) {
        oppPiece->ClearMoves();
        oppPiece->FetchMoves(_oppPieces, _teamPieces, _board);
        // if the oppPiece is checking the king, then the move is invalid
        if (oppPiece->IsCheckingKing()) {
            checking = true;
            break;
        }
    }

    // reset position and moves
    _movingPiece->UnMove(&move);

    for (auto oppPiece : _oppPieces) {
        oppPiece->ClearMoves();
        oppPiece->FetchMoves(_oppPieces, _teamPieces, _board);
    }

    // return checking value
    return checking;
}

void Piece::PreventMoveIntoCheck(const std::vector<Piece *> &_teamPieces, const std::vector<Piece *> &_oppPieces, const Board &_board) {
    if (updatedNextMoves) return;

    for (auto move : validMoves) {
        std::pair<char, int> movPos {};
        move.GetPosition(&movPos);
    }

    // track indexes of moves which need to be erased
    int eraseMoveIndexes[validMoves.size()];

    for (int i = 0; i < validMoves.size(); i++) {
        std::pair<char, int> movPos {};
        validMoves[i].GetPosition(&movPos);
        eraseMoveIndexes[i] = MoveLeadsToCheck(_teamPieces, _oppPieces,
                                               _board, &validMoves[i], this);
    }

    // now remove the marked moves
    int emiIter = 0;
    for (auto iter = validMoves.begin(); iter < validMoves.end();){
        if (eraseMoveIndexes[emiIter] == 1) {
            std::pair<char, int> movPos {};
            iter->GetPosition(&movPos);
            iter = validMoves.erase(iter);
        } else {
            iter++;
        }

        emiIter++;
    }


    updatedNextMoves = true;
}

// Clear Moves

void Piece::ClearMoves() {
    validMoves.clear();
    updatedMoves = false;
}

void Piece::ClearNextMoves() {
    updatedNextMoves = false;
}

/*
 * TARGETING INFO / POSITIONAL INFO FETCHING
 */

// Target / Targeting Status fetching

bool Piece::IsTargetingPosition(std::pair<char, int> _targetPosition) {
    // checks if any of the pieces moves are targeting the specified position _targetPosition

    return std::any_of(validMoves.begin(), validMoves.end(), [&](AvailableMove move){
        return (move.GetPosition().first == _targetPosition.first && move.GetPosition().second == _targetPosition.second);
    });
}

bool Piece::IsCheckingKing() {
    if (captured) return false;

    return (std::any_of(validMoves.begin(), validMoves.end(), [](AvailableMove move){
        if (move.GetTarget() == nullptr) return false;

        return (move.GetTarget()->info->pieceID == 'K');
    }));
}

// Fetching piece if on a particular position

Piece* Piece::GetTeamPieceOnPosition(const std::vector<Piece *> &_teamPieces, std::pair<char, int> _targetPos) {
    for (auto teamPiece : _teamPieces) {
        // ignore captured pieces
        if (teamPiece->captured) continue;

        // check non-captured piece
        if (teamPiece->info->gamepos.first == _targetPos.first && teamPiece->info->gamepos.second == _targetPos.second) {
            return teamPiece;
        }
    }

    return nullptr;
}

Piece* Piece::GetOppPieceOnPosition(const std::vector<Piece *> &_oppPieces, std::pair<char, int> _targetPos) {
    for (auto oppPiece : _oppPieces) {
        // ignore captured pieces
        if (oppPiece->captured) continue;

        // check non-captured piece
        if (oppPiece->info->gamepos.first == _targetPos.first && oppPiece->info->gamepos.second == _targetPos.second) {
            return oppPiece;
        }
    }

    // no capturable pieces;
    return nullptr;
}

Piece* Piece::GetPieceOnPosition(const std::vector<Piece *> &_teamPieces, const std::vector<Piece *> &_oppPieces, std::pair<char, int> _targetPos) {
    Piece* piece = GetTeamPieceOnPosition(_teamPieces, _targetPos);
    if (piece != nullptr) return piece;

    piece = GetOppPieceOnPosition(_oppPieces, _targetPos);
    return piece;
}


/*
 * MAKING A MOVE
 */

// Making / Testing a move

void Piece::MoveTo(std::pair<char, int> _movepos, Board* _board) {
    // prevent moves when captured
    if  (captured) return;

    // check if distance being moved is 2 forward, and is pawn
    canPassant = false;
    if (info->name == "Pawn" && _movepos.second == info->gamepos.second + (2 * dir)) {
        canPassant = true;
        passantTimer = 2;
    }

    // Update game position and movement values
    lastMoveDisplayTimer = 2;
    info->lastpos = info->gamepos;
    info->gamepos = _movepos;
    hasMoved = true;

    // Start animation
    animStartTick = frameTick.currTick;

    // Remake rect
    SetRects(_board);
}

void Piece::UpdateCheckerVars() {
    passantTimer -= 1;
    canPassant = passantTimer > 0;
    lastMoveDisplayTimer -= 1;
}

void Piece::Captured(bool _captured) {
    // update captured value to prevent piece from being interacted with or displayed.
    captured = _captured;

    // Close textures
}

void Piece::TempMoveTo(AvailableMove* _tempmove) {
    info->lastpos = info->gamepos;
    info->gamepos = _tempmove->GetPosition();

    //make temp capture
    if (_tempmove->GetTarget() != nullptr) {
        _tempmove->GetTarget()->captured = true;
    }
}

void Piece::UnMove(AvailableMove* _tempmove) {
    info->gamepos = info->lastpos;

    //uncapture
    if (_tempmove->GetTarget() != nullptr){
        _tempmove->GetTarget()->captured = false;
    }
}

// Promotions

bool Piece::ReadyToPromote(const Board& _board) {
    // Determine if piece at end of column
    int eoc = (info->colID == 'W') ? _board.GetRowsColumns().first : 1;
    return (canPromote && info->gamepos.second == eoc && !captured);
}

void Piece::UpdatePromoteInfo(Piece* _promotedTo) {
    promoted = true;
    promotedTo = _promotedTo;
}

Piece* Piece::GetPromotedTo() const {
    return promotedTo;
}

/*
 * SELECTING PIECES
 */

void Piece::UpdateClickedStatus(std::vector<Piece*>* _teamPieces) {
    if (captured) {
        clicked = false;
        heldClick = false;
        mouseDown = false;
        return;
    }

    SDL_Rect boardRect, pieceRect;
    rm->FetchResource(boardRect, RectID::BOARDPOS_RECT);
    rm->FetchResource(pieceRect, RectID::PIECE_RECT);

    // Assume not clicked
    clicked = false;

    // if another piece is currently clicked, ignore any attempt to click on current piece
    for (Piece* piece : *_teamPieces) {
        if (piece == this) continue;

        if (piece->IsClicked()) {
            return;
        }
    }

    // mousedown over the boardpos
    if (mouse.IsUnheldActive() && mouse.InRect(boardRect)) {
        heldClick = true;
        clicked = true;
    }

    // mouse held down dragging piece
    if (heldClick && mouse.InRect(pieceRect)) {
        clicked = true;
    }

    // mousedown released over boardpos
    if (heldClick && mouse.ClickOnRelease(boardRect)) {
        heldClick = false;
        clicked = true;
    }

    // mousedown released not over boardpos
    if (heldClick && !mouse.IsHeldActive()) {
        heldClick = false;
    }
}

void Piece::SetSelected(bool _selected) {
    // prevent selection when captured
    if  (captured) return;

    selected = _selected;
}

void Piece::UnselectPiece() {
    selected = false;
}

std::pair<bool, bool> Piece::CanCastle() {
    return {canCastleQueenside, canCastleKingside};
}