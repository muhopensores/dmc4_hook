#pragma once

class uStageSetTimeSlow {
public:
    char pad_0000[4992];     // 0x0000
    float m_time_left;       // 0x1380
    float m_duration;        // 0x1384
    float m_enemy_work_rate; // 0x1388
    float m_stage_work_rate; // 0x138C
    char pad_1390[3288];     // 0x1390
}; // Size: 0x2068
static_assert(sizeof(uStageSetTimeSlow) == 0x2068);
