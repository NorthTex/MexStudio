#ifndef Header_ExecProgram
#define Header_ExecProgram


#include <QString>


class ExecProgram {

	private:

		static void pathSet(const QString & path);

	private:

		void setProgramAndArguments(const QString & data);
		void setWinProcModifier(QProcess & process) const;

		QString pathExtend(void) const;

	public:

		// Input parameters
		QStringList m_arguments;

		QString 
			m_additionalSearchPaths,
			m_workingDirectory,
			m_program;

		#ifdef Q_OS_WIN
			QProcess::CreateProcessArgumentModifier m_winProcModifier;
		#endif

		// Output parameters. Only assigned by synchronous execAndWait
		// If false then program either did not run or crashed

		bool m_normalRun;
		int m_exitCode;

		QString 
			m_standardOutput,
			m_standardError;

	public:

		ExecProgram(void);
		
		ExecProgram(
			const QString & shellCommandLine,
			const QString & additionalSearchPaths,
			const QString & workingDirectory = QString());
		
		ExecProgram(
			const QString & progName,
			const QStringList & arguments,
			const QString & additionalSearchPaths = QString(),
			const QString & workingDirectory = QString());

	public:

		void execAndNoWait(QProcess & process) const;
		bool execDetached(void) const;
		bool execAndWait(void);
};


#endif
