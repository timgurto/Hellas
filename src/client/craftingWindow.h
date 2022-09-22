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

  template <typename T>
  static MatchingRecipes recipesMatching(
      const std::multimap<T, const CRecipe *> &indexedRecipes, T key) {
    auto recipes = MatchingRecipes{};
    auto startIt = indexedRecipes.lower_bound(key);
    auto endIt = indexedRecipes.upper_bound(key);
    for (auto it = startIt; it != endIt; ++it) recipes.insert(it->second);
    return recipes;
  }
};

class MaterialFilter : public CraftingWindowFilter {
 public:
  MaterialFilter(const Client &client);
  std::string buttonText() const override { return "Material"s; }
  void indexRecipe(const CRecipe &recipe) override;
  void populateConfigurationPanel(Element &panel) const override;
  MatchingRecipes getMatchingRecipes() const override;

 private:
  using IndexedRecipes = std::multimap<const ClientItem *, const CRecipe *>;
  IndexedRecipes m_indexedRecipes;
  mutable ChoiceList *m_list{nullptr};
  const Client &m_client;
};

class ToolFilter : public CraftingWindowFilter {
 public:
  ToolFilter(const Client &client);
  std::string buttonText() const override { return "Tool req."s; }
  void indexRecipe(const CRecipe &recipe) override;
  void populateConfigurationPanel(Element &panel) const override;
  MatchingRecipes getMatchingRecipes() const override;

 private:
  using IndexedRecipes = std::multimap<std::string, const CRecipe *>;
  IndexedRecipes m_indexedRecipes;
  mutable ChoiceList *m_list{nullptr};
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

class CategoryFilter : public CraftingWindowFilter {
 public:
  std::string buttonText() const override { return "Category"s; }
  void indexRecipe(const CRecipe &recipe) override;
  void populateConfigurationPanel(Element &panel) const override;
  MatchingRecipes getMatchingRecipes() const override;

 private:
  using IndexedRecipes = std::multimap<std::string, const CRecipe *>;
  IndexedRecipes m_indexedRecipes;
  mutable ChoiceList *m_list{nullptr};
};

class QualityFilter : public CraftingWindowFilter {
 public:
  std::string buttonText() const override { return "Quality"s; }
  void indexRecipe(const CRecipe &recipe) override;
  void populateConfigurationPanel(Element &panel) const override;
  MatchingRecipes getMatchingRecipes() const override;

 private:
  using IndexedRecipes = std::multimap<Item::Quality, const CRecipe *>;
  IndexedRecipes m_indexedRecipes;
  mutable ChoiceList *m_list{nullptr};
};
