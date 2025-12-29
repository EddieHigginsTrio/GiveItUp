#include <SFML/Graphics.hpp>
#include "ButtonManager.hpp"
#include "InventoryWindow.hpp"
#include "EquipmentWindow.hpp"
#include "Player.hpp"
#include "Enemy.hpp"
#include "TileMap.hpp"
#include <iostream>
#include <vector>

int main()
{
    auto renderWindow = sf::RenderWindow(sf::VideoMode({1280u, 720u}), "CMake SFML Project");
    renderWindow.setFramerateLimit(144);
    renderWindow.requestFocus();  // 창 생성 후 포커스 요청

    // 타일맵 생성 및 로드
    TileMap tileMap(60, 33);
    if (!tileMap.loadFromFile("test3.tilemap")) {
        std::cout << "test.tilemap not found, creating simple level..." << std::endl;
        tileMap.createSimpleLevel();
    } else {
        std::cout << "Loaded test.tilemap successfully!" << std::endl;
    }

    // 플레이어 생성 (타일맵의 spawn 위치 사용)
    sf::Vector2i playerSpawn = tileMap.getPlayerSpawn();
    sf::Vector2f playerStartPos;
    if (playerSpawn.x >= 0 && playerSpawn.y >= 0) {
        // spawn 위치가 설정되어 있으면 사용 (픽셀 좌표 - 그대로 사용)
        playerStartPos = sf::Vector2f(static_cast<float>(playerSpawn.x), static_cast<float>(playerSpawn.y));
        std::cout << "Player spawn from tilemap: (" << playerSpawn.x << ", " << playerSpawn.y << ")" << std::endl;
    } else {
        // 설정되지 않았으면 기본값 사용
        playerStartPos = {100.f, 100.f};
        std::cout << "Player spawn not set, using default position" << std::endl;
    }
    Player player(playerStartPos);
    std::cout << "Player position: (" << playerStartPos.x << ", " << playerStartPos.y << ")" << std::endl;
    std::cout << "Map size: " << tileMap.getWidth() * TileMap::TILE_SIZE << " x " << tileMap.getHeight() * TileMap::TILE_SIZE << std::endl;

    // 장비 변경 추적용
    OptionalItem lastEquippedWeapon;

    // 적 생성 (타일맵의 enemy spawn 위치 사용)
    std::vector<Enemy> enemies;
    const auto& enemySpawns = tileMap.getEnemySpawns();
    if (!enemySpawns.empty()) {
        for (const auto& spawn : enemySpawns) {
            float x = static_cast<float>(std::get<0>(spawn));
            float y = static_cast<float>(std::get<1>(spawn));
            enemies.emplace_back(sf::Vector2f{x, y});
            std::cout << "Enemy spawn from tilemap: (" << x << ", " << y << ")" << std::endl;
        }
    } else {
        // enemy spawn이 없으면 기본 적 생성
        std::cout << "No enemy spawns in tilemap, creating default enemies" << std::endl;
        enemies.emplace_back(sf::Vector2f{400.f, 100.f});
        enemies.emplace_back(sf::Vector2f{700.f, 100.f});
        enemies.emplace_back(sf::Vector2f{1000.f, 100.f});
    }

    // 델타 타임 계산용 클럭
    sf::Clock clock;

    // 카메라 (게임 월드용) - 플레이어 위치를 중심으로 초기화
    sf::View gameView(sf::FloatRect({0.f, 0.f}, {1280.f, 720.f}));
    // 초기 카메라를 플레이어 중심으로 설정
    sf::Vector2f initialCameraCenter = playerStartPos + sf::Vector2f(Player::WIDTH / 2.f, Player::HEIGHT / 2.f);
    gameView.setCenter(initialCameraCenter);
    std::cout << "Initial camera center: (" << initialCameraCenter.x << ", " << initialCameraCenter.y << ")" << std::endl;

    // UI용 뷰 (고정)
    sf::View uiView(sf::FloatRect({0.f, 0.f}, {1280.f, 720.f}));

    // 폰트 로드
    sf::Font font;
    if (!font.openFromFile("/System/Library/Fonts/Supplemental/Arial.ttf"))
    {
        std::cerr << "Failed to load font!" << std::endl;
        return -1;
    }

    // 스프라이트 시트 로드
    sf::Texture itemsTexture;
    if (!itemsTexture.loadFromFile("items.png"))
    {
        std::cerr << "Failed to load items.png!" << std::endl;
        return -1;
    }

    sf::Texture weaponsTexture;
    if (!weaponsTexture.loadFromFile("weapons.png"))
    {
        std::cerr << "Failed to load weapons.png!" << std::endl;
        return -1;
    }

    // 배경 텍스처 로드
    sf::Texture backgroundTexture;
    if (!backgroundTexture.loadFromFile("mountain.png"))
    {
        std::cerr << "Failed to load mountain.png!" << std::endl;
        return -1;
    }
    sf::Sprite backgroundSprite(backgroundTexture);
    // 타일맵 크기에 맞게 배경 스케일 조정
    // 타일맵: 60x33 타일 = 1920x1056 픽셀
    // mountain.png: 2816x1536 픽셀
    float mapPixelHeight = 33 * 32;  // 1056
    float bgScale = mapPixelHeight / 1536.f;
    backgroundSprite.setScale({bgScale, bgScale});

    // 플레이어에 무기 텍스처 설정
    player.setWeaponTexture(&weaponsTexture);

    // 드래그 앤 드롭 매니저
    DragDropManager dragDropManager;
    dragDropManager.setItemsTexture(&itemsTexture);
    dragDropManager.setWeaponsTexture(&weaponsTexture);
    dragDropManager.setRenderWindow(&renderWindow);
    dragDropManager.setUIView(&uiView);

    // 가방 인벤토리 (왼쪽)
    InventoryWindow bagInventory({50.f, 100.f}, font, "Bag");
    bagInventory.setDragDropManager(&dragDropManager);
    bagInventory.setItemsTexture(&itemsTexture);
    bagInventory.setWeaponsTexture(&weaponsTexture);
    bagInventory.setRenderWindow(&renderWindow);
    bagInventory.setUIView(&uiView);
    bagInventory.setVisible(false);  // 기본값: 숨김

    // 창고 인벤토리 (오른쪽)
    InventoryWindow storageInventory({400.f, 100.f}, font, "Storage");
    storageInventory.setDragDropManager(&dragDropManager);
    storageInventory.setItemsTexture(&itemsTexture);
    storageInventory.setWeaponsTexture(&weaponsTexture);
    storageInventory.setRenderWindow(&renderWindow);
    storageInventory.setUIView(&uiView);
    storageInventory.setVisible(false);  // 기본값: 숨김

    // 장비 창 (오른쪽 상단)
    EquipmentWindow equipmentWindow({750.f, 100.f}, font);
    equipmentWindow.setDragDropManager(&dragDropManager);
    equipmentWindow.setItemsTexture(&itemsTexture);
    equipmentWindow.setWeaponsTexture(&weaponsTexture);
    equipmentWindow.setRenderWindow(&renderWindow);
    equipmentWindow.setUIView(&uiView);
    equipmentWindow.setVisible(false);  // 기본값: 숨김

    // 테스트용 아이템 배치 (가방에 몇 개 아이템 추가)
    // weapons.png 스프라이트 시트 기준:
    // Row 0: 검들 (0-4), 도끼 (5-6), 활/석궁 (7)
    // Row 1: 헬멧들 (0-3), 마법사 모자 (4), 방패들 (5-7)
    // Row 2: 반지 (0-3), 목걸이 (4-5), 망토 (6), 갑옷들 (7)
    // Row 3: 무기들 (0-4), 장갑들 (6-7)
    // Row 4: 부츠들 (0-1), 장갑 (6-7)

    // 장비 가능한 아이템들 (스프라이트 사용)
    bagInventory.setItem(0, Item(1, "Iron Sword", SpriteSheetType::Weapons, 0, 0, EquipmentSlot::Weapon));
    bagInventory.setItem(1, Item(2, "Wood Shield", SpriteSheetType::Weapons, 5, 1, EquipmentSlot::Shield));
    bagInventory.setItem(2, Item(3, "Gold Sword", SpriteSheetType::Weapons, 1, 0, EquipmentSlot::Weapon));
    bagInventory.setItem(5, Item(4, "Blue Sword", SpriteSheetType::Weapons, 2, 0, EquipmentSlot::Weapon));
    bagInventory.setItem(10, Item(5, "Chain Armor", SpriteSheetType::Weapons, 7, 2, EquipmentSlot::Armor));
    bagInventory.setItem(15, Item(6, "Iron Helmet", SpriteSheetType::Weapons, 2, 1, EquipmentSlot::Helmet));
    bagInventory.setItem(20, Item(7, "Iron Gloves", SpriteSheetType::Weapons, 6, 4, EquipmentSlot::Gloves));
    bagInventory.setItem(25, Item(8, "Leather Boots", SpriteSheetType::Weapons, 0, 4, EquipmentSlot::Boots));

    // items.png에서 의류 아이템들
    bagInventory.setItem(3, Item(9, "Leather Vest", SpriteSheetType::Items, 0, 0, EquipmentSlot::Armor));
    bagInventory.setItem(6, Item(10, "Chain Mail", SpriteSheetType::Items, 2, 0, EquipmentSlot::Armor));
    bagInventory.setItem(11, Item(11, "Blue Robe", SpriteSheetType::Items, 4, 0, EquipmentSlot::Armor));

    // 추가 무기들
    bagInventory.setItem(7, Item(12, "Battle Axe", SpriteSheetType::Weapons, 5, 0, EquipmentSlot::Weapon));
    bagInventory.setItem(12, Item(13, "Silver Shield", SpriteSheetType::Weapons, 7, 1, EquipmentSlot::Shield));
    bagInventory.setItem(16, Item(14, "Wizard Hat", SpriteSheetType::Weapons, 3, 1, EquipmentSlot::Helmet));

    // 드롭 콜백 설정 (인벤토리 -> 인벤토리)
    dragDropManager.setDropCallback([&](InventoryWindow* sourceInv, int sourceSlot,
                                         InventoryWindow* targetInv, int targetSlot) {
        // 드래그 중인 아이템 가져오기
        Item draggedItem = dragDropManager.getDraggedItem();
        DragSource source = dragDropManager.getSource();

        if (targetSlot < 0)
        {
            // 드롭 실패 - 원래 위치로 복구
            if (source.type == DragSourceType::Inventory && sourceInv)
            {
                sourceInv->setItem(sourceSlot, draggedItem);
            }
            else if (source.type == DragSourceType::Equipment && source.equipment)
            {
                source.equipment->setItemByIndex(sourceSlot, draggedItem);
            }
            std::cout << "Drop cancelled - item returned to original slot" << std::endl;
            return;
        }

        // 인벤토리에서 인벤토리로
        if (source.type == DragSourceType::Inventory && sourceInv)
        {
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
        }
        // 장비에서 인벤토리로
        else if (source.type == DragSourceType::Equipment && source.equipment)
        {
            // 타겟 슬롯에 아이템이 있으면 교환 (장비 가능한 경우만)
            OptionalItem targetItem = targetInv->getItem(targetSlot);

            // 드래그한 아이템을 인벤토리에 배치
            targetInv->setItem(targetSlot, draggedItem);

            // 타겟에 아이템이 있었으면 장비로 이동 (장비 가능한 경우)
            if (targetItem && source.equipment->canEquipItem(*targetItem, sourceSlot))
            {
                source.equipment->setItemByIndex(sourceSlot, targetItem);
            }

            std::cout << "Unequipped " << draggedItem.name << std::endl;
        }
    });

    // 하이라이트 클리어 콜백 설정
    dragDropManager.setClearHighlightsCallback([&]() {
        bagInventory.clearAllHighlights();
        storageInventory.clearAllHighlights();
        equipmentWindow.clearAllHighlights();
    });

    // 장비 드롭 콜백 설정
    dragDropManager.setEquipmentDropCallback([&](const DragSource& source,
                                                  EquipmentWindow* targetEquip, int targetSlot) {
        Item draggedItem = dragDropManager.getDraggedItem();

        // 드롭 실패 - 원래 위치로 복구
        if (targetSlot < 0 || targetEquip == nullptr)
        {
            if (source.type == DragSourceType::Inventory && source.inventory)
            {
                source.inventory->setItem(source.slotIndex, draggedItem);
            }
            else if (source.type == DragSourceType::Equipment && source.equipment)
            {
                source.equipment->setItemByIndex(source.slotIndex, draggedItem);
            }
            std::cout << "Equipment drop cancelled" << std::endl;
            return;
        }

        // 장비 슬롯에 맞는지 확인
        if (!targetEquip->canEquipItem(draggedItem, targetSlot))
        {
            // 원래 위치로 복구
            if (source.type == DragSourceType::Inventory && source.inventory)
            {
                source.inventory->setItem(source.slotIndex, draggedItem);
            }
            else if (source.type == DragSourceType::Equipment && source.equipment)
            {
                source.equipment->setItemByIndex(source.slotIndex, draggedItem);
            }
            std::cout << "Cannot equip " << draggedItem.name << " in this slot" << std::endl;
            return;
        }

        // 타겟 장비 슬롯에 아이템이 있으면 교환
        OptionalItem targetItem = targetEquip->getItem(targetSlot);

        // 드래그한 아이템을 장비 슬롯에 배치
        targetEquip->setItemByIndex(targetSlot, draggedItem);

        // 타겟에 아이템이 있었으면 소스로 이동
        if (targetItem)
        {
            if (source.type == DragSourceType::Inventory && source.inventory)
            {
                source.inventory->setItem(source.slotIndex, targetItem);
            }
            else if (source.type == DragSourceType::Equipment && source.equipment)
            {
                source.equipment->setItemByIndex(source.slotIndex, targetItem);
            }
        }

        std::cout << "Equipped " << draggedItem.name << std::endl;
    });

    // 버튼 매니저 생성
    ButtonManager buttonManager;

    // 창 포커스 상태 (이벤트 기반 추적, 초기값 true)
    bool windowHasFocus = true;

    while (renderWindow.isOpen())
    {
        while (const std::optional event = renderWindow.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                renderWindow.close();
            }

            // 포커스 이벤트 처리
            if (event->is<sf::Event::FocusGained>())
            {
                windowHasFocus = true;
            }
            if (event->is<sf::Event::FocusLost>())
            {
                windowHasFocus = false;
            }

            // 키보드 단축키: B = Bag, I = Equipment(Inventory)
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
            {
                if (keyPressed->code == sf::Keyboard::Key::B)
                {
                    bagInventory.setVisible(!bagInventory.isVisible());
                }
                if (keyPressed->code == sf::Keyboard::Key::I)
                {
                    equipmentWindow.setVisible(!equipmentWindow.isVisible());
                }
            }

            // 클릭 좌표 로깅
            if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>())
            {
                // 현재 마우스 위치도 함께 출력
                sf::Vector2i currentMousePos = sf::Mouse::getPosition(renderWindow);
                sf::Vector2f uiCoords = renderWindow.mapPixelToCoords(mousePressed->position, uiView);
                sf::Vector2f currentUiCoords = renderWindow.mapPixelToCoords(currentMousePos, uiView);
                std::cout << "Mouse clicked - Event Pixel: (" << mousePressed->position.x
                          << ", " << mousePressed->position.y << ") Event UI: ("
                          << uiCoords.x << ", " << uiCoords.y << ")" << std::endl;
                std::cout << "              - Current Pixel: (" << currentMousePos.x
                          << ", " << currentMousePos.y << ") Current UI: ("
                          << currentUiCoords.x << ", " << currentUiCoords.y << ")" << std::flush << std::endl;
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

            // 장비 창 이벤트 처리
            if (equipmentWindow.handleEvent(*event))
            {
                continue;
            }

            // 클릭-토글 방식이므로 mouseReleased에서 취소하지 않음
            // 드래그 취소는 ESC 키 또는 빈 공간 클릭으로 처리

            buttonManager.handleEvent(*event);
        }
        // 델타 타임 계산
        float deltaTime = clock.restart().asSeconds();

        // 장비창에서 무기 변경 감지 및 플레이어에 반영
        OptionalItem currentWeapon = equipmentWindow.getItem(EquipmentSlot::Weapon);
        bool weaponChanged = false;
        if (currentWeapon.has_value() != lastEquippedWeapon.has_value())
        {
            weaponChanged = true;
        }
        else if (currentWeapon.has_value() && lastEquippedWeapon.has_value())
        {
            weaponChanged = (currentWeapon->id != lastEquippedWeapon->id);
        }

        if (weaponChanged)
        {
            player.equipWeapon(currentWeapon);
            lastEquippedWeapon = currentWeapon;
            if (currentWeapon)
            {
                std::cout << "Weapon equipped: " << currentWeapon->name
                          << " (sprite: " << currentWeapon->spriteX << ", " << currentWeapon->spriteY << ")" << std::endl;
            }
            else
            {
                std::cout << "Weapon unequipped" << std::endl;
            }
        }

        // 플레이어 입력 및 업데이트 (창이 포커스를 가지고 있을 때만)
        if (windowHasFocus)
        {
            player.handleInput();
        }
        player.update(deltaTime, &tileMap);

        // 적 업데이트 및 충돌 감지
        for (auto& enemy : enemies)
        {
            enemy.update(deltaTime, &tileMap);

            // 플레이어와 적의 충돌 감지 (적 -> 플레이어)
            if (enemy.isAlive())
            {
                auto intersection = player.getBounds().findIntersection(enemy.getBounds());
                if (intersection.has_value())
                {
                    player.takeHit(enemy.getDamage(), enemy.getKnockbackForce(), enemy.getCenter());
                }
            }

            // 플레이어 공격 충돌 감지 (플레이어 -> 적)
            if (player.isAttacking() && enemy.isAlive())
            {
                sf::FloatRect attackHitbox = player.getAttackHitbox();
                auto attackHit = attackHitbox.findIntersection(enemy.getBounds());
                if (attackHit.has_value())
                {
                    enemy.takeDamage(player.getAttackDamage(), player.getAttackKnockback(), player.getCenter());
                }
            }
        }

        // 카메라를 플레이어 중심으로 부드럽게 이동 (lerp)
        sf::Vector2f playerCenter = player.getPosition() + sf::Vector2f(Player::WIDTH / 2.f, Player::HEIGHT / 2.f);
        sf::Vector2f currentCenter = gameView.getCenter();
        float smoothSpeed = 5.f;  // 카메라 스무딩 속도 (높을수록 빠름)
        sf::Vector2f newCenter = currentCenter + (playerCenter - currentCenter) * smoothSpeed * deltaTime;

        // 카메라를 타일맵 범위 내로 제한
        sf::Vector2f viewSize = gameView.getSize();
        float mapWidth = tileMap.getWidth() * tileMap.getTileSize();
        float mapHeight = tileMap.getHeight() * tileMap.getTileSize();

        // 카메라 중심의 최소/최대값 계산
        float minX = viewSize.x / 2.f;
        float maxX = mapWidth - viewSize.x / 2.f;
        float minY = viewSize.y / 2.f;
        float maxY = mapHeight - viewSize.y / 2.f;

        // 맵이 화면보다 작은 경우 중앙에 고정
        if (maxX < minX) newCenter.x = mapWidth / 2.f;
        else newCenter.x = std::max(minX, std::min(newCenter.x, maxX));

        if (maxY < minY) newCenter.y = mapHeight / 2.f;
        else newCenter.y = std::max(minY, std::min(newCenter.y, maxY));

        gameView.setCenter(newCenter);

        renderWindow.clear(sf::Color{30, 30, 30});

        // 게임 월드 렌더링 (카메라 적용)
        renderWindow.setView(gameView);

        // 배경 렌더링 (월드 좌표 (0,0)에 고정)
        backgroundSprite.setPosition({0.f, 0.f});
        renderWindow.draw(backgroundSprite);

        renderWindow.draw(tileMap);
        for (const auto& enemy : enemies)
        {
            renderWindow.draw(enemy);
        }
        renderWindow.draw(player);

        // UI 렌더링 (고정 뷰)
        renderWindow.setView(uiView);
        renderWindow.draw(buttonManager);
        renderWindow.draw(bagInventory);
        renderWindow.draw(storageInventory);
        renderWindow.draw(equipmentWindow);
        renderWindow.draw(dragDropManager);  // 고스트 이미지 (항상 최상위)
        renderWindow.display();
    }
}
