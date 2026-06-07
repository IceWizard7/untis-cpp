#pragma once

#include "base_entity.hpp"

class Subject final : public Base_Entity {
public:
    Subject(str n, str ln, int i);
    ~Subject() override;

    std::tuple<int, int, int> color();

    [[nodiscard]] str to_string() const override;
};
