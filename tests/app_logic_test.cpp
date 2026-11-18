#include "../app_logic.hpp"
#include <cassert>
#include <cstdio>
#include <initializer_list>

int main() {
  using namespace camera;
  assert(touch_inside(290,155,265,135,50,42));
  assert(!touch_inside(290,185,265,135,50,42)); // gap between Record and Settings
  assert(touch_inside(290,210,258,190,62,50));
  assert(!touch_inside(320,210,258,190,62,50));
  TouchSmoother smooth;uint16_t sx=100,sy=40;
  assert(smooth.update(true,sx,sy)&&sx==100&&sy==40); // immediate DOWN
  sx=102;sy=42;assert(smooth.update(true,sx,sy)&&sx==101&&sy==41);
  sx=300;sy=200;assert(smooth.update(true,sx,sy)&&sx==102&&sy==42); // isolated spike
  sx=101;sy=41;assert(smooth.update(true,sx,sy)&&sx==102&&sy==42);
  assert(smooth.update(false,sx,sy)); // one missing report stays pressed
  assert(!smooth.update(false,sx,sy));
  sx=200;sy=130;assert(smooth.update(true,sx,sy)&&sx==200&&sy==130); // fresh tap
  TouchReleaseGate gate;
  assert(gate.accept(true));
  gate.suppress(); // Back is pressed; finger remains on the new camera screen.
  assert(!gate.accept(true));
  assert(!gate.accept(true));
  assert(!gate.accept(false));
  assert(gate.accept(false));
  assert(gate.accept(true)); // A new tap is accepted after release.
  assert(battery_percent(0) == 0);
  assert(battery_percent(3300) == 0);
  assert(battery_percent(3800) == 50);
  assert(battery_percent(4200) == 100);
  assert(battery_percent(5000) == 100);
  unsigned previous = 0;
  for (unsigned mv = 0; mv <= 5000; ++mv) {
    const auto level = battery_percent(mv);
    assert(level <= 100 && level >= previous);
    previous = level;
  }

  uint16_t x, y;
  assert(touch_position(0, 0, 3, x, y) && x == 319 && y == 0);
  assert(touch_position(239, 319, 3, x, y) && x == 0 && y == 239);
  assert(touch_position(0, 0, 1, x, y) && x == 0 && y == 239);
  for (unsigned r = 0; r < 4; ++r) {
    for (unsigned rx : {0u,239u}) for (unsigned ry : {0u,319u}) {
      assert(touch_position(rx, ry, r, x, y));
      assert(x < ((r & 1) ? 320 : 240));
      assert(y < ((r & 1) ? 240 : 320));
    }
  }
  assert(touch_position_with_border(255,0,1,x,y)&&x==0&&y==0);
  assert(touch_position_with_border(255,335,1,x,y)&&x==319&&y==0);
  assert(!touch_position_with_border(256,0,1,x,y));
  assert(!touch_position_with_border(0,336,1,x,y));
  uint16_t opposite_x, opposite_y;
  for (unsigned rx : {0u,100u,239u}) for (unsigned ry : {0u,150u,319u}) {
    assert(touch_position(rx, ry, 1, x, y));
    assert(touch_position(rx, ry, 3, opposite_x, opposite_y));
    assert(x + opposite_x == 319 && y + opposite_y == 239);
  }
  assert(!touch_position(240, 0, 3, x, y));
  assert(!touch_position(0, 320, 3, x, y));
  std::puts("PASS: battery curve, touch rotation/bounds, screen-transition release gate");
}
