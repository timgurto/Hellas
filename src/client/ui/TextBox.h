#ifndef TEXT_BOX_H
#define TEXT_BOX_H

#include <string>

#include "../../Optional.h"
#include "Element.h"
#include "LinkedLabel.h"

class TextBox : public Element {
 public:
  enum ValidInput { ALL, NUMERALS, LETTERS };

  TextBox(Client &client, const ScreenRect &rect, ValidInput validInput = ALL);

  const std::string &text() const { return _text; }
  void text(const std::string &text);
  bool hasText() const { return !_text.empty(); }
  size_t textAsNum() const;

  class Focus {
   public:
    Focus &operator=(TextBox *rhs);
    operator TextBox *() { return _focus; }
    TextBox *operator->() { return _focus; }
    operator bool() const { return _focus != nullptr; }

   private:
    TextBox *_focus{nullptr};
  };

  using OnChangeFunction = void (*)(void *);
  void setOnChange(OnChangeFunction function, void *data = nullptr);

  static void addText(Client &client, const char *newText);
  static void backspace(Client &client);

  virtual void refresh();

  static void click(Element &e, const ScreenPoint &mousePos);

  void forcePascalCase();
  void maskContents(char mask = '?') { _inputMask = mask; }

 private:
  std::string _text;
  Optional<char> _inputMask;

  ValidInput _validInput;
  bool isInputValid(char c) const;

  OnChangeFunction _onChangeFunction{nullptr};
  void *_onChangeData{nullptr};
  void onChange();

  static const size_t MAX_TEXT_LENGTH{200};

  static const px_t HEIGHT{14};
};

#endif
