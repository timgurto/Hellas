# Econ-sim MMO
MMO economy simulator

&copy; 2015 Tim Gurto

## Contents
[Glossary of ambiguous terms](#glossary)  
[Command-line arguments](#arguments)  
[Client states](#states)  
[Login sequence](#login)  
[Message codes](#messages)  


<a id='glossary'></a>
## Glossary of ambiguous terms
**character**, the *avatar* which represents a user in-game

**class**, a category to which an item belongs, denoting functionality.  For example, to cut down a tree a user must have an "axe"-class item in his inventory.

**client**, the *program* which connects to the server

**item**, something which exists virtually in-game, located in players' inventories

**entity**, (client only) something which exists physically in-game, with a location and image, and supporting user interaction

**object**, (server only) something which exists physically in-game, with a location and functionality

**server**, the *program* which manages the state of the game world and connects to clients

**terminal**, a generic term for server or client

**user**, the *account* playing on a client.  Identified by a unique username.


<a id='arguments'></a>
## Command-line arguments

`-height `*`value`* the height of the window

`-left `*`value`* the x co-ordinate of the window

`-new` (server only) generate a new world instead of attempting to load existing data

`-server` run the program as a server (default is as client)

`-server-ip`*`value`* (client only) attempt to connect to server at specific IP address

`-server-port`*`value`* (client only) attempt to connect to server at specific port

`-top `*`value`* the y co-ordinate of the window

`-username `*`value`* the user as whom the client should log in

`-width `*`value`* the width of the window


<a id='states'></a>
## Client states
**Disconnected**, no socket connection.  Attempts to connect to server every 3s (`Client::CONNECT_RETRY_DELAY`).

**Connected**, socket connection exists but user has not been verified by server.  Client immediately attempts to log in.

**Logged in**, server has accepted user and begun sending data.

**Loaded**, client has received enough data to begin playing.

**Invalid username**, server has rejected client's username.  Until a login screen is added, this leaves the client in a zombie state.


<a id='login'></a>
## Login sequence
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
## Message codes
Detailed below are the types of messages which can be sent between client and server.
"Client requests" are sent from a client to the server, and "server commands" and "warnings and errors" are sent from the server to a client.
### Client requests
Code | Name                  | Syntax                     | Description
---: | --------------------- | -------------------------- | -----------
0    | CL_PING               | `[0]`                      | A ping, to measure latency and reassure the server
1    | CL_I_AM               | `[1,username]`             | "My name is `username`"
10   | CL_LOCATION           | `[10,x,y]`                 | "My location has changed, and is now (`x`,`y`)"
50   | CL_COLLECT_BRANCH     | `[50,serial]`              | "I want to collect branch #`serial`"
51   | CL_COLLECT_TREE       | `[51,serial]`              | "I want to collect tree #`serial`"

### Server commands
Code | Name                  | Syntax                     | Description
---: | --------------------- | -------------------------- | -----------
100  | SV_PING_REPLY         | `[100]`                    | A reply to a ping from a client
101  | SV_WELCOME            | `[101]`                    | "You have been successfully registered"
110  | SV_USER_DISCONNECTED  | `[110,username]`           | "User `username` has disconnected"
120  | SV_MAP_SIZE           | `[120,x,y]`                | "The map size is `x`&times;`y`"
121  | SV_TERRAIN            | `[121,x,y,n,t0,t1,t2,...]` | A package of map details.  "The `n` horizontal map tiles starting from (`x`,`y`) are of types `t0`, `t1`, `t2`, ..."
122  | SV_LOCATION           | `[122,username,x,y]`       | "User `username` is located at (`x`,`y`)"
123  | SV_INVENTORY          | `[123,slot,id,quantity]`   | "Your inventory slot #`slot` contains a stack of `quantity` items of type `id`"
150  | SV_BRANCH             | `[150,serial,x,y]`         | "Branch #`serial` is located at (`x`,`y`)"
151  | SV_TREE               | `[151,serial,x,y]`         | "Tree #`serial` is located at (`x`,`y`)"
152  | SV_REMOVE_BRANCH      | `[152,serial]`             | "Branch #`serial` no longer exists"
153  | SV_REMOVE_TREE        | `[153,serial]`             | "Tree #`serial` no longer exists"

### Warnings and errors
Code | Name                  | Syntax                     | Description
---: | --------------------- | -------------------------- | -----------
900  | SV_DUPLICATE_USERNAME | `[900]`                    | The client has attempted to connect with a username already in use
901  | SV_INVALID_USERNAME   | `[901]`                    | The client has attempted to connect with an invalid username
902  | SV_SERVER_FULL        | `[902]`                    | There is no room for more clients
910  | SV_SV_TOO_FAR         | `[910]`                    | "You are too far away to perform that action"
911  | SV_DOESNT_EXIST       | `[911]`                    | "The object you are trying to use does not exist"
912  | SV_INVENTORY_FULL     | `[912]`                    | "You cannot receive an item because your inventory is full"
950  | SV_AXE_NEEDED         | `[950]`                    | "You cannot cut down a tree without an axe"
