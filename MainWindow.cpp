#include "MainWindow.h"
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

int windowCenter = 40;  // just guessing
int windowWidth = 400;  // just guessing

// Helper: convert GDCM imate to QImage (grayscale)
static QImage gdcmImageToQImage(const gdcm::Image& gimg, int windowCenter, int windowWidth) {
    const unsigned int* dims = gimg.GetDimensions();
    int w = dims[0];
    int h = dims[1];

    gdcm::PixelFormat pf = gimg.GetPixelFormat();
    int bits = pf.GetBitsAllocated();       // e.g. 16/8
    int samples = pf.GetSamplesPerPixel();  // usually 1 (grayscale), or 3 (RGB)

    std::vector<char> buffer(gimg.GetBufferLength());
    gimg.GetBuffer(buffer.data());

    QImage img(w, h, QImage::Format_Grayscale8);

    if (bits == 8) {
        // direct copy
        const unsigned char* src = reinterpret_cast<unsigned char*>(buffer.data());
        for (int y = 0; y < h; ++y) {
            uchar* scan = img.scanLine(y);
            memcpy(scan, src + y*w, w);
        }
    } else if (bits == 16) {
        // rescale 16-git -> 8-bit
        const uint16_t* src = reinterpret_cast<uint16_t*>(buffer.data());

        uint16_t minVal = std::numeric_limits<uint16_t>::max();
        uint16_t maxVal = std::numeric_limits<uint16_t>::min();
        for (int i = 0; i < w*h; i++) {
            if (src[i] < minVal) minVal = src[i];
            if (src[i] > maxVal) maxVal = src[i];
        }
        
        // if no WL specified -> auto fit
        if (windowCenter < 0 || windowWidth < 0) {
            windowCenter = (minVal + maxVal) / 2;
            windowWidth = maxVal - minVal;
        }

        int wc = windowCenter;
        int ww = windowWidth;

        for (int y = 0; y < h; ++y) {
            uchar* scan = img.scanLine(y);
            for (int x = 0; x < w; ++x) {
                int idx = y*w + x;
                int p = src[idx];

                int minWin = wc - ww/2;
                int maxWin = wc + ww/2;

                if (p <= minWin)
                    scan[x] = 0;
                else if (p > maxWin)
                    scan[x] = 255;
                else
                    scan[x] = (uchar)(((p - minWin) / (double)ww) * 255.0);
            }
        }
    } else {
        // unsupported format for now..
        img.fill(Qt::black);
    }

    return img.flipped(Qt::Orientation::Vertical);
}

// ---------------- ImageView implementation ----------------
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

// ---------------- MainWindow implementation ----------------
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    m_view = new ImageView(this);
    setCentralWidget(m_view);

    auto toolWidget = createToolBarWidget();
    QToolBar* toolbar = new QToolBar(this);
    toolbar->addWidget(toolWidget);
    addToolBar(Qt::TopToolBarArea, toolbar);

    // Search box
    metaFilter = new QLineEdit();
    metaFilter->setPlaceholderText("Search DICOM tags...");

    // Tree widget
    metaTree = new QTreeWidget();
    metaTree->setColumnCount(2);
    metaTree->setHeaderLabels(QStringList() << "Tag" << "Value");

    // Layout inside dock
    QWidget* metaWidget = new QWidget();
    QVBoxLayout* vbox = new QVBoxLayout(metaWidget);
    vbox->setContentsMargins(2,2,2,2);
    vbox->addWidget(metaFilter);
    vbox->addWidget(metaTree);

    QDockWidget* dock = new QDockWidget("DICOM Metadata", this);
    dock->setWidget(metaWidget);
    addDockWidget(Qt::RightDockWidgetArea, dock);
    connect(metaFilter, &QLineEdit::textChanged, this, &MainWindow::filterMetadata);

    statusBar()->showMessage("Ready");

    connect(m_view, &ImageView::roiFinished, this, &MainWindow::onROIFinished);
}

QWidget* MainWindow::createToolBarWidget() {
    QWidget* w = new QWidget;
    auto h = new QHBoxLayout(w);
    h->setContentsMargins(4,4,4,4);

    QPushButton* loadBaseBtn = new QPushButton("Load Base");
    QPushButton* loadOverlayBtn = new QPushButton("Load Overlay");
    QLabel* opacityLabel = new QLabel("Overlay opacity");
    QSlider* opacitySlider = new QSlider(Qt::Horizontal);
    opacitySlider->setRange(0, 100);
    opacitySlider->setValue(60);
    QCheckBox* overlayToggle = new QCheckBox("Show Overlay");
    overlayToggle->setChecked(true);

    QPushButton* drawRoiBtn = new QPushButton("Draw ROI");
    QPushButton* clearRoiBtn = new QPushButton("Clear ROI");

    QPushButton* loadDicomBtn = new QPushButton("Load DICOM");
    QPushButton* loadSeriesBtn = new QPushButton("Load DICOM Series");
    QPushButton* nextBtn = new QPushButton("Next");
    QPushButton* prevBtn = new QPushButton("Prev");

    QSlider* wcSlider = new QSlider(Qt::Horizontal);
    wcSlider->setRange(-1000, 3000);
    wcSlider->setValue(windowCenter);
    QSlider* wwSlider = new QSlider(Qt::Horizontal);
    wwSlider->setRange(1, 4000);
    wwSlider->setValue(windowWidth);

    h->addWidget(loadBaseBtn);
    h->addWidget(loadOverlayBtn);
    h->addWidget(opacityLabel);
    h->addWidget(opacitySlider);
    h->addWidget(overlayToggle);
    h->addWidget(drawRoiBtn);
    h->addWidget(clearRoiBtn);
    h->addWidget(loadDicomBtn);
    h->addWidget(loadSeriesBtn);
    h->addWidget(nextBtn);
    h->addWidget(prevBtn);
    h->addWidget(wcSlider);
    h->addWidget(wwSlider);
    h->addStretch();

    connect(loadBaseBtn, &QPushButton::clicked, this, &MainWindow::onLoadBase);
    connect(loadOverlayBtn, &QPushButton::clicked, this, &MainWindow::onLoadOverlay);
    connect(opacitySlider, &QSlider::valueChanged, this, &MainWindow::onOpacityChanged);
    connect(overlayToggle, &QCheckBox::toggled, this, &MainWindow::onToggleOverlay);
    connect(drawRoiBtn, &QPushButton::clicked, this, &MainWindow::onStartDrawROI);
    connect(clearRoiBtn, &QPushButton::clicked, this, &MainWindow::onClearROI);
    connect(loadDicomBtn, &QPushButton::clicked, this, &MainWindow::onLoadDicom);
    connect(loadSeriesBtn, &QPushButton::clicked, this, &MainWindow::onLoadDicomSeries);
    connect(nextBtn, &QPushButton::clicked, this, &MainWindow::onNextSlice);
    connect(prevBtn, &QPushButton::clicked, this, &MainWindow::onPrevSlice);
    connect(wcSlider, &QSlider::valueChanged, this, [=](int v){windowCenter = v; showSlice(currentSlice);});
    connect(wwSlider, &QSlider::valueChanged, this, [=](int v) {windowWidth = v; showSlice(currentSlice);});

    return w;
}

void MainWindow::onLoadDicomSeries() {
    QString dirPath = QFileDialog::getExistingDirectory(this, "Select DICOM Series Folder");
    if (dirPath.isEmpty()) return;

    // collect files
    QDir dir(dirPath);
    QStringList filters;
    filters << "*.dcm" << "*.dicom" << "*"; // fallback
    dicomFiles.clear();
    for (const QFileInfo& fi : dir.entryInfoList(filters, QDir::Files)) {
        dicomFiles.push_back(fi.absoluteFilePath());
    }

    if (dicomFiles.empty()) {
        statusBar()->showMessage("No DICOM files found in folder");
        return;
    }

    currentSlice = 0;
    showSlice(currentSlice);
}

void MainWindow::showSlice(int index) {
    if (index < 0 || index >= (int)dicomFiles.size()) return;

    gdcm::ImageReader reader;
    reader.SetFileName(dicomFiles[index].toStdString().c_str());
    if (!reader.Read()) {
        statusBar()->showMessage("Failed to read slice " + dicomFiles[index]);
        return;
    }

    gdcm::Image& gimg = reader.GetImage();
    QImage qimg = gdcmImageToQImage(gimg, windowCenter, windowWidth);
    m_view->loadBaseImage(qimg);
    m_view->fitInView(m_view->scene()->sceneRect(), Qt::KeepAspectRatio);

    statusBar()->showMessage(QString("Slice %1 / %2").arg(index+1).arg(dicomFiles.size()));
    loadDicomMetadata(dicomFiles[index]);
}

void MainWindow::onNextSlice() {
    if (currentSlice + 1 < (int)dicomFiles.size()) {
        currentSlice++;
        showSlice(currentSlice);
    }
}

void MainWindow::onPrevSlice() {
    if (currentSlice > 0) {
        currentSlice--;
        showSlice(currentSlice);
    }
}

void MainWindow::onLoadDicom() {
    QString fname = QFileDialog::getOpenFileName(this, "Load DICOM file", QString(), "*");
    if (fname.isEmpty()) return;

    gdcm::ImageReader reader;
    reader.SetFileName(fname.toStdString().c_str());
    if(!reader.Read()) {
        statusBar()->showMessage("Failed to read DICOM file");
        return;
    }

    gdcm::Image& gimg = reader.GetImage();
    QImage qimg = gdcmImageToQImage(gimg, windowCenter, windowWidth);

    if (qimg.isNull()) {
        statusBar()->showMessage("Failed to convert DICOM to QImage");
        return;
    }

    m_view->loadBaseImage(qimg);
    m_view->fitInView(m_view->scene()->sceneRect(), Qt::KeepAspectRatio);
    statusBar()->showMessage("DICOM loaded: " + fname);
}

void MainWindow::onLoadBase() {
    QString fname = QFileDialog::getOpenFileName(this, "Load base image", QString(), "Images (*.png *.jpg *.bmp *.tif)");
    if (fname.isEmpty()) return;
    QImage img(fname);
    if (img.isNull()) {
        statusBar()->showMessage("Failed to load image");
        return;
    }
    m_view->loadBaseImage(img);
    m_view->fitInView(m_view->scene()->sceneRect(), Qt::KeepAspectRatio);
    statusBar()->showMessage("Base image loaded: " + fname);
}

void MainWindow::onLoadOverlay() {
    QString fname = QFileDialog::getOpenFileName(this, "Load overlay image", QString(), "Images (*.png *.jpg *.bmp *.tif)");
    if (fname.isEmpty()) return;
    QImage img(fname);
    if (img.isNull()) {
        statusBar()->showMessage("Failed to load overlay");
        return;
    }
    m_view->loadOverlayImage(img);
    m_view->setOverlayOpacity(0.6);
    statusBar()->showMessage("Overlay image loaded: " + fname);
}

void MainWindow::onOpacityChanged(int value) {
    qreal o = value / 100.0;
    m_view->setOverlayOpacity(o);
    statusBar()->showMessage(QString("Overlay opacity: %1%").arg(value));
}

void MainWindow::onToggleOverlay(bool checked) {
    m_view->setOverlayVisible(checked);
}

void MainWindow::onStartDrawROI() {
    m_view->setDrawingEnabled(true);
    statusBar()->showMessage("Draw ROI: click/drag on image");
}

void MainWindow::onClearROI() {
    // remove rectangles (z value 2)
    for (auto item : m_view->scene()->items()) {
        if (item->zValue() == 2) {
            m_view->scene()->removeItem(item);
            delete item;
        }
    }
    statusBar()->showMessage("ROI cleared");
}

void MainWindow::onROIFinished(const QRectF& rect) {
    // Convert scene rect to image (pixel) coords if needed.
    // If base image at (0,0) and no transforms were applied, scene coords == pixel coords.
    QRectF pixelRect = rect; // for now assume 1:1 mapping
    statusBar()->showMessage(QString("ROI: x=%1 y=%2 w=%3 h=%4")
                                 .arg(pixelRect.x()).arg(pixelRect.y())
                                 .arg(pixelRect.width()).arg(pixelRect.height()));
}

void MainWindow::loadDicomMetadata(const QString& file) {
    metaTree->clear();

    gdcm::Reader reader;
    reader.SetFileName(file.toStdString().c_str());
    if (!reader.Read()) return;

    const gdcm::DataSet& ds = reader.GetFile().GetDataSet();
    gdcm::StringFilter sf;
    sf.SetFile(reader.GetFile());

    const gdcm::Dicts& dicts = gdcm::Global::GetInstance().GetDicts();
    const gdcm::Dict& pubDict = dicts.GetPublicDict();

    for (gdcm::DataSet::ConstIterator it = ds.Begin(); it != ds.End(); ++it) {
        const gdcm::DataElement& de = *it;
        gdcm::Tag tag = de.GetTag();

        QString tagStr = QString("(%1,%2)")
            .arg(tag.GetGroup(), 4, 16, QLatin1Char('0'))
            .arg(tag.GetElement(), 4, 16, QLatin1Char('0'))
            .toUpper();
        
        QString value = QString::fromStdString(sf.ToString(de));

        // lookup name in DICOM dictionary
        const gdcm::DictEntry& entry = pubDict.GetDictEntry(tag);
        QString keyword = QString::fromStdString(entry.GetKeyword());

        QTreeWidgetItem* item = new QTreeWidgetItem(metaTree);
        item->setText(0, tagStr + " " + keyword);
        item->setText(1, value);
    }

    metaTree->expandAll();
}

void MainWindow::filterMetadata(const QString& text) {
    for (int i = 0; i < metaTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = metaTree->topLevelItem(i);
        bool match = item->text(0).contains(text, Qt::CaseInsensitive) ||
                     item->text(1).contains(text, Qt::CaseInsensitive);
        item->setHidden(!match);
    }
}
