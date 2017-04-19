#ifndef REMOTE_CLIENT_H
#define REMOTE_CLIENT_H

#include <string>
#include <Windows.h>

class RemoteClient{
public:
    RemoteClient(const std::string &args = "");
    ~RemoteClient();

private:
    STARTUPINFO _si;
    PROCESS_INFORMATION _pi;

    static const std::string
        CLIENT_BINARY_PATH,
        DEFAULT_ARGS;
};

#endif
