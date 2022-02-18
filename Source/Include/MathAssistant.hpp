#ifndef Header_MathAssistant
#define Header_MathAssistant


#include <QProcess>


class MathAssistant : public QObject {

	Q_OBJECT

	private:

		using Error = QProcess::ProcessError;
		using Status = QProcess::ExitStatus;

	public:

		static MathAssistant * instance();

		void exec();

	signals:

		void formulaReceived(QString formula);

	private slots:

		void processError(Error);
		void processFinished(int exitCode,Status);

	private:

		MathAssistant();

		static MathAssistant * m_Instance;

		QProcess process;
		QString lastClipboardText;

};

#endif
