#ifndef Header_GrammarCheck_Config
#define Header_GrammarCheck_Config


#include <QString>
#include <QVariant>


class GrammarCheckerConfig {

	public:

		bool 
			longRangeRepetitionCheck,
			languageToolAutorun,
			badWordCheck;

		int 
			maxRepetitionLongRangeMinWordLength,
			maxRepetitionLongRangeDelta,
			maxRepetitionDelta;

		QString 
			languageToolIgnoredRules,
			languageToolArguments,
			languageToolJavaPath,
			languageToolPath,
			languageToolURL,
			wordlistsDir,
			specialIds1,
			specialIds2,
			specialIds3,
			specialIds4,
			configDir,
			appDir;
};


Q_DECLARE_METATYPE(GrammarCheckerConfig);


#endif
