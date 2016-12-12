#ifndef AUTHENTICATION_H
#define AUTHENTICATION_H
#include <string>

struct AuthStruct {
    std::uint64_t sessionId;
    std::string login;
    std::string password;
};

#endif // AUTHENTICATION_H

