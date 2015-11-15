# Login sequence
[Home](index.md)

Sequence | Server                                      | Client                                  | [Client state](clientStates.md)
-------: | ------------------------------------------- | --------------------------------------- | ----------------
1        | `Socket::bind()`                            |                                         | Disconnected
2        | `Socket::listen()`                          |                                         | 
3        | `select()` (looped)                         |                                         | 
4        |                                             | `connect()`                             | 
5        | `accept()`                                  |                                         | Connected
6        |                                             | Send `CL_I_AM`                          | 
7        | Receive `CL_I_AM`                           |                                         | 
8        | Send `SV_WELCOME`                           |                                         | 
9        | Send `SV_MAP_SIZE`                          |                                         | 
10       | Send `SV_LOCATION` for every other user     |                                         | 
11       | Send `SV_OBJECT` for each object            |                                         | 
12       | Send `SV_INVENTORY` for each inventory item |                                         | 
13       | Send `SV_LOCATION` for user's location      |                                         | 
14       |                                             | Receive `SV_WELCOME`                    | Logged in
15       |                                             | Receive `SV_MAP_SIZE`                   | 
16       |                                             | Receive others' `SV_LOCATION` messages  | 
17       |                                             | Receive `SV_OBJECT` messages            | 
18       |                                             | Receive `SV_INVENTORY` messages         | 
19       |                                             | Receive own `SV_LOCATION`               | Loaded

### Possible error cases

#### Server is full
Sequence | Server                                      | Client                                  | Client state
-------: | ------------------------------------------- | --------------------------------------- | ----------------
5        | `accept()`                                  |                                         | Connected
6        | Send `SV_SERVER_FULL`                       |                                         | 
7        | Wait 5s                                     | Receive `SV_SERVER_FULL`                | Disconnected
8        |`closesocket()`                              |                                         | 

#### The user is already logged in
Sequence | Server                                      | Client                                  | Client state
-------: | ------------------------------------------- | --------------------------------------- | ----------------
7        | Receive `CL_I_AM`                           |                                         | 
8        | Send `SV_DUPLICATE_USERNAME`                |                                         | 
9        |                                             | Receive `SV_DUPLICATE_USERNAME`         | Invalid username

#### The user has used an invalid username
Sequence | Server                                      | Client                                  | Client state
-------: | ------------------------------------------- | --------------------------------------- | ----------------
7        | Receive `CL_I_AM`                           |                                         | 
8        | Send `SV_INVALID_USERNAME`                  |                                         | 
9        |                                             | Receive `SV_INVALID_USERNAME`           | Invalid username

See also: [Client states](clientStates.md), [Message codes](messages.md)
