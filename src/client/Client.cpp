#include "Client.h"

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
#include "SDLWrappers.h"
#include "SpriteType.h"
#include "Tooltip.h"
#include "UIGroup.h"
#include "Unlocks.h"
#include "ui/Button.h"
#include "ui/CombatantPanel.h"
#include "ui/ConfirmationWindow.h"
#include "ui/ContainerGrid.h"
#include "ui/Element.h"
#include "ui/LinkedLabel.h"
#include "ui/ProgressBar.h"

extern Args cmdLineArgs;
extern Renderer renderer;

Client::CommonImages Client::images;
TTF_Font *Client::_defaultFont = nullptr;

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

const size_t Client::INVENTORY_SIZE = 15;
const size_t Client::GEAR_SLOTS = 8;
std::vector<std::string> Client::GEAR_SLOT_NAMES;

std::map<std::string, int> Client::_messageCommands;
std::map<int, std::string> Client::_errorMessages;

const int Client::MIXING_CHANNELS = 32;

Client::Client()
    : _currentCursor(&images.cursorNormal),

      avatarSpriteType(Avatar::DRAW_RECT),
      avatarCombatantType(MAX_PLAYER_HEALTH),

      _character({}, {}, *this),
      _target(*this),

      _connection(*this),

      _dataLoaded(false),

      _inventory(INVENTORY_SIZE, std::make_pair(ClientItem::Instance{}, 0)),

      _time(SDL_GetTicks()),
      _lastPingReply(_time),
      _lastPingSent(_time),

      _debug(*this, "client.log") {
  initStatics();
  drawLoadingScreen("Reading configuration file");

  _config.loadFromFile("client-config.xml");
  _connection.initialize();

  initUI();
  _wordWrapper = WordWrapper{_defaultFont, _chatLog->contentWidth()};

#ifdef _DEBUG
  _debug(toString(cmdLineArgs));
  if (Socket::debug == nullptr) Socket::debug = &_debug;
#endif

  if (cmdLineArgs.contains("auto-login")) _shouldAutoLogIn = true;

  drawLoadingScreen("Initializing audio");
  int ret = (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 1, 512) < 0);
  if (ret < 0) {
    showErrorMessage("SDL_mixer failed to initialize.", Color::CHAT_ERROR);
  }
  Mix_AllocateChannels(MIXING_CHANNELS);

  avatarSpriteType.useCustomDrawHeight(40);

  _entities.insert(&_character);

  gameData.unlocks.linkToKnownRecipes(_knownRecipes);
  gameData.unlocks.linkToKnownConstructions(_knownConstructions);

  initializeUsername();

  SDLWrappers::StopTextInput();
}

void Client::initialiseData() {
  // Tell Avatars to use blood particles
  avatarCombatantType.damageParticles(findParticleProfile("blood"));

  // Match up ranged weapons with their ammo items
  for (auto &pair : gameData.items) {
    auto &item = const_cast<ClientItem &>(pair.second);
    item.fetchAmmoItem();
  }

  populateBuildList();

  // Tell quest nodes about their quests
  for (auto &questPair : gameData.quests) {
    auto &quest = questPair.second;

    auto startNode = findObjectType(quest.info().startsAt);
    if (startNode) startNode->startsQuest(quest);

    auto endNode = findObjectType(quest.info().endsAt);
    if (endNode) endNode->endsQuest(quest);
  }

  avatarSpriteType.useCustomShadowWidth(16);
  avatarSpriteType.useCustomDrawHeight(50);
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

void Client::initStatics() {
  static auto alreadyInitialised = false;
  if (alreadyInitialised) return;
  alreadyInitialised = true;

  _defaultFont = TTF_OpenFont("AdvoCut.ttf", 10);
  images.initialise();
  SDL_ShowCursor(SDL_DISABLE);
  initializeMessageNames();
}

Client::~Client() {
  for (const Sprite *entityConst : _entities) {
    Sprite *entity = const_cast<Sprite *>(entityConst);
    if (entity == &_character) continue;
    delete entity;
  }

  // Some entities will destroy their own windows, and remove them from this
  // list.
  for (Window *window : _windows) delete window;

  auto uniqueUIElements = std::set<Element *>{};
  for (Element *mainUIElement : _ui) uniqueUIElements.insert(mainUIElement);
  for (Element *loginElement : _loginUI) uniqueUIElements.insert(loginElement);
  for (Element *element : uniqueUIElements) delete element;

  delete groupUI;

  Socket::debug = nullptr;
}

void Client::cleanUpStatics() {
  SDL_ShowCursor(SDL_ENABLE);
  if (_defaultFont) TTF_CloseFont(_defaultFont);
  Mix_Quit();
}

void Client::run() {
  _state = RUNNING;
  if (!_dataLoaded) {
    drawLoadingScreen("Loading data");
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

  drawLoadingScreen("Initializing login screen");
  initLoginScreen();
  _connection.state(Connection::TRYING_TO_CONNECT);

  drawLoadingScreen("");

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

    SDL_Delay(1);

    while (_freeze)
      ;
  }
  _state = FINISHED;
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
  if (_loaded && _time - _lastPingReply > SERVER_TIMEOUT) {
    disconnect();
#ifndef TESTING
    infoWindow("Disconnected from server.");
#endif
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
      sendMessage({CL_MOVE_TO,
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

  showQueuedErrorMessages();

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
  for (auto &terrainPair : gameData.terrain)
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
  for (auto &pair : gameData.quests) {
    pair.second.update(_timeElapsed);
  }

  if (_mouseMoved) checkMouseOver();

  updateUI();

  _connection.getNewMessages();
  // Draw
  draw();
}

void Client::startCrafting() {
  if (_activeRecipe != nullptr) {
    sendMessage({CL_CRAFT, _activeRecipe->id()});
    const ClientItem *product = toClientItem(_activeRecipe->product());
    prepareAction("Crafting " + product->name());
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

void Client::addEntity(Sprite *sprite) { _entities.insert(sprite); }

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
  window->onAddToClientWindowList(*this);
}

void Client::removeWindow(Window *window) { _windows.remove(window); }

void Client::showWindowInFront(Window *window) {
  removeWindow(window);
  addWindow(window);
  window->show();
}

void Client::showWindowInFront(void *window) {
  auto asWindow = reinterpret_cast<Window *>(window);
  asWindow->client()->showWindowInFront(asWindow);
}

void Client::addUI(Element *element) { _ui.push_front(element); }

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

void Client::dropItemOnConfirmation(const ContainerGrid::GridInUse &toDrop) {
  std::string windowText =
      "Dropping a soulbound item will destroy it.  Are you sure?";
  std::string msgArgs = makeArgs(toDrop.object(), toDrop.slot());

  if (_confirmDropSoulboundItem != nullptr) {
    removeWindow(_confirmDropSoulboundItem);
    delete _confirmDropSoulboundItem;
  }
  _confirmDropSoulboundItem =
      new ConfirmationWindow(*this, windowText, CL_DROP, msgArgs);
  addWindow(_confirmDropSoulboundItem);
  _confirmDropSoulboundItem->show();
}

void Client::scrapItemOnConfirmation(const ContainerGrid::GridInUse &toScrap,
                                     std ::string itemName) {
  std::string windowText = "Are you sure you want to scrap "s + itemName + "?"s;
  std::string msgArgs = makeArgs(toScrap.object(), toScrap.slot());

  if (_confirmScrapItem != nullptr) {
    removeWindow(_confirmScrapItem);
    delete _confirmScrapItem;
  }
  _confirmScrapItem =
      new ConfirmationWindow(*this, windowText, CL_SCRAP_ITEM, msgArgs);
  addWindow(_confirmScrapItem);
  _confirmScrapItem->show();
}

bool Client::outsideCullRange(const MapPoint &loc, px_t hysteresis) const {
  px_t testCullDist = CULL_DISTANCE + hysteresis;
  return abs(loc.x - _character.location().x) > testCullDist ||
         abs(loc.y - _character.location().y) > testCullDist;
}

const ParticleProfile *Client::findParticleProfile(
    const std::string &id) const {
  ParticleProfile dummy(id);
  auto it = gameData.particleProfiles.find(&dummy);
  if (it == gameData.particleProfiles.end()) return nullptr;
  return *it;
}

void Client::addParticles(const ParticlesToAdd &details) {
#ifdef _DEBUG
  return;
#endif

  for (size_t i = 0; i != details.quantity(); ++i) {
    auto *particle = details.profile().instantiate(details.location(), *this);

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
  auto floatingText = floatingTextProfile->instantiate(location, *this);

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
  auto it = gameData.soundProfiles.find(SoundProfile(id));
  if (it == gameData.soundProfiles.end()) return nullptr;
  return &*it;
}

const Projectile::Type *Client::findProjectileType(
    const std::string &id) const {
  auto dummy = Projectile::Type{id, {}, this};
  auto it = gameData.projectileTypes.find(&dummy);
  if (it != gameData.projectileTypes.end()) return *it;
  return nullptr;
}

ClientObjectType *Client::findObjectType(const std::string &id) {
  auto dummy = ClientObjectType{id};
  auto it = gameData.objectTypes.find(&dummy);
  if (it == gameData.objectTypes.end()) return nullptr;
  return const_cast<ClientObjectType *>(*it);
}

ClientNPCType *Client::findNPCType(const std::string &id) {
  auto ot = findObjectType(id);
  return dynamic_cast<ClientNPCType *>(ot);
}

const ClientItem *Client::findItem(const std::string &id) const {
  auto it = gameData.items.find(id);
  if (it == gameData.items.end()) return nullptr;
  return &it->second;
}

const CNPCTemplate *Client::findNPCTemplate(const std::string &id) const {
  auto it = gameData.npcTemplates.find(id);
  if (it == gameData.npcTemplates.end()) return nullptr;
  return &it->second;
}

Avatar *Client::findUser(const std::string &username) {
  if (username == _character.name()) return &_character;
  auto it = _otherUsers.find(username);
  if (it == _otherUsers.end()) return nullptr;
  return it->second;
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
  auto it = gameData.spells.find(spellID);
  if (it == gameData.spells.end()) return nullptr;
  return it->second;
}

bool Client::isAtWarWith(const Avatar &user) const {
  return isAtWarWithObjectOwner({ClientObject::Owner::PLAYER, user.name()});
}

void Client::addUser(const std::string &name, const MapPoint &location) {
  auto pUser = new Avatar(name, location, *this);
  _otherUsers[name] = pUser;
  _entities.insert(pUser);

  groupUI->refresh();
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
  auto window = new InfoWindow(*this, text);
  addWindow(window);
  window->show();
}

void Client::onSpellHit(const MapPoint &location, void *data) {
  auto &spell = *reinterpret_cast<ClientSpell *>(data);
  if (spell.impactParticles())
    spell.client().addParticles(spell.impactParticles(), location);

  if (spell.sounds()) spell.sounds()->playOnce(spell.client(), "impact"s);
}

void Client::CommonImages::initialise() {
  shadow = {"Images/shadow.png"};

  cursorNormal = {"Images/Cursors/normal.png"s, Color::MAGENTA};
  cursorGather = {"Images/Cursors/gather.png"s, Color::MAGENTA};
  cursorContainer = {"Images/Cursors/container.png"s, Color::MAGENTA};
  cursorAttack = {"Images/Cursors/attack.png"s, Color::MAGENTA};
  cursorStartsQuest = {"Images/Cursors/startsQuest.png"s, Color::MAGENTA};
  cursorEndsQuest = {"Images/Cursors/endsQuest.png"s, Color::MAGENTA};
  cursorRepair = {"Images/Cursors/repair.png"s, Color::MAGENTA};
  cursorVehicle = {"Images/Cursors/vehicle.png"s, Color::MAGENTA};
  cursorText = {"Images/Cursors/text.png"s, Color::MAGENTA};

  startQuestIcon = {"Images/UI/startQuest.png", Color::MAGENTA};
  endQuestIcon = {"Images/UI/endQuest.png", Color::MAGENTA};
  startQuestIndicator = {"Images/questStart.png", Color::MAGENTA};
  endQuestIndicator = {"Images/questEnd.png", Color::MAGENTA};

  eliteWreath = {"Images/UI/wreathElite.png", Color::MAGENTA};
  bossWreath = {"Images/UI/wreathBoss.png", Color::MAGENTA};

  itemHighlightMouseOver = {"Images/Items/highlight.png"s, Color::MAGENTA};
  itemHighlightMatch = {"Images/Items/highlightGood.png"s, Color::MAGENTA};
  itemHighlightNoMatch = {"Images/Items/highlightBad.png"s, Color::MAGENTA};

  generateItemDamageOverlays();

  itemQualityMask = Texture{"Images/ui/item-quality-mask.png"};

  cityIcon = {"Images/ui/city.png"s};
  playerIcon = {"Images/ui/player.png"s};

  loginBackgroundBack = {"Images/loginBack.png"s};
  loginBackgroundFront = {"Images/loginFront.png"s, Color::MAGENTA};

  basePassive = {"Images/targetPassive.png"s, Color::MAGENTA};
  baseAggressive = {"Images/targetAggressive.png"s, Color::MAGENTA};

  scrollArrowWhiteUp = {"Images/arrowUp.png", Color::MAGENTA};
  scrollArrowWhiteDown = {"Images/arrowDown.png", Color::MAGENTA};
  scrollArrowGreyUp = {"Images/arrowUpGrey.png", Color::MAGENTA};
  scrollArrowGreyDown = {"Images/arrowDownGrey.png", Color::MAGENTA};

  icons.initialise("Images/Icons");

  map = {"Images/map.png"};
  mapCityFriendly = {"Images/UI/map-city-friendly.png", Color::MAGENTA};
  mapCityNeutral = {"Images/UI/map-city-neutral.png", Color::MAGENTA};
  mapCityEnemy = {"Images/UI/map-city-enemy.png", Color::MAGENTA};
  mapRespawn = {"Images/UI/map-respawn.png", Color::MAGENTA};

  logoDiscord = {"Images/logo-discord.png", Color::MAGENTA};
  logoBTC = {"Images/logo-bitcoin.png", Color::MAGENTA};
  btcQR = {"Images/btc-qr.png", Color::MAGENTA};
}

void Client::CommonImages::generateItemDamageOverlays() {
  itemDamaged = itemDamaged = {Client::ICON_SIZE, Client::ICON_SIZE};
  renderer.pushRenderTarget(itemDamaged);
  renderer.setDrawColor(Color::DURABILITY_LOW);
  renderer.fill();
  renderer.popRenderTarget();
  itemDamaged.setAlpha(0x7f);
  itemDamaged.setBlend();

  itemBroken = {Client::ICON_SIZE, Client::ICON_SIZE};
  renderer.pushRenderTarget(itemBroken);
  renderer.setDrawColor(Color::DURABILITY_BROKEN);
  renderer.fill();
  renderer.popRenderTarget();
  itemBroken.setAlpha(0x7f);
  itemBroken.setBlend();
}
