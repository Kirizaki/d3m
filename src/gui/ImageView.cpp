#include "gui/MainWindow.h"
#include "dicom/DicomUtils.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QSlider>
#include <QLabel>
#include <QCheckBox>
#include <QStatusBar>
#include <QMouseEvent>
#include <QPen>
#include <QBrush>
#include <QToolBar>
#include <QDockWidget>
#include <QLineEdit>
#include <QStringList>
#include <QDebug>
#include <QComboBox>

#include <gdcmImageReader.h>
#include <gdcmImage.h>
#include <gdcmPixelFormat.h>
#include <gdcmDataElement.h>
#include <gdcmFileMetaInformation.h>
#include <gdcmAttribute.h>
#include <gdcmReader.h>
#include <gdcmDataSet.h>
#include <gdcmStringFilter.h>
#include <gdcmGlobal.h>
#include <gdcmDicts.h>
#include <gdcmDictEntry.h>
#include <gdcmDict.h>
#include <gdcmDictEntry.h>

#include <optional>

ImageView::ImageView(QWidget* parent) : QGraphicsView(parent) {
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
}

void ImageView::loadBaseImage(const QImage& img) {
    m_scene->clear();
    m_baseItem = m_scene->addPixmap(QPixmap::fromImage(img));
    m_baseItem->setZValue(0);
    m_scene->setSceneRect(m_baseItem->boundingRect());
    // recreate overlay item placeholder
    m_overlayItem = m_scene->addPixmap(QPixmap());
    m_overlayItem->setZValue(1);
}

void ImageView::loadOverlayImage(const QImage& img) {
    if (!m_overlayItem) {
        m_overlayItem = m_scene->addPixmap(QPixmap::fromImage(img));
        m_overlayItem->setZValue(1);
    } else {
        m_overlayItem->setPixmap(QPixmap::fromImage(img));
    }
    // align overlay to scene rect
    if (m_baseItem) {
        m_overlayItem->setPos(m_baseItem->pos());
    }
}

void ImageView::setOverlayOpacity(qreal o) {
    if (m_overlayItem) m_overlayItem->setOpacity(o);
}

void ImageView::setOverlayVisible(bool v) {
    if (m_overlayItem) m_overlayItem->setVisible(v);
}

void ImageView::setDrawingEnabled(bool enabled) {
    m_drawingEnabled = enabled;
    setCursor(enabled ? Qt::CrossCursor : Qt::ArrowCursor);
}

// Mouse events: simple rectangle drawing in scene coordinates
void ImageView::mousePressEvent(QMouseEvent* event) {
    if (m_drawingEnabled && event->button() == Qt::LeftButton) {
        m_startScenePoint = mapToScene(event->pos());
        if (m_currentRect) {
            m_scene->removeItem(m_currentRect);
            delete m_currentRect;
            m_currentRect = nullptr;
        }
        m_currentRect = m_scene->addRect(QRectF(m_startScenePoint, QSizeF()), QPen(Qt::yellow, 2));
        m_currentRect->setZValue(2);
        return; // eat event while drawing
    }
    QGraphicsView::mousePressEvent(event);
}

void ImageView::mouseMoveEvent(QMouseEvent* event) {
    if (m_drawingEnabled && m_currentRect) {
        QPointF cur = mapToScene(event->pos());
        QRectF r(m_startScenePoint, cur);
        r = r.normalized();
        m_currentRect->setRect(r);
        return;
    }
    QGraphicsView::mouseMoveEvent(event);
}

void ImageView::mouseReleaseEvent(QMouseEvent* event) {
    if (m_drawingEnabled && event->button() == Qt::LeftButton && m_currentRect) {
        QRectF r = m_currentRect->rect();
        emit roiFinished(r);
        // leave the rectangle on scene for now; user can Clear ROI
        m_currentRect = nullptr;
        m_drawingEnabled = false;
        setCursor(Qt::ArrowCursor);
        return;
    }
    QGraphicsView::mouseReleaseEvent(event);
}
