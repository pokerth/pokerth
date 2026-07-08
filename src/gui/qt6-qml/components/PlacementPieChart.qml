import QtQuick

// Kreisdiagramm der Platzierungs-Verteilung (Season Stats), gezeichnet mit
// Canvas – bewusst ohne QtCharts, um keine zusätzliche Qt-Modul-Abhängigkeit
// einzuführen. `values` sind die Häufigkeiten je Platz (Index 0 = Platz 1),
// `colors` die zugehörige Palette (Config.StaticData.placementColors). Slices
// werden – wie auf pokerth.net – mit weißer Trennlinie abgesetzt.
Canvas {
    id: pie

    property var values: []
    property var colors: []

    // Neu zeichnen, sobald sich Daten, Palette oder Größe ändern.
    onValuesChanged: requestPaint()
    onColorsChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    onPaint: {
        var ctx = getContext("2d")
        ctx.reset()

        var total = 0
        for (var i = 0; i < values.length; ++i)
            total += Number(values[i]) || 0
        if (total <= 0)
            return

        var cx = width / 2
        var cy = height / 2
        var r = Math.min(width, height) / 2 - 2

        // Start oben (−90°) und im Uhrzeigersinn – wie Chart.js.
        var start = -Math.PI / 2
        ctx.lineWidth = 2
        ctx.strokeStyle = "#ffffff"
        for (var j = 0; j < values.length; ++j) {
            var v = Number(values[j]) || 0
            if (v <= 0)
                continue
            var end = start + (v / total) * 2 * Math.PI
            ctx.beginPath()
            ctx.moveTo(cx, cy)
            ctx.arc(cx, cy, r, start, end)
            ctx.closePath()
            ctx.fillStyle = colors[j % colors.length]
            ctx.fill()
            ctx.stroke()
            start = end
        }
    }
}
