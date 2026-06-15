#pragma once

#include "MaterialPreset.h"
#include "ToolPreset.h"

#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVector>

enum class JobOrigin
{
    BottomLeft,
    BottomRight
};

enum class SweepDirection
{
    AlongX,
    AlongY
};

struct SurfacingJob
{
    double lengthX = 100.0;
    double widthY = 50.0;
    double totalDepth = 1.0;
    double depthPerPass = 0.5;
    double outsideMargin = 0.0;
    double safeZ = 5.0;
    double approachZ = 1.0;
    JobOrigin origin = JobOrigin::BottomLeft;
    SweepDirection sweepDirection = SweepDirection::AlongX;
    QString outputFileName = QStringLiteral("surfacage.nc");
    ToolPreset tool;
    MaterialPreset material;
};

struct ToolpathGeometry
{
    QRectF workpiece;
    QRectF centerBounds;
    QPointF origin;
    QVector<QPointF> points;
    int scanLineCount = 0;
    double xyLengthPerPass = 0.0;
};

struct JobSummary
{
    int depthPassCount = 0;
    int scanLineCount = 0;
    double finalDepth = 0.0;
    double feedXY = 0.0;
    double plungeFeed = 0.0;
    int spindleSpeed = 0;
    double estimatedPathLength = 0.0;
};

struct GenerationResult
{
    bool success = false;
    QString error;
    QString gcode;
    ToolpathGeometry geometry;
    JobSummary summary;
};
