#include "bb_runtime.h"

int main(int argc, char** argv) {
    bbInit(argc, argv);
    void* __gosub_ret__ = nullptr;
    bb_Graphics(640, 480, 0, 2);
    bb_SetBuffer(bb_BackBuffer());
    bb_AppTitle("Blitz2D Basic Example");
    var_player = bb_LoadImage("ship.png");
    var_x = 320;
    var_y = 240;
    while (!bb_KeyHit(1)) {
        bb_Cls();
        if (bb_KeyDown(203)) {
            var_x = (var_x - 5);
        }
        if (bb_KeyDown(205)) {
            var_x = (var_x + 5);
        }
        bb_DrawImage(var_player, var_x, var_y);
        bb_Flip();
    }
    bbEnd(); return 0;
    bbEnd();
    return 0;
}
