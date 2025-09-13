#pragma once
#include <cstdint>

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

} // namespace dicom
