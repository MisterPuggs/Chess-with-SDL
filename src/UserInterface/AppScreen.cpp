//
// Created by cew05 on 16/06/2024.
//

#include "include/AppScreen.h"

/*
 * Generic App screen definitions
 */

AppScreen::AppScreen() {
    screenRect = window.currentRect;

    // Create base states
    stateManager->NewResource(false, SCREEN_CLOSED);
    stateManager->NewResource(false, WINDOW_CLOSED);
    stateManager->NewResource(false, NO_INPUT);

    // Create texture for background
    textureManager->NewTexture(nullptr, BACKGROUND);
}

bool AppScreen::CreateTextures() {
    // Create UserInterface background texture
    // ...

    for (auto &menu : *menuManager->AccessMap()) {
        if (!menu.second->CreateTextures()) {
            return false;
        }
    }

    for (auto &button : *buttonManager->AccessMap()) {
       if (!button.second->CreateTextures()) {
           return false;
       }
    }

    return true;
}

bool AppScreen::LoadScreen() {
    // Non functional
    // ...
    return true;
}

bool AppScreen::Display() {
    bool screenClosed = false;
    stateManager->FetchResource(screenClosed, SCREEN_CLOSED);
    if (screenClosed) return true;

    // Display background
    SDL_SetRenderDrawColor(window.renderer, 0, 150, 150, 255);
    SDL_RenderFillRect(window.renderer, &window.currentRect);
    SDL_SetRenderDrawColor(window.renderer, 0, 0, 0, 0);

    // Display menus
    for (auto &menu : *menuManager->AccessMap()) {
        if (!menu.second->Display()) return false;
    }

    // Display buttons
    for (auto &button : *buttonManager->AccessMap()) {
        if (!button.second->Display()) return false;
    }

    return true;
}


void AppScreen::ResizeScreen() {
    // Get sf
    std::pair<double, double> sf = {(double)window.currentRect.w / screenRect.w,
                                    (double)window.currentRect.h / screenRect.h};

    // Update screenrect
    screenRect.w = window.currentRect.w;
    screenRect.h = window.currentRect.h;

    // resize menus
    for (auto& menu : *menuManager->AccessMap()) {
        SDL_Rect menuRect = menu.second->FetchMenuRect();
        menu.second->UpdateSize({menuRect.w * sf.first, menuRect.h * sf.second});
    }

    // resize buttons
    // ...
}


void AppScreen::UpdateButtonStates() {
    // If ignoreInput, return
    bool ignoreInput;
    stateManager->FetchResource(ignoreInput, NO_INPUT);
    if (ignoreInput) return;

    // Update buttons in menus
    for (auto& menu : *menuManager->AccessMap()) {
        menu.second->UpdateButtonStates();
    }

    // Update other buttons
    for (auto& button : *buttonManager->AccessMap()) {
        button.second->UpdateClickedStatus();
    }
}

void AppScreen::HandleEvents() {
    SDL_Event event;

    while (SDL_PollEvent(&event) != 0) {
        // Check for window close
        if (event.type == SDL_QUIT) {
            stateManager->ChangeResource(true, WINDOW_CLOSED);
            break;
        }

        // Check other events
        switch (event.type) {
            case SDL_MOUSEBUTTONDOWN:
                mouse.MouseDown(true);
                break;
            case SDL_MOUSEBUTTONUP:
                mouse.MouseDown(false);
                break;
            default:
                break;
        }
    }

    mouse.Update();
}


void AppScreen::CheckButtons() {
    // Child classes will overwrite this function to check button states and execute functions
}

bool AppScreen::FetchScreenState(int _stateID) {
    bool result;
    if (!stateManager->FetchResource(result, _stateID)) {
        result = false;
        printf("FAILED TO FETCH ID %d CAUSE : %s\n", _stateID, GenericManager<bool>::GetIssueString(stateManager->GetIssue()).c_str());
    }
    return result;
}

void AppScreen::UpdateState(bool _state, int _stateID) {
    if (!stateManager->ResourceExists(_stateID)) {
        return;
    }

    stateManager->ChangeResource(_state, _stateID);
}