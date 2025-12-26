#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <optional>

struct Item
{
    int id = 0;
    std::string name;
    sf::Color color;  // 플레이스홀더 (나중에 sf::Texture*로 교체 가능)

    Item() = default;

    Item(int id, const std::string& name, const sf::Color& color)
        : id(id), name(name), color(color)
    {
    }
};

using OptionalItem = std::optional<Item>;
