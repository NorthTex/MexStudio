#ifndef Header_RandomTextGenerator
#define Header_RandomTextGenerator


#include "qdocument.h"

#include <QDialog>
#include <tuple>


namespace Ui { class RandomTextGenerator; }


class RandomTextGenerator : public QDialog {

	Q_OBJECT
	Q_DISABLE_COPY(RandomTextGenerator)

	private:

		Ui::RandomTextGenerator * ui;

		QList<QString> words;

		QTextStream textStream;
		QStringList lines;

		QString chars , text;

	private:

		void newWordForStream(const QString & word);
		void newWordForText(const QString & word);

		std::tuple<bool,int> findConfig() const;

	private slots:

		void generateText();
		void resetWords();

	protected:

		virtual void changeEvent(QEvent *);

	public:

		explicit RandomTextGenerator(QWidget * parent = 0,const QStringList & textLines = QStringList());
		virtual ~RandomTextGenerator();
};


#endif
