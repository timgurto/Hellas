#include <cassert>

#include "Client.h"
#include "ui/Label.h"

extern Renderer renderer;

void Client::initializeBuildWindow(){
    static const px_t
        BUTTON_WIDTH = 80,
        BUTTON_HEIGHT = Element::TEXT_HEIGHT,
        WIN_HEIGHT = toInt(7.5 * BUTTON_HEIGHT);

    _buildWindow = new Window(Rect(100, 50, BUTTON_WIDTH, WIN_HEIGHT), "Construction");
    _buildList = new ChoiceList(Rect(0, 0, BUTTON_WIDTH, WIN_HEIGHT), BUTTON_HEIGHT);
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
        listElement->addChild(new Label(Rect(2, 0, listElement->width(), listElement->height()),
                                       ot->name()));
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
