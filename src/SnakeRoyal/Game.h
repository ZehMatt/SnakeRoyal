#pragma once

#include "Config.h"
#include "Painter.h"

enum class RoundState : uint8_t
{
    IDLE = 1,
    RUNNING,
    RESTARTING,
};

struct RoundData_t
{
    RoundState state = RoundState::IDLE;
    uint32_t timeout = 0;
};

class Game
{
    HWND _hWnd = nullptr;
    uint32_t _tick = 0;
    bool _headless = false;
    bool _hasFocus = true;
    uint32_t _randState = 0;
    RoundData_t _roundData;

public:
    void setHeadless(bool headless);
    bool getHeadless() const;
    void init(HWND hWnd);
    void restart(uint32_t delayInTicks);
    void startRound();
    void update();
    void draw();

    uint32_t getRandState() const;
    void setRandState(uint32_t state);
    uint32_t getRand();

    RoundState getRoundState() const;
    void setRoundState(RoundState state, uint32_t delay = 0);
    RoundData_t& getRoundData();

    uint32_t getClientTime() const;
    uint32_t getHostTime() const;

    void setFocus(bool hasFocus);
    bool hasFocus() const;

    uint32_t getTick() const;
    void setTick(uint32_t tick);

    void createFood();

private:
    void drawInfo(Painter& painter);
};

extern Game gGame;