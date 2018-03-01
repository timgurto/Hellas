#pragma once

class Button;
class ClientItem;

struct ClientObjectAction {
    std::string label;
    std::string tooltip;
    std::string textInput;
    const ClientItem *cost{ nullptr };
};
