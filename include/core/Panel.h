#include "core/BaseComponent.h"

#include <Windows.h>
#include "core/Graphic.h"

namespace custom {
    class Panel : public BaseComponent {
    private:
        std::vector<std::shared_ptr<BaseComponent>> children;
        COLORREF bgColor;

    public:
        Panel(int x, int y, int w, int h, COLORREF color) {
            bounds = {x, y, w, h};
            bgColor = color;
        }

        void addChild(std::shared_ptr<BaseComponent> child) {
            children.push_back(child);
        }

        void update(int mX, int mY, bool lDown) override {
            // Update self
            isHovered = (mX >= bounds.x && mX <= bounds.x + bounds.w &&
                         mY >= bounds.y && mY <= bounds.y + bounds.h);

            // Update all children
            for (auto& child : children) {
                child->update(mX, mY, lDown);
            }
        }

        void draw() override {
            auto& gfx = Graphic::getInstance();
            // 1. Draw background
            gfx.draw_rect(bounds.x, bounds.y, bounds.w, bounds.h, bgColor, 10);
            
            // 2. Draw children on top
            for (auto& child : children) {
                child->draw();
            }
        }
    };
}