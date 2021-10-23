#ifndef Header_UtilsVersion
#define Header_UtilsVersion

#define TEXSTUDIO "TeXstudio"


#define TXSVERSION "4.0.2"
#define TXSVERSION_NUMERIC 0x040002



//#define IS_DEVELOPMENT_VERSION (TXSVERSION_NUMERIC & 0x000001)
#define IS_DEVELOPMENT_VERSION 0 // odd numbers have not been used at all, git version gives a much clearer insight about the used version

extern const char * TEXSTUDIO_GIT_REVISION;

#ifdef QT_NO_DEBUG
	#define COMPILED_DEBUG_OR_RELEASE "R"
#else
	#define COMPILED_DEBUG_OR_RELEASE "D"
#endif


#include "mostQtHeaders.h"


int gitRevisionToInt(const char *);


class Version {

	public:

		enum VersionCompareResult { Invalid = -2 , Lower = -1 , Same = 0 , Higher = 1 };

		static VersionCompareResult compareStringVersion(const QString &,const QString &);
		static VersionCompareResult compareIntVersion(const QList<int> &,const QList<int> &);

		static QList<int> parseVersionNumber(const QString &);
		static bool versionNumberIsValid(const QString &);
		static int parseGitRevisionNumber(const QString &);

		Version()
			: revision(0) {}

		Version(QString number,int rev = 0)
			: versionNumber(number)
			, revision(rev) {}

		Version(QString number,QString type,int rev = 0)
			: versionNumber(number)
			, type(type)
			, revision(rev) {}

		static Version current();

		QString platform;       // "win" or "mac" or "linux"
		QString versionNumber;  // "2.10.2"
		QString type;           // "stable", "release candidate" or "development"
		int revision;           // 5310, now changed to revision after tag as deliverd by "git describe"

		bool operator > (const Version &) const;

		bool isEmpty() const;
		bool isValid() const;

};

#endif
