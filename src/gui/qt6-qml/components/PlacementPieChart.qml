import QtQuick

// Pie chart of the placement distribution (season stats), drawn with a
// Canvas – deliberately without QtCharts, so as not to introduce another Qt
// module dependency. `values` are the frequencies per place (index 0 = place 1),
// `colors` the matching palette (Config.StaticData.placementColors). Slices are
// set apart with a white separating line – just like on pokerth.net.
Canvas {
    id: pie

    property var values: []
    property var colors: []

    // Redraw as soon as data, palette or size change.
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

        // Start at the top (−90°) and go clockwise – like Chart.js.
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
