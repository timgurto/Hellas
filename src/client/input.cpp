#include <SDL_scancode.h>

#include "../Message.h"
#include "../TerrainList.h"
#include "Client.h"
#include "Particle.h"
#include "Renderer.h"
#include "Tooltip.h"
#include "ui/ContainerGrid.h"
#include "ui/TextBox.h"

extern Renderer renderer;

void Client::handleInput(double delta) {
  SDL_Event e;
  while (SDL_PollEvent(&e) != 0) {
    std::ostringstream oss;
    switch (e.type) {
      case SDL_QUIT:
        _loop = false;
        break;

      case SDL_TEXTINPUT:
        TextBox::addText(*this, e.text.text);
        break;

      case SDL_KEYDOWN:
        if (SDL_IsTextInputActive()) {
          // Text input

          switch (e.key.keysym.sym) {
            case SDLK_ESCAPE:
              if (textBoxInFocus == _chatTextBox) {
                _chatTextBox->text("");
                _chatTextBox->hide();
              }
              textBoxInFocus = nullptr;
              break;

            case SDLK_BACKSPACE:
              TextBox::backspace(*this);
              break;

            case SDLK_RETURN:
            case SDLK_KP_ENTER:
              if (textBoxInFocus == _chatTextBox) {
                const std::string &text = _chatTextBox->text();
                if (text != "") {
                  if (text.at(0) == '/') {
                    // Perform command
                    performCommand(text);
                  } else {
                    performCommand("/say " + text);
                  }
                }
                _chatTextBox->text("");
                _chatTextBox->hide();
                textBoxInFocus = nullptr;
              }
              break;
          }

        } else {
          // Regular key input

          switch (e.key.keysym.sym) {
            case SDLK_ESCAPE: {
              if (_target.exists()) {
                clearTarget();
                break;
              }

              if (_chatTextBox->visible()) {
                _chatTextBox->text("");
                _chatTextBox->hide();
                break;
              }

              Window *frontMostVisibleWindow = nullptr;
              for (Window *window : _windows)
                if (window->visible()) {
                  frontMostVisibleWindow = window;
                  break;
                }

              if (_actionLength != 0)
                sendMessage(CL_CANCEL_ACTION);
              else if (frontMostVisibleWindow != nullptr)
                frontMostVisibleWindow->hide();
              else
                _escapeWindow->show();
              break;
            }

            case SDLK_SLASH:
              if (!_chatTextBox->visible()) {
                _chatTextBox->show();
                textBoxInFocus = _chatTextBox;
                _chatTextBox->text("/");
              }
              break;

            case SDLK_RETURN:
            case SDLK_KP_ENTER:
              if (_chatTextBox->visible()) {
                // Send command/chat
                const std::string &text = _chatTextBox->text();
                if (text != "") {
                  if (text.at(0) == '/') {
                    // Perform command
                    performCommand(text);
                  } else {
                    performCommand("/say " + text);
                  }
                  _chatTextBox->text("");
                  _chatTextBox->hide();
                  textBoxInFocus = nullptr;
                }
                break;
              } else {
                _chatTextBox->show();
                textBoxInFocus = _chatTextBox;
              }
              break;

            case SDLK_DELETE:
              if (containerGridBeingDraggedFrom.item() != nullptr) {
                ContainerGrid::scrapItem(*this);
              }
              break;

            case SDLK_b:
              _buildWindow->toggleVisibility();
              removeWindow(_buildWindow);
              addWindow(_buildWindow);
              break;

            case SDLK_c:
              _craftingWindow->toggleVisibility();
              removeWindow(_craftingWindow);
              addWindow(_craftingWindow);
              break;

            case SDLK_g:
              _gearWindow->toggleVisibility();
              removeWindow(_gearWindow);
              addWindow(_gearWindow);
              break;

            case SDLK_h:
              _helpWindow.window->toggleVisibility();
              removeWindow(_helpWindow.window);
              addWindow(_helpWindow.window);
              break;

            case SDLK_i:
              _inventoryWindow->toggleVisibility();
              removeWindow(_inventoryWindow);
              addWindow(_inventoryWindow);
              break;

            case SDLK_k:
              _classWindow->toggleVisibility();
              removeWindow(_classWindow);
              addWindow(_classWindow);
              break;

            case SDLK_l:
              _chatContainer->toggleVisibility();
              break;

            case SDLK_m:
              _mapWindow->toggleVisibility();
              removeWindow(_mapWindow);
              addWindow(_mapWindow);
              break;

            case SDLK_o:
              _socialWindow->toggleVisibility();
              removeWindow(_socialWindow);
              addWindow(_socialWindow);
              break;

            case SDLK_q:
              _questLog->toggleVisibility();
              removeWindow(_questLog);
              addWindow(_questLog);
              break;

            case SDLK_r:
              if (!_lastWhisperer.empty()) {
                _chatTextBox->show();
                textBoxInFocus = _chatTextBox;
                _chatTextBox->text(std::string("/whisper ") + _lastWhisperer +
                                   " ");
              }
              break;

            case SDLK_BACKQUOTE:
            case SDLK_1:
            case SDLK_2:
            case SDLK_3:
            case SDLK_4:
            case SDLK_5:
            case SDLK_6:
            case SDLK_7:
            case SDLK_8:
            case SDLK_9:
            case SDLK_0:
            case SDLK_MINUS:
            case SDLK_EQUALS:
              onHotbarKeyDown(e.key.keysym.sym);
              break;

            case SDLK_F1:
              if (!isDebug()) break;

              {
                auto loc = toMapPoint(_mouse) - _offset;
                sendMessage({DG_TELEPORT, makeArgs(loc.x, loc.y)});
              }
              break;

            case SDLK_LALT:
            case SDLK_RALT:
              onMouseMove();  // Clear UI tooltip
              break;
          }
        }
        break;

      case SDL_KEYUP:
        if (SDL_IsTextInputActive()) {
        } else {
          switch (e.key.keysym.sym) {
            case SDLK_BACKQUOTE:
            case SDLK_1:
            case SDLK_2:
            case SDLK_3:
            case SDLK_4:
            case SDLK_5:
            case SDLK_6:
            case SDLK_7:
            case SDLK_8:
            case SDLK_9:
            case SDLK_0:
            case SDLK_MINUS:
            case SDLK_EQUALS:
              onHotbarKeyUp(e.key.keysym.sym);
              break;

            case SDLK_LALT:
            case SDLK_RALT:
              onMouseMove();  // Clear UI tooltip
              break;
          }
        }
        break;

      case SDL_MOUSEMOTION: {
        px_t x, y;
        SDL_GetMouseState(&x, &y);
        _mouse.x = toInt(x * SCREEN_X / static_cast<double>(renderer.width()));
        _mouse.y = toInt(y * SCREEN_Y / static_cast<double>(renderer.height()));

        onMouseMove();

        break;
      }

      case SDL_MOUSEBUTTONDOWN:
        switch (e.button.button) {
          case SDL_BUTTON_LEFT: {
            _leftMouseDown = true;
            textBoxInFocus = nullptr;

            auto leftMouseDownWasCaptured = false;

            // 1. Window
            for (Window *window : _windows)
              if (window->visible() && collision(_mouse, window->rect())) {
                showWindowInFront(window);
                window->onLeftMouseDown(_mouse);
                leftMouseDownWasCaptured = true;
                break;
              }
            if (leftMouseDownWasCaptured) break;

            // 2. UI element
            for (Element *element : _ui)
              if (element->visible() && element->canReceiveMouseEvents() &&
                  collision(_mouse, element->rect())) {
                element->onLeftMouseDown(_mouse);
                leftMouseDownWasCaptured = true;
                break;
              }

            if (leftMouseDownWasCaptured) break;

            // 3. Entity
            if (_currentMouseOverEntity)
              _currentMouseOverEntity->onLeftClick();
            else
              clearTarget();
            refreshTargetBuffs();

            break;
          }

          case SDL_BUTTON_RIGHT: {
            auto rightMouseDownWasCaptured = false;

            // 1. Window
            for (Window *window : _windows)
              if (window->visible() && collision(_mouse, window->rect())) {
                window->onRightMouseDown(_mouse);
                rightMouseDownWasCaptured = true;
                break;
              }
            if (rightMouseDownWasCaptured) break;

            // 2. UI element
            for (Element *element : _ui)
              if (element->visible() && element->canReceiveMouseEvents() &&
                  collision(_mouse, element->rect())) {
                element->onRightMouseDown(_mouse);
                rightMouseDownWasCaptured = true;
                break;
              }
            if (rightMouseDownWasCaptured) break;

            // 3. Entity
            if (_currentMouseOverEntity) {
              _currentMouseOverEntity->onRightClick();
              refreshTargetBuffs();
              break;
            }

            // 4. Map
            const auto targetLocation = toMapPoint(_mouse) - _offset;
            sendMessage({CL_PATHFIND_TO_LOCATION,
                         makeArgs(targetLocation.x, targetLocation.y)});
          }
        }
        break;

      case SDL_MOUSEBUTTONUP:
        if (!_loaded) break;

        switch (e.button.button) {
          case SDL_BUTTON_LEFT: {
            _leftMouseDown = false;

            // Propagate event to windows
            bool mouseUpOnWindow = false;
            for (Window *window : _windows)
              if (window->visible() && collision(_mouse, window->rect())) {
                window->onLeftMouseUp(_mouse);
                mouseUpOnWindow = true;
                break;
              }
            // Propagate event to UI elements
            for (Element *element : _ui)
              if (!mouseUpOnWindow && element->visible() &&
                  element->canReceiveMouseEvents() &&
                  collision(_mouse, element->rect())) {
                auto mouseHitAnElement = element->onLeftMouseUp(_mouse);
                if (mouseHitAnElement) {
                  mouseUpOnWindow = true;
                  break;
                }
              }

            hideTargetMenu();

            if (mouseUpOnWindow) break;

            // Use item
            auto useItem = containerGridInUse.item();
            if (useItem) {
              px_t x = toInt(_mouse.x - offset().x),
                   y = toInt(_mouse.y - offset().y);
              if (useItem->type()->constructsObject()) {
                auto messageCode = isCtrlPressed()
                                       ? CL_CONSTRUCT_FROM_ITEM_FOR_CITY
                                       : CL_CONSTRUCT_FROM_ITEM;
                sendMessage(
                    {messageCode, makeArgs(containerGridInUse.slot(), x, y)});
                prepareAction(std::string("Constructing ") +
                              _inventory[containerGridInUse.slot()]
                                  .first.type()
                                  ->constructsObject()
                                  ->name());
              }
              break;

              // Construct without item
            } else if (_selectedConstruction != nullptr) {
              px_t x = toInt(_mouse.x - offset().x),
                   y = toInt(_mouse.y - offset().y);
              auto messageCode =
                  isCtrlPressed() ? CL_CONSTRUCT_FOR_CITY : CL_CONSTRUCT;
              sendMessage(
                  {messageCode, makeArgs(_selectedConstruction->id(), x, y)});
              prepareAction(std::string("Constructing ") +
                            _selectedConstruction->name());
              break;
            }

            // Dragged item onto map -> drop.
            if (!mouseUpOnWindow &&
                containerGridBeingDraggedFrom.item() != nullptr) {
              ContainerGrid::dropItem(*this);
            }

            break;
          }

          case SDL_BUTTON_RIGHT:
            _rightMouseDown = false;
            containerGridInUse.clear();
            _selectedConstruction = nullptr;
            _constructionFootprint = Texture();
            _buildList->clearSelection();

            // Propagate event to windows
            auto eventWasCaptured = false;
            for (Window *window : _windows)
              if (window->visible() && collision(_mouse, window->rect())) {
                window->onRightMouseUp(_mouse);
                eventWasCaptured = true;
                break;
              }
            // Propagate event to UI elements
            for (Element *element : _ui)
              if (!eventWasCaptured && element->visible() &&
                  element->canReceiveMouseEvents() &&
                  collision(_mouse, element->rect())) {
                auto mouseHitAnElement = element->onRightMouseUp(_mouse);
                if (mouseHitAnElement) {
                  eventWasCaptured = true;
                  break;
                }
              }

            // Use item
            const auto *useItem = containerGridInUse.item();
            if (useItem && useItem->type()->constructsObject()) {
              _constructionFootprint =
                  useItem->type()->constructsObject()->image();
              break;
            }
            _constructionFootprint = Texture();

            if (useItem && useItem->type()->castsSpellOnUse()) {
              sendMessage({CL_CAST_SPELL_FROM_ITEM, containerGridInUse.slot()});
              containerGridInUse.clear();
              break;
            }

            break;
        }

        break;

      case SDL_MOUSEWHEEL: {
        const auto scrollingDown = e.wheel.y < 0;
        auto eventWasCaptured = false;
        for (Window *window : _windows)
          if (window->visible() && collision(_mouse, window->rect())) {
            scrollingDown ? window->onScrollDown(_mouse)
                          : window->onScrollUp(_mouse);
            eventWasCaptured = true;
            break;
          }
        for (Element *element : _ui)
          if (!eventWasCaptured && element->visible() &&
              element->canReceiveMouseEvents() &&
              collision(_mouse, element->rect())) {
            scrollingDown ? element->onScrollDown(_mouse)
                          : element->onScrollUp(_mouse);
            eventWasCaptured = true;
            break;
          }
      } break;

      case SDL_WINDOWEVENT:
        switch (e.window.event) {
          case SDL_WINDOWEVENT_SIZE_CHANGED:
          case SDL_WINDOWEVENT_RESIZED:
          case SDL_WINDOWEVENT_MAXIMIZED:
          case SDL_WINDOWEVENT_RESTORED:
            renderer.updateSize();
            for (Window *window : _windows) window->forceRefresh();
            for (Element *element : _ui) element->forceRefresh();
            Tooltip::forceAllToRedraw();
            SpriteType::forceAllShadowsToRedraw();
            ImageWithHighlight::forceAllToRedraw();
            populateClassWindow();
            redrawFogOfWar();
            break;
        }

      default:
          // Unhandled event
          ;
    }
  }
  // Poll keys (whether they are currently pressed; not key events)
  if (_loggedIn && !SDL_IsTextInputActive()) {
    bool up = _keyboardState.isPressed(SDL_SCANCODE_UP) ||
              _keyboardState.isPressed(SDL_SCANCODE_W),
         down = _keyboardState.isPressed(SDL_SCANCODE_DOWN) ||
                _keyboardState.isPressed(SDL_SCANCODE_S),
         left = _keyboardState.isPressed(SDL_SCANCODE_LEFT) ||
                _keyboardState.isPressed(SDL_SCANCODE_A),
         right = _keyboardState.isPressed(SDL_SCANCODE_RIGHT) ||
                 _keyboardState.isPressed(SDL_SCANCODE_D);
    if (up != down || left != right) {
      auto speed = _stats.speed;
      if (_character.isDriving()) speed *= _character.vehicleSpeed();

      const auto diagSpeed = speed / SQRT_2;
      const auto dist = delta * speed, diagDist = delta * diagSpeed;

      auto newLocation = _character.location();

      if (up != down) {
        if (up && !down)
          newLocation.y -= (left != right) ? diagDist : dist;
        else if (down && !up)
          newLocation.y += (left != right) ? diagDist : dist;
      }
      if (left && !right)
        newLocation.x -= (up != down) ? diagDist : dist;
      else if (right && !left)
        newLocation.x += (up != down) ? diagDist : dist;

      applyCollisionChecksToPlayerMovement(newLocation);

      // New location is now finalised.

      auto thisDirection = normaliseVector(newLocation - _character.location());
      if (thisDirection != _lastDirection) {
        sendMessage({CL_MOVE_TO, makeArgs(_character.location().x,
                                          _character.location().y)});
        _timeSinceLocUpdate = 0;
      }
      _lastDirection = thisDirection;

      _character.location(newLocation);
      _serverHasOutOfDateLocationInfo = true;

      cullObjects();

      _mapWindow->markChanged();

      _mouseMoved = true;
    }
  }
}

bool Client::isCtrlPressed() {
  const auto *keyboardState = SDL_GetKeyboardState(nullptr);
  if (keyboardState[SDL_SCANCODE_LCTRL] == SDL_PRESSED) return true;
  if (keyboardState[SDL_SCANCODE_RCTRL] == SDL_PRESSED) return true;
  return false;
}

bool Client::isAltPressed() {
  const auto *keyboardState = SDL_GetKeyboardState(nullptr);
  if (keyboardState[SDL_SCANCODE_LALT] == SDL_PRESSED) return true;
  if (keyboardState[SDL_SCANCODE_RALT] == SDL_PRESSED) return true;
  return false;
}

bool Client::isShiftPressed() {
  const auto *keyboardState = SDL_GetKeyboardState(nullptr);
  if (keyboardState[SDL_SCANCODE_LSHIFT] == SDL_PRESSED) return true;
  if (keyboardState[SDL_SCANCODE_RSHIFT] == SDL_PRESSED) return true;
  return false;
}

void Client::onMouseMove() {
  _mouseMoved = true;

  Element::resetTooltip();

  for (auto *containerGrid : _containerGrids) containerGrid->clearMouseOver();

  auto mouseUpOnWindow = false;
  for (Window *window : _windows)
    if (window->visible() && collision(_mouse, window->rect())) {
      window->onMouseMove(_mouse);
      mouseUpOnWindow = true;
      break;
    }

  if (mouseUpOnWindow) return;

  for (Element *element : _ui)
    if (element->visible() && element->canReceiveMouseEvents() &&
        collision(_mouse, element->rect())) {
      element->onMouseMove(_mouse);
      break;
    }
}

Sprite *Client::getEntityAtMouse() {
  const MapPoint mouseOffset = toMapPoint(_mouse) - _offset;
  Sprite::set_t::iterator mouseOverIt = _entities.end();
  static const px_t LOOKUP_MARGIN = 320;
  Sprite topEntity(nullptr, {0, mouseOffset.y - LOOKUP_MARGIN}, *this),
      bottomEntity(nullptr, {0, mouseOffset.y + LOOKUP_MARGIN}, *this);
  auto lowerBound = _entities.lower_bound(&topEntity),
       upperBound = _entities.upper_bound(&bottomEntity);

  for (auto flat : std::vector<bool>{true, false})  // Favour non-flat entities
    for (auto it = lowerBound; it != upperBound; ++it) {
      const auto &entity = **it;
      if (entity.type()->isFlat() != flat) continue;
      if (&entity == &_character) continue;  // Can't interact with self
      if (entity.isDecoration()) continue;

      auto *asObject = dynamic_cast<const ClientObject *>(&entity);
      if (asObject && asObject->owner().type == ClientObject::Owner::NO_ACCESS)
        continue;

      if (!entity.collidesWithMouse(mouseOffset))
        continue;  // Crude collision check
      if (!entity.mouseIsOverRealPixel(mouseOffset))
        continue;  // Detailed check
      mouseOverIt = it;
    }

  if (mouseOverIt != _entities.end())
    return *mouseOverIt;
  else
    return nullptr;
}

void Client::checkMouseOver() {
  _currentCursor = &Client::images.cursorNormal;
  if (isAltPressed()) _currentCursor = &Client::images.cursorRepair;

  _terrainTooltip = {};

  // Check whether mouse is over a window
  _mouseOverWindow = false;
  for (const Window *window : _windows)
    if (window->visible() && collision(_mouse, window->rect())) {
      _mouseOverWindow = true;
      _currentMouseOverEntity = nullptr;
      return;
    }

  // Check if mouse is over an entity
  const Sprite *const oldMouseOverEntity = _currentMouseOverEntity;
  _currentMouseOverEntity = getEntityAtMouse();

  if (_currentMouseOverEntity) {
    _currentCursor = &_currentMouseOverEntity->cursor();
    return;
  }

  updateTerrainTooltip();
}

void Client::updateTerrainTooltip() {
  _terrainUnderCursor = _map.getTerrainAtPoint(toMapPoint(_mouse) - _offset);

  auto it = gameData.terrain.find(_terrainUnderCursor);
  if (it != gameData.terrain.end()) {
    _terrainTooltip.addTags(it->second, *this);
  }

  auto allowedTerrain = TerrainList::findList(_allowedTerrain);
  if (!allowedTerrain) allowedTerrain = &TerrainList::defaultList();
  if (!allowedTerrain->allows(_terrainUnderCursor)) {
    _terrainTooltip.addGap();
    _terrainTooltip.setColor(Color::TOOLTIP_BODY);
    _terrainTooltip.addLine("Not traversible");
  }
}
