/*
 * combinedsyntaxhighlighter.cpp - a class to apply multiple syntax highlighters on a single QTextEdit
 * Copyright (C) 2008  Yandex LLC (Michail Pishchagin)
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

#include "combinedsyntaxhighlighter.h"

#include <QList>

#include "syntaxhighlighter.h"

CombinedSyntaxHighlighter::CombinedSyntaxHighlighter(QTextEdit* parent)
	: QSyntaxHighlighter(parent)
{
}

CombinedSyntaxHighlighter::CombinedSyntaxHighlighter(QTextDocument* parent)
	: QSyntaxHighlighter(parent)
{
}

void CombinedSyntaxHighlighter::highlightBlock(const QString &text)
{
	foreach(SyntaxHighlighter* highlighter, findChildren<SyntaxHighlighter*>()) {
		highlighter->highlightBlock(text);
	}
}
