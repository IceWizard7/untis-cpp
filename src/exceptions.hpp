#pragma once


#include "config.hpp"

#include <stdexcept>
#include <string>

class WebUntisAPIError : public std::runtime_error {
public:
    explicit WebUntisAPIError(const str& msg)
        : std::runtime_error(msg) {}
};


class NotAuthenticatedError final : public WebUntisAPIError {
public:
    explicit NotAuthenticatedError(const str& msg)
        : WebUntisAPIError(msg) {}
};


class NoRightForMethod final : public WebUntisAPIError {
public:
    explicit NoRightForMethod(const str& error, const str& method_name)
        : WebUntisAPIError(error + " (method: " + method_name + ")") {}
};


class MethodNotFound final : public WebUntisAPIError {
public:
    explicit MethodNotFound(const str& error, const str& method_name)
        : WebUntisAPIError(error + " (method: " + method_name + ")") {}
};
