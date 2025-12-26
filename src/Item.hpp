#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <optional>

enum class EquipmentSlot
{
    None,
    Weapon,
    Shield,
    Helmet,
    Armor,
    Gloves,
    Boots
};

enum class SpriteSheetType
{
    None,
    Items,    // items.png (의류/방어구)
    Weapons   // weapons.png (무기, 헬멧, 방패 등)
};

struct Item
{
    int id = 0;
    std::string name;
    sf::Color color;  // 스프라이트 없을 때 폴백 색상
    EquipmentSlot equipSlot = EquipmentSlot::None;

    // 스프라이트 시트 정보
    SpriteSheetType sheetType = SpriteSheetType::None;
    int spriteX = 0;  // 스프라이트 시트 내 X 인덱스 (0-7)
    int spriteY = 0;  // 스프라이트 시트 내 Y 인덱스 (0-4)

    Item() = default;

    Item(int id, const std::string& name, const sf::Color& color, EquipmentSlot slot = EquipmentSlot::None)
        : id(id), name(name), color(color), equipSlot(slot)
    {
    }

    // 스프라이트 시트 사용 생성자
    Item(int id, const std::string& name, SpriteSheetType sheet, int sx, int sy, EquipmentSlot slot = EquipmentSlot::None)
        : id(id), name(name), color(sf::Color::White), equipSlot(slot)
        , sheetType(sheet), spriteX(sx), spriteY(sy)
    {
    }

    bool hasSprite() const { return sheetType != SpriteSheetType::None; }
};

using OptionalItem = std::optional<Item>;
