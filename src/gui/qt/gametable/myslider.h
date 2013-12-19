/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/
#ifndef MYSLIDER_H
#define MYSLIDER_H

#include <QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets>
#endif

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

#endif

