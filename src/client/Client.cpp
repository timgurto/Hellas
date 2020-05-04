#include "Client.h"

#include <SDL.h>
#include <SDL_mixer.h>

#include <algorithm>
#include <cassert>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "../XmlReader.h"
#include "../curlUtil.h"
#include "../messageCodes.h"
#include "../server/User.h"
#include "../util.h"
#include "../versionUtil.h"
#include "CDataLoader.h"
#include "ClientCombatant.h"
#include "ClientNPC.h"
#include "LogSDL.h"
#include "Particle.h"
#include "SpriteType.h"
#include "Tooltip.h"
#include "Unlocks.h"
#include "ui/Button.h"
#include "ui/CombatantPanel.h"
#include "ui/ConfirmationWindow.h"
#include "ui/ContainerGrid.h"
#include "ui/Element.h"
#include "ui/LinkedLabel.h"
#include "ui/ProgressBar.h"

extern Args cmdLineArgs;

Client *Client::_instance = nullptr;

LogSDL *Client::_debugInstance = nullptr;

const px_t Client::SCREEN_X = 640;
const px_t Client::SCREEN_Y = 360;

const ms_t Client::MAX_TICK_LENGTH = 100;
const ms_t Client::SERVER_TIMEOUT = 10000;
const ms_t Client::PING_FREQUENCY = 5000;

const px_t Client::ICON_SIZE = 16;
const px_t Client::HEADING_HEIGHT = 14;
const px_t Client::LINE_GAP = 6;

const double Client::MOVEMENT_SPEED = 80;
const Hitpoints Client::MAX_PLAYER_HEALTH = 50;

const px_t Client::ACTION_DISTANCE = Podes{4}.toPixels();

const px_t Client::CULL_DISTANCE = 450;  // Just copy Server::CULL_DISTANCE
const px_t Client::CULL_HYSTERESIS_DISTANCE = 50;

const size_t Client::INVENTORY_SIZE = 10;
const size_t Client::GEAR_SLOTS = 8;
std::vector<std::string> Client::GEAR_SLOT_NAMES;

Color Client::SAY_COLOR;
Color Client::WHISPER_COLOR;

bool Client::isClient = false;

std::map<std::string, int> Client::_messageCommands;
std::map<int, std::string> Client::_errorMessages;

const int Client::MIXING_CHANNELS = 32;

Client::Client()
    : _cursorNormal("Images/Cursors/normal.png"s, Color::MAGENTA),
      _cursorGather("Images/Cursors/gather.png"s, Color::MAGENTA),
      _cursorContainer("Images/Cursors/container.png"s, Color::MAGENTA),
      _cursorAttack("Images/Cursors/attack.png"s, Color::MAGENTA),
      _cursorStartsQuest("Images/Cursors/startsQuest.png"s, Color::MAGENTA),
      _cursorEndsQuest("Images/Cursors/endsQuest.png"s, Color::MAGENTA),
      _cursorRepair("Images/Cursors/repair.png"s, Color::MAGENTA),
      _cursorVehicle("Images/Cursors/vehicle.png"s, Color::MAGENTA),
      _currentCursor(&_cursorNormal),

      _character({}, {}),

      _activeRecipe(nullptr),

      _selectedConstruction(nullptr),
      _multiBuild(false),

      _actionTimer(0),
      _actionLength(0),

      _connection(*this),

      _loop(true),
      _running(false),
      _freeze(false),
      _dataLoaded(false),

      _defaultFont(nullptr),

      _mouse(0, 0),
      _mouseMoved(false),
      _mouseOverWindow(false),
      _leftMouseDown(false),
      _leftMouseDownEntity(nullptr),
      _rightMouseDown(false),
      _rightMouseDownEntity(nullptr),
      _rightMouseDownWasOnUI(false),

      _basePassive(std::string("Images/targetPassive.png"), Color::MAGENTA),
      _baseAggressive(std::string("Images/targetAggressive.png"),
                      Color::MAGENTA),
      _inventory(INVENTORY_SIZE, std::make_pair(ClientItem::Instance{}, 0)),

      _time(SDL_GetTicks()),
      _timeElapsed(0),
      _lastPingReply(_time),
      _lastPingSent(_time),
      _latency(0),
      _fps(0),

      _loggedIn(false),
      _loaded(false),

      _timeSinceLocUpdate(0),

      _tooltipNeedsRefresh(false),

      _currentMouseOverEntity(nullptr),

      _confirmDropItem(nullptr),

      _drawingFinished(false),

      _channelsPlaying(0),

      _numEntities(0),

      _debug("client.log") {
  _defaultFont = TTF_OpenFont("AdvoCut.ttf", 10);
  drawLoadingScreen("Reading configuration file", 0.1);

  isClient = true;
  _instance = this;
  _debugInstance = &_debug;

  _config.loadFromFile("client-config.xml");
  _connection.initialize(_config.serverHostDirectory);

  _shadowImage = {"Images/shadow.png"};

  initUI();
  _wordWrapper = WordWrapper{_defaultFont, _chatLog->contentWidth()};

  initializeMessageNames();

  SDL_ShowCursor(SDL_DISABLE);

#ifdef _DEBUG
  _debug(toString(cmdLineArgs));
  if (Socket::debug == nullptr) Socket::debug = &_debug;
#endif

  if (cmdLineArgs.contains("auto-login")) _shouldAutoLogIn = true;

  drawLoadingScreen("Initializing audio", 0.5);
  int ret = (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 1, 512) < 0);
  if (ret < 0) {
    showErrorMessage("SDL_mixer failed to initialize.", Color::CHAT_ERROR);
  }
  Mix_AllocateChannels(MIXING_CHANNELS);

  _entities.insert(&_character);

  Unlocks::linkToKnownRecipes(_knownRecipes);
  Unlocks::linkToKnownConstructions(_knownConstructions);

  initializeUsername();

  SDL_StopTextInput();

  drawLoadingScreen("", 1);
}

void Client::initialiseData() {
  // Tell Avatars to use blood particles
  Avatar::_combatantType.damageParticles(findParticleProfile("blood"));

  // Match up ranged weapons with their ammo items
  for (auto &pair : _items) {
    auto &item = const_cast<ClientItem &>(pair.second);
    item.fetchAmmoItem();
  }

  // Initialize object-type strengths
  for (auto *objectType : _objectTypes) {
    auto nonConstType = const_cast<ClientObjectType *>(objectType);
    nonConstType->calculateAndInitDurability();
  }

  populateBuildList();

  // Tell quest nodes about their quests
  for (auto &questPair : _quests) {
    auto &quest = questPair.second;

    auto startNode = findObjectType(quest.info().startsAt);
    if (startNode) startNode->startsQuest(quest);

    auto endNode = findObjectType(quest.info().endsAt);
    if (endNode) endNode->endsQuest(quest);
  }

  Avatar::_spriteType.useCustomShadowWidth(16);
  Avatar::_spriteType.useCustomDrawHeight(40);
}

void Client::initializeGearSlotNames() {
  if (!GEAR_SLOT_NAMES.empty()) return;
  GEAR_SLOT_NAMES.push_back("Head");
  GEAR_SLOT_NAMES.push_back("Jewelry");
  GEAR_SLOT_NAMES.push_back("Body");
  GEAR_SLOT_NAMES.push_back("Shoulders");
  GEAR_SLOT_NAMES.push_back("Hands");
  GEAR_SLOT_NAMES.push_back("Feet");
  GEAR_SLOT_NAMES.push_back("Weapon");
  GEAR_SLOT_NAMES.push_back("Offhand");
}

Client::~Client() {
  SDL_ShowCursor(SDL_ENABLE);
  ContainerGrid::cleanup();
  Element::cleanup();
  if (_defaultFont != nullptr) TTF_CloseFont(_defaultFont);
  for (const Sprite *entityConst : _entities) {
    Sprite *entity = const_cast<Sprite *>(entityConst);
    if (entity == &_character) continue;
    delete entity;
  }

  for (auto type : _objectTypes) delete type;
  for (auto profile : _particleProfiles) delete profile;
  for (auto projectileType : _projectileTypes) delete projectileType;
  for (const auto &spellPair : _spells) delete spellPair.second;

  // Some entities will destroy their own windows, and remove them from this
  // list.
  for (Window *window : _windows) delete window;

  cleanupSocialWindow();

  auto uniqueUIElements = std::set<Element *>{};
  for (Element *mainUIElement : _ui) uniqueUIElements.insert(mainUIElement);
  for (Element *loginElement : _loginUI) uniqueUIElements.insert(loginElement);
  for (Element *element : uniqueUIElements) delete element;

  Mix_Quit();

  _instance = nullptr;

  Socket::debug = nullptr;
}

void Client::run() {
  _running = true;
  if (!_dataLoaded) {
    drawLoadingScreen("Loading data", 0.6);
    bool shouldLoadDefaultData = true;
    if (cmdLineArgs.contains("load-test-data-first")) {
      CDataLoader::FromPath(*this, "testing/data/minimal").load();
      shouldLoadDefaultData = false;
    }
    if (cmdLineArgs.contains("data")) {
      CDataLoader::FromPath(*this, cmdLineArgs.getString("data")).load();
      shouldLoadDefaultData = false;
    }
    if (shouldLoadDefaultData) CDataLoader::FromPath(*this).load();
  }
  initialiseData();

  drawLoadingScreen("Initializing login screen", 0.9);
  initLoginScreen();
  _connection.state(Connection::TRYING_TO_CONNECT);

  ms_t timeAtLastTick = SDL_GetTicks();
  while (_loop) {
    _time = SDL_GetTicks();

    _timeElapsed = _time - timeAtLastTick;
    if (_timeElapsed > MAX_TICK_LENGTH) _timeElapsed = MAX_TICK_LENGTH;
    timeAtLastTick = _time;
    _fps = toInt(1000.0 / _timeElapsed);

    if (_loggedIn)
      gameLoop();
    else
      loginScreenLoop();

    while (_freeze)
      ;
  }
  _running = false;
  _connection.state(Connection::FINISHED);
}

void Client::exitGame(void *pClient) {
  auto &client = *reinterpret_cast<Client *>(pClient);
  client._loop = false;
}

void Client::gameLoop() {
  const double delta =
      _timeElapsed / 1000.0;  // Fraction of a second that has elapsed

  // Send ping
  if (_time - _lastPingSent > PING_FREQUENCY) {
    sendMessage({CL_PING, makeArgs(_time)});
    _lastPingSent = _time;
  }

  // Ensure server connectivity
  if (_time - _lastPingReply > SERVER_TIMEOUT) {
    disconnect();
    infoWindow("Disconnected from server.");
    _serverConnectionIndicator->set(Indicator::FAILED);
    _loggedIn = false;
  }

  // Update server with current location
  auto shouldTellServerAboutLocation = _serverHasOutOfDateLocationInfo;
  if (!shouldTellServerAboutLocation)
    _timeSinceLocUpdate = 0;
  else {
    _timeSinceLocUpdate += _timeElapsed;
    if (_timeSinceLocUpdate > TIME_BETWEEN_LOCATION_UPDATES) {
      sendMessage({CL_LOCATION,
                   makeArgs(_character.location().x, _character.location().y)});
      _tooltipNeedsRefresh = true;
      _timeSinceLocUpdate = 0;
      _serverHasOutOfDateLocationInfo = false;
    }
  }

  // Deal with any messages from the server
  if (!_messages.empty()) {
    handleBufferedMessages(_messages.front());
    _messages.pop();
  }

  handleInput(delta);

  // Update entities
  std::vector<Sprite *> entitiesToReorder;
  for (Sprite::set_t::iterator it = _entities.begin(); it != _entities.end();) {
    Sprite::set_t::iterator next = it;
    ++next;
    Sprite *const toUpdate = *it;
    if (toUpdate->markedForRemoval()) {
      _entities.erase(it);
      it = next;
      continue;
    }
    toUpdate->update(delta);
    if (toUpdate->yChanged()) {
      // Sprite has moved up or down, and must be re-ordered in set.
      entitiesToReorder.push_back(toUpdate);
      _entities.erase(it);
      toUpdate->yChanged(false);
    }
    it = next;
  }
  for (Sprite *entity : entitiesToReorder) _entities.insert(entity);
  entitiesToReorder.clear();

  updateOffset();

  // Update cast bar
  if (_actionLength > 0) {
    _actionTimer = min(_actionTimer + _timeElapsed, _actionLength);
    _castBar->show();
  }

  _channelsPlaying = Mix_Playing(-1);
  _numEntities = _entities.size();

  // Update terrain animation
  for (auto &terrainPair : _terrain)
    terrainPair.second.advanceTime(_timeElapsed);

  // Update spell cooldowns
  auto aSpellHasCooledDown{false}, aCooldownHasTicked{false};
  for (auto &pair : _spellCooldowns) {
    if (pair.second == 0)
      continue;

    else if (pair.second < _timeElapsed) {
      pair.second = 0;
      aSpellHasCooledDown = true;

    } else {
      pair.second -= _timeElapsed;
      aCooldownHasTicked = true;
    }
  }
  if (aSpellHasCooledDown)
    refreshHotbar();
  else if (aCooldownHasTicked)
    refreshHotbarCooldowns();

  // Update buff times
  for (auto &pair : _buffTimeRemaining)
    pair.second = _timeElapsed > pair.second ? 0 : pair.second - _timeElapsed;
  for (auto &pair : _debuffTimeRemaining)
    pair.second = _timeElapsed > pair.second ? 0 : pair.second - _timeElapsed;

  // Update quest time limits
  for (auto &pair : _quests) {
    pair.second.update(_timeElapsed);
  }

  if (_mouseMoved) checkMouseOver();

  updateUI();

  _connection.getNewMessages();
  // Draw
  draw();
  SDL_Delay(5);
}

void Client::startCrafting() {
  if (_instance->_activeRecipe != nullptr) {
    _instance->sendMessage({CL_CRAFT, _instance->_activeRecipe->id()});
    const ClientItem *product =
        toClientItem(_instance->_activeRecipe->product());
    _instance->prepareAction("Crafting " + product->name());
  }
}

bool Client::playerHasItem(const Item *item, size_t quantity) const {
  for (size_t i = 0; i != INVENTORY_SIZE; ++i) {
    const auto &slot = _inventory[i];
    if (slot.first.type() == item) {
      if (slot.second >= quantity)
        return true;
      else
        quantity -= slot.second;
    }
  }
  return false;
}

void Client::removeEntity(Sprite *const toRemove) {
  const Sprite::set_t::iterator it = _entities.find(toRemove);
  if (it == _entities.end()) return;

  _entities.erase(it);
  delete toRemove;
}

void Client::cullObjects() {
  closeWindowsFromOutOfRangeObjects();

  std::list<std::pair<Serial, Sprite *> > objectsToRemove;
  for (auto pair : _objects) {
    if (pair.second->canAlwaysSee()) continue;
    if (outsideCullRange(pair.second->location(), CULL_HYSTERESIS_DISTANCE))
      objectsToRemove.push_back(pair);
  }
  for (auto pair : objectsToRemove) {
    if (pair.second == _currentMouseOverEntity)
      _currentMouseOverEntity = nullptr;
    if (pair.second == targetAsEntity()) clearTarget();
    removeEntity(pair.second);
    _objects.erase(_objects.find(pair.first));
  }

  std::list<Avatar *> usersToRemove;
  for (auto pair : _otherUsers)
    if (outsideCullRange(pair.second->location(), CULL_HYSTERESIS_DISTANCE)) {
      _debug("Removing other user");
      usersToRemove.push_back(pair.second);
    }
  for (Avatar *avatar : usersToRemove) {
    if (_currentMouseOverEntity == avatar) _currentMouseOverEntity = nullptr;
    _otherUsers.erase(_otherUsers.find(avatar->name()));
    removeEntity(avatar);
  }
}

TTF_Font *Client::defaultFont() const { return _defaultFont; }

void Client::updateOffset() {
  _offset = {SCREEN_X / 2 - _character.location().x,
             SCREEN_Y / 2 - _character.location().y};
  _intOffset = {toInt(_offset.x), toInt(_offset.y)};
}

void Client::drawGearParticles(const ClientItem::vect_t &gear,
                               const MapPoint &location, double delta) {
  for (const auto &pair : gear) {
    const auto item = pair.first;
    if (item.type() == nullptr) continue;
    for (const auto &particles : item.type()->particles()) {
      auto gearOffsetScreen = ClientItem::gearOffset(item.type()->gearSlot());
      auto gearOffset = MapPoint{static_cast<double>(gearOffsetScreen.x),
                                 static_cast<double>(gearOffsetScreen.y)};

      auto particleX = location.x + gearOffset.x + particles.offset.x;
      auto particleY = location.y;
      auto altitude = -gearOffset.y - particles.offset.y;
      addParticlesWithCustomAltitude(altitude, particles.profile,
                                     {particleX, particleY}, delta);
    }
  }
}

void Client::prepareAction(const std::string &msg) { _actionMessage = msg; }

void Client::startAction(ms_t actionLength) {
  _actionTimer = 0;
  _actionLength = actionLength;
  if (actionLength == 0) _castBar->hide();
}

void Client::addWindow(Window *window) {
  assert(window != nullptr);
  assert(!window->title().empty());
  _windows.push_front(window);
}

void Client::removeWindow(Window *window) { _windows.remove(window); }

void Client::showWindowInFront(Window *window) {
  removeWindow(window);
  addWindow(window);
  window->show();
}

void Client::addUI(Element *element) { _ui.push_back(element); }

void Client::addChatMessage(const std::string &msg, const Color &color) {
  auto wrappedLines = _wordWrapper.wrap(msg);
  for (const auto &line : wrappedLines) {
    Label *label = new Label({}, line);
    label->setColor(color);
    _chatLog->addChild(label);
  }
  _chatLog->scrollToBottom();
}

void Client::closeWindowsFromOutOfRangeObjects() {
  for (auto pair : _objects) {
    auto &obj = *pair.second;

    if (distance(playerCollisionRect(), obj.collisionRect()) <= ACTION_DISTANCE)
      continue;

    obj.hideWindow();

    // Hide quest windows from this object
    // Note: this doesn't check that the object itself is the source of
    // the quest.  A more correct solution would make sure that there
    // are no watched objects of the same type.
    for (auto *questFromThisObject : obj.startsQuests()) {
      auto *questWindow = questFromThisObject->window();
      if (questWindow) const_cast<Window *>(questWindow)->hide();
    }
    for (auto *questFromThisObject : obj.completableQuests()) {
      auto *questWindow = questFromThisObject->window();
      if (questWindow) const_cast<Window *>(questWindow)->hide();
    }
  }
}

void Client::dropItemOnConfirmation(Serial serial, size_t slot,
                                    const ClientItem *item) {
  std::string windowText =
      "Are you sure you want to destroy " + item->name() + "?";
  std::string msgArgs = makeArgs(serial, slot);

  if (_confirmDropItem != nullptr) {
    removeWindow(_confirmDropItem);
    delete _confirmDropItem;
  }
  _confirmDropItem = new ConfirmationWindow(windowText, CL_DROP, msgArgs);
  addWindow(_confirmDropItem);
  _confirmDropItem->show();
}

bool Client::outsideCullRange(const MapPoint &loc, px_t hysteresis) const {
  px_t testCullDist = CULL_DISTANCE + hysteresis;
  return abs(loc.x - _character.location().x) > testCullDist ||
         abs(loc.y - _character.location().y) > testCullDist;
}

const ParticleProfile *Client::findParticleProfile(const std::string &id) {
  ParticleProfile dummy(id);
  auto it = _particleProfiles.find(&dummy);
  if (it == _particleProfiles.end()) return nullptr;
  return *it;
}

void Client::addParticles(const ParticlesToAdd &details) {
#ifdef _DEBUG
  return;
#endif

  for (size_t i = 0; i != details.quantity(); ++i) {
    auto *particle = details.profile().instantiate(details.location());

    if (details.customAltitude() != 0)
      particle->addToAltitude(details.customAltitude());

    addEntity(particle);
  }
}

void Client::addParticles(const ParticleProfile *profile,
                          const MapPoint &location) {
  if (profile == nullptr) return;
  auto details = ParticlesToAdd{*profile, location};
  details.discrete();
  addParticles(details);
}

void Client::addParticles(const ParticleProfile *profile,
                          const MapPoint &location, size_t qty) {
  if (profile == nullptr) return;
  auto details = ParticlesToAdd{*profile, location};
  details.exactNumber(qty);
  addParticles(details);
}

void Client::addParticles(const ParticleProfile *profile,
                          const MapPoint &location, double delta) {
  if (profile == nullptr) return;
  auto details = ParticlesToAdd{*profile, location};
  details.continuous(delta);
  addParticles(details);
}

void Client::addParticles(const std::string &profileName,
                          const MapPoint &location) {
  const ParticleProfile *profile = findParticleProfile(profileName);
  if (profile == nullptr) return;
  auto details = ParticlesToAdd{*profile, location};
  details.discrete();
  addParticles(details);
}

void Client::addParticles(const std::string &profileName,
                          const MapPoint &location, double delta) {
  const ParticleProfile *profile = findParticleProfile(profileName);
  if (profile == nullptr) return;
  auto details = ParticlesToAdd{*profile, location};
  details.continuous(delta);
  addParticles(details);
}

void Client::addParticlesWithCustomAltitude(double altitude,
                                            const std::string &profileName,
                                            const MapPoint &location,
                                            double delta) {
  const ParticleProfile *profile = findParticleProfile(profileName);
  if (profile == nullptr) return;
  auto details = ParticlesToAdd{*profile, location};
  details.continuous(delta);
  details.customAltitude(altitude);
  addParticles(details);
}

void Client::addFloatingCombatText(const std::string &text,
                                   const MapPoint &location, Color color) {
  auto floatingTextProfile = findParticleProfile("floatingText");
  if (!floatingTextProfile) return;
  auto floatingText = floatingTextProfile->instantiate(location);

  auto outline = Texture{_defaultFont, text, color};
  auto front = Texture{_defaultFont, text, Color::FLOATING_CORE};

  auto canvas = Texture{front.width() + 2, front.height() + 2};
  canvas.setBlend(SDL_BLENDMODE_BLEND);

  renderer.pushRenderTarget(canvas);
  outline.draw(0, 1);
  outline.draw(1, 0);
  outline.draw(2, 1);
  outline.draw(1, 2);
  front.draw(1, 1);
  renderer.popRenderTarget();

  floatingText->setImageManually(canvas);
  addEntity(floatingText);
}

void Client::initializeUsername() {
  if (cmdLineArgs.contains("username")) {
    _username = cmdLineArgs.getString("username");
    return;
  }
  if (isDebug()) {
    setRandomUsername();
    return;
  }
  _username = "";

  // Attempt to use last-used name  char *appDataPath = nullptr;
  char *appDataPath = nullptr;
  _dupenv_s(&appDataPath, nullptr, "LOCALAPPDATA");
  if (!appDataPath) return;
  auto sessionFile = std::string{appDataPath} + "\\Hellas\\session.txt"s;
  free(appDataPath);

  auto xr = XmlReader::FromFile(sessionFile);
  if (!xr) return;
  auto user = xr.findChild("user");
  if (!user) return;
  xr.findAttr(user, "name", _username);
  xr.findAttr(user, "passwordHash", _savedPwHash);
}

void Client::setRandomUsername() {
  _username.clear();
  _username.push_back('A' + rand() % 26);
  for (int i = 0; i != 2; ++i) _username.push_back('a' + rand() % 26);
}

const SoundProfile *Client::findSoundProfile(const std::string &id) const {
  auto it = _soundProfiles.find(SoundProfile(id));
  if (it == _soundProfiles.end()) return nullptr;
  return &*it;
}

const Projectile::Type *Client::findProjectileType(
    const std::string &id) const {
  auto dummy = Projectile::Type{id, {}};
  auto it = _projectileTypes.find(&dummy);
  if (it != _projectileTypes.end()) return *it;
  return nullptr;
}

ClientObjectType *Client::findObjectType(const std::string &id) {
  auto dummy = ClientObjectType{id};
  auto it = _objectTypes.find(&dummy);
  if (it == _objectTypes.end()) return nullptr;
  return const_cast<ClientObjectType *>(*it);
}

ClientNPCType *Client::findNPCType(const std::string &id) {
  auto ot = findObjectType(id);
  return dynamic_cast<ClientNPCType *>(ot);
}

const ClientItem *Client::findItem(const std::string &id) const {
  auto it = _items.find(id);
  if (it == _items.end()) return nullptr;
  return &it->second;
}

const CNPCTemplate *Client::findNPCTemplate(const std::string &id) const {
  auto it = _npcTemplates.find(id);
  if (it == _npcTemplates.end()) return nullptr;
  return &it->second;
}

bool Client::isAtWarWithObjectOwner(const ClientObject::Owner &owner) const {
  // Cities
  if (owner.type == ClientObject::Owner::CITY)
    return isAtWarWithCityDirectly(owner.name);

  if (owner.type != ClientObject::Owner::PLAYER) return false;

  // Players
  assert(owner.type == ClientObject::Owner::PLAYER);
  std::string ownerPlayersCity = "";
  auto it = _userCities.find(owner.name);
  if (it != _userCities.end()) ownerPlayersCity = it->second;

  auto ownerIsInACity = !ownerPlayersCity.empty();
  auto playerIsInACity = !_character.cityName().empty();
  if (!playerIsInACity) {  // No city: use personal wars
    if (ownerIsInACity)
      return isAtWarWithCityDirectly(ownerPlayersCity);
    else
      return isAtWarWithPlayerDirectly(owner.name);
  } else {  // City: use city's wars
    if (ownerIsInACity)
      return isCityAtWarWithCityDirectly(ownerPlayersCity);
    else
      return isCityAtWarWithPlayerDirectly(owner.name);
  }
}

std::string Client::getUserCity(const std::string &name) const {
  auto it = _userCities.find(name);
  if (it == _userCities.end()) return {};
  return it->second;
}

const ClientSpell *Client::findSpell(const std::string &spellID) const {
  auto it = _spells.find(spellID);
  if (it == _spells.end()) return nullptr;
  return it->second;
}

bool Client::isAtWarWith(const Avatar &user) const {
  return isAtWarWithObjectOwner({ClientObject::Owner::PLAYER, user.name()});
}

void Client::addUser(const std::string &name, const MapPoint &location) {
  auto pUser = new Avatar(name, location);
  _otherUsers[name] = pUser;
  _entities.insert(pUser);
}

int Client::totalTalentPointsAllocated() {
  auto sum = 0;
  for (const auto &pair : _pointsInTrees) sum += pair.second;
  return sum;
}

void Client::applyCollisionChecksToPlayerMovement(MapPoint &pendingDest) const {
  auto loc = _character.location();

  auto collisionRect = _character.collisionRect();

  if (_character.isDriving()) {
    collisionRect = _character.vehicle()->collisionRect();
  }

  // If current location is bad, allow all attempts to move.
  // This should help players to get out of rare bad situations.
  if (!isLocationValidForPlayer(collisionRect)) return;

  // First check: rectangle around entire journey
  auto rawDisplacement = MapPoint{pendingDest.x - loc.x, pendingDest.y - loc.y};
  auto displacementX = abs(rawDisplacement.x);
  auto displacementY = abs(rawDisplacement.y);
  auto journeyRect = collisionRect;
  if (rawDisplacement.x < 0) journeyRect.x -= displacementX;
  journeyRect.w += displacementX;
  if (rawDisplacement.y < 0) journeyRect.y -= displacementY;
  journeyRect.h += displacementY;
  if (isLocationValidForPlayer(journeyRect)) return;  // Proposal is fine.

  // Try x alone
  auto xOnlyRect = journeyRect;
  xOnlyRect.y = collisionRect.y;
  xOnlyRect.h = collisionRect.h;
  if (isLocationValidForPlayer(xOnlyRect)) {
    pendingDest.y = loc.y;
    return;
  }

  // Try y alone
  auto yOnlyRect = journeyRect;
  yOnlyRect.x = collisionRect.x;
  yOnlyRect.w = collisionRect.w;
  if (isLocationValidForPlayer(yOnlyRect)) {
    pendingDest.x = loc.x;
    return;
  }

  pendingDest = loc;
  return;
}

bool Client::isLocationValidForPlayer(const MapPoint &location) const {
  auto rect = _character.collisionRectRaw() + location;
  return isLocationValidForPlayer(rect);
}

bool Client::isLocationValidForPlayer(const MapRect &rect) const {
  // Map edges
  if (rect.x < 0 || rect.y < 0) return false;
  static const auto X_LIM = _map.width() * Map::TILE_W - Map::TILE_W / 2;
  if (rect.x + rect.w > X_LIM) return false;
  static const auto Y_LIM = _map.height() * Map::TILE_H;
  if (rect.y + rect.h > Y_LIM) return false;

  // Terrain
  auto allowedTerrain = _allowedTerrain;
  if (_character.isDriving()) {
    const auto &vehicleType = *_character.vehicle()->objectType();
    allowedTerrain = vehicleType.allowedTerrain();
    if (allowedTerrain.empty())
      allowedTerrain = TerrainList::defaultList().id();
  }
  auto applicableTerrainList = TerrainList::findList(allowedTerrain);
  if (!applicableTerrainList)
    applicableTerrainList = &TerrainList::defaultList();
  auto terrainTypesCovered = _map.terrainTypesOverlapping(rect);
  for (auto terrainType : terrainTypesCovered)
    if (!applicableTerrainList->allows(terrainType)) return false;

  // Objects
  for (const auto *sprite : _entities) {
    auto *obj = dynamic_cast<const ClientObject *>(sprite);
    if (!obj) continue;
    if (!obj->collides()) continue;
    if (obj->isDead()) continue;

    if (_character.isDriving() && _character.vehicle() == obj) continue;

    // Allow collisions between users and users/NPCs
    if (obj->classTag() == 'u' || obj->classTag() == 'n') continue;

    if (obj->collisionRect().overlaps(rect)) return false;
  }

  return true;
}

bool Client::isWindowRegistered(const Window *toFind) {
  for (auto window : _windows) {
    if (window == toFind) return true;
  }
  return false;
}

void Client::infoWindow(const std::string &text) {
  auto window = new InfoWindow(text);
  addWindow(window);
  window->show();
}

void Client::onSpellHit(const MapPoint &location, const void *data) {
  auto &spell = *reinterpret_cast<const ClientSpell *>(data);
  if (spell.impactParticles())
    instance().addParticles(spell.impactParticles(), location);

  if (spell.sounds()) spell.sounds()->playOnce("impact"s);
}
