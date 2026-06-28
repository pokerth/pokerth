import QtQuick
import QtQuick.VectorImage

// Echter Vektor-SVG-Wrapper. Wird auf Qt >= 6.8 (CMake-Auswahl) als
// components/SvgIcon.qml ins Resource aliased -> SVG-Icons rendern vektoriell
// (scharf bei jeder Größe, keine Rasterisierung). Schnittstelle (source /
// fillMode / width / height / visible / smooth / rotation / layer) ist
// quellkompatibel zur Image-Fallback-Variante.
VectorImage {
    fillMode: VectorImage.PreserveAspectFit
    // CurveRenderer rastert die SVG-Kurven analytisch auf der GPU mit
    // eingebautem, auflösungsunabhängigem Antialiasing. Der Default
    // (GeometryRenderer, trianguliert) braucht für glatte Kanten MSAA, das die
    // Szene nicht aktiviert -> Rundungen (z.B. Pokerchip-Logo) wirken sonst
    // „verpixelt"/treppig (sichtbar u.a. auf dem Steam Deck, natives 1280×800).
    preferredRendererType: VectorImage.CurveRenderer
}
