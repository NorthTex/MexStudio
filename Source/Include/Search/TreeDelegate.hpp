#ifndef Header_Search_TreeDelegate
#define Header_Search_TreeDelegate


#include <QItemDelegate>


class SearchTreeDelegate : public QItemDelegate {

	Q_OBJECT

	private:

		using Option = const QStyleOptionViewItem &;
		using Index = const QModelIndex &;
		using Painter = QPainter *;

	public:

		SearchTreeDelegate(QString editorFontFamily,QObject * parent = nullptr);

	protected:

		void paint(Painter,Option,Index) const;
		QSize sizeHint(Option,Index) const;

	private:

		QString m_editorFontFamily;
};


#endif
