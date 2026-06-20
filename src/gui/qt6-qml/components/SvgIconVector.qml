import QtQuick
import QtQuick.VectorImage

// Echter Vektor-SVG-Wrapper. Wird auf Qt >= 6.8 (CMake-Auswahl) als
// components/SvgIcon.qml ins Resource aliased -> SVG-Icons rendern vektoriell
// (scharf bei jeder Größe, keine Rasterisierung). Schnittstelle (source /
// fillMode / width / height / visible / smooth / rotation / layer) ist
// quellkompatibel zur Image-Fallback-Variante.
VectorImage {
    fillMode: VectorImage.PreserveAspectFit
}
