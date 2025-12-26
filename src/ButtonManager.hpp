#pragma once

#include "Button.hpp"
#include <vector>
#include <memory>

class ButtonManager : public sf::Drawable
{
public:
    // 버튼 추가 (나중에 추가된 버튼이 위에 그려짐 = 높은 z-order)
    void addButton(std::shared_ptr<Button> button)
    {
        m_buttons.push_back(std::move(button));
    }

    // 버튼 제거
    void removeButton(const std::shared_ptr<Button>& button)
    {
        m_buttons.erase(
            std::remove(m_buttons.begin(), m_buttons.end(), button),
            m_buttons.end()
        );
    }

    // 버튼을 맨 위로 이동
    void bringToFront(const std::shared_ptr<Button>& button)
    {
        auto it = std::find(m_buttons.begin(), m_buttons.end(), button);
        if (it != m_buttons.end())
        {
            auto btn = *it;
            m_buttons.erase(it);
            m_buttons.push_back(btn);
        }
    }

    // 이벤트 처리 (z-order 역순으로, 최상위 버튼 우선)
    bool handleEvent(const sf::Event& event)
    {
        // 역순으로 순회 (마지막에 추가된 버튼 = 최상위)
        for (auto it = m_buttons.rbegin(); it != m_buttons.rend(); ++it)
        {
            if ((*it)->handleEvent(event))
            {
                // 최상위 버튼이 이벤트를 소비하면 나머지 버튼의 호버 상태 해제
                for (auto other = m_buttons.rbegin(); other != m_buttons.rend(); ++other)
                {
                    if (other != it)
                    {
                        (*other)->clearHover();
                    }
                }
                return true;
            }
        }
        return false;
    }

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        // 순서대로 그리기 (먼저 추가된 버튼이 아래에)
        for (const auto& button : m_buttons)
        {
            target.draw(*button, states);
        }
    }

    std::vector<std::shared_ptr<Button>> m_buttons;
};
