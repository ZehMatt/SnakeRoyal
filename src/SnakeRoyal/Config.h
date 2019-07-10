#pragma once

static constexpr int MAX_PLAYERS = 24;

static constexpr int TILE_MAP_GRID_W = 48;
static constexpr int TILE_MAP_GRID_H = 32;

static constexpr int TILE_SIZE_W = 8;
static constexpr int TILE_SIZE_H = 8;

static constexpr int TILE_MAP_SIZE_W = TILE_MAP_GRID_W * (TILE_SIZE_W + 1);
static constexpr int TILE_MAP_SIZE_H = TILE_MAP_GRID_H * (TILE_SIZE_H + 1);

static constexpr int TILE_MAP_MARGIN_TOP = 30;
static constexpr int TILE_MAP_MARGIN_BOTTOM = 60;
static constexpr int TILE_MAP_MARGIN_RIGHT = 30;
static constexpr int TILE_MAP_MARGIN_LEFT = 20;

static constexpr int PLAYER_LIST_W = 250;
static constexpr int PLAYER_LIST_H = TILE_MAP_SIZE_H;
static constexpr int PLAYER_LIST_X = TILE_MAP_SIZE_W + TILE_MAP_MARGIN_LEFT
                                     + 10;
static constexpr int PLAYER_LIST_Y = TILE_MAP_MARGIN_TOP;

static constexpr int WINDOW_SIZE_W = TILE_MAP_MARGIN_LEFT + TILE_MAP_SIZE_W
                                     + TILE_SIZE_W + TILE_MAP_MARGIN_RIGHT
                                     + PLAYER_LIST_W + 10;
static constexpr int WINDOW_SIZE_H = TILE_MAP_MARGIN_TOP + TILE_MAP_SIZE_H
                                     + TILE_SIZE_H + TILE_MAP_MARGIN_BOTTOM;

static constexpr double GAME_TICK_RATE = 1.0 / 20.0;
// The maximum threshold time is GAME_TICK_RATE * GAME_TICK_DELTA_THRESHOLD
// 100 ms seems to be a good value, clients with 100ms latency should still have
// bearable gameplay.
static constexpr int GAME_TICK_DELTA_THRESHOLD = 4;

static constexpr int GAME_ROUND_RESTART_TICKS = 60;