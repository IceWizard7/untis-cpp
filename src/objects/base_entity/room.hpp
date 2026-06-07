#pragma once

#include "base_entity.hpp"

class Room final : public Base_Entity {
public:
    Room(str n, str ln, int i);

    ~Room() override;

    [[nodiscard]] str to_string() const override;
};
