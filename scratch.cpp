#include <iostream>
#include <algorithm>

int main() {
    bool isPrimaryHoriz = true;
    double delta = 5.0; // scrolling right
    double MIN_LEN = 40.0;
    
    struct { double x, y, w, h; } MON_BOX = { 0, 0, 100, 100 };
    struct { double x, y, w, h; } TARGET_POS = { -60, 0, 100, 100 }; // 40px visible on left
    
    const double VISIBLE_LEN = isPrimaryHoriz ? 
        std::max(0.0, std::min(MON_BOX.x + MON_BOX.w, TARGET_POS.x + TARGET_POS.w) - std::max(MON_BOX.x, TARGET_POS.x)) :
        std::max(0.0, std::min(MON_BOX.y + MON_BOX.h, TARGET_POS.y + TARGET_POS.h) - std::max(MON_BOX.y, TARGET_POS.y));

    bool crossingEdge = false;
    if (isPrimaryHoriz) {
        if (delta > 0.F) // content moves right, boundary is left edge
            crossingEdge = TARGET_POS.x < MON_BOX.x && TARGET_POS.x + TARGET_POS.w > MON_BOX.x;
        else // content moves left, boundary is right edge
            crossingEdge = TARGET_POS.x < MON_BOX.x + MON_BOX.w && TARGET_POS.x + TARGET_POS.w > MON_BOX.x + MON_BOX.w;
    } else {
        if (delta > 0.F)
            crossingEdge = TARGET_POS.y < MON_BOX.y && TARGET_POS.y + TARGET_POS.h > MON_BOX.y;
        else
            crossingEdge = TARGET_POS.y < MON_BOX.y + MON_BOX.h && TARGET_POS.y + TARGET_POS.h > MON_BOX.y + MON_BOX.h;
    }

    std::cout << "Visible: " << VISIBLE_LEN << ", crossing: " << crossingEdge << "\n";
}
