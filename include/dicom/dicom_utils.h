#pragma once
#include <cstdint>
#include <QImage>
#include <QString>

namespace d3m {

struct Tag {
    uint16_t group;
    uint16_t element;
};

// Patient Info
inline constexpr Tag PatientName                = {0x0010, 0x0010};
inline constexpr Tag PatientId                  = {0x0010, 0x0020};

// Study/Series Info
inline constexpr Tag StudyDate                  = {0x0008, 0x0020};
inline constexpr Tag Modality                   = {0x0008, 0x0060};
inline constexpr Tag SeriesDesc                 = {0x0008, 0x103E};
inline constexpr Tag SeriesInstanceUID          = {0x0020, 0x000E};

// Image Geometry
inline constexpr Tag InstanceNumber             = {0x0020, 0x0013};
inline constexpr Tag ImagePositionPatient       = {0x0020, 0x0032};
inline constexpr Tag ImageOrientationPatient    = {0x0020, 0x0037};
inline constexpr Tag SliceThickness             = {0x0018, 0x0050};
inline constexpr Tag PixelSpacing               = {0x0028, 0x0030};

// Acquisition Timing
inline constexpr Tag AcquisitionTime            = {0x0008, 0x0032};
inline constexpr Tag TriggerTime                = {0x0018, 0x1060};
inline constexpr Tag SliceLocation              = {0x0018, 0x1041};

struct SliceInfo {
    QImage image;       // the slice image (converted to QImage)
    QString filePath;   // path to the DICOM file
    QString seriesUID;
    QString seriesDesc;

    // geometry
    int instanceNumber = -1;
    double pixelSpacingX = 0.0;
    double pixelSpacingY = 0.0;
    double sliceThickness = 0.0;
    double imagePosX = 0.0;
    double imagePosY = 0.0;
    double imagePosZ = 0.0;
    double sliceLocation = 0;

    // orientation cosines
    double rowCosX = 0.0;
    double rowCosY = 0.0;
    double rowCosZ = 0.0;
    double colCosX = 0.0;
    double colCosY = 0.0;
    double colCosZ = 0.0;

    // optionaL window/level used to render this slice
    int windowCenter = -1;
    int windowWidth = -1;
};

} // namespace dicom
