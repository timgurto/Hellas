# Econ-sim MMO
MMO economy simulator

&copy; 2015 Tim Gurto

## Contents

#### Building
[Building in Visual Studio](#vs) 
[Building with GCC](#gcc)   

#### Running
[Command-line arguments](#arguments)  
[Keyboard controls](#keyboard)  

#### Client programming guide
[Glossary of ambiguous terms](#glossary) 
[Item classes](#classes)  
[Game data](#data)  
[Client states](#states)  
[Login sequence](#login)  
[Message codes](#messages)  

## Building
To clone this repository: `git clone --recursive https://github.com/timgurto/mmo`

<a id='vs'></a>
### Building in Visual Studio
1. Open mmo.sln in Visual Studio 2012 or later.
2. Build the project.

<a id='gcc'></a>
### Building with MinGW
1. Run `make`.

## Running

<a id='keyboard'></a>
### Keyboard shortcuts

`w`, `↑` Move up

`s`, `↓` Move down

`a`, `←` Move left

`d`, `→` Move right

`Esc` Exit any open UI window, or exit the client

`Enter` Open/close text-entry window, to send commands to the server

`[` Open text-entry window, and add a `[` character to it

`c` Open/close crafting window

`i` Open/close inventory


<a id='arguments'></a>
### Command-line arguments

#### Server arguments

`-new` generate a new world instead of attempting to load existing data

#### Client arguments

`-debug` (client only) displays additional information in the client, to assist with debugging

`-height `*`value`* the height of the window

`-left `*`value`* the x co-ordinate of the window

`-server` run the program as a server (default is as client)

`-server-ip`*`value`* (client only) attempt to connect to server at specific IP address

`-server-port`*`value`* (client only) attempt to connect to server at specific port

`-top `*`value`* the y co-ordinate of the window

`-username `*`value`* the user as whom the client should log in

`-width `*`value`* the width of the window


## Client programming guide

<a id='glossary'></a>
### Glossary of ambiguous terms
**character**, the *avatar* which represents a user in-game

**class**, a category to which an item belongs, denoting functionality.  For example, to cut down a tree a user must have an "axe"-class item in his inventory.  An item can have many classes or none at all.

**client**, the *program* which connects to the server

**item**, something which exists virtually in-game, located in players' inventories

**element**, (client only) a part of the GUI, possibly containing child elements

**entity**, (client only) something which exists physically in-game, with a location and image, and supporting user interaction

**gather**, when the user performs an action on an object in order to extract items from it.  Some objects can be gathered multiple times before they disappear.

**object**, something which exists physically in-game, with a location and functionality

**server**, the *program* which manages the state of the game world and connects to clients

**terminal**, a generic term for server or client

**user**, the *account* playing on a client.  Identified by a unique username.

<a id='classes'></a>
### Item classes
The classes themselves are not coded anywhere authoritative, but are deduced by inspecting the classes on each defined item.

**axe**, required to chop down trees

**container**, has an inventory that can store items.

**structure**, can be transformed into a physical object by "constructing" it

<a id='data'></a>
### Game data
The data files used by the server can be viewed here:
 - [Object types](Data/objectTypes.xml)
 - [Object types (client-side)](Data/objectTypesClient.xml)
 - [Items](Data/items.xml)

<a id='states'></a>
### Client states
**Disconnected**, no socket connection.  Attempts to connect to server every 3s (`Client::CONNECT_RETRY_DELAY`).

**Connected**, socket connection exists but user has not been verified by server.  Client immediately attempts to log in.

**Logged in**, server has accepted user and begun sending data.

**Loaded**, client has received enough data to begin playing.

**Invalid username**, server has rejected client's username.  Until a login screen is added, this leaves the client in a zombie state.


<a id='login'></a>
### Login sequence
Sequence | Server                                      | Client                                  | Client state
-------: | ------------------------------------------- | --------------------------------------- | ----------------
1        | `Socket::bind()`                            |                                         | Disconnected
2        | `Socket::listen()`                          |                                         | 
3        | `select()` (looped)                         |                                         | 
4        |                                             | `connect()`                             | 
5        | `accept()`<sup>1</sup>                      |                                         | Connected
6        |                                             | Send `CL_I_AM`                          | 
7        | Receive `CL_I_AM`<sup>2</sup>               |                                         | 
8        | Send `SV_WELCOME`                           |                                         | 
9        | Send `SV_MAP_SIZE`                          |                                         | 
10       | Send `SV_LOCATION` for every other user     |                                         | 
11       | Send `SV_BRANCH` for each branch            |                                         | 
12       | Send `SV_INVENTORY` for each inventory item |                                         | 
13       | Send `SV_LOCATION` for user's location      |                                         | 
14       |                                             | Receive `SV_WELCOME`                    | Logged in
15       |                                             | Receive `SV_MAP_SIZE`                   | 
16       |                                             | Receive others' `SV_LOCATION` messages  | 
17       |                                             | Receive `SV_BRANCH` messages            | 
18       |                                             | Receive `SV_INVENTORY` messages         | 
19       |                                             | Receive own `SV_LOCATION`               | Loaded
         |                                             |                                         | 
         | **Possible error cases:**                   |                                         |
1        | <sup>1</sup> Send `SV_SERVER_FULL`          |                                         | 
2        | Wait 5s                                     | Receive `SV_SERVER_FULL`                | Disconnected
3        |`closesocket()`                              |                                         | 
         |                                             |                                         | 
1        | <sup>2</sup> Send `SV_DUPLICATE_USERNAME`   |                                         | 
2        |                                             | Receive `SV_DUPLICATE_USERNAME`         | Invalid username
         |                                             |                                         | 
1        | <sup>2</sup> Send `SV_INVALID_USERNAME`     |                                         | 
2        |                                             | Receive `SV_INVALID_USERNAME`           | Invalid username


<a id='messages'></a>
### Message codes
Detailed below are the types of messages which can be sent between client and server.
"Client requests" are sent from a client to the server, and "server commands" and "warnings and errors" are sent from the server to a client.
#### Client requests
Code | Name                  | Syntax                     | Description
---: | --------------------- | -------------------------- | -----------
0    | CL_PING               | `[0]`                      | A ping, to measure latency and reassure the server
1    | CL_I_AM               | `[1,username]`             | "My name is `username`"
10   | CL_LOCATION           | `[10,x,y]`                 | "My location has changed, and is now (`x`,`y`)"
20   | CL_CANCEL_ACTION      | `[20]`                     | "I don't want to finish my current action"
21   | CL_CRAFT              | `[21,id]`                  | "I want to craft an item using recipe `id`"
22   | CL_CONSTRUCT          | `[22,slot,x,y]`            | "I want to construct the item in inventory slot #`slot`, at location (`x`,`y`)""
23   | CL_GATHER             | `[23,serial]`              | "I want to gather object #`serial`
24   | CL_DROP               | `[24,slot]`                | "I want to drop the item in inventory slot #`slot`

#### Server commands
Code | Name                  | Syntax                     | Description
---: | --------------------- | -------------------------- | -----------
100  | SV_PING_REPLY         | `[100]`                    | A reply to a ping from a client
101  | SV_WELCOME            | `[101]`                    | "You have been successfully registered"
110  | SV_USER_DISCONNECTED  | `[110,username]`           | "User `username` has disconnected"
120  | SV_MAP_SIZE           | `[120,x,y]`                | "The map size is `x`&times;`y`"
121  | SV_TERRAIN            | `[121,x,y,n,t0,t1,t2,...]` | A package of map details.  "The `n` horizontal map tiles starting from (`x`,`y`) are of types `t0`, `t1`, `t2`, ..."
122  | SV_LOCATION           | `[122,username,x,y]`       | "User `username` is located at (`x`,`y`)"
123  | SV_INVENTORY          | `[123,slot,type,quantity]` | "Your inventory slot #`slot` contains a stack of `quantity` `type`s"
124  | SV_OBJECT             | `[124,serial,x,y,type]`    | "Object #`serial` is located at (`x`,`y`), and is a `type`"
125  | SV_REMOVE_OBJECT      | `[125,serial]`             | "Object #`serial` no longer exists"
130  | SV_ACTION_STARTED     | `[130,time]`               | "You have begun an action that will take `t` milliseconds"
131  | SV_ACTION_FINISHED    | `[131]`                    | "You have completed an action"
150  | SV_OWNER              | `[150,serial,owner]`       | "Object #`serial` is owned by `owner`"

#### Warnings and errors
Code | Name                  | Syntax                     | Description
---: | --------------------- | -------------------------- | -----------
900  | SV_DUPLICATE_USERNAME | `[900]`                    | The client has attempted to connect with a username already in use
901  | SV_INVALID_USERNAME   | `[901]`                    | The client has attempted to connect with an invalid username
902  | SV_SERVER_FULL        | `[902]`                    | There is no room for more clients
910  | SV_TOO_FAR            | `[910]`                    | "You are too far away to perform that action"
911  | SV_DOESNT_EXIST       | `[911]`                    | "The object you are trying to use does not exist"
912  | SV_INVENTORY_FULL     | `[912]`                    | "You cannot receive an item because your inventory is full"
913  | SV_NEED_MATERIALS     | `[913]`                    | "You do not have enough materials to craft that item"
914  | SV_INVALID_ITEM       | `[914]`                    | "You tried to craft an item that does not exist"
915  | SV_CANNOT_CRAFT       | `[915]`                    | "You tried to craft an item that cannot be crafted"
916  | SV_ACTION_INTERRUPTED | `[916]`                    | "Action interrupted"
917  | SV_EMPTY_SLOT         | `[917]`                    | "You tried to manipulate an empty inventory slot"
918  | SV_INVALID_SLOT       | `[918]`                    | "You tried to manipulate an inventory slot that does not exist"
919  | SV_CANNOT_CONSTRUCT   | `[919]`                    | "You tried to construct an item that is not a structure"
920  | SV_ITEM_NEEDED        | `[920,reqItemClass]`       | "You tried to perform an action, without the necessary `reqItemClass`"
921  | SV_BLOCKED            | `[921]`                    | "You tried to perform an action at a location that is blocked"
