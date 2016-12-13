#include "Client.h"
#include "ClientVehicle.h"

ClientVehicle::ClientVehicle(size_t serial, const ClientVehicleType *type, const Point &loc):
ClientObject(serial, type, loc){}

void ClientVehicle::mountOrDismount(void *object){
    const ClientVehicle &obj = *static_cast<const ClientVehicle *>(object);
    Client &client = *Client::_instance;
    client.sendMessage(CL_TOGGLE_MOUNT, makeArgs(obj.serial()));
}
