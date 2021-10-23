#ifndef Header_LatexReference
#define Header_LatexReference


#include <QObject>
#include <QString>
#include <QHash>


class LatexReference : public QObject {

	Q_OBJECT

	private:

		using Command = const QString &;

		struct Anchor {

			Anchor()
				: name(QString())
				, start_pos(-1)
				, end_pos(-1) {}

			Anchor(const QString & name)
				: name(name)
				, start_pos(-1)
				, end_pos(-1) {}

			QString name;
			int start_pos;
			int end_pos;

		};

	private:

		//	maps cmds to their anchors (of their section)
		QHash<QString,Anchor> anchors , sectionAnchors;
		QString filename , html;

	public:

		explicit LatexReference(QObject * parent = 0);

		bool contains(Command);
		void setFile(QString filename);

		QString tooltipText(Command);
		QString getSectionText(Command);
		QString getPartialText(Command);

	protected:

		void makeIndex();

};


#endif
