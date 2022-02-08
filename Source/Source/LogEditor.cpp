/***************************************************************************
 *   copyright       : (C) 2003-2007 by Pascal Brachet                     *
 *   http://www.xm1math.net/texmaker/                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "LogEditor.hpp"
#include "configmanager.h"


LogEditor::LogEditor(QWidget * parent) 
	: QTextEdit(parent) {

	highlighter = new LogHighlighter(document());
	
	const auto config = ConfigManager::getInstance();
	const auto fontFamily = config -> getOption("LogView/FontFamily");

	if(fontFamily.isValid())
		setFontFamily(fontFamily.toString());
	
	bool ok;
	int fontSize = config -> getOption("LogView/FontSize").toInt(& ok);
	
	if(ok && fontSize > 0)
		setFontPointSize(fontSize);

	UtilsUi::enableTouchScrolling(this);
}


LogEditor::~LogEditor(){}


void LogEditor::wheelEvent(QWheelEvent * event){

	const auto config = ConfigManager::getInstance();
	
	if(! config -> getOption("Editor/Mouse Wheel Zoom").toBool())
		event -> setModifiers(event -> modifiers() & ~Qt::ControlModifier);

	QTextEdit::wheelEvent(event);
}


void LogEditor::insertLine(const QString & line){
    append(line);
}


void LogEditor::setCursorPosition(int para,int index){

	auto cursor = textCursor();
	int i = 0;

	auto textblock = document() -> begin();
	
	while(textblock.isValid()){

		if(i == para)
			break;
		
		i++;
		
		textblock = textblock.next();
	}
	
	cursor.movePosition(QTextCursor::End);
	setTextCursor(cursor);

	cursor.setPosition(textblock.position() + index,QTextCursor::MoveAnchor);
	setTextCursor(cursor);
	
	ensureCursorVisible();
}


void LogEditor::mouseDoubleClickEvent(QMouseEvent *){
	emit clickOnLogLine(textCursor().blockNumber());
}

void LogEditor::paintEvent(QPaintEvent * event){

	auto rect = cursorRect();
	rect.setX(0);
	rect.setWidth(viewport() -> width());
	
	QPainter painter(viewport());
	
	auto brush = palette().base();
	const auto color = UtilsUi::mediumLightColor(brush.color(),110);
	brush.setColor(color);
	
	painter.fillRect(rect,brush);
	painter.end();
	
	QTextEdit::paintEvent(event);
}
