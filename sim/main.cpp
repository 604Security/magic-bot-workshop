// Magic Bot Workshop CYD emulator: runs the real firmware/cyd/cyd_menu sketch on a laptop and saves pictures.
//   make sim            -> docs/screenshots/cyd-*.png
// Idea from TalkingSasquach's SquachWatch-CYD PC emulator (https://github.com/skizzophrenic/SquachWatch-CYD);
// this one is written from scratch for the Magic Bot Workshop.
// A pretend Sparkle (below) answers the sketch's Wi-Fi questions; scenes tap the screen and change Sparkle's
// answers, then save what the screen shows.
#include <Arduino.h>
#include <map>
#include <vector>

unsigned long g_now = 1000;
int g_led[3] = {0, 0, 0};
SerialShim Serial;

// ---------------------------------------------------------------- the pretend Sparkle
struct FakeSparkle {
  std::string hatState = "waiting";
  int hatHouse = 0, hatStart = 0;
  int peak = 20;
  bool canReply = true;
  long replyAt = 0;
  std::string reply = "You are a coding wizard!";
  int spell = -1, spellAt = 0;
  int potionState = 0;
  int danceLeft = 0, danceCount = 4;
  int jpeg = 0;
} sp;

String fakeSparkle(const String& pq, int* jpegSize) {
  std::string p = pq.s.substr(0, pq.s.find('?'));
  char b[200];
  if (p == "/capture") { *jpegSize = 9000; return String(); }
  if (p == "/api/hat")
    snprintf(b, sizeof(b), "{\"state\":\"%s\",\"house\":%d,\"start\":%d}", sp.hatState.c_str(), sp.hatHouse, sp.hatStart);
  else if (p == "/api/sound") {
    snprintf(b, sizeof(b), "{\"level\":%d,\"peak\":%d}", sp.peak, sp.peak);
    sp.peak = 20;  // like the real one: the loudest since you last asked
  } else if (p == "/api/owl")
    snprintf(b, sizeof(b), sp.canReply ? "{\"fly\":3000,\"reply\":\"%s\",\"replyAt\":%ld}" : "{\"fly\":3000}",
             sp.reply.c_str(), sp.replyAt);
  else if (p == "/api/wand") snprintf(b, sizeof(b), "{\"spell\":%d,\"at\":%d}", sp.spell, sp.spellAt);
  else if (p == "/api/potion") snprintf(b, sizeof(b), "{\"state\":%d}", sp.potionState);
  else if (p == "/api/dance") snprintf(b, sizeof(b), "{\"left\":%d,\"total\":8000,\"count\":%d}", sp.danceLeft, sp.danceCount);
  else if (p == "/api/score") snprintf(b, sizeof(b), "{\"claps\":0,\"score\":0}");
  else snprintf(b, sizeof(b), "{}");
  return String(b);
}

// ---------------------------------------------------------------- the real sketch
#include "../firmware/cyd/cyd_menu/cyd_menu.ino"

SimTap g_tap;
WiFiShim WiFi;
MDNSShim MDNS;
TJpgShim TJpgDec;

// ---------------------------------------------------------------- helpers for scenes
const char* g_out = "sim/out";

void shot(const char* name) {
  char path[256];
  snprintf(path, sizeof(path), "%s/%s.ppm", g_out, name);
  FILE* f = fopen(path, "wb");
  fprintf(f, "P6\n%d %d\n255\n", tft.W, tft.H);
  for (int i = 0; i < tft.W * tft.H; i++) {
    uint16_t v = tft.fb[i];
    unsigned char rgb[3] = {(unsigned char)((v >> 11) << 3), (unsigned char)(((v >> 5) & 63) << 2), (unsigned char)((v & 31) << 3)};
    fwrite(rgb, 1, 3, f);
  }
  fclose(f);
  // remember the RGB light's colour so the picture frame can show it
  snprintf(path, sizeof(path), "%s/%s.led", g_out, name);
  f = fopen(path, "w");
  fprintf(f, "%d %d %d\n", g_led[0], g_led[1], g_led[2]);
  fclose(f);
  printf("  %s\n", name);
}

void tap(int x, int y) {
  g_tap.down = false;  // lift the finger first, like a real tap
  loop();
  g_tap = {true, x, y};
  loop();
}

void run(int times) { for (int i = 0; i < times; i++) loop(); }

void toMenu() {
  game = G_MENU;
  drawMenu();
}

void open(int g) {
  toMenu();
  tap(4 + (g % 4) * 79 + 37, 38 + (g / 4) * 100 + 47);
}

// ---------------------------------------------------------------- the scenes
int main(int argc, char** argv) {
  if (argc > 1) g_out = argv[1];
  std::srand(7);
  printf("CYD emulator (font 4 %s):\n", TFT_eSPI::font4Loaded ? "loaded" : "NOT loaded - big text will be invisible!");
  setup();
  shot("cyd-menu");

  // Sorting Hat: waiting, thinking, then each house
  open(G_HAT);
  run(2);
  shot("cyd-hat-waiting");
  const char* houses[4] = {"gryffindor", "ravenclaw", "hufflepuff", "slytherin"};
  for (int h = 0; h < 4; h++) {
    sp.hatState = "thinking";
    sp.hatStart += 10;
    run(20);
    if (h == 0) shot("cyd-hat-thinking");
    sp.hatState = "done";
    sp.hatHouse = h;
    run(1);
    char n[40];
    snprintf(n, sizeof(n), "cyd-hat-%s", houses[h]);
    shot(n);
  }

  // Spy Camera (a pretend photo)
  open(G_CAM);
  run(1);
  shot("cyd-camera");

  // Snitch: play until 7, then on to the win
  open(G_SNITCH);
  for (int i = 0; i < 4000 && snScore < 7; i++) {
    if (snInBox() && !snMsgUntil) sp.peak = 2500;  // clap when it's in the box!
    loop();
  }
  for (int i = 0; i < 200 && (snMsgUntil || !(snX > 40 && snX < 120)); i++) loop();
  shot("cyd-snitch");
  for (int i = 0; i < 8000 && !snWon; i++) {
    if (snInBox() && !snMsgUntil) sp.peak = 2500;
    loop();
  }
  shot("cyd-snitch-win");

  // Owl Post: the letter buttons, then a letter from Sparkle
  open(G_OWL);
  run(1);
  shot("cyd-owl");
  sp.replyAt = 4242;
  run(1);
  shot("cyd-owl-letter");

  // Wand Wave: waiting, then two spells
  open(G_WAND);
  run(1);
  shot("cyd-wand-waiting");
  sp.spell = 0; sp.spellAt = 100;
  run(1);
  shot("cyd-wand-lumos");
  sp.spell = 3; sp.spellAt = 200;
  run(1);
  shot("cyd-wand-expelliarmus");

  // Potions: pick unicorn, rainbow, star; stir; the result; the recipe book
  open(G_POTION);
  run(1);
  tap(6 + 0 * 78 + 36, 38 + 30);   // unicorn
  tap(6 + 2 * 78 + 36, 38 + 30);   // rainbow
  tap(6 + 3 * 78 + 36, 104 + 30);  // star
  shot("cyd-potion-pick");
  sp.potionState = 1; run(1);
  sp.potionState = 2; run(1);
  sp.potionState = 3; run(1);
  shot("cyd-potion-result");
  sp.potionState = 0; run(1);
  tap(240, 214);  // Recipes
  shot("cyd-potion-book");

  // Dance: the counter, then hearts
  open(G_DANCE);
  run(1);
  shot("cyd-dance");
  sp.danceLeft = 5000;
  run(6);
  shot("cyd-dance-hearts");
  sp.danceLeft = 0;

  // Badges: tap the medal on the menu
  toMenu();
  tap(300, 18);
  shot("cyd-badges");

  // Disco
  open(G_DISCO);
  run(12);
  shot("cyd-disco");
  return 0;
}
