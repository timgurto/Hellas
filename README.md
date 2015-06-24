# Econ-sim MMO
MMO economy simulator

## Command-line arguments

`-height `*`value`* the height of the window

`-left `*`value`* the x co-ordinate of the window

`-server` run the program as a server (default is as client)

`-server-ip`*`value`* for a client, attempt to connect to server at specific IP address

`-server-port`*`value`* for a client, attempt to connect to server at specific port

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
Server | Client | Client state
------ | ------ | ------
`Socket::bind()` | | Disconnected
`Socket::listen()` | | 
`select()` (looped) | | 
 | `connect()` | 
`accept()`<sup>1</sup> | | Connected
 | Send `CL_I_AM` | 
Receive `CL_I_AM`<sup>2</sup> | | 
Send `SV_WELCOME` |  | 
Send `SV_LOCATION` for every other user |  | 
Send `SV_BRANCH` for each branch |  | 
Send `SV_INVENTORY` for each inventory item |  | 
Send `SV_LOCATION` for user's location |  | 
 | Receive `SV_WELCOME` | Logged in
 | Receive others' `SV_LOCATION` messages | 
 | Receive `SV_BRANCH` messages | 
 | Receive `SV_INVENTORY` messages | 
 | Receive own `SV_LOCATION` | Loaded
 | | 
**Possible error cases:** | |
<sup>1</sup> Send `SV_SERVER_FULL` | | 
Wait 5s | Receive `SV_SERVER_FULL` | Disconnected
`closesocket()` | | 
 | | 
<sup>2</sup> Send `SV_DUPLICATE_USERNAME` | | 
 | Receive `SV_DUPLICATE_USERNAME` | Invalid username
 | | 
<sup>2</sup> Send `SV_INVALID_USERNAME` | | 
 | Receive `SV_INVALID_USERNAME` | Invalid username

## Glossary of ambiguous terms
**character**, the *avatar* which represents a user in-game

**client**, the *program* which connects to the server

**server**, the *program* which manages the state of the game world and connects to clients

**terminal**, a generic term for server or client

**user**, the *account* playing on a client.  Identified by a unique username.
