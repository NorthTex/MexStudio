#ifndef Header_Latex_Repository
#define Header_Latex_Repository


#include "mostQtHeaders.h"


class LatexPackageInfo {

	public:

		QString name , shortDescription;
		bool installed;

	public:

		LatexPackageInfo(const QString & name = QString(),const QString & description = QString(),bool installed = false)
			: name(name)
			, shortDescription(description)
			, installed(installed) {}

};


Q_DECLARE_METATYPE(LatexPackageInfo)


class LatexRepository : public QObject {

	Q_OBJECT

	private:

		using Name = const QString &;
		using Repository = const LatexRepository &;

	public:

		static LatexRepository * instance();

		enum DataSource { None , Static , Texlive , Miktex };

		DataSource dataSource();
		bool packageExists(Name);
		QString shortDescription(Name);

	private:

		LatexRepository();
		LatexRepository(Repository);
		LatexRepository & operator = (Repository);

		bool loadStaticPackageList(const QString & file);

		static LatexRepository * m_Instance;

		QHash<QString, LatexPackageInfo> packages; // name, short description
		DataSource m_dataSource;

};


#endif
