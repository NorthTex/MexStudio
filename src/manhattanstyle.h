/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef Header_ManhattenStyle
#define Header_ManhattenStyle


#include <QStyle>
#include <QProxyStyle>


QT_BEGIN_NAMESPACE
	class QLinearGradient;
	class QBrush;
QT_END_NAMESPACE


class ManhattanStylePrivate;


class ManhattanStyle : public QProxyStyle {

	Q_OBJECT

	private:

		using Painter = QPainter *;
		using Widget = const QWidget *;
		using Style = const QStyleOption *;
		using ComplexStyle = const QStyleOptionComplex *;
		using Pixmap = const QPixmap &;
		using Rectangle = const QRect &;

	public:

		ManhattanStyle(const QString &);

		~ManhattanStyle();

		bool isValid();

		void drawComplexControl(ComplexControl,ComplexStyle,Painter,Widget = nullptr) const;
		void drawPrimitive(PrimitiveElement,Style,Painter,Widget = nullptr) const;
		void drawControl(ControlElement,Style,Painter,Widget = nullptr) const;

		QSize sizeFromContents(ContentsType,Style,const QSize & size,Widget) const;
		QRect subControlRect(ComplexControl,ComplexStyle,SubControl,Widget) const;
		QRect subElementRect(SubElement element,Style,Widget) const;

		SubControl hitTestComplexControl(ComplexControl,ComplexStyle,const QPoint & pos,Widget = nullptr) const;
		QRect itemRect(Painter,Rectangle,int flags,bool enabled,Pixmap,const QString & text,int len = -1) const;

		QPixmap standardPixmap(StandardPixmap,Style,Widget = nullptr) const;
		QPixmap generatedIconPixmap(QIcon::Mode iconMode,Pixmap,Style) const;

		int styleHint(StyleHint,Style = nullptr,Widget = nullptr,QStyleHintReturn * returnData = nullptr) const;
		int pixelMetric(PixelMetric,Style = nullptr,Widget = nullptr) const;

		QPalette standardPalette() const;

		void polish(QWidget *);
		void polish(QPalette &);
		void polish(QApplication *);

		void unpolish(QWidget *);
		void unpolish(QApplication *);

	protected:

		bool event(QEvent *);

	protected Q_SLOTS:

		QIcon standardIconImplementation(StandardPixmap icon,Style,Widget) const;
		int layoutSpacingImplementation(QSizePolicy::ControlType,QSizePolicy::ControlType,
										Qt::Orientation,Style = nullptr,
										Widget = nullptr) const;

	private:

		ManhattanStylePrivate * d;
		Q_DISABLE_COPY(ManhattanStyle)

};


#endif
