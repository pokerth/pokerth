#include <QtGui>
/*
* This QSlider Extension was sponsored by: AZEVEDO Filipe aka Nox P@sNox <pasnox@gmail.com>
* http://pasnox.tuxfamily.org
* Thx a lot!
*/

class Slider : public QSlider
{
	Q_OBJECT

public:
	Slider( QWidget* parent = 0 )
		: QSlider( parent ) {
	}

	Slider( Qt::Orientation orientation, QWidget* parent = 0 )
		: QSlider( orientation, parent ) {
	}

protected:
	int pixelPosToRangeValue(int pos) const { // getted from QSlider.cpp with little adapt
		QStyleOptionSlider opt;
		initStyleOption(&opt);
		QRect gr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);
		QRect sr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
		int sliderMin, sliderMax, sliderLength;

		if (orientation() == Qt::Horizontal) {
			sliderLength = sr.width();
			sliderMin = gr.x();
			sliderMax = gr.right() - sliderLength + 1;
		} else {
			sliderLength = sr.height();
			sliderMin = gr.y();
			sliderMax = gr.bottom() - sliderLength + 1;
		}
		return QStyle::sliderValueFromPosition(minimum(), maximum(), pos - sliderMin,
											   sliderMax - sliderMin, opt.upsideDown);
	}

	int pick(const QPoint &pt) const { // getted from QSlider.cpp with little adapt
		return orientation() == Qt::Horizontal ? pt.x() : pt.y();
	}

	virtual void mousePressEvent( QMouseEvent* ev ) { // getted from QSlider.cpp with little adapt
		if (maximum() == minimum() || (ev->buttons() ^ ev->button())) {
			ev->ignore();
			return;
		}
#ifdef QT_KEYPAD_NAVIGATION
		if (QApplication::keypadNavigationEnabled())
			setEditFocus(true);
#endif
		ev->accept();

		//if ((ev->button() & style()->styleHint(QStyle::SH_Slider_AbsoluteSetButtons)) == ev->button())
		{
			QStyleOptionSlider opt;
			initStyleOption(&opt);
			const QRect sliderRect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
			const QPoint center = sliderRect.center() - sliderRect.topLeft();
			// to take half of the slider off for the setSliderPosition call we use the center - topLeft

			setSliderPosition(pixelPosToRangeValue(pick(ev->pos() - center)));
			triggerAction(SliderMove);
			setRepeatAction(SliderNoAction);
			//d->pressedControl = QStyle::SC_SliderHandle;
			update();
		}

		//if (d->pressedControl == QStyle::SC_SliderHandle)
		{
			QStyleOptionSlider opt;
			initStyleOption(&opt);
			setRepeatAction(SliderNoAction);
			QRect sr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
			//d->clickOffset = pick(ev->pos() - sr.topLeft());
			update(sr);
			setSliderDown(true);
		}

		QSlider::mousePressEvent( ev ); // should not be needed but as we can't update private d pointer it's needed.
	}
};
