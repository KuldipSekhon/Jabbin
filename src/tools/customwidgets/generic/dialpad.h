/*
 *   Copyright (C) 2007 Ivan Cukic <ivan.cukic+kde@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser/Library General Public License version 2,
 *   or (at your option) any later version, as published by the Free
 *   Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser/Library General Public License for more details
 *
 *   You should have received a copy of the GNU Lesser/Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef DIALPAD_H
#define DIALPAD_H

#include <QFrame>

namespace CustomWidgets {

/**
 * This class is implementing a dial pad with
 */
class Dialpad : public QFrame {
    Q_OBJECT

public:
    enum CallStatus {
        Default,
        Calling, Accepting, Rejecting, Terminating,
        Accepted, Rejected, InProgress, Terminated, Incoming
    };

    /**
     * Creates a new Dialpad
     */
    Dialpad(QWidget * parent = 0);

    /**
     * Destroys this Dialpad
     */
    ~Dialpad();

Q_SIGNALS:
    /**
     * This signal is emitted when a button is clicked
     */
    void buttonClicked(char key);

protected Q_SLOTS:
    /**
     * One of the dialpad buttons is clicked
     * sender() must be set for this to work
     */
    void dialpadButtonClicked();

private:
    class Private;
    Private * const d;
};

} // namespace CustomWidgets

#endif // DIALPAD_H

