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

class FilterRecipesByMaterial : public CraftingWindowFilter {
 public:
  FilterRecipesByMaterial(const Client &client);
  std::string buttonText() const override { return "Material"s; }
  void indexRecipe(const CRecipe &recipe) override;
  void populateConfigurationPanel(Element &panel) const override;
  MatchingRecipes getMatchingRecipes() const override;

 private:
  using IndexedRecipes = std::multimap<const ClientItem *, const CRecipe *>;
  IndexedRecipes m_indexedRecipes;
  mutable ChoiceList *m_materialList{nullptr};
  const Client &m_client;
};

class FilterRecipesByTool : public CraftingWindowFilter {
 public:
  FilterRecipesByTool(const Client &client);
  std::string buttonText() const override { return "Tool"s; }
  void indexRecipe(const CRecipe &recipe) override;
  void populateConfigurationPanel(Element &panel) const override;
  MatchingRecipes getMatchingRecipes() const override;

 private:
  using IndexedRecipes = std::multimap<std::string, const CRecipe *>;
  IndexedRecipes m_indexedRecipes;
  mutable ChoiceList *m_toolsList{nullptr};
  const Client &m_client;
};

class FilterRecipesByLvlReq : public CraftingWindowFilter {
 public:
  std::string buttonText() const override { return "Level req"s; }
  void indexRecipe(const CRecipe &recipe) override;
  void populateConfigurationPanel(Element &panel) const override;
  MatchingRecipes getMatchingRecipes() const override;

 private:
  using IndexedRecipes = std::multimap<Level, const CRecipe *>;
  IndexedRecipes m_indexedRecipes;
  mutable TextBox *m_minLevel{nullptr}, *m_maxLevel{nullptr};
};
