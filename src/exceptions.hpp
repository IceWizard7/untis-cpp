#pragma once

#include <stdexcept>
#include <string>

class WebUntisAPIError : public std::runtime_error {
public:
    explicit WebUntisAPIError(const std::string& msg)
        : std::runtime_error(msg) {}
};


class NotAuthenticatedError : public WebUntisAPIError {
public:
    explicit NotAuthenticatedError(const std::string& msg)
        : WebUntisAPIError(msg) {}
};


class NoRightForMethod : public WebUntisAPIError {
public:
    explicit NoRightForMethod(const std::string& error, const std::string& method_name)
        : WebUntisAPIError(error + " (method: " + method_name + ")") {}
};


class MethodNotFound : public WebUntisAPIError {
public:
    explicit MethodNotFound(const std::string& error, const std::string& method_name)
        : WebUntisAPIError(error + " (method: " + method_name + ")") {}
};
