#pragma once

#include <map>

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
    m_list =
        new ChoiceList(panel.rectToFill(), Client::ICON_SIZE, *panel.client());
    panel.addChild(m_list);
  }

 protected:
  using Keys = std::set<K>;
  virtual Keys getKeysFromRecipe(const CRecipe &recipe) = 0;

  Keys uniqueIndexedKeys() const {
    Keys uniqueKeys;
    for (const auto &pair : m_indexedRecipes) uniqueKeys.insert(pair.first);
    return uniqueKeys;
  }

  IndexedRecipes m_indexedRecipes;
  mutable ChoiceList *m_list{nullptr};
};

class MaterialFilter : public ListBasedFilter<ClientItemAlphabetical> {
 public:
  MaterialFilter(const Client &client);
  std::string buttonText() const override { return "Material"s; }
  void populateConfigurationPanel(Element &panel) const override;
  MatchingRecipes getMatchingRecipes() const override;
  std::set<ClientItemAlphabetical> getKeysFromRecipe(
      const CRecipe &recipe) override;

 private:
  const Client &m_client;
};

class ToolFilter : public ListBasedFilter<std::string> {
 public:
  ToolFilter(const Client &client);
  std::string buttonText() const override { return "Tool req."s; }
  void populateConfigurationPanel(Element &panel) const override;
  MatchingRecipes getMatchingRecipes() const override;
  std::set<std::string> getKeysFromRecipe(const CRecipe &recipe) override;

 private:
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
  void populateConfigurationPanel(Element &panel) const override;
  MatchingRecipes getMatchingRecipes() const override;
  std::set<std::string> getKeysFromRecipe(const CRecipe &recipe) override;
};

class QualityFilter : public ListBasedFilter<Item::Quality> {
 public:
  std::string buttonText() const override { return "Quality"s; }
  void populateConfigurationPanel(Element &panel) const override;
  MatchingRecipes getMatchingRecipes() const override;
  std::set<Item::Quality> getKeysFromRecipe(const CRecipe &recipe) override;
};
