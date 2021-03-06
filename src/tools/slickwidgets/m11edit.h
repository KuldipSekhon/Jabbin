/*
 * m11edit.h
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

#ifndef M11EDIT_H
#define M11EDIT_H

#include <QLineEdit>

#include "utils.h"

namespace Ya
{
	class M11Edit : public QLineEdit
	{
		Q_OBJECT
		public:
			M11Edit(QWidget *parent = 0);
			M11Edit(const QString &, QWidget *parent = 0);
			virtual ~M11Edit();

		protected:
			// reimplemented
			virtual void paintEvent(QPaintEvent *);
			virtual void focusInEvent(QFocusEvent *);
			virtual void focusOutEvent(QFocusEvent *);
			virtual void enterEvent(QEvent *);
			virtual void leaveEvent(QEvent *);

		protected:
			int m_margin;
			bool m_rounded;
			Utils::MouseState m_mouseState;
	};
}

#endif
