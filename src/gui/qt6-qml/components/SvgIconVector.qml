import QtQuick
import QtQuick.VectorImage

// Real vector SVG wrapper. On Qt >= 6.8 (CMake selection) it is aliased into
// the resource as components/SvgIcon.qml -> SVG icons render as vectors
// (sharp at any size, no rasterization). The interface (source / fillMode /
// width / height / visible / smooth / rotation / layer) is source compatible
// with the image fallback variant.
VectorImage {
    fillMode: VectorImage.PreserveAspectFit
    // The CurveRenderer rasterizes the SVG curves analytically on the GPU with
    // built-in, resolution independent antialiasing. The default
    // (GeometryRenderer, triangulated) needs MSAA for smooth edges, which the
    // scene does not enable -> otherwise roundings (e.g. the poker chip logo) look
    // "pixelated"/stair-stepped (visible on the Steam Deck among others, native 1280×800).
    preferredRendererType: VectorImage.CurveRenderer
}
