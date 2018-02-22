#include "Client.h"


void Client::initToasts() {
const auto
    TOAST_H = 30,
    TOAST_W = 200;

    auto x = (SCREEN_X - TOAST_W) / 2;
    _toasts = new List({ x, 0, TOAST_W, SCREEN_Y }, TOAST_H);
    addUI(_toasts);
}

struct Toast {
    Texture icon;
    std::string text;
};

std::list<Toast> toastInfo;

void Client::toast(const std::string &icon, const std::string &text) {
    Toast t;
    t.icon = _icons[icon];
    t.text = text;

    toastInfo.push_back(t);
    populateToastsList();
}

void Client::populateToastsList() {
    _toasts->clearChildren();

    for (auto &info : toastInfo) {
        auto toast = new Element();
        _toasts->addChild(toast);
        toast->height(toast->height() - 3);

        // Background
        toast->addChild(new ColorBlock({ 0, 0, toast->width(), toast->height() },
                Element::BACKGROUND_COLOR));
        toast->addChild(new ShadowBox({ 0, 0, toast->width(), toast->height() }));

        // Icon
        const auto gap = (toast->height() - info.icon.height()) / 2;
        toast->addChild(new Picture(gap, gap, info.icon));

        // Text
        const px_t
            textX = info.icon.width() + 2 * gap,
            textW = toast->width() - textX - gap;
        static auto wordWrapper = WordWrapper{ _defaultFont, textW };
        auto lines = wordWrapper.wrap(info.text);
        auto totalTextHeight = lines.size() * Element::TEXT_HEIGHT;
        auto y = (static_cast<int>(toast->height()) - static_cast<int>(totalTextHeight)) / 2;
        for (const auto &line : lines) {
            toast->addChild(new Label({ textX, y, textW, Element::TEXT_HEIGHT }, line,
                    Element::CENTER_JUSTIFIED));
            y += Element::TEXT_HEIGHT;
        }
    }
}
