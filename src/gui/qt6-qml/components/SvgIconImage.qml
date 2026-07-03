import QtQuick
import QtQuick.Window

// Image-basierter SVG-Wrapper (QtSvg-Rasterizer). Fallback für Qt < 6.8
// (QtQuick.VectorImage gibt es dort nicht) – wird per CMake-Auswahl als
// components/SvgIcon.qml ins Resource aliased. sourceSize ist an die
// Anzeigegröße × devicePixelRatio gekoppelt, damit die SVGs auf High-DPI
// möglichst scharf rastern. Schnittstelle ist quellkompatibel zur
// VectorImage-Variante (source / fillMode / width / height / visible …).
Image {
    fillMode: Image.PreserveAspectFit
    smooth: true
    mipmap: true
    sourceSize.width: width > 0 ? Math.ceil(width * Screen.devicePixelRatio) : 0
    sourceSize.height: height > 0 ? Math.ceil(height * Screen.devicePixelRatio) : 0
}
