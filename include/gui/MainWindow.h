#pragma once

#include "dicom/DicomUtils.h"
#include "gui/ImageView.h"

#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QTreeWidget>
#include <QComboBox>

#include <vector>

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
    QSlider* sliceSlider;
    std::map<QString, std::vector<d3m::SliceInfo>> seriesMap;
    QLineEdit* metaFilter = nullptr;
    QTreeWidget* metaTree = nullptr;
    std::vector<QString> dicomFiles;
    int currentSlice = 0;
    QString currentSeriesUID = 0;
    ImageView* m_view = nullptr;
    QComboBox* seriesCombo = nullptr;
    bool showSlice(int index);
    QWidget* createToolBarWidget();
    void loadDicomMetadata(const QString& file);
    void filterMetadata(const QString& text);
    void extractSliceMetadata(const QString& file);
};
