#ifndef Header_CursorPosition
#define Header_CursorPosition


class QDocument;
class QDocumentLineHandle;
class QDocumentCursor;


struct QDocumentLineTrackedHandle {

	public:

		QDocumentLineTrackedHandle(const QDocumentCursor &);

		QDocumentLineHandle * dlh() const {
			return m_dlh;
		}

		QDocument * doc() const {
			return m_doc;
		}

		int oldLineNumber() const {
			return m_oldLineNumber;
		}

		int lineNumber() const;
		bool isValid() const;

	protected:

		QDocumentLineHandle * m_dlh;
		QDocument * m_doc;

		mutable int m_oldLineNumber;

};


class CursorPosition: public QDocumentLineTrackedHandle {

	public:

		CursorPosition(const QDocumentCursor &);

		QDocumentCursor toCursor();

		bool equals(const CursorPosition &) const;

		int columnNumber() const {
			return m_columnNumber;
		}

		void setColumnNumber(int col){
			m_columnNumber = col;
		}

	private:

		int m_columnNumber;

};


#endif
