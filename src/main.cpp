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

    // 드래그 앤 드롭 매니저
    DragDropManager dragDropManager;

    // 가방 인벤토리 (왼쪽)
    InventoryWindow bagInventory({50.f, 100.f}, font, "Bag");
    bagInventory.setDragDropManager(&dragDropManager);

    // 창고 인벤토리 (오른쪽)
    InventoryWindow storageInventory({400.f, 100.f}, font, "Storage");
    storageInventory.setDragDropManager(&dragDropManager);

    // 테스트용 아이템 배치 (가방에 몇 개 아이템 추가)
    bagInventory.setItem(0, Item(1, "Sword", sf::Color::Red));
    bagInventory.setItem(1, Item(2, "Shield", sf::Color::Blue));
    bagInventory.setItem(2, Item(3, "Potion", sf::Color::Green));
    bagInventory.setItem(5, Item(4, "Gold", sf::Color::Yellow));
    bagInventory.setItem(10, Item(5, "Armor", sf::Color::Cyan));
    bagInventory.setItem(15, Item(6, "Helmet", sf::Color::Magenta));

    // 드롭 콜백 설정
    dragDropManager.setDropCallback([&](InventoryWindow* sourceInv, int sourceSlot,
                                         InventoryWindow* targetInv, int targetSlot) {
        // 드래그 중인 아이템 가져오기
        Item draggedItem = dragDropManager.getDraggedItem();

        if (targetSlot < 0)
        {
            // 드롭 실패 - 원래 위치로 복구
            sourceInv->setItem(sourceSlot, draggedItem);
            std::cout << "Drop cancelled - item returned to original slot" << std::endl;
            return;
        }

        if (sourceInv == targetInv && sourceSlot == targetSlot)
        {
            // 같은 슬롯에 드롭 - 원래 위치로 복구
            sourceInv->setItem(sourceSlot, draggedItem);
            return;
        }

        // 타겟 슬롯에 아이템이 있으면 교환
        OptionalItem targetItem = targetInv->getItem(targetSlot);

        // 드래그한 아이템을 타겟에 배치
        targetInv->setItem(targetSlot, draggedItem);

        // 타겟에 아이템이 있었으면 소스로 이동
        if (targetItem)
        {
            sourceInv->setItem(sourceSlot, targetItem);
        }

        std::cout << "Moved " << draggedItem.name;
        if (sourceInv != targetInv)
        {
            std::cout << " between inventories";
        }
        std::cout << std::endl;
    });

    // 하이라이트 클리어 콜백 설정
    dragDropManager.setClearHighlightsCallback([&]() {
        bagInventory.clearAllHighlights();
        storageInventory.clearAllHighlights();
    });

    // 버튼 매니저 생성
    ButtonManager buttonManager;

    while (renderWindow.isOpen())
    {
        while (const std::optional event = renderWindow.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                renderWindow.close();
            }

            // 드래그 매니저 이벤트 처리 (ESC로 취소, 마우스 이동 등)
            if (dragDropManager.handleEvent(*event))
            {
                continue;
            }

            // 인벤토리 이벤트 처리
            if (bagInventory.handleEvent(*event))
            {
                continue;
            }
            if (storageInventory.handleEvent(*event))
            {
                continue;
            }

            // 드래그 중 빈 공간에서 마우스 릴리즈 - 취소 처리
            if (const auto* mouseReleased = event->getIf<sf::Event::MouseButtonReleased>())
            {
                if (mouseReleased->button == sf::Mouse::Button::Left && dragDropManager.isDragging())
                {
                    dragDropManager.cancelDrag();
                    continue;
                }
            }

            buttonManager.handleEvent(*event);
        }

        renderWindow.clear(sf::Color{30, 30, 30});
        renderWindow.draw(buttonManager);
        renderWindow.draw(bagInventory);
        renderWindow.draw(storageInventory);
        renderWindow.draw(dragDropManager);  // 고스트 이미지 (항상 최상위)
        renderWindow.display();
    }
}
