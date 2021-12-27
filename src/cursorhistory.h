/***************************************************************************
 *   copyright       : (C) 2012 by Tim Hoffmann                            *
 *   http://texstudio.sourceforge.net/                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef Header_CursorHistory
#define Header_CursorHistory


#include "mostQtHeaders.h"
#include "cursorposition.h"

#include "Latex/Document.hpp"


using CursorPosList = std::list<CursorPosition>;


class CursorHistory : public QObject {

	Q_OBJECT

	public:

		explicit CursorHistory(LatexDocuments *);

		uint maxLength(){
			return m_maxLength;
		}

		bool insertPos(QDocumentCursor,bool deleteBehindCurrent = true);
		QDocumentCursor currentPos();

		void setInsertionEnabled(bool);
		void setForwardAction(QAction * forward);
		void setBackAction(QAction * back);

		QAction * forwardAction(){
			return m_forwardAction;
		}

		QAction * backAction(){
			return m_backAction;
		}

		int count(){
			return history.size();
		}

		void debugPrint();
		void clear();

	public slots:

		QDocumentCursor forward(const QDocumentCursor & current = QDocumentCursor());
		QDocumentCursor back(const QDocumentCursor & current = QDocumentCursor());

	private slots:

		void aboutToDeleteDoc(LatexDocument *);
		void documentClosed(QObject *);

		void lineDeleted(QDocumentLineHandle *,int hint = -1);

	private:

		void updateNavActions();
		void removeEntry(CursorPosList::iterator &);
		bool currentEntryValid();
		void validate();

		CursorPosList::iterator prevValidEntry(const CursorPosList::iterator & start);
		CursorPosList::iterator nextValidEntry(const CursorPosList::iterator & start);

		CursorPosList::iterator currentEntry;
		CursorPosList history;

		QAction * m_backAction;
		QAction * m_forwardAction;

		bool m_insertionEnabled;
		uint m_maxLength;

};


#endif
