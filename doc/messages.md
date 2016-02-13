# Message codes
[Home](index.md)

Detailed here are the types of messages which can be sent between client and server.
"Client requests" are sent from a client to the server, and "server commands" and "warnings and errors" are sent from the server to a client.
#### Client requests
Code | Name                    | Syntax                             | Description
---: | ----------------------- | ---------------------------------- | -----------
0    | `CL_PING`               | `[0]`                              | A ping, to measure latency and reassure the server
1    | `CL_I_AM`               | `[1,username]`                     | "My name is `username`"
10   | `CL_LOCATION`           | `[10,x,y]`                         | "I want to move to (`x`,`y`)"
20   | `CL_CANCEL_ACTION`      | `[20]`                             | "I don't want to finish my current action"
21   | `CL_CRAFT`              | `[21,id]`                          | "I want to craft an item using recipe `id`"
22   | `CL_CONSTRUCT`          | `[22,slot,x,y]`                    | "I want to construct the item in inventory slot #`slot`, at location (`x`,`y`)"
23   | `CL_GATHER`             | `[23,serial]`                      | "I want to gather object #`serial`"
24   | `CL_DROP`               | `[24,slot]`                        | "I want to drop the item in inventory slot #`slot`"
25   | `CL_SWAP_ITEMS`         | `[25,slot1,slot2]`                 | "I want to swap the items in inventory slots #`slot1` and #`slot2`"

#### Server commands                                        
Code | Name                    | Syntax                     | Description
---: | ----------------------- | -------------------------- | -----------
100  | `SV_PING_REPLY`         | `[100]`                    | A reply to a ping from a client
101  | `SV_WELCOME`            | `[101]`                    | "You have been successfully registered"
110  | `SV_USER_DISCONNECTED`  | `[110,username]`           | "User `username` has disconnected"
120  | `SV_MAP_SIZE`           | `[120,x,y]`                | "The map size is `x`&times;`y`"
121  | `SV_TERRAIN`            | `[121,x,y,n,t0,t1,t2,...]` | A package of map details.  "The `n` horizontal map tiles starting from (`x`,`y`) are of types `t0`, `t1`, `t2`, ..."
122  | `SV_LOCATION`           | `[122,username,x,y]`       | "User `username` is located at (`x`,`y`)"
123  | `SV_INVENTORY`          | `[123,slot,type,quantity]` | "Your inventory slot #`slot` contains a stack of `quantity` `type`s"
124  | `SV_OBJECT`             | `[124,serial,x,y,type]`    | "Object #`serial` is located at (`x`,`y`), and is a `type`"
125  | `SV_REMOVE_OBJECT`      | `[125,serial]`             | "Object #`serial` no longer exists"
130  | `SV_ACTION_STARTED`     | `[130,time]`               | "You have begun an action that will take `t` milliseconds"
131  | `SV_ACTION_FINISHED`    | `[131]`                    | "You have completed an action"
150  | `SV_OWNER`              | `[150,serial,owner]`       | "Object #`serial` is owned by `owner`"

#### Warnings and errors                                          
Code | Name                    | Syntax               | Description
---: | ----------------------- | -------------------- | -----------
900  | `SV_DUPLICATE_USERNAME` | `[900]`              | The client has attempted to connect with a username already in use
901  | `SV_INVALID_USERNAME`   | `[901]`              | The client has attempted to connect with an invalid username
902  | `SV_SERVER_FULL`        | `[902]`              | There is no room for more clients
910  | `SV_TOO_FAR`            | `[910]`              | "You are too far away to perform that action"
911  | `SV_DOESNT_EXIST`       | `[911]`              | "The object you are trying to use does not exist"
912  | `SV_INVENTORY_FULL`     | `[912]`              | "You cannot receive an item because your inventory is full"
913  | `SV_NEED_MATERIALS`     | `[913]`              | "You do not have enough materials to craft that item"
914  | `SV_INVALID_ITEM`       | `[914]`              | "You tried to craft an item that does not exist"
915  | `SV_CANNOT_CRAFT`       | `[915]`              | "You tried to craft an item that cannot be crafted"
916  | `SV_ACTION_INTERRUPTED` | `[916]`              | "Action interrupted"
917  | `SV_EMPTY_SLOT`         | `[917]`              | "You tried to manipulate an empty inventory slot"
918  | `SV_INVALID_SLOT`       | `[918]`              | "You tried to manipulate an inventory slot that does not exist"
919  | `SV_CANNOT_CONSTRUCT`   | `[919]`              | "You tried to construct an item that is not a structure"
920  | `SV_ITEM_NEEDED`        | `[920,reqItemClass]` | "You tried to perform an action, without the necessary `reqItemClass`"
921  | `SV_BLOCKED`            | `[921]`              | "You tried to perform an action at a location that is blocked"
922  | `SV_NEED_TOOLS`         | `[922]`              | "You tried to craft an item, but need additional tools"
