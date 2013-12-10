/**
 *  Copyright 2013 Mike Reed
 *
 *  COMP 590 -- Fall 2013
 */

#include "GSlide.h"
#include "GColor.h"
#include "GPaint.h"
#include "GRandom.h"
#include "GTime.h"

#include <cassert>
#include <cstdlib>
#include <ctime>

enum EBlockType {
  eBlockType_Stick,
  eBlockType_Square,
  eBlockType_T,
  eBlockType_LeftS,
  eBlockType_RightS,

  kNumBlockTypes
};

// Offsets from center
static const uint32_t kBlockShape[kNumBlockTypes][4][2] = {
  { {-1, 0}, {0, 0}, {1, 0}, {2, 0} }, // eBlockType_Stick
  { {0, 0}, {1, 0}, {0, -1}, {1, -1} }, // eBlockType_Square
  { {-1, 0}, {0, 0}, {1, 0}, {0, -1} }, // eBlockType_T
  { {-1, 0}, {0, 0}, {0, -1}, {1, -1} }, // eBlockType_LeftS
  { {0, 0}, {1, 0}, {0, -1}, {-1, -1} } // eBlockType_RightS
};

static const uint32_t kBoardSzX = 10;
static const uint32_t kBoardSzY = 50;

static const uint32_t kPlayAreaX = 180;
static const uint32_t kPlayAreaY = 450;

static const uint32_t kScreenSizeX = 640;
static const uint32_t kScreenSizeY = 480;

static inline uint32_t playStartX() {
  return (kScreenSizeX >> 1) - (kPlayAreaX >> 1);
}

static inline uint32_t playEndX() {
  return (kScreenSizeX >> 1) + (kPlayAreaX >> 1);
}

static inline uint32_t playStartY() {
  return 0;
}

static inline uint32_t playEndY() {
  return kPlayAreaY;
}

static inline uint32_t tileSzX() {
  return kPlayAreaX / kBoardSzX;
}

static inline uint32_t tileSzY() {
  return kPlayAreaY / kBoardSzY;
}

class Tile {
private:
  GColor m_Color;

  bool m_Exists;
  bool m_Controlled;
  bool m_IsCenter;
  
public:
  Tile() : m_Exists(false), m_Controlled(false), m_IsCenter(false) { }
  Tile(GColor c) : m_Color(c), m_Controlled(true), m_Exists(true), m_IsCenter(false) { }

  void Draw(uint32_t x, uint32_t y, GContext* ctx) {
    if(!Exists())
      return;

    uint32_t tx = playStartX() + x * tileSzX();
    uint32_t ty = playEndY() - (y+1) * tileSzY();
    GIRect tile = GIRect::MakeLTRB(tx, ty, tx + tileSzX(), ty + tileSzY());
    GPaint paint;
    paint.setColor(m_Color);
    ctx->drawRect(tile, paint);
  }

  bool Center() const { return m_IsCenter; }
  void SetCenter() { m_IsCenter = true; }
  bool Exists() const { return m_Exists; }
  void Destroy() { m_Exists = false; m_IsCenter = false; }
  void Relenquish() { m_Controlled = false; m_IsCenter = false; }
  bool Controlled() const { return m_Exists && m_Controlled; }
};

class Board {
  EBlockType m_CurrentType;
  GRandom m_Random;
  Tile m_Board[kBoardSzX][kBoardSzY];

  bool NeedsMoveDown() {
    bool hasControlled = false;
    for(uint32_t x = 0; x < kBoardSzX; x++) {
      for(uint32_t y = 1; y < kBoardSzY; y++) {
        Tile &b = m_Board[x][y];
        if(b.Controlled()) {
          hasControlled = true;
        }

        if(b.Controlled()) {
          Tile &nb = m_Board[x][y - 1];
          if(y-1 == 0 && nb.Exists())
            return false;
          else if(nb.Exists() && !nb.Controlled())
            return false;
        }
      }
    }

    if(hasControlled)
      return true;
    else 
      return false;
  }

  void Drop(uint32_t x, uint32_t y) {
    assert(y > 0);
    m_Board[x][y-1] = m_Board[x][y];
    m_Board[x][y].Destroy();
  }

  void DropControlled() {
    for(uint32_t x = 0; x < kBoardSzX; x++) {
      for(uint32_t y = 0; y < kBoardSzY; y++) {
        Tile &b = m_Board[x][y];
        if(b.Controlled()) {
          Drop(x, y);
        }
      }
    }
  }

  void Uncontrol() {
    for(uint32_t x = 0; x < kBoardSzX; x++) {
      for(uint32_t y = 0; y < kBoardSzY; y++) {
        m_Board[x][y].Relenquish();
      }
    }
  }

  bool HasControlled() {
    for(uint32_t x = 0; x < kBoardSzX; x++) {
      for(uint32_t y = 0; y < kBoardSzY; y++) {
        if(m_Board[x][y].Controlled())
          return true;
      }
    }
    return false;
  }

  void ClearLines() {
    int drop = 0;
    for(uint32_t y = 0; y < kBoardSzY; y++) {
      bool lineClear = true;
      for(uint32_t x = 0; x < kBoardSzX; x++) {
        if(!(m_Board[x][y].Exists())) {
          lineClear = false;
          break;
        }
      }

      if(lineClear) {
        drop++;
        for(uint32_t x = 0; x < kBoardSzX; x++) {
          m_Board[x][y].Destroy();
        }
      }

      if(y > 0) {
        for(uint32_t x = 0; x < kBoardSzX; x++) {
          for(uint32_t i = 0; i < drop; i++) {
            Drop(x, y-i);
          }
        }
      }
    }
  }

  GColor RandColor() {
    return GColor::Make(1, m_Random.nextF(), m_Random.nextF(), m_Random.nextF());
  }

  bool NewPiece(EBlockType ty) {

    // Check for valid enum
    switch(ty) {
    case eBlockType_Stick: break;
    case eBlockType_Square: break;
    case eBlockType_T: break; 
    case eBlockType_LeftS: break;
    case eBlockType_RightS: break;

    default:
      assert(!"Unrecognized piece type!");
    }

    uint32_t cenX = 4;
    uint32_t cenY = 49;
    bool safe = true;
    for(uint32_t i = 0; i < 4; i++) {
      uint32_t x = cenX + kBlockShape[ty][i][0];
      uint32_t y = cenY + kBlockShape[ty][i][1];

      if(m_Board[x][y].Exists()) {
        safe = false;
        break;
      }
    }

    if(!safe)
      return false;

    GColor c = RandColor();
    for(uint32_t i = 0; i < 4; i++) {
      uint32_t x = cenX + kBlockShape[ty][i][0];
      uint32_t y = cenY + kBlockShape[ty][i][1];

      m_Board[x][y] = Tile(c);
    }
    m_Board[cenX][cenY].SetCenter();
    return true;
  }

 public:
  bool Step() {
    // If all blocks below the controlled exist, then
    // move all of the controlled blocks down
    bool bMoveDown = NeedsMoveDown();

    // Move all of the controlled blocks down.
    if(bMoveDown) {
      DropControlled();
      ClearLines();
    } else {
      if(!HasControlled()) {
        int r = m_Random.nextRange(0, kNumBlockTypes-1);
        NewPiece((EBlockType)r);
      } else {
        Uncontrol();
      }
    }
  }

  void Draw(GContext *ctx) {
    for(uint32_t x = 0; x < kBoardSzX; x++) {
      for(uint32_t y = 0; y < kBoardSzY; y++) {
        m_Board[x][y].Draw(x, y, ctx);
      }
    }
  }

  void MoveDown() {
    if(NeedsMoveDown()) {
      DropControlled();
      ClearLines();
    }
  }

  void MoveRight() {
    bool canMoveRight = true;
    for(uint32_t x = 0; x < kBoardSzX; x++) {
      for(uint32_t y = 0; y < kBoardSzY; y++) {
        Tile &b = m_Board[x][y];
        if(b.Controlled()) {
          if(x == kBoardSzX - 1) {
            canMoveRight = false;
          } else {
            Tile &nb = m_Board[x+1][y];
            if(nb.Exists() && !nb.Controlled()) {
              canMoveRight = false;
            }
          }
        }
      }
    }

    if(!canMoveRight)
      return;

    for(int32_t x = kBoardSzX - 1; x >= 0; x--) {
      for(int32_t y = 0; y < kBoardSzY; y++) {
        if(m_Board[x][y].Controlled()) {
          m_Board[x+1][y] = m_Board[x][y];
          m_Board[x][y].Destroy();
        }
      }
    }
  }

  void MoveLeft() {
    bool canMoveLeft = true;
    for(uint32_t x = 0; x < kBoardSzX; x++) {
      for(uint32_t y = 0; y < kBoardSzY; y++) {
        Tile &b = m_Board[x][y];
        if(b.Controlled()) {
          if(x == 0) {
            canMoveLeft = false;
          } else {
            Tile &nb = m_Board[x-1][y];
            if(nb.Exists() && !nb.Controlled()) {
              canMoveLeft = false;
            }
          }
        }
      }
    }

    if(!canMoveLeft)
      return;

    for(int32_t x = 0; x < kBoardSzX; x++) {
      for(int32_t y = 0; y < kBoardSzY; y++) {
        if(m_Board[x][y].Controlled()) {
          m_Board[x-1][y] = m_Board[x][y];
          m_Board[x][y].Destroy();
        }
      }
    }
  }

  void Rotate() {
    
  }

  Board(const GRandom &rnd) : m_Random(rnd) { }
};

static const GMSec kStepTime = 1000;

class TetrisSlide : public GSlide {
private:
  GMSec m_LastTime;
  Board m_Board;
  TetrisSlide(const GRandom &init) : m_Board(init), m_LastTime(GTime::GetMSec())  { }

protected:
  virtual void onDraw(GContext* ctx) {

    GMSec now = GTime::GetMSec();
    if(m_LastTime + kStepTime < now) {
      m_Board.Step();
      m_LastTime = now;
    }

    ctx->clear(GColor::Make(1, 0, 0, 0));

    // Clear the play area
    GIRect playAreaRect =
      GIRect::MakeLTRB(playStartX(), playStartY(), playEndX(), playEndY());
    GPaint white;
    white.setColor(GColor::Make(1, 1, 1, 1));
    ctx->drawRect(playAreaRect, white);

    m_Board.Draw(ctx);
  }

  virtual bool onHandleKey(int ascii) {
    switch(ascii) {
    case ((int)'D'):
    case ((int)'d'):
      m_Board.MoveRight();
    return true;
    case ((int)'A'):
    case ((int)'a'):
      m_Board.MoveLeft();
    return true;
    case ((int)'S'):
    case ((int)'s'):
      m_Board.MoveDown();
    return true;

    case ((int)'W'):
    case ((int)'w'):
      m_Board.Rotate();
    }
    return false;
  }

  virtual const char* onName() {
    return "Tetris Move - [ A S D ] Rotate - W";
  }

public:
  static GSlide* Create(void*) {
    GRandom rnd(time(NULL));
    return new TetrisSlide(rnd);
  }
};

GSlide::Registrar TetrisSlide_reg(TetrisSlide::Create);
