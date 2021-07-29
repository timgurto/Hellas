#include "CQuest.h"

#include <sstream>
#include "../Rect.h"
#include "../util.h"
#include "CQuest.h"
#include "Client.h"
#include "WordWrapper.h"
#include "ui/Line.h"
#include "ui/Window.h"

CQuest::CQuest(Client &client, const Info &info)
    : _client(&client), _info(info), _progress(info.objectives.size(), 0) {}

void CQuest::generateWindow(CQuest *quest, Serial startObjectSerial,
                            Transition pendingTransition) {
  const auto WIN_W = 200_px, WIN_H = 200_px;

  if (quest->_window) {
    quest->_client->removeWindow(quest->_window);
    delete quest->_window;
  }

  quest->_window = Window::WithRectAndTitle(
      {0, 0, WIN_W, WIN_H}, quest->_info.name, quest->_client->mouse());
  quest->_window->center();

  const auto BOTTOM = quest->_window->contentHeight();
  const auto GAP = 2_px, BUTTON_W = 90_px, BUTTON_H = 16_px,
             CONTENT_W = WIN_W - 2 * GAP;
  auto y = GAP;

  // Body: brief/debrief
  const auto BODY_H = BOTTOM - 2 * GAP - BUTTON_H - y;
  auto body = new List({GAP, y, CONTENT_W, BODY_H});
  quest->_window->addChild(body);
  auto ww = WordWrapper{Element::font(), body->contentWidth()};
  auto showBriefing =
      pendingTransition == ACCEPT || pendingTransition == INFO_ONLY;
  const auto &bodyText =
      showBriefing ? quest->_info.brief : quest->_info.debrief;
  auto lines = ww.wrap(bodyText);
  for (auto line : lines) {
    auto isHelpText = false;

    if (!line.empty() && line.front() == '?') {
      isHelpText = true;
      line = line.substr(1);
    }
    auto label = new Label({}, line);
    body->addChild(label);
    if (isHelpText) {
      label->setColor(Color::TOOLTIP_INSTRUCTION);
    }
  }

  // Body: objectives
  auto shouldShowObjectives = showBriefing && !quest->_info.objectives.empty();
  if (shouldShowObjectives) {
    body->addGap();
    auto heading = new Label({}, "Objectives:");
    heading->setColor(Color::WINDOW_HEADING);
    body->addChild(heading);

    for (auto &objective : quest->_info.objectives) {
      auto text = objective.text;
      if (objective.qty > 1) text += " ("s + toString(objective.qty) + ")"s;
      body->addChild(new Label({}, text));
    }
  }

  // Body: reward
  if (!quest->_info.rewards.empty()) {
    body->addGap();
    auto heading = new Label({}, "Reward:");
    heading->setColor(Color::WINDOW_HEADING);
    body->addChild(heading);
  }
  for (const auto &reward : quest->_info.rewards) {
    auto shouldShowReward = reward.type != Info::Reward::NONE;
    if (!shouldShowReward) continue;

    auto rewardDescription = ""s;
    const Tooltip *rewardTooltip{nullptr};
    switch (reward.type) {
      case Info::Reward::LEARN_SPELL: {
        const auto *spell = quest->_client->findSpell(reward.id);
        auto name = spell ? spell->name() : reward.id;
        rewardDescription = "Learn spell: " + name;
        if (spell) rewardTooltip = &spell->tooltip();
        break;
      }
      case Info::Reward::LEARN_CONSTRUCTION: {
        const auto *type = quest->_client->findObjectType(reward.id);
        auto name = type ? type->name() : reward.id;
        rewardDescription = "Learn construction: " + name;
        if (type) rewardTooltip = &type->constructionTooltip(*quest->_client);
        break;
      }
      case Info::Reward::LEARN_RECIPE: {
        const auto &recipes = quest->_client->gameData.recipes;
        const auto it = recipes.find(reward.id);
        if (it == recipes.end()) break;
        rewardDescription = "Learn recipe: " + it->name();
        rewardTooltip = &it->tooltip(quest->_client->gameData.tagNames);
        break;
      }
      case Info::Reward::RECEIVE_ITEM: {
        const auto *item = quest->_client->findItem(reward.id);
        auto name = item ? item->name() : reward.id;
        rewardDescription = "Receive item: ";
        if (reward.itemQuantity > 1)
          rewardDescription += toString(reward.itemQuantity) + "x ";
        rewardDescription += name;
        if (item) rewardTooltip = &item->tooltip();
        break;
      }
    }
    auto rewardLabel = new Label({}, rewardDescription);
    if (rewardTooltip) rewardLabel->setTooltip(*rewardTooltip);
    body->addChild(rewardLabel);
  }

  y += BODY_H + GAP;

  // Transition button
  if (pendingTransition != INFO_ONLY) {
    const auto TRANSITION_BUTTON_RECT = ScreenRect{GAP, y, BUTTON_W, BUTTON_H};
    auto transitionName =
        pendingTransition == ACCEPT ? "Accept quest"s : "Complete quest"s;
    auto transitionFun =
        pendingTransition == ACCEPT ? acceptQuest : completeQuest;
    Button *transitionButton =
        new Button(TRANSITION_BUTTON_RECT, transitionName,
                   [=]() { transitionFun(quest, startObjectSerial); });
    if (pendingTransition == ACCEPT) transitionButton->id("accept");
    quest->_window->addChild(transitionButton);
  }

  quest->_client->addWindow(quest->_window);
  quest->_window->show();
}

void CQuest::acceptQuest(CQuest *quest, Serial startObjectSerial) {
  // Send message
  quest->_client->sendMessage(
      {CL_ACCEPT_QUEST, makeArgs(quest->_info.id, startObjectSerial)});

  // Close and remove window
  quest->_window->hide();
  // TODO: better cleanup.  Lots of unused windows in the background may take
  // up significant memory.  Note that this function is called from a button
  // click (which subsequently changes the appearance of the button), meaning
  // it is unsafe to delete the window here.

  // Show specified help topic
  if (!quest->_info.helpTopicOnAccept.empty())
    quest->_client->showHelpTopic(quest->_info.helpTopicOnAccept);
}

void CQuest::completeQuest(CQuest *quest, Serial endObject) {
  // Send message
  quest->_client->sendMessage(
      {CL_COMPLETE_QUEST, makeArgs(quest->_info.id, endObject)});

  // Close and remove window
  quest->_window->hide();
  // TODO: better cleanup.  Lots of unused windows in the background may take
  // up significant memory.  Note that this function is called from a button
  // click (which subsequently changes the appearance of the button), meaning
  // it is unsafe to delete the window here.

  // Show specified help topic
  if (!quest->_info.helpTopicOnComplete.empty())
    quest->_client->showHelpTopic(quest->_info.helpTopicOnComplete);
}

void CQuest::setProgress(size_t objective, int progress) {
  _progress[objective] = progress;
}

int CQuest::getProgress(size_t objective) const {
  return _progress.at(objective);
}

std::string CQuest::nameInProgressUI() const {
  auto ret = nameAndLevel();
  if (_timeRemaining > 0)
    ret += " (" + msAsShortTimeDisplay(_timeRemaining) + ")";
  return ret;
}

std::string CQuest::nameAndLevel() const {
  auto oss = std::ostringstream{};
  oss << '[' << _info.level << "] " << _info.name;
  return oss.str();
}

Color CQuest::difficultyColor() const {
  return getDifficultyColor(_info.level, _client->character().level());
}

void CQuest::update(ms_t timeElapsed) {
  if (_timeRemaining > 0) {
    if (timeElapsed > _timeRemaining)
      _timeRemaining = 0;
    else
      _timeRemaining -= timeElapsed;

    auto oldTimeDisplay = _lastTimeDisplay;
    _lastTimeDisplay = msAsShortTimeDisplay(_timeRemaining);
    if (_lastTimeDisplay != oldTimeDisplay) _client->refreshQuestProgress();
  }
}
