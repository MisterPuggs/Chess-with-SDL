//
// Created by cew05 on 19/06/2024.
//

#include "include/Menu.h"

Menu::Menu(std::pair<int, int> _position, std::pair<int, int> _size, const std::string &_title) {
    menuRect = {_position.first, _position.second, _size.first, _size.second};
    menuTitle = _title;

    // Add in close closeButton (this is external to buttonManager)
    closeButton = new Button({menuRect.x + menuRect.w - 40, menuRect.y + 10},
                                  {30, 30}, ICON_CLOSE);
    closeButton->LockRatio(true);
    closeButton->LockSize(true);

    // Create background texture
    textureManager->NewTexture(nullptr, BACKGROUND);
}

bool Menu::CreateTextures() {
    // Open textures
    tm->OpenTexture(MENU_SHEET);
    TTF_Font* font = TTF_OpenFont("../Resources/Fonts/CF/TCFR.ttf", 100);

    // Open texture sheet
    SDL_Texture* menuSheet = tm->AccessTexture(MENU_SHEET);
    std::pair<int, int> menuGrid = tm->AccessInfo(MENU_SHEET)->grid;
    SDL_Rect menuSheetTile;

    // Get size of tile in menuSheet
    int w, h;
    SDL_QueryTexture(menuSheet, nullptr, nullptr, &w, &h);
    menuSheetTile = {0, 0, w / menuGrid.first, h / menuGrid.second};

    // Create texture to draw to
    SDL_Texture* background = SDL_CreateTexture(window.renderer,
                                                SDL_PIXELFORMAT_RGBA8888,
                                                SDL_TEXTUREACCESS_TARGET,
                                                menuRect.w, menuRect.h);

    // Set renderTarget
    if (SDL_SetRenderTarget(window.renderer, background) != 0) {
        LogError("Failed to set render target", SDL_GetError());
        return false;
    }

    // Set blending modes
    SDL_SetRenderDrawBlendMode(window.renderer, SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(background, SDL_BLENDMODE_BLEND);

    // Determine the scale factor
    double sf = (menuRect.w > menuRect.h) ? menuRect.h/2 / menuSheetTile.h : menuRect.w/2 / menuSheetTile.w;
    if (sf > 1) sf = 1;

    // Draw top layer
    SDL_Rect destRect = {0, 0, int(menuSheetTile.w * sf), int(menuSheetTile.h * sf)};
    for (int r = 0; r < 3; r++) {
        // Fetch tile and return to left side
        menuSheetTile.y = r * menuSheetTile.h;
        menuSheetTile.x = 0;
        destRect.x = 0;

        // Draw TL, Middle, TR
        if (r == 1 && menuRect.h > 2*destRect.h) destRect.h = menuRect.h - 2*destRect.h;
        else destRect.h = int(menuSheetTile.h * sf);
        SDL_RenderCopy(window.renderer, menuSheet, &menuSheetTile, &destRect);

        if (menuRect.w > 2*destRect.w) {
            menuSheetTile.x += menuSheetTile.w;
            destRect.x += destRect.w;
            destRect.w = menuRect.w - 2 * destRect.w;
            SDL_RenderCopy(window.renderer, menuSheet, &menuSheetTile, &destRect);
        }

        destRect.w = int(menuSheetTile.w * sf);
        destRect.x = menuRect.w - destRect.w;
        menuSheetTile.x = 2*menuSheetTile.w;
        SDL_RenderCopy(window.renderer, menuSheet, &menuSheetTile, &destRect);

        destRect.y += destRect.h;
    }

    // If the title is not empty, draw the title
    if (!menuTitle.empty()) {
        SDL_Surface *textSurface = TTF_RenderText_Blended(font, menuTitle.c_str(), {255, 255, 255, 255});
        SDL_Texture *textTexture = SDL_CreateTextureFromSurface(window.renderer, textSurface);

        // Resize texture
        int labelW, labelH;
        SDL_QueryTexture(textTexture, nullptr, nullptr, &labelW, &labelH);
        sf = (double) labelH / (menuRect.h / 14.0);
        labelH = int(menuRect.h / 14.0);
        labelW = int((double) labelW / sf);

        // Centralise label in button
        destRect = {menuRect.w / 2 - labelW / 2, int(labelH/10), labelW, labelH};
        SDL_RenderCopy(window.renderer, textTexture, nullptr, &destRect);
    }

    // reset renderTarget
    SDL_SetRenderTarget(window.renderer, nullptr);

    // Update the texture
    if (!textureManager->UpdateTexture(background, BACKGROUND)) {
        LogError(TextureManager::GetIssueString(textureManager->GetIssue()), "");
    }

    // Create the button textures
    for (auto &button : *buttonManager->AccessMap()) {
        button.second->CreateTextures();
    }
    closeButton->CreateTextures();

    // Close textures
    tm->CloseTexture(MENU_SHEET);
    TTF_CloseFont(font);

    return true;
}

bool Menu::LoadMenu(bool _load) {
    // No functionality
    //...

    return true;
}

bool Menu::Display() {
    if (!active) return true;

    // Display background
    if (showBackground) {
        SDL_Texture* background = textureManager->AccessTexture(BACKGROUND);
        if (textureManager->GetIssue() != TextureManager::Issue::NO_ISSUE) {
            LogError(TextureManager::GetIssueString(textureManager->GetIssue()), "");
            return false;
        }
        if (SDL_RenderCopy(window.renderer, background, nullptr, &menuRect) != 0) {
            LogError("Failed to draw menu background", SDL_GetError());
            return false;
        }
    }

    // Show close button
    if (canClose) closeButton->Display();


    // Display all other buttons in button manager
    for (auto& button : *buttonManager->AccessMap()) {
        if (!button.second->Display()) {
            return false;
        }
    }

    return true;
}

void Menu::CanClose(bool _closable) {
    canClose = _closable;
}

void Menu::LockSize(bool _lock) {
    lockSize = _lock;
}

void Menu::LockRatio(bool _lock) {
    lockRatio = _lock;
}

SDL_Rect Menu::FetchMenuRect() {
    return menuRect;
}

void Menu::UpdatePosition(std::pair<int, int> _position) {
    menuRect.x = _position.first;
    menuRect.y = _position.second;
}

void Menu::UpdateSize(std::pair<int, int> _size) {
    if (lockSize) return;

    std::pair<double, double> sf = {(double)_size.first / menuRect.w,
                                    (double)_size.second / menuRect.h};

    // If ratio locked, use height sf for width;
    if (lockRatio) {
        sf.first = sf.second;
    }

    // Update size
    menuRect.w = int((double)menuRect.w * sf.first);
    menuRect.h = int((double)menuRect.h * sf.second);

    // Update buttons
    for (auto& button : *buttonManager->AccessMap()) {
        SDL_Rect buttonRect = button.second->FetchButtonRect();
        button.second->UpdateSize({int(buttonRect.w * sf.first), int(buttonRect.h * sf.second)});
        button.second->UpdatePosition({int(buttonRect.x * sf.first), int(buttonRect.y * sf.second)});
    }
}


GenericManager<Button*>* Menu::AccessButtonManager() {
    return buttonManager;
}

void Menu::UpdateButtonStates() {
    for (auto& button : *buttonManager->AccessMap()) {
        button.second->UpdateClickedStatus();
    }
}

void Menu::CheckButtons() {
    Button* button;

    // Check close button
    if (canClose) {
        if (closeButton->IsClicked()) CloseMenu();
    }

    // Check other buttons done in overrides of this function
    // ...
}

void Menu::CloseMenu() {
    if (!canClose) return;

    active = false;
}



