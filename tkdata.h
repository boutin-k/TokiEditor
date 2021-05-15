#ifndef TKDATA_H
#define TKDATA_H

#include <cstring>
#include <QString>
#include <QSize>

#include <iostream>

static constexpr uint32_t tkMagicNumber{0x7F23F5AD};
static constexpr uint32_t tkGridMinWidth{30U};
static constexpr uint32_t tkGridMaxWidth{100U};
static constexpr uint32_t tkGridMinHeight{20U};
static constexpr uint32_t tkGridMaxHeight{100U};
static constexpr uint32_t tkVersion{3U};

// clang-format off
enum TkAction : uint32_t { bridge, teleport, crate, bloc, actionCount };

enum TkLayer : uint32_t { ground, background, foreground, layerCount };

// clang-format on

struct __attribute__((__packed__)) levelData {
  // clang-format off
  uint32_t magicNumber = {tkMagicNumber};
  uint32_t version     = {tkVersion};
  uint32_t gridWidth   = {tkGridMinWidth};
  uint32_t gridHeight  = {tkGridMinHeight};
  char tileFile[128]   = {0};
  char backFile[128]   = {0};
  char eggsFile[128]   = {0};
  char musicFile[128]  = {0};
  uint8_t action[TkAction::actionCount] = {0U, 0U, 0U, 0U};
  // clang-format on

  bool operator==(const levelData &d) {return !std::memcmp(this, &d, sizeof(levelData));}
  bool operator!=(const levelData &d) {return !(*this == d);}

  ulong getTileNumber() { return gridWidth * gridHeight; }
};

#endif // TKDATA_H
