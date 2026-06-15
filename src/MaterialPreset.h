#pragma once

#include <QString>

struct MaterialPreset
{
    QString name;
    int spindleSpeed = 12000;
    double feedXY = 800.0;
    double plungeFeed = 200.0;
    double recommendedDepthPerPass = 0.5;
};

