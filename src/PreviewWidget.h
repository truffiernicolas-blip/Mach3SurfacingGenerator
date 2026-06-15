#pragma once

#include "SurfacingJob.h"

#include <QGraphicsView>

class PreviewWidget : public QGraphicsView
{
    Q_OBJECT

public:
    explicit PreviewWidget(QWidget *parent = nullptr);

    void setToolpath(const ToolpathGeometry &geometry, double toolDiameter);
    void clearToolpath();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void fitScene();

    QGraphicsScene *m_scene = nullptr;
    bool m_hasContent = false;
};

