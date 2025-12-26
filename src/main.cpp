#include <SFML/Graphics.hpp>
#include "ButtonManager.hpp"
#include "InventoryWindow.hpp"
#include <iostream>

int main()
{
    auto renderWindow = sf::RenderWindow(sf::VideoMode({1920u, 1080u}), "CMake SFML Project");
    renderWindow.setFramerateLimit(144);

    // 폰트 로드
    sf::Font font;
    if (!font.openFromFile("/System/Library/Fonts/Supplemental/Arial.ttf"))
    {
        std::cerr << "Failed to load font!" << std::endl;
        return -1;
    }

    // 인벤토리 윈도우 생성
    InventoryWindow inventory({300.f, 100.f}, font);
    inventory.setSlotCallback([](int slotIndex) {
        std::cout << "Slot " << (slotIndex + 1) << " clicked!" << std::endl;
    });

    // 버튼 매니저 생성
    ButtonManager buttonManager;

    // 인벤토리 열기 버튼
    auto openButton = std::make_shared<Button>(sf::Vector2f{50.f, 50.f}, sf::Vector2f{200.f, 40.f}, font, "Open Inventory");
    openButton->setCallback([&inventory]() {
        inventory.setVisible(true);
    });
    buttonManager.addButton(openButton);

    while (renderWindow.isOpen())
    {
        while (const std::optional event = renderWindow.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                renderWindow.close();
            }

            // 인벤토리 윈도우가 보이면 먼저 이벤트 처리
            if (inventory.handleEvent(*event))
            {
                continue;
            }

            buttonManager.handleEvent(*event);
        }

        renderWindow.clear(sf::Color{30, 30, 30});
        renderWindow.draw(buttonManager);
        renderWindow.draw(inventory);
        renderWindow.display();
    }
}
