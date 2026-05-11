#pragma once
#include <vector>
#include <memory>

namespace custom {
    struct Rect { int x, y, w, h; };

    class BaseComponent {
    public:
        Rect bounds;
        bool isHovered = false;
        
        virtual ~BaseComponent() = default;
        virtual void update(int mX, int mY, bool lDown) = 0;
        virtual void draw() = 0;
    };
}