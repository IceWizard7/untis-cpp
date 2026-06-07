#pragma once

#include "base_entity.hpp"

class Department final : public Base_Entity {
public:
    Department(str n, str ln, int i);
    ~Department() override;

    [[nodiscard]] str to_string() const override;
};
