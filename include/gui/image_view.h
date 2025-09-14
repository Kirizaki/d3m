#pragma once

#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QTreeWidget>
#include <QComboBox>

class ImageView : public QGraphicsView {
    Q_OBJECT
public:
    explicit ImageView(QWidget* parent = nullptr);

    void loadBaseImage(const QImage& img);
    void loadOverlayImage(const QImage& img);
    void setOverlayOpacity(qreal o);
    void setOverlayVisible(bool v);
    void setDrawingEnabled(bool enabled);

signals:
    void roiFinished(const QRectF& roiSceneCoords);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    QGraphicsScene* m_scene = nullptr;
    QGraphicsPixmapItem* m_baseItem = nullptr;
    QGraphicsPixmapItem* m_overlayItem = nullptr;
    QGraphicsRectItem* m_currentRect = nullptr;
    bool m_drawingEnabled = false;
    QPointF m_startScenePoint;
};