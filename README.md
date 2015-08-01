# Econ-sim MMO
MMO economy simulator
&copy; 2015 Tim Gurto

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

## Client states
**Disconnected**, no socket connection.  Attempts to connect to server every 3s (`Client::CONNECT_RETRY_DELAY`).

**Connected**, socket connection exists but user has not been verified by server.  Client immediately attempts to log in.

**Logged in**, server has accepted user and begun sending data.

**Loaded**, client has received enough data to begin playing.

**Invalid username**, server has rejected client's username.  Until a login screen is added, this leaves the client in a zombie state.

## Login process
Sequence | Server | Client | Client state
-----: | ------ | ------ | ------
1 | `Socket::bind()` | | Disconnected
2 | `Socket::listen()` | | 
3 | `select()` (looped) | | 
4 | | `connect()` | 
5 | `accept()`<sup>1</sup> | | Connected
6 | | Send `CL_I_AM` | 
7 | Receive `CL_I_AM`<sup>2</sup> | | 
8 | Send `SV_WELCOME` |  | 
9 | Send `SV_MAP_SIZE` |  | 
10 | Send `SV_LOCATION` for every other user |  | 
11 | Send `SV_BRANCH` for each branch |  | 
12 | Send `SV_INVENTORY` for each inventory item |  | 
13 | Send `SV_LOCATION` for user's location |  | 
14 | | Receive `SV_WELCOME` | Logged in
15 | | Receive `SV_MAP_SIZE` | 
16 | | Receive others' `SV_LOCATION` messages | 
17 | | Receive `SV_BRANCH` messages | 
18 | | Receive `SV_INVENTORY` messages | 
19 | | Receive own `SV_LOCATION` | Loaded
 | | | 
 | **Possible error cases:** | |
1 |<sup>1</sup> Send `SV_SERVER_FULL` | | 
2 |Wait 5s | Receive `SV_SERVER_FULL` | Disconnected
3 |`closesocket()` | | 
 | | | 
1 |<sup>2</sup> Send `SV_DUPLICATE_USERNAME` | | 
2 | | Receive `SV_DUPLICATE_USERNAME` | Invalid username
 | | | 
1 |<sup>2</sup> Send `SV_INVALID_USERNAME` | | 
2 | | Receive `SV_INVALID_USERNAME` | Invalid username

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
