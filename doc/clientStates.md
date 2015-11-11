# Client states
[Home](index.md)

**Disconnected**, no socket connection.  Attempts to connect to server every 3s (`Client::CONNECT_RETRY_DELAY`).

**Connected**, socket connection exists but user has not been verified by server.  Client immediately attempts to log in.

**Logged in**, server has accepted user and begun sending data.

**Loaded**, client has received enough data to begin playing.

**Invalid username**, server has rejected client's username.  Until a login screen is added, this leaves the client in a zombie state.

See also: [Login sequence](login.md)
