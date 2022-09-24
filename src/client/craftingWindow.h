#pragma once

#include <string.h>
#include <map>
#include "ui/Label.h"

class ChoiceList;
class CRecipe;
class TextBox;

class CraftingWindowFilter {
 public:
  using MatchingRecipes = std::set<const CRecipe *>;

  virtual std::string buttonText() const = 0;

  // Build/change an internal model of the data
  virtual void indexRecipe(const CRecipe &recipe) = 0;

  // Set user up to configure filter
  virtual void populateConfigurationPanel(Element &panel) const = 0;

  // Interpret configuration and apply it to the data model
  virtual MatchingRecipes getMatchingRecipes() const = 0;
};

template <typename K>
class ListBasedFilter : public CraftingWindowFilter {
 public:
  using Key = K;
  using IndexedRecipes = std::multimap<Key, const CRecipe *>;

  static MatchingRecipes recipesMatching(const IndexedRecipes &indexedRecipes,
                                         Key key) {
    auto recipes = MatchingRecipes{};
    auto startIt = indexedRecipes.lower_bound(key);
    auto endIt = indexedRecipes.upper_bound(key);
    for (auto it = startIt; it != endIt; ++it) recipes.insert(it->second);
    return recipes;
  }

  void indexRecipe(const CRecipe &recipe) {
    for (const auto key : getKeysFromRecipe(recipe))
      m_indexedRecipes.insert(std::make_pair(key, &recipe));
  }

  void populateConfigurationPanel(Element &panel) const override {
    m_list = new ChoiceList(panel.rectToFill(), itemHeight(), *panel.client());
    panel.addChild(m_list);

    for (const auto category : uniqueIndexedKeys()) {
      auto *entry = new Element;
      m_list->addChild(entry);
      entry->id(toStringID(category));
      populateEntry(*entry, category);
    }
    m_list->verifyBoxes();
  }

 protected:
  using Keys = std::set<K>;

  IndexedRecipes m_indexedRecipes;
  mutable ChoiceList *m_list{nullptr};

 private:
  virtual Keys getKeysFromRecipe(const CRecipe &recipe) = 0;

  Keys uniqueIndexedKeys() const {
    Keys uniqueKeys;
    for (const auto &pair : m_indexedRecipes) uniqueKeys.insert(pair.first);
    return uniqueKeys;
  }

  virtual px_t itemHeight() const { return Element::TEXT_HEIGHT; }
  virtual std::string toStringID(Key key) const = 0;

  virtual void populateEntry(Element &entry, Key key) const = 0;
};

class MaterialFilter : public ListBasedFilter<ClientItemAlphabetical> {
 public:
  MaterialFilter(const Client &client);
  std::string buttonText() const override { return "Material"s; }

 private:
  std::string toStringID(Key key) const override { return key->id(); }
  px_t itemHeight() const override { return 16; }
  MatchingRecipes getMatchingRecipes() const override;
  Keys getKeysFromRecipe(const CRecipe &recipe) override;
  void populateEntry(Element &entry, Key key) const override;

  const Client &m_client;
};

class ToolFilter : public ListBasedFilter<std::string> {
 public:
  ToolFilter(const Client &client);
  std::string buttonText() const override { return "Tool req."s; }

 private:
  px_t itemHeight() const override { return 16; }
  std::string toStringID(Key key) const override { return key; }
  MatchingRecipes getMatchingRecipes() const override;
  Keys getKeysFromRecipe(const CRecipe &recipe) override;
  void populateEntry(Element &entry, Key key) const override;

  const Client &m_client;
};

class LvlReqFilter : public CraftingWindowFilter {
 public:
  std::string buttonText() const override { return "Level req."s; }
  void indexRecipe(const CRecipe &recipe) override;
  void populateConfigurationPanel(Element &panel) const override;
  MatchingRecipes getMatchingRecipes() const override;

 private:
  using IndexedRecipes = std::multimap<Level, const CRecipe *>;
  IndexedRecipes m_indexedRecipes;
  mutable TextBox *m_minLevel{nullptr}, *m_maxLevel{nullptr};
};

class CategoryFilter : public ListBasedFilter<std::string> {
 public:
  std::string buttonText() const override { return "Category"s; }

 private:
  std::string toStringID(Key key) const override { return key; }
  MatchingRecipes getMatchingRecipes() const override;
  Keys getKeysFromRecipe(const CRecipe &recipe) override;
  void populateEntry(Element &entry, Key key) const override;
};

class QualityFilter : public ListBasedFilter<Item::Quality> {
 public:
  std::string buttonText() const override { return "Quality"s; }

 private:
  std::string toStringID(Key key) const override { return toString(key); }
  MatchingRecipes getMatchingRecipes() const override;
  Keys getKeysFromRecipe(const CRecipe &recipe) override;
  void populateEntry(Element &entry, Key key) const override;
};
