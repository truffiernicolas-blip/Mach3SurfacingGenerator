#include "PreviewWidget.h"

#include <QBrush>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QResizeEvent>

PreviewWidget::PreviewWidget(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(new QGraphicsScene(this))
{
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing);
    setBackgroundBrush(QColor(38, 42, 48));
    setFrameShape(QFrame::NoFrame);
    setMinimumSize(500, 420);
    setDragMode(QGraphicsView::ScrollHandDrag);
}

void PreviewWidget::setToolpath(const ToolpathGeometry &geometry, double toolDiameter)
{
    m_scene->clear();

    auto mapPoint = [](const QPointF &point) {
        return QPointF(point.x(), -point.y());
    };
    auto mapRect = [&](const QRectF &rect) {
        return QRectF(mapPoint(rect.bottomLeft()), mapPoint(rect.topRight())).normalized();
    };

    const QPen workpiecePen(QColor(75, 160, 230), 0.8);
    const QBrush workpieceBrush(QColor(75, 160, 230, 45));
    m_scene->addRect(mapRect(geometry.workpiece), workpiecePen, workpieceBrush);

    if (!geometry.points.isEmpty()) {
        QPainterPath path(mapPoint(geometry.points.constFirst()));
        for (qsizetype index = 1; index < geometry.points.size(); ++index) {
            path.lineTo(mapPoint(geometry.points.at(index)));
        }

        QPen pathPen(QColor(255, 190, 70), 1.4);
        pathPen.setCosmetic(true);
        m_scene->addPath(path, pathPen);

        const QPointF start = mapPoint(geometry.points.constFirst());
        const double markerSize = std::max(toolDiameter * 0.18, 1.5);
        m_scene->addEllipse(QRectF(start.x() - markerSize / 2.0,
                                  start.y() - markerSize / 2.0,
                                  markerSize,
                                  markerSize),
                            QPen(Qt::NoPen),
                            QBrush(QColor(90, 220, 130)));
    }

    const QPointF origin = mapPoint(geometry.origin);
    const double crossSize = std::max(toolDiameter * 0.35, 3.0);
    QPen originPen(QColor(235, 80, 80), 1.8);
    originPen.setCosmetic(true);
    m_scene->addLine(origin.x() - crossSize, origin.y(),
                     origin.x() + crossSize, origin.y(), originPen);
    m_scene->addLine(origin.x(), origin.y() - crossSize,
                     origin.x(), origin.y() + crossSize, originPen);

    QPen boundsPen(QColor(180, 180, 180, 100), 0.8, Qt::DashLine);
    boundsPen.setCosmetic(true);
    m_scene->addRect(mapRect(geometry.centerBounds), boundsPen);

    const QRectF contentBounds = m_scene->itemsBoundingRect();
    const double padding = std::max(contentBounds.width(), contentBounds.height()) * 0.08 + 1.0;
    m_scene->setSceneRect(contentBounds.adjusted(-padding, -padding, padding, padding));
    m_hasContent = true;
    fitScene();
}

void PreviewWidget::clearToolpath()
{
    m_scene->clear();
    m_hasContent = false;
}

void PreviewWidget::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    fitScene();
}

void PreviewWidget::fitScene()
{
    if (m_hasContent && !m_scene->sceneRect().isEmpty()) {
        fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    }
}

