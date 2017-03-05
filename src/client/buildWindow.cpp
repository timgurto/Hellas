#include <cassert>

#include "Client.h"
#include "TooltipBuilder.h"
#include "ui/CheckBox.h"
#include "ui/Label.h"
#include "ui/Line.h"

extern Renderer renderer;

void Client::initializeBuildWindow(){
    static const px_t
        BUTTON_WIDTH = 100,
        BUTTON_HEIGHT = Element::TEXT_HEIGHT,
        WIN_HEIGHT = toInt(10.5 * BUTTON_HEIGHT);

    _buildWindow = new Window(Rect(100, 50, BUTTON_WIDTH, WIN_HEIGHT), "Construction");
    static const px_t GAP = 2;
    _buildWindow->addChild(new CheckBox(Rect(0, GAP, BUTTON_WIDTH, Element::TEXT_HEIGHT),
                                        _multiBuild, "Build multiple"));
    px_t y = Element::TEXT_HEIGHT + GAP;
    _buildWindow->addChild(new Line(0, y, BUTTON_WIDTH));
    y += GAP;
    _buildList = new ChoiceList(Rect(0, y, BUTTON_WIDTH, WIN_HEIGHT - y), BUTTON_HEIGHT);
    _buildWindow->addChild(_buildList);
    _buildList->setPreRefreshFunction(populateBuildList, _buildList);
}

void Client::populateBuildList(Element &e){
    const Client &client = *Client::_instance;
    ChoiceList &list = dynamic_cast<ChoiceList &>(e);
    list.clearChildren();
    for (const std::string &id : client._knownConstructions){
        const ClientObjectType *ot = *client._objectTypes.find(&ClientObjectType(id));
        Element *listElement = new Element(Rect());
        list.addChild(listElement);
        Label *label = new Label(Rect(2, 0, listElement->width(), listElement->height()),
                                 ot->name());
        label->setTooltip(ot->materialsTooltip());
        listElement->addChild(label);
        listElement->setLeftMouseUpFunction(chooseConstruction);
        listElement->id(id);
    }
    list.verifyBoxes();
}

void Client::chooseConstruction(Element &e, const Point &mousePos){
    Client &client = *Client::_instance;
    const std::string selectedID = client._buildList->getSelected();
    if (selectedID.empty()){
        client._selectedConstruction = nullptr;
        client._constructionFootprint = Texture();
    } else {
        const ClientObjectType *ot = *client._objectTypes.find(&ClientObjectType(selectedID));
        client._selectedConstruction = ot;
        client._constructionFootprint = ot->image();
    }
}
