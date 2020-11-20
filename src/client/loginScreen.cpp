#include <picosha2.h>

#include <algorithm>
#include <thread>

#include "../Message.h"
#include "../XmlWriter.h"
#include "../curlUtil.h"
#include "../threadNaming.h"
#include "../versionUtil.h"
#include "Client.h"
#include "Particle.h"
#include "ui/Indicator.h"
#include "ui/Label.h"
#include "ui/TextBox.h"
#include "ui/Window.h"

extern Renderer renderer;
extern Args cmdLineArgs;

void Client::loginScreenLoop() {
  const double delta =
      _timeElapsed / 1000.0;  // Fraction of a second that has elapsed

  if (_connection.shouldAttemptReconnection()) {
    _connection = {*this};
    _connection.initialize();

    std::thread{[this]() {
      setThreadName("Connecting to server");
      _connection.connect();
    }}.detach();
  }

  if (_releaseNotesStatus == RELEASE_NOTES_DOWNLOADED) {
    showReleaseNotes();
    _releaseNotesStatus = RELEASE_NOTES_DISPLAYED;
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

  showQueuedErrorMessages();

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
  images.loginBackgroundBack.draw();

  // Particles
  for (auto particle : _loginParticles) particle->draw();

  // Braziers
  images.loginBackgroundFront.draw(_config.loginFrontOffset);

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

void Client::updateLoginButton() {
  loginScreenElements.loginButton->clearTooltip();
  loginScreenElements.loginButton->disable();
  loginScreenElements.nameBox->forcePascalCase();

  if (_connection.state() != Connection::CONNECTED)
    ;  // loginButton->setTooltip("Not connected to server");

  else if (!isUsernameValid(loginScreenElements.nameBox->text()))
    ;  // loginButton->setTooltip("Please enter a valid username");

  else
    loginScreenElements.loginButton->enable();
}

void Client::updateCreateButton(void *pClient) {
  const Client &client = *reinterpret_cast<const Client *>(pClient);

  client.loginScreenElements.createButton->clearTooltip();
  client.loginScreenElements.createButton->disable();
  client.loginScreenElements.newNameBox->forcePascalCase();

  if (client._connection.state() != Connection::CONNECTED)
    ;  // createButton->setTooltip("Not connected to server");

  else if (!isUsernameValid(client.loginScreenElements.newNameBox->text()))
    ;  // createButton->setTooltip("Please enter a valid username");

  else if (client.loginScreenElements.classList->getSelected().empty())
    ;  // createButton->setTooltip("Please choose a class");

  else
    client.loginScreenElements.createButton->enable();
}

void Client::updateClassDescription(Client &client) {
  updateCreateButton(&client);

  client.loginScreenElements.classDescription->clearChildren();
  auto classID = client.loginScreenElements.classList->getSelected();
  if (classID.empty()) return;

  const auto &description = client.gameData.classes[classID].description();
  auto wrapper =
      WordWrapper{client.defaultFont(),
                  client.loginScreenElements.classDescription->contentWidth()};
  auto lines = wrapper.wrap(description);

  for (const auto &line : lines)
    client.loginScreenElements.classDescription->addChild(new Label({}, line));
}

void Client::initCreateWindow() {
  const auto L_PANE_W = 120_px, R_PANE_W = 200_px, MID_PANE = 50_px,
             PANE_H = 120_px, MARGIN = 10_px, BUTTON_HEIGHT = 20,
             BUTTON_WIDTH = 100, WIN_W = L_PANE_W + R_PANE_W + 3 * MARGIN,
             WIN_H = PANE_H + 3 * MARGIN + BUTTON_HEIGHT,
             WIN_X = (SCREEN_X - WIN_W) / 2, WIN_Y = (SCREEN_Y - WIN_H) / 2,
             GAP = 2;

  _createWindow = Window::WithRectAndTitle({WIN_X, WIN_Y, WIN_W, WIN_H},
                                           "Create Account"s, mouse());
  addWindow(_createWindow);

  auto infoPane =
      new Element({L_PANE_W + 2 * MARGIN, MARGIN, R_PANE_W, PANE_H});
  _createWindow->addChild(infoPane);

  {
    auto inputPane = new Element({MARGIN, MARGIN, L_PANE_W, PANE_H});
    _createWindow->addChild(inputPane);
    auto y = 0_px;

    inputPane->addChild(new Label({0, y, 100, Element::TEXT_HEIGHT}, "Name:"s));
    loginScreenElements.newNameBox = new TextBox(
        *this, {MID_PANE, y, L_PANE_W - MID_PANE, 0}, TextBox::LETTERS);
    loginScreenElements.newNameBox->setOnChange(updateCreateButton, this);
    inputPane->addChild(loginScreenElements.newNameBox);
    infoPane->addChild(new Label({0, y, R_PANE_W, Element::TEXT_HEIGHT},
                                 "(Names must contain 3-20 characters)"s));
    y += loginScreenElements.newNameBox->height() + GAP;

    inputPane->addChild(
        new Label({0, y, 100, Element::TEXT_HEIGHT}, "Password:"s));
    loginScreenElements.newPwBox =
        new TextBox(*this, {MID_PANE, y, L_PANE_W - MID_PANE, 0});
    loginScreenElements.newPwBox->maskContents();
    inputPane->addChild(loginScreenElements.newPwBox);
    y += loginScreenElements.newPwBox->height() + GAP;

    inputPane->addChild(
        new Label({0, y, 100, Element::TEXT_HEIGHT}, "Class:"s));
    const auto CLASS_LIST_HEIGHT = 50_px;
    loginScreenElements.classList =
        new ChoiceList({MID_PANE, y, L_PANE_W - MID_PANE, CLASS_LIST_HEIGHT},
                       Element::TEXT_HEIGHT, *this);
    inputPane->addChild(loginScreenElements.classList);
    for (const auto &pair : gameData.classes) {
      auto label = new Label({}, " "s + pair.second.name());
      label->id(pair.first);
      loginScreenElements.classList->addChild(label);
    }

    loginScreenElements.classList->onSelect = updateClassDescription;
    loginScreenElements.classDescription =
        new List({0, y, R_PANE_W, PANE_H - y});
    infoPane->addChild(loginScreenElements.classDescription);
  }

  const auto BUTTON_X = (WIN_H - BUTTON_WIDTH) / 2,
             BUTTON_Y = WIN_H - BUTTON_HEIGHT - MARGIN;
  loginScreenElements.createButton =
      new Button({BUTTON_X, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT},
                 "Create account"s, [this]() { createAccount(); });
  _createWindow->addChild(loginScreenElements.createButton);
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
  auto username = loginScreenElements.newNameBox->text();
  loginScreenElements.newNameBox->text(username);

  _username = username;
  const auto &selectedClass = loginScreenElements.classList->getSelected();
  auto pwHash =
      picosha2::hash256_hex_string(loginScreenElements.newPwBox->text());
  sendMessage(
      {CL_LOGIN_NEW, makeArgs(username, pwHash, selectedClass, version())});

  saveUsernameAndPassword(_username, pwHash);
}

void Client::login() {
  _shouldAutoLogIn = false;

  auto enteredName = loginScreenElements.nameBox->text();
  if (!enteredName.empty()) {
    _username = enteredName;
  }

  auto pwHash = ""s;
  auto shouldCallCreateInsteadOfLogin = !_autoClassID.empty();
  if (shouldCallCreateInsteadOfLogin) {
    pwHash = picosha2::hash256_hex_string(loginScreenElements.newPwBox->text());
    sendMessage(
        {CL_LOGIN_NEW, makeArgs(_username, pwHash, _autoClassID, version())});
  } else {
    pwHash = _savedPwHash;
    if (pwHash.empty())
      pwHash = picosha2::hash256_hex_string(loginScreenElements.pwBox->text());
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

  loginScreenElements.nameBox = new TextBox(
      *this, {BUTTON_X, Y, BUTTON_W, Element::TEXT_HEIGHT}, TextBox::LETTERS);
  loginScreenElements.nameBox->text(_username);
  textBoxInFocus = loginScreenElements.nameBox;
  loginScreenElements.nameBox->setOnChange(
      [](void *pClient) {
        auto &client = *reinterpret_cast<Client *>(pClient);
        client.updateLoginButton();
        client.loginScreenElements.pwBox->text("");
        client._savedPwHash.clear();
      },
      this);
  _loginUI.push_back(loginScreenElements.nameBox);
  Y += loginScreenElements.nameBox->height() + GAP;

  _loginUI.push_back(
      new OutlinedLabel({BUTTON_X, Y, BUTTON_W, Element::TEXT_HEIGHT + 5},
                        "Password:", Element::CENTER_JUSTIFIED));
  Y += Element::TEXT_HEIGHT + 1;

  loginScreenElements.pwBox =
      new TextBox(*this, {BUTTON_X, Y, BUTTON_W, Element::TEXT_HEIGHT});
  loginScreenElements.pwBox->maskContents();
  if (!_savedPwHash.empty()) loginScreenElements.pwBox->text("SAVED PW");
  loginScreenElements.pwBox->setOnChange(
      [](void *pClient) {
        auto &client = *reinterpret_cast<Client *>(pClient);
        client._savedPwHash.clear();
      },
      this);
  if (loginScreenElements.nameBox->hasText())
    textBoxInFocus = loginScreenElements.pwBox;
  _loginUI.push_back(loginScreenElements.pwBox);
  Y += loginScreenElements.nameBox->height() + GAP;

  Y += Element::TEXT_HEIGHT + GAP;

  loginScreenElements.loginButton = new Button(
      {BUTTON_X, Y, BUTTON_W, BUTTON_HEIGHT}, "Login", [this]() { login(); });
  _loginUI.push_back(loginScreenElements.loginButton);
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
    label = new OutlinedLabel({LEFT_MARGIN, y, 200, Element::TEXT_HEIGHT + 5},
                              "Server: " + _connection.serverIP);
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
    const auto RELEASE_NOTES_Y = 5_px, RELEASE_NOTES_W = 175_px,
               RELEASE_NOTES_H = 235_px;
    const auto RELEASE_NOTES_RECT =
        ScreenRect{SCREEN_X - RELEASE_NOTES_W - 5, RELEASE_NOTES_Y,
                   RELEASE_NOTES_W, RELEASE_NOTES_H};
    auto releaseNotesBackground =
        new ColorBlock(RELEASE_NOTES_RECT, Color::BLACK);
    releaseNotesBackground->setAlpha(0xbf);
    releaseNotesBackground->ignoreMouseEvents();
    _loginUI.push_back(releaseNotesBackground);
    loginScreenElements.releaseNotes = new List(RELEASE_NOTES_RECT);
    loginScreenElements.releaseNotes->addChild(
        new Label({}, "Fetching release notes . . ."));
    _loginUI.push_back(loginScreenElements.releaseNotes);
#ifndef TESTING
    std::thread([this]() {
      _releaseNotesRaw =
          readFromURL("https://playhellas.com/release-notes.txt");
      _releaseNotesStatus = RELEASE_NOTES_DOWNLOADED;
    }).detach();
#endif

    _loginUI.push_back(
        new Button({SCREEN_X - BUTTON_W - GAP, SCREEN_Y - BUTTON_HEIGHT - GAP,
                    BUTTON_W, BUTTON_HEIGHT},
                   "Quit", [this]() { _loop = false; }));

    auto y = 250_px, BUTTON_X = SCREEN_X - BUTTON_W - GAP;

    auto createButton = new Button(
        {BUTTON_X, y, BUTTON_W, BUTTON_HEIGHT}, "Create new account", [this]() {
          showWindowInFront(_createWindow);
          textBoxInFocus = loginScreenElements.newNameBox;
        });
    _loginUI.push_back(createButton);
    updateCreateButton(this);

    auto discordButton =
        new Button({GAP, y, BUTTON_W, BUTTON_HEIGHT}, {},
                   []() { system("explorer \"https://discord.gg/zHfEW77\""); });
    discordButton->setTooltip("discord.gg/zHfEW77"s);
    discordButton->addChild(new Picture({2, 2, images.logoDiscord}));
    discordButton->addChild(
        new Label({26, 0, BUTTON_W - 26, BUTTON_HEIGHT}, "Join community"s,
                  Element::CENTER_JUSTIFIED, Element::CENTER_JUSTIFIED));
    _loginUI.push_back(discordButton);
    y += BUTTON_HEIGHT + 2;

    auto donateButton =
        new Button({GAP, y, BUTTON_W, BUTTON_HEIGHT}, {}, [this]() {
          auto *&window = loginScreenElements.donateWindow;
          if (!window) {
            const auto PADDING = 5_px, WIN_W = 225_px,
                       QR_X = (WIN_W - images.btcQR.width()) / 2;
            window = Window::WithRectAndTitle(
                {0, 0, WIN_W, 0}, "Donate to the developer", mouse());
            addWindow(window);
            auto y = PADDING;

            window->addChild(
                new Label({0, y, WIN_W, Element::TEXT_HEIGHT},
                          "Any donations are very welcome, and will",
                          Element::CENTER_JUSTIFIED));
            y += Element::TEXT_HEIGHT;
            window->addChild(
                new Label({0, y, WIN_W, Element::TEXT_HEIGHT},
                          "contribute to the game's development and upkeep.",
                          Element::CENTER_JUSTIFIED));
            y += Element::TEXT_HEIGHT;
            y += PADDING;

            window->addChild(new Picture({QR_X, y, images.btcQR}));
            y += images.btcQR.height() + PADDING;

            window->addChild(
                new Label({0, y, WIN_W, Element::TEXT_HEIGHT},
                          "Bitcoin: 15M23kfQ9NCTBzh1aY6vhr6XdphUibqxsC",
                          Element::CENTER_JUSTIFIED));
            y += Element::TEXT_HEIGHT + PADDING;

            window->resize(WIN_W, y);
            window->center();
          }
          showWindowInFront(window);
        });
    donateButton->addChild(new Picture({2, 2, images.logoBTC}));
    donateButton->addChild(new Label({26, 0, BUTTON_W - 26, BUTTON_HEIGHT},
                                     "Donate"s, Element::CENTER_JUSTIFIED,
                                     Element::CENTER_JUSTIFIED));
    _loginUI.push_back(donateButton);
  }

  loginScreenElements.loginErrorLabel = new OutlinedLabel({0, 0, 200, 15}, {});
  _loginUI.push_back(loginScreenElements.loginErrorLabel);

  _loginUI.push_back(_toasts);
}
void Client::showReleaseNotes() {
  /*
  **0.13.9-19 (November 19, 2020)
  *Bugs
  Item1
  Item2
  *Features
  Item1
  Item2

  **0.13.9-9 (November 18, 2020)
  etc.
  */

  auto &list = *loginScreenElements.releaseNotes;
  list.clearChildren();

  auto wordWrapper = WordWrapper{Element::font(), list.contentWidth()};

  auto iss = std::istringstream{_releaseNotesRaw};
  char buffer[1024];
  while (iss.getline(buffer, 1023)) {
    auto rawLine = std::string{buffer};
    const auto isHeading = rawLine.substr(0, 2) == "**";
    const auto isSubheading = !isHeading && rawLine.substr(0, 1) == "*";
    if (isHeading)
      rawLine = rawLine.substr(2);
    else if (isSubheading)
      rawLine = rawLine.substr(1);
    else if (rawLine != "\r")
      rawLine = "- " + rawLine;

    auto wrappedLines = wordWrapper.wrap(rawLine);
    for (const auto &line : wrappedLines) {
      auto label = new Label{{}, line};
      if (isHeading) {
        label->setColor(Color::RELEASE_NOTES_VERSION);
        label->setJustificationH(Element::CENTER_JUSTIFIED);
      } else if (isSubheading) {
        label->setColor(Color::RELEASE_NOTES_SUBHEADING);
        label->setJustificationH(Element::CENTER_JUSTIFIED);
      } else {
        label->setColor(Color::RELEASE_NOTES_BODY);
      }
      list.addChild(label);
    }
  }
}

void Client::handleLoginInput(double delta) {
  auto mouseEventWasOnWindow = false;
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
        switch (e.key.keysym.sym) {
          case SDLK_ESCAPE:
            _loop = false;
            break;

          case SDLK_BACKSPACE:
            TextBox::backspace(*this);
            break;

          case SDLK_TAB:
            if (textBoxInFocus == loginScreenElements.nameBox)
              textBoxInFocus = loginScreenElements.pwBox;
            else if (textBoxInFocus == loginScreenElements.pwBox)
              textBoxInFocus = loginScreenElements.nameBox;

            else if (textBoxInFocus == loginScreenElements.newNameBox)
              textBoxInFocus = loginScreenElements.newPwBox;
            else if (textBoxInFocus == loginScreenElements.newPwBox)
              textBoxInFocus = loginScreenElements.newNameBox;
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
        for (Element *element : _loginUI)
          if (!eventWasCaptured && element->visible() &&
              element->canReceiveMouseEvents() &&
              collision(_mouse, element->rect())) {
            scrollingDown ? element->onScrollDown(_mouse)
                          : element->onScrollUp(_mouse);
            eventWasCaptured = true;
            break;
          }
      } break;

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
