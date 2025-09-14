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

double getNumericTag(const gdcm::File& f,
                     const gdcm::DataSet& ds,
                     uint16_t group,
                     uint16_t element,
                     int index = 0) {
    gdcm::Tag tag(group, element);
    if (!ds.FindDataElement(tag)) return 0.0;

    gdcm::DataElement de = ds.GetDataElement(tag);
    gdcm::StringFilter sf;
    sf.SetFile(f);

    QString value = QString::fromStdString(sf.ToString(de));
    // DICOM often uses \ to sepearate numbers
    QStringList parts = value.split('\\');
    if (index < parts.size()) {
        auto p = parts[index];
        bool ok = false;
        auto result = parts[index].toDouble(&ok);
        if (ok) return result;
    }
    return 0.0;
}

int windowCenter = 40;  // just guessing
int windowWidth = 400;  // just guessing

// Helper: convert GDCM imate to QImage (grayscale)
static QImage gdcmImageToQImage(const gdcm::Image& gimg, int windowCenter, int windowWidth) {
    const unsigned int* dims = gimg.GetDimensions();
    int w = dims[0];
    int h = dims[1];

    gdcm::PixelFormat pf = gimg.GetPixelFormat();
    int bits = pf.GetBitsAllocated();       // e.g. 16/8

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
    metaTree->setHeaderLabels(QStringList() << "Tag" << "Value" << "VR");

    // Layout inside dock
    QWidget* metaWidget = new QWidget();
    QVBoxLayout* vbox = new QVBoxLayout(metaWidget);
    vbox->setContentsMargins(2,2,2,2);
    vbox->addWidget(metaFilter);
    vbox->addWidget(metaTree);

    // Vertical Dock
    QDockWidget* dock = new QDockWidget("DICOM Metadata", this);
    dock->setWidget(metaWidget);
    addDockWidget(Qt::RightDockWidgetArea, dock);

    // Combo Box for DICOM series selector
    seriesCombo = new QComboBox(this);
    toolbar->addWidget(seriesCombo);

    connect(metaFilter, &QLineEdit::textChanged, this, &MainWindow::filterMetadata);
    connect(m_view, &ImageView::roiFinished, this, &MainWindow::onROIFinished);
    connect(seriesCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index){
        if (index < 0) return;
        QString uid = seriesCombo->itemData(index).toString();
        currentSeriesUID = uid;
        currentSlice = 0;
        showSlice(currentSlice);
    });

    statusBar()->showMessage("Ready");
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
    QPushButton* prevBtn = new QPushButton("Prev");
    QPushButton* nextBtn = new QPushButton("Next");

    // QSlider* wcSlider = new QSlider(Qt::Horizontal);
    // wcSlider->setRange(-1000, 3000);
    // wcSlider->setValue(windowCenter);
    // QSlider* wwSlider = new QSlider(Qt::Horizontal);
    // wwSlider->setRange(1, 4000);
    // wwSlider->setValue(windowWidth);

    sliceSlider = new QSlider(Qt::Horizontal);
    sliceSlider->setRange(0,0);
    sliceSlider->setValue(0);

    h->addWidget(loadBaseBtn);
    h->addWidget(loadOverlayBtn);
    h->addWidget(opacityLabel);
    h->addWidget(opacitySlider);
    h->addWidget(overlayToggle);
    h->addWidget(drawRoiBtn);
    h->addWidget(clearRoiBtn);
    h->addWidget(loadDicomBtn);
    h->addWidget(loadSeriesBtn);
    h->addWidget(prevBtn);
    h->addWidget(nextBtn);
    // h->addWidget(wcSlider);
    // h->addWidget(wwSlider);
    h->addWidget(sliceSlider);
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
    // connect(wcSlider, &QSlider::valueChanged, this, [=](int v){windowCenter = v; showSlice(currentSlice);});
    // connect(wwSlider, &QSlider::valueChanged, this, [=](int v) {windowWidth = v; showSlice(currentSlice);});
    connect(sliceSlider, &QSlider::valueChanged, this, [this](int v) {showSlice(v);});

    return w;
}

void MainWindow::onLoadDicomSeries() {
    // QString dirPath = QFileDialog::getExistingDirectory(this, "Select DICOM Series Folder");
    auto dirPath = QString("/Users/greg/repo/mri/gk/");
    if (dirPath.isEmpty()) return;

    QDir dir(dirPath);
    QStringList filters;
    filters << "*.dcm" << "*.dicom" << "*";
    QStringList files = dir.entryList(filters, QDir::Files);
    if (files.isEmpty()) return;

    seriesMap.clear();

    for (const QString& f : files) {
        QString fullPath = dir.absoluteFilePath(f);

        gdcm::ImageReader r;
        r.SetFileName(fullPath.toStdString().c_str());
        if (!r.Read()) continue;

        gdcm::Image& gimg = r.GetImage();
        QImage qimg = gdcmImageToQImage(gimg, windowCenter, windowWidth);

        const gdcm::File& file = r.GetFile();
        const gdcm::DataSet& ds = file.GetDataSet();

        d3m::SliceInfo slice;
        slice.image      = qimg;
        slice.filePath   = fullPath;
        slice.instanceNumber = (int)getNumericTag(file, ds, d3m::InstanceNumber.group, d3m::InstanceNumber.element);
        slice.pixelSpacingX  = getNumericTag(file, ds, d3m::PixelSpacing.group, d3m::PixelSpacing.element, 0);
        slice.pixelSpacingY  = getNumericTag(file, ds, d3m::PixelSpacing.group, d3m::PixelSpacing.element, 1);
        slice.sliceThickness = getNumericTag(file, ds, d3m::SliceThickness.group, d3m::SliceThickness.element);
        slice.imagePosX      = getNumericTag(file, ds, d3m::ImagePositionPatient.group, d3m::ImagePositionPatient.element, 0);
        slice.imagePosY      = getNumericTag(file, ds, d3m::ImagePositionPatient.group, d3m::ImagePositionPatient.element, 1);
        slice.imagePosZ      = getNumericTag(file, ds, d3m::ImagePositionPatient.group, d3m::ImagePositionPatient.element, 2);
        slice.rowCosX        = getNumericTag(file, ds, d3m::ImageOrientationPatient.group, d3m::ImageOrientationPatient.element, 0);
        slice.rowCosY        = getNumericTag(file, ds, d3m::ImageOrientationPatient.group, d3m::ImageOrientationPatient.element, 1);
        slice.rowCosZ        = getNumericTag(file, ds, d3m::ImageOrientationPatient.group, d3m::ImageOrientationPatient.element, 2);
        slice.colCosX        = getNumericTag(file, ds, d3m::ImageOrientationPatient.group, d3m::ImageOrientationPatient.element, 3);
        slice.colCosY        = getNumericTag(file, ds, d3m::ImageOrientationPatient.group, d3m::ImageOrientationPatient.element, 4);
        slice.colCosZ        = getNumericTag(file, ds, d3m::ImageOrientationPatient.group, d3m::ImageOrientationPatient.element, 5);

        slice.sliceLocation = slice.imagePosZ; // fallback if instanceNumber missing

        // Series UID
        gdcm::Tag seriesUIDTag(d3m::SeriesInstanceUID.group, d3m::SeriesInstanceUID.element);
        if (ds.FindDataElement(seriesUIDTag)) {
            gdcm::DataElement de = ds.GetDataElement(seriesUIDTag);
            gdcm::StringFilter sf;
            sf.SetFile(file);
            slice.seriesUID = QString::fromStdString(sf.ToString(de));
        }

        // Series Description (optional)
        gdcm::Tag seriesDescTag(d3m::SeriesDesc.group, d3m::SeriesDesc.element);
        if (ds.FindDataElement(seriesDescTag)) {
            gdcm::DataElement de = ds.GetDataElement(seriesDescTag);
            gdcm::StringFilter sf;
            sf.SetFile(file);
            slice.seriesDesc = QString::fromStdString(sf.ToString(de));
        }

        if (!slice.seriesUID.isEmpty()) {
            seriesMap[slice.seriesUID].push_back(slice);
        }
    }

    // Sort slices inside each series
    for (auto& kv : seriesMap) {
        auto& stack = kv.second;
        std::sort(stack.begin(), stack.end(), [](const d3m::SliceInfo& a, const d3m::SliceInfo& b) {
            if (a.instanceNumber > 0 && b.instanceNumber > 0)
                return a.instanceNumber < b.instanceNumber;
            return a.sliceLocation < b.sliceLocation;
        });
    }

    // Populate combo box
    seriesCombo->clear();
    for (const auto& kv : seriesMap) {
        QString uid = kv.first;
        QString desc = kv.second.front().seriesDesc.isEmpty() ? uid : kv.second.front().seriesDesc;
        seriesCombo->addItem(desc, uid);
    }
}

bool MainWindow::showSlice(int index) {
    auto it = seriesMap.find(currentSeriesUID);
    if (it == seriesMap.end()) return false;
    const auto& stack = it->second;
    if (stack.empty()) return false;

    int maxIndex = static_cast<int>(stack.size()) - 1;
    if (index < 0) index = 0;
    if (index > maxIndex) index = maxIndex;
    currentSlice = index;

    sliceSlider->setRange(0, maxIndex);
    sliceSlider->setValue(index);

    const d3m::SliceInfo& slice = stack[index];
    m_view->loadBaseImage(slice.image);
    m_view->fitInView(m_view->scene()->sceneRect(), Qt::KeepAspectRatio);

    loadDicomMetadata(slice.filePath);

    statusBar()->showMessage(QString("Series: %1 | Slice %2 / %3")
        .arg(slice.seriesDesc.isEmpty() ? "Unknown" : slice.seriesDesc)
        .arg(index+1).arg(stack.size()));
    return true;
}

void MainWindow::onNextSlice() {
    currentSlice++;
    if (!showSlice(currentSlice))
        currentSlice--;
}

void MainWindow::onPrevSlice() {
    if (currentSlice > 0) {
        currentSlice--;
        if (!showSlice(currentSlice))
            currentSlice++;
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
        gdcm::VR vr = entry.GetVR();
        QString vrStr = QString::fromStdString(gdcm::VR::GetVRString(vr));

        QTreeWidgetItem* item = new QTreeWidgetItem(metaTree);
        item->setText(0, tagStr + " " + keyword);
        item->setText(1, value);
        item->setText(2, vrStr);
    }

    metaTree->expandAll();
}

void MainWindow::filterMetadata(const QString& text) {
    for (int i = 0; i < metaTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = metaTree->topLevelItem(i);
        bool match = item->text(0).contains(text, Qt::CaseInsensitive) ||
                     item->text(1).contains(text, Qt::CaseInsensitive) ||
                     item->text(2).contains(text, Qt::CaseInsensitive);
        item->setHidden(!match);
    }
}

void MainWindow::extractSliceMetadata(const QString& file) {
    gdcm::Reader reader;
    reader.SetFileName(file.toStdString().c_str());
    if (!reader.Read()) return;

    const gdcm::File& f = reader.GetFile();
    const gdcm::DataSet& ds = f.GetDataSet();

    double pixelSpacingX = getNumericTag(f, ds, d3m::PixelSpacing.group, d3m::PixelSpacing.element, 0);
    double pixelSpacingY = getNumericTag(f, ds, d3m::PixelSpacing.group, d3m::PixelSpacing.element, 1);
    double sliceThickness = getNumericTag(f, ds, d3m::SliceThickness.group, d3m::SliceThickness.element);
    double imagePosX = getNumericTag(f, ds, d3m::ImagePositionPatient.group, d3m::ImagePositionPatient.element, 0);
    double imagePosY = getNumericTag(f, ds, d3m::ImagePositionPatient.group, d3m::ImagePositionPatient.element, 1);
    double imagePosZ = getNumericTag(f, ds, d3m::ImagePositionPatient.group, d3m::ImagePositionPatient.element, 2);
    double rowCosX = getNumericTag(f, ds, d3m::ImageOrientationPatient.group, d3m::ImageOrientationPatient.element, 0);
    double rowCosY = getNumericTag(f, ds, d3m::ImageOrientationPatient.group, d3m::ImageOrientationPatient.element, 1);
    double rowCosZ = getNumericTag(f, ds, d3m::ImageOrientationPatient.group, d3m::ImageOrientationPatient.element, 2);
    double colCosX = getNumericTag(f, ds, d3m::ImageOrientationPatient.group, d3m::ImageOrientationPatient.element, 3);
    double colCosY = getNumericTag(f, ds, d3m::ImageOrientationPatient.group, d3m::ImageOrientationPatient.element, 4);
    double colCosZ = getNumericTag(f, ds, d3m::ImageOrientationPatient.group, d3m::ImageOrientationPatient.element, 5);

    QDebug(QtMsgType::QtInfoMsg) << "Pixel spacing: " << pixelSpacingX << pixelSpacingY;
    QDebug(QtMsgType::QtInfoMsg) << "Pixel spacing: " << sliceThickness;
    QDebug(QtMsgType::QtInfoMsg) << "Pixel spacing: " << imagePosX << imagePosY << imagePosZ;
    QDebug(QtMsgType::QtInfoMsg) << "Pixel spacing: " << rowCosX << rowCosY << rowCosZ;
    QDebug(QtMsgType::QtInfoMsg) << "Pixel spacing: " << colCosX << colCosY << colCosZ;
}
