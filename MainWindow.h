#pragma once

#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <vector>
#include <QTreeWidget>

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

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);
private slots:
    void onLoadDicomSeries();
    void onNextSlice();
    void onPrevSlice();
    void onLoadDicom();
    void onLoadBase();
    void onLoadOverlay();
    void onOpacityChanged(int value);
    void onToggleOverlay(bool checked);
    void onStartDrawROI();
    void onClearROI();
    void onROIFinished(const QRectF& rect);

private:
    QLineEdit* metaFilter = nullptr;
    QTreeWidget* metaTree = nullptr;
    std::vector<QString> dicomFiles;
    int currentSlice = 0;
    ImageView* m_view = nullptr;
    void showSlice(int index);
    QWidget* createToolBarWidget();
    void loadDicomMetadata(const QString& file);
    void filterMetadata(const QString& text);
};
