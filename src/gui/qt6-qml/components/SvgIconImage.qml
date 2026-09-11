import QtQuick
import QtQuick.Window

// Image-based SVG wrapper (QtSvg rasterizer). Fallback for Qt < 6.8
// (QtQuick.VectorImage does not exist there) – aliased into the resource as
// components/SvgIcon.qml by the CMake selection. sourceSize is coupled to the
// display size × devicePixelRatio so that the SVGs rasterize as sharply as
// possible on high DPI. The interface is source compatible with the
// VectorImage variant (source / fillMode / width / height / visible …).
Image {
    fillMode: Image.PreserveAspectFit
    smooth: true
    mipmap: true
    sourceSize.width: width > 0 ? Math.ceil(width * Screen.devicePixelRatio) : 0
    sourceSize.height: height > 0 ? Math.ceil(height * Screen.devicePixelRatio) : 0
}
