/*
 * m11checkbox.h
 * Copyright (C) 2008  Yandex LLC (Alexei Matiouchkine)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef M11CHECKBOX_H
#define M11CHECKBOX_H

#include <QCheckBox>
#include <QStylePainter>

#include "utils.h"

namespace Ya
{
	class M11CheckBox : public ::QCheckBox
	{
		Q_OBJECT
		public:
			M11CheckBox();
			M11CheckBox(const QString &);
			virtual ~M11CheckBox();

		protected:
			// reimplemented
			virtual void paintEvent(QPaintEvent *);
			virtual void enterEvent(QEvent *);
			virtual void leaveEvent(QEvent *);
			virtual void mousePressEvent(QMouseEvent *);
			virtual void mouseReleaseEvent(QMouseEvent *);

			// internal helpers
			virtual void drawCombo(QStylePainter *p, const QRect &, const Qt::CheckState checkState);
		protected:
			int m_margin;
			bool m_rounded;
			Utils::MouseState m_mouseState;

		protected:
			QPixmap imgComboOff;
			QPixmap imgComboOn;
			QPixmap imgComboOnOff;
	};
}

#endif
