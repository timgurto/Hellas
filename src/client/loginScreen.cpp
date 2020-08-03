#include <picosha2.h>

#include <algorithm>
#include <thread>

#include "../Message.h"
#include "../XmlWriter.h"
#include "../versionUtil.h"
#include "Client.h"
#include "Particle.h"
#include "ui/Indicator.h"
#include "ui/Label.h"
#include "ui/TextBox.h"
#include "ui/Window.h"

extern Renderer renderer;
extern Args cmdLineArgs;

static TextBox *nameBox{nullptr};
static TextBox *newNameBox{nullptr};
static TextBox *pwBox{nullptr};
static TextBox *newPwBox{nullptr};
static Button *loginButton{nullptr};
static Button *createButton{nullptr};
static ChoiceList *classList{nullptr};
static List *classDescription{nullptr};
static OutlinedLabel *loginErrorLabel{nullptr};

static void showError(const std::string &message, const Color &color) {
  loginErrorLabel->changeText(message);
  loginErrorLabel->setColor(color);
}

void Client::loginScreenLoop() {
  const double delta =
      _timeElapsed / 1000.0;  // Fraction of a second that has elapsed

  if (_connection.shouldAttemptReconnection()) {
    _connection = {*this};

    std::thread{connectToServerStatic, this}.detach();
  }

  if (_shouldAutoLogIn && _connection.state() == Connection::CONNECTED) login();

  // Send ping
  if (_time - _lastPingSent > PING_FREQUENCY) {
    sendMessage({CL_PING, _time});
    _lastPingSent = _time;
  }

  // Ensure server connectivity
  if (_time - _lastPingReply > SERVER_TIMEOUT) {
    _serverConnectionIndicator->set(Indicator::FAILED);
    disconnect();
    _loggedIn = false;
  }

  _connection.getNewMessages();

  // Deal with any messages from the server
  if (!_messages.empty()) {
    handleBufferedMessages(_messages.front());
    _messages.pop();
  }

  handleLoginInput(delta);

  updateLoginParticles(delta);

  updateToasts();

  drawLoginScreen();
}

void Client::updateLoginParticles(double delta) {
  // Update existing particles
  for (auto it = _loginParticles.begin(); it != _loginParticles.end();) {
    (*it)->update(delta);
    auto nextIterator = it;
    ++nextIterator;
    if ((*it)->markedForRemoval()) _loginParticles.erase(it);
    it = nextIterator;
  }

  // Add new particles
  std::vector<const ParticleProfile *> profiles;
  profiles.push_back(findParticleProfile("loginFireLarge"));
  profiles.push_back(findParticleProfile("loginFire"));
  const MapPoint LEFT_BRAZIER(153, 281), RIGHT_BRAZIER(480, 282);

  for (const ParticleProfile *profile : profiles) {
    if (profile == nullptr) continue;
    size_t qty = profile->numParticlesContinuous(delta);
    for (size_t i = 0; i != qty; ++i) {
      Particle *p = profile->instantiate(LEFT_BRAZIER, *this);
      _loginParticles.push_back(profile->instantiate(LEFT_BRAZIER, *this));
      _loginParticles.push_back(profile->instantiate(RIGHT_BRAZIER, *this));
    }
  }
}

void Client::drawLoginScreen() const {
  renderer.setDrawColor(Color::BLACK);
  renderer.clear();

  // Background
  _loginBack.draw();

  // Particles
  for (auto particle : _loginParticles) particle->draw();

  // Braziers
  _loginFront.draw(_config.loginFrontOffset);

  // UI
  for (Element *element : _loginUI) element->draw();

  // Windows
  for (windows_t::const_reverse_iterator it = _windows.rbegin();
       it != _windows.rend(); ++it)
    (*it)->draw();

  // Tooltip
  drawTooltip();

  // Cursor
  _currentCursor->draw(_mouse);

  renderer.present();
}

void Client::connectToServerStatic(Client *client) {
  client->_connection.connect();
}

void Client::updateLoginButton() {
  loginButton->clearTooltip();
  loginButton->disable();
  nameBox->forcePascalCase();

  if (_connection.state() != Connection::CONNECTED)
    ;  // loginButton->setTooltip("Not connected to server");

  else if (!isUsernameValid(nameBox->text()))
    ;  // loginButton->setTooltip("Please enter a valid username");

  else
    loginButton->enable();
}

void Client::updateCreateButton(void *pClient) {
  createButton->clearTooltip();
  createButton->disable();
  newNameBox->forcePascalCase();

  const Client &client = *reinterpret_cast<const Client *>(pClient);

  if (client._connection.state() != Connection::CONNECTED)
    ;  // createButton->setTooltip("Not connected to server");

  else if (!isUsernameValid(newNameBox->text()))
    ;  // createButton->setTooltip("Please enter a valid username");

  else if (classList->getSelected().empty())
    ;  // createButton->setTooltip("Please choose a class");

  else
    createButton->enable();
}

void Client::updateClassDescription(Client &client) {
  updateCreateButton(&client);

  classDescription->clearChildren();
  auto classID = classList->getSelected();
  if (classID.empty()) return;

  const auto &description = Client::gameData.classes[classID].description();
  static auto wrapper =
      WordWrapper{client.defaultFont(), classDescription->contentWidth()};
  auto lines = wrapper.wrap(description);

  for (const auto &line : lines)
    classDescription->addChild(new Label({}, line));
}

void Client::initCreateWindow() {
  const auto L_PANE_W = 120_px, R_PANE_W = 200_px, MID_PANE = 50_px,
             PANE_H = 120_px, MARGIN = 10_px, BUTTON_HEIGHT = 20,
             BUTTON_WIDTH = 100, WIN_W = L_PANE_W + R_PANE_W + 3 * MARGIN,
             WIN_H = PANE_H + 3 * MARGIN + BUTTON_HEIGHT,
             WIN_X = (SCREEN_X - WIN_W) / 2, WIN_Y = (SCREEN_Y - WIN_H) / 2,
             GAP = 2;

  _createWindow =
      Window::WithRectAndTitle({WIN_X, WIN_Y, WIN_W, WIN_H}, "Create Account"s);
  addWindow(_createWindow);

  auto infoPane =
      new Element({L_PANE_W + 2 * MARGIN, MARGIN, R_PANE_W, PANE_H});
  _createWindow->addChild(infoPane);

  {
    auto inputPane = new Element({MARGIN, MARGIN, L_PANE_W, PANE_H});
    _createWindow->addChild(inputPane);
    auto y = 0_px;

    inputPane->addChild(new Label({0, y, 100, Element::TEXT_HEIGHT}, "Name:"s));
    newNameBox =
        new TextBox({MID_PANE, y, L_PANE_W - MID_PANE, 0}, TextBox::LETTERS);
    newNameBox->setOnChange(updateCreateButton, this);
    inputPane->addChild(newNameBox);
    infoPane->addChild(new Label({0, y, R_PANE_W, Element::TEXT_HEIGHT},
                                 "(Names must contain 3-20 characters)"s));
    y += newNameBox->height() + GAP;

    inputPane->addChild(
        new Label({0, y, 100, Element::TEXT_HEIGHT}, "Password:"s));
    newPwBox = new TextBox({MID_PANE, y, L_PANE_W - MID_PANE, 0});
    newPwBox->maskContents();
    inputPane->addChild(newPwBox);
    y += newPwBox->height() + GAP;

    inputPane->addChild(
        new Label({0, y, 100, Element::TEXT_HEIGHT}, "Class:"s));
    const auto CLASS_LIST_HEIGHT = 50_px;
    classList =
        new ChoiceList({MID_PANE, y, L_PANE_W - MID_PANE, CLASS_LIST_HEIGHT},
                       Element::TEXT_HEIGHT, *this);
    inputPane->addChild(classList);
    for (const auto &pair : gameData.classes) {
      auto label = new Label({}, " "s + pair.second.name());
      label->id(pair.first);
      classList->addChild(label);
    }

    classList->onSelect = updateClassDescription;
    classDescription = new List({0, y, R_PANE_W, PANE_H - y});
    infoPane->addChild(classDescription);
  }

  const auto BUTTON_X = (WIN_H - BUTTON_WIDTH) / 2,
             BUTTON_Y = WIN_H - BUTTON_HEIGHT - MARGIN;
  createButton = new Button({BUTTON_X, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT},
                            "Create account"s, [this]() { createAccount(); });
  _createWindow->addChild(createButton);
}

static void saveUsernameAndPassword(const std::string &username,
                                    const std::string &pwHash) {
  char *appDataPath = nullptr;
  _dupenv_s(&appDataPath, nullptr, "LOCALAPPDATA");
  if (!appDataPath) return;
  CreateDirectory((std::string{appDataPath} + "\\Hellas").c_str(), NULL);
  auto sessionFile = std::string{appDataPath} + "\\Hellas\\session.txt"s;
  free(appDataPath);

  auto xw = XmlWriter{sessionFile};
  auto user = xw.addChild("user");
  xw.setAttr(user, "name", username);
  xw.setAttr(user, "passwordHash", pwHash);
  xw.publish();
}

void Client::createAccount() {
  auto username = newNameBox->text();
  newNameBox->text(username);

  _username = username;
  const auto &selectedClass = classList->getSelected();
  auto pwHash = picosha2::hash256_hex_string(newPwBox->text());
  sendMessage(
      {CL_LOGIN_NEW, makeArgs(username, pwHash, selectedClass, version())});

  saveUsernameAndPassword(_username, pwHash);
}

void Client::login() {
  _shouldAutoLogIn = false;

  auto enteredName = nameBox->text();
  if (!enteredName.empty()) {
    _username = enteredName;
  }

  auto pwHash = ""s;
  auto shouldCallCreateInsteadOfLogin = !_autoClassID.empty();
  if (shouldCallCreateInsteadOfLogin) {
    pwHash = picosha2::hash256_hex_string(newPwBox->text());
    sendMessage(
        {CL_LOGIN_NEW, makeArgs(_username, pwHash, _autoClassID, version())});
  } else {
    pwHash = _savedPwHash;
    if (pwHash.empty()) pwHash = picosha2::hash256_hex_string(pwBox->text());
    sendMessage({CL_LOGIN_EXISTING, makeArgs(_username, pwHash, version())});
  }

  saveUsernameAndPassword(_username, pwHash);
}

void Client::initLoginScreen() {
  initCreateWindow();

  // UI elements
  const px_t BUTTON_W = 100, BUTTON_HEIGHT = 20, SCREEN_MID_X = 335,
             BUTTON_X = SCREEN_MID_X - BUTTON_W / 2, GAP = 20, LEFT_MARGIN = 10,
             LABEL_GAP = Element::TEXT_HEIGHT + 2;
  px_t Y = (SCREEN_Y - BUTTON_HEIGHT) / 2;

  _loginUI.push_back(
      new OutlinedLabel({BUTTON_X, Y, BUTTON_W, Element::TEXT_HEIGHT + 5},
                        "Name:", Element::CENTER_JUSTIFIED));
  Y += Element::TEXT_HEIGHT + 1;

  nameBox = new TextBox({BUTTON_X, Y, BUTTON_W, Element::TEXT_HEIGHT},
                        TextBox::LETTERS);
  nameBox->text(_username);
  TextBox::focus(nameBox);
  nameBox->setOnChange([](void *pClient) {
    auto &client = *reinterpret_cast<Client *>(pClient);
    client.updateLoginButton();
    pwBox->text("");
    client._savedPwHash.clear();
  });
  _loginUI.push_back(nameBox);
  Y += nameBox->height() + GAP;

  _loginUI.push_back(
      new OutlinedLabel({BUTTON_X, Y, BUTTON_W, Element::TEXT_HEIGHT + 5},
                        "Password:", Element::CENTER_JUSTIFIED));
  Y += Element::TEXT_HEIGHT + 1;

  pwBox = new TextBox({BUTTON_X, Y, BUTTON_W, Element::TEXT_HEIGHT});
  pwBox->maskContents();
  if (!_savedPwHash.empty()) pwBox->text("SAVED PW");
  pwBox->setOnChange([](void *pClient) {
    auto &client = *reinterpret_cast<Client *>(pClient);
    client._savedPwHash.clear();
  });
  if (nameBox->hasText()) TextBox::focus(pwBox);
  _loginUI.push_back(pwBox);
  Y += nameBox->height() + GAP;

  SDL_StartTextInput();

  Y += Element::TEXT_HEIGHT + GAP;

  loginButton = new Button({BUTTON_X, Y, BUTTON_W, BUTTON_HEIGHT}, "Login",
                           [this]() { login(); });
  _loginUI.push_back(loginButton);
  updateLoginButton();

  // Left-hand content
  {
    auto y = 318_px;

    // Server-connection indicator
    const px_t INDICATOR_LABEL_X = LEFT_MARGIN + 15, INDICATOR_Y_OFFSET = -1;
    _serverConnectionIndicator =
        new Indicator({LEFT_MARGIN, y - INDICATOR_Y_OFFSET});
    _loginUI.push_back(_serverConnectionIndicator);
    auto label =
        new OutlinedLabel({INDICATOR_LABEL_X, y, 200, Element::TEXT_HEIGHT + 5},
                          "Connection with server"s);
    _loginUI.push_back(label);
    y += LABEL_GAP;

    // Server IP
    std::string serverIP;
    if (cmdLineArgs.contains("server-ip"))
      serverIP = cmdLineArgs.getString("server-ip");
    else {
      serverIP = _defaultServerAddress;
    }
    label = new OutlinedLabel({LEFT_MARGIN, y, 200, Element::TEXT_HEIGHT + 5},
                              "Server: " + serverIP);
    _loginUI.push_back(label);
    y += LABEL_GAP;

    // Client version
    label = new OutlinedLabel({LEFT_MARGIN, y, 200, Element::TEXT_HEIGHT + 5},
                              "Client version: " + version());
    _loginUI.push_back(label);
    y += LABEL_GAP;
  }

  // Right-hand content
  {
    _loginUI.push_back(
        new Button({SCREEN_X - BUTTON_W - GAP, SCREEN_Y - BUTTON_HEIGHT - GAP,
                    BUTTON_W, BUTTON_HEIGHT},
                   "Quit", [this]() { _loop = false; }));

    auto y = 250_px, BUTTON_X = SCREEN_X - BUTTON_W - GAP;

    auto createButton = new Button({BUTTON_X, y, BUTTON_W, BUTTON_HEIGHT},
                                   "Create new account", [this]() {
                                     showWindowInFront(_createWindow);
                                     TextBox::focus(newNameBox);
                                   });
    _loginUI.push_back(createButton);
    updateCreateButton(this);
  }

  // Images
  _loginFront = Texture(std::string("Images/loginFront.png"), Color::MAGENTA);
  _loginBack = Texture(std::string("Images/loginBack.png"));

  loginErrorLabel = new OutlinedLabel({0, 0, 200, 15}, {});
  _loginUI.push_back(loginErrorLabel);

  _loginUI.push_back(_toasts);
}

void Client::handleLoginInput(double delta) {
  auto mouseEventWasOnWindow = false;
  static SDL_Event e;
  while (SDL_PollEvent(&e) != 0) {
    std::ostringstream oss;
    switch (e.type) {
      case SDL_QUIT:
        _loop = false;
        break;

      case SDL_TEXTINPUT:
        TextBox::addText(e.text.text);
        break;

      case SDL_KEYDOWN:
        switch (e.key.keysym.sym) {
          case SDLK_ESCAPE:
            _loop = false;
            break;

          case SDLK_BACKSPACE:
            TextBox::backspace();
            break;

          case SDLK_TAB:
            if (TextBox::focus() == nameBox)
              TextBox::focus(pwBox);
            else if (TextBox::focus() == pwBox)
              TextBox::focus(nameBox);

            else if (TextBox::focus() == newNameBox)
              TextBox::focus(newPwBox);
            else if (TextBox::focus() == newPwBox)
              TextBox::focus(newNameBox);
            break;

          case SDLK_RETURN:
          case SDLK_KP_ENTER:
            if (_createWindow && _createWindow->visible())
              createAccount();
            else
              login();
            break;
        }
        break;

      case SDL_MOUSEMOTION: {
        px_t x, y;
        SDL_GetMouseState(&x, &y);
        _mouse.x = toInt(x * SCREEN_X / static_cast<double>(renderer.width()));
        _mouse.y = toInt(y * SCREEN_Y / static_cast<double>(renderer.height()));

        Element::resetTooltip();
        for (Element *element : _loginUI)
          if (element->visible()) element->onMouseMove(_mouse);
        for (Window *window : _windows)
          if (window->visible()) window->onMouseMove(_mouse);

        break;
      }

      case SDL_MOUSEBUTTONDOWN:
        switch (e.button.button) {
          case SDL_BUTTON_LEFT:
            for (Window *window : _windows)
              if (window->visible() && collision(_mouse, window->rect())) {
                window->onLeftMouseDown(_mouse);
                mouseEventWasOnWindow = true;
                break;
              }
            if (mouseEventWasOnWindow) break;
            for (Element *element : _loginUI)
              if (element->visible() && collision(_mouse, element->rect())) {
                element->onLeftMouseDown(_mouse);
                mouseEventWasOnWindow = true;
              }
            break;

          case SDL_BUTTON_RIGHT:
            for (Window *window : _windows)
              if (window->visible() && collision(_mouse, window->rect())) {
                window->onRightMouseDown(_mouse);
                mouseEventWasOnWindow = true;
                break;
              }
            if (mouseEventWasOnWindow) break;
            for (Element *element : _loginUI)
              if (element->visible() && collision(_mouse, element->rect())) {
                element->onRightMouseDown(_mouse);
                mouseEventWasOnWindow = true;
              }
            break;
        }
        break;

      case SDL_MOUSEBUTTONUP:
        switch (e.button.button) {
          case SDL_BUTTON_LEFT:
            for (Window *window : _windows)
              if (window->visible() && collision(_mouse, window->rect())) {
                window->onLeftMouseUp(_mouse);
                mouseEventWasOnWindow = true;
                break;
              }
            if (mouseEventWasOnWindow) break;
            for (Element *element : _loginUI)
              if (element->visible() && collision(_mouse, element->rect())) {
                element->onLeftMouseUp(_mouse);
                mouseEventWasOnWindow = true;
                break;
              }
            break;

          case SDL_BUTTON_RIGHT:
            for (Window *window : _windows)
              if (window->visible() && collision(_mouse, window->rect())) {
                window->onRightMouseUp(_mouse);
                mouseEventWasOnWindow = true;
                break;
              }
            if (mouseEventWasOnWindow) break;
            for (Element *element : _loginUI)
              if (element->visible() && collision(_mouse, element->rect())) {
                element->onRightMouseUp(_mouse);
                break;
              }
            break;
        }
        break;

      case SDL_WINDOWEVENT:
        switch (e.window.event) {
          case SDL_WINDOWEVENT_SIZE_CHANGED:
          case SDL_WINDOWEVENT_RESIZED:
          case SDL_WINDOWEVENT_MAXIMIZED:
          case SDL_WINDOWEVENT_RESTORED:
            renderer.updateSize();
            for (Element *element : _loginUI) element->forceRefresh();
            break;
        }

      default:
          // Unhandled event
          ;
    }
  }
}
