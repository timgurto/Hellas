# Message codes
[Home](index.md)

Detailed here are the types of messages which can be sent between client and server.
"Client requests" are sent from a client to the server, and "server commands" and "warnings and errors" are sent from the server to a client.

The message text itself is divided by ASCII control characters; they will be represented below by the following symbols:

Symbol  | ASCII value  | Meaning
------- | ------------ | --------
&laquo; | `0x02` (STX) | Start message
&raquo; | `0x03` (ETX) | End message
&#8226; | `0x1F` (US)  | Value delimiter

#### Client requests
Code | Name                     | Syntax                                                               | Description
---: | ------------------------ | -------------------------------------------------------------------- | -----------
0    | `CL_PING`                | &laquo;0&raquo;                                                      | A ping, to measure latency and reassure the server
1    | `CL_I_AM`                | &laquo;1&#8226;username&raquo;                                       | "My name is `username`"
10   | `CL_LOCATION`            | &laquo;10&#8226;x&#8226;y&raquo;                                     | "I want to move to (`x`,`y`)"
20   | `CL_CANCEL_ACTION`       | &laquo;20&raquo;                                                     | "I don't want to finish my current action"
21   | `CL_CRAFT`               | &laquo;21&#8226;id&raquo;                                            | "I want to craft an item using recipe `id`"
22   | `CL_CONSTRUCT`           | &laquo;22&#8226;slot&#8226;x&#8226;y&raquo;                          | "I want to construct the item in inventory slot #`slot`, at location (`x`,`y`)"
23   | `CL_GATHER`              | &laquo;23&#8226;serial&raquo;                                        | "I want to gather object #`serial`"
24   | `CL_DECONSTRUCT`         | &laquo;23&#8226;serial&raquo;                                        | "I want to deconstruct object #`serial`"
30   | `CL_DROP`                | &laquo;30&#8226;serial&#8226;slot&raquo;                             | "I want to drop object #`serial`'s item #`slot`."<br>A serial of `0` uses the user's inventory.
31   | `CL_SWAP_ITEMS`          | &laquo;31&#8226;serial1&#8226;slot1&#8226;serial2&#8226;slot2&raquo; | "I want to swap object #`serial1`'s item #`slot1` with object #`serial2`'s item #`slot2`"<br>A serial of `0` uses the user's inventory.
32   | `CL_SET_MERCHANT_SLOT`   | &laquo;32&#8226;serial&#8226;slot&#8226;ware&#8226;wareQty&#8226;price&#8226;priceQty&raquo; | "I want to set object #`serial`'s merchant slot #`slot` to sell `wareQty` `ware`s for `priceQty` `price`s"
33   | `CL_CLEAR_MERCHANT_SLOT` | &laquo;33&#8226;serial&#8226;slot                                    | "I want to clear object #`serial`'s merchant slot #`slot`.
40   | `CL_GET_INVENTORY`       | &laquo;40&#8226;serial&raquo;                                        | "Tell me what object #`serial`'s container holds."
60   | `CL_SAY`                 | &laquo;60&#8226;message&raquo;                                       | "I want to say '`message`' to everybody."
61   | `CL_WHISPER`             | &laquo;61&#8226;username&#8226;message&raquo;                        | "I want to say '`message`' to user `username`."

#### Server commands                                        
Code | Name                    | Syntax                                                                         | Description
---: | ----------------------- | ------------------------------------------------------------------------------ | -----------
100  | `SV_PING_REPLY`         | &laquo;100&raquo;                                                              | A reply to a ping from a client
101  | `SV_WELCOME`            | &laquo;101&raquo;                                                              | "You have been successfully registered"
110  | `SV_USER_DISCONNECTED`  | &laquo;110&#8226;username&raquo;                                               | "User `username` has disconnected"
120  | `SV_MAP_SIZE`           | &laquo;120&#8226;x&#8226;y&raquo;                                              | "The map size is `x`&times;`y`"
121  | `SV_TERRAIN`            | &laquo;121&#8226;x&#8226;y&#8226;n&#8226;t0&#8226;t1&#8226;t2&#8226;...&raquo; | A package of map details.  "The `n` horizontal map tiles starting from (`x`,`y`) are of types `t0`, `t1`, ..."
122  | `SV_LOCATION`           | &laquo;122&#8226;username&#8226;x&#8226;y&raquo;                               | "User `username` is located at (`x`,`y`)"
123  | `SV_INVENTORY`          | &laquo;123&#8226;slot&#8226;type&#8226;quantity&raquo;                         | "Your inventory slot #`slot` contains a stack of `quantity` `type`s"
124  | `SV_OBJECT`             | &laquo;124&#8226;serial&#8226;x&#8226;y&#8226;type&raquo;                      | "Object #`serial` is located at (`x`,`y`), and is a `type`"
125  | `SV_REMOVE_OBJECT`      | &laquo;125&#8226;serial&raquo;                                                 | "Object #`serial` no longer exists"
126  | `SV_MERCHANT_SLOT`      | &laquo;32&#8226;serial&#8226;slot&#8226;ware&#8226;wareQty&#8226;price&#8226;priceQty&raquo; | "Object #`serial`'s merchant slot #`slot` is selling `wareQty` `ware`s for `priceQty` `price`s"
130  | `SV_ACTION_STARTED`     | &laquo;130&#8226;time&raquo;                                                   | "You have begun an action that will take `t` milliseconds"
131  | `SV_ACTION_FINISHED`    | &laquo;131&raquo;                                                              | "You have completed an action"
150  | `SV_OWNER`              | &laquo;150&#8226;serial&#8226;owner&raquo;                                     | "Object #`serial` is owned by `owner`"
200  | `SV_SAY`                | &laquo;61&#8226;username&#8226;message&raquo;                                  | "User `username` said '`message`'."
201  | `SV_WHISPER`            | &laquo;61&#8226;username&#8226;message&raquo;                                  | "User `username` said '`message` to you'."

#### Warnings and errors                                          
Code | Name                       | Syntax                               | Description
---: | -------------------------- | ------------------------------------ | -----------
900  | `SV_DUPLICATE_USERNAME`    | &laquo;900&raquo;                    | The client has attempted to connect with a username already in use
901  | `SV_INVALID_USERNAME`      | &laquo;901&raquo;                    | The client has attempted to connect with an invalid username
902  | `SV_SERVER_FULL`           | &laquo;902&raquo;                    | There is no room for more clients
903  | `SV_INVALID_USER`          | &laquo;903&raquo;                    | "You tried to interact with a nonexistent user"
910  | `SV_TOO_FAR`               | &laquo;910&raquo;                    | "You are too far away to perform that action"
911  | `SV_DOESNT_EXIST`          | &laquo;911&raquo;                    | "The object you are trying to use does not exist"
912  | `SV_INVENTORY_FULL`        | &laquo;912&raquo;                    | "You cannot receive an item because your inventory is full"
913  | `SV_NEED_MATERIALS`        | &laquo;913&raquo;                    | "You do not have enough materials to craft that item"
914  | `SV_INVALID_ITEM`          | &laquo;914&raquo;                    | "You tried to craft an item that does not exist"
915  | `SV_CANNOT_CRAFT`          | &laquo;915&raquo;                    | "You tried to craft an item that cannot be crafted"
916  | `SV_ACTION_INTERRUPTED`    | &laquo;916&raquo;                    | "Action interrupted"
917  | `SV_EMPTY_SLOT`            | &laquo;917&raquo;                    | "You tried to manipulate an empty inventory slot"
918  | `SV_INVALID_SLOT`          | &laquo;918&raquo;                    | "You tried to manipulate an inventory slot that does not exist"
919  | `SV_CANNOT_CONSTRUCT`      | &laquo;919&raquo;                    | "You tried to construct an item that is not a structure"
920  | `SV_ITEM_NEEDED`           | &laquo;920&#8226;reqItemClass&raquo; | "You tried to perform an action, without the necessary `reqItemClass`"
921  | `SV_BLOCKED`               | &laquo;921&raquo;                    | "You tried to perform an action at a location that is blocked"
922  | `SV_NEED_TOOLS`            | &laquo;922&raquo;                    | "You tried to craft an item, but need additional tools"
923  | `SV_CANNOT_DECONSTRUCT`    | &laquo;923&raquo;                    | "You tried to deconstruct an object that cannot be deconstructed"
924  | `SV_NO_PERMISSION`         | &laquo;924&raquo;                    | "You don't have permission to do that"
925  | `SV_NOT_MERCHANT`          | &laquo;924&raquo;                    | "You tried to use a non-merchant object as a merchant"
926  | `SV_INVALID_MERCHANT_SLOT` | &laquo;924&raquo;                    | "You tried to use a merchant object's invalid merchant slot"
