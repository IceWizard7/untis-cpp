#pragma once

#include "base_entity.hpp"

class Class final : public Base_Entity {
public:
    Class(str n, str ln, int i);

    ~Class() override;

    [[nodiscard]] str to_string() const override;
};
