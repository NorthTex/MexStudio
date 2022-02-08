#ifndef Header_Latex_Completer_Config
#define Header_Latex_Completer_Config

//having the configuration in a single file allows to change it,
//without having a relationship between completer and configmanager
//so modifying one doesn't lead to a recompilation of the other


#include "modifiedQObject.h"
#include "usermacro.h"
#include "codesnippet.h"


class LatexCompleterConfig {

	public:

		enum CaseSensitive {
			CCS_CASE_INSENSITIVE,
			CCS_CASE_SENSITIVE,
			CCS_FIRST_CHARACTER_CASE_SENSITIVE
		};

		enum PreferedCompletionTab {
			CPC_TYPICAL,
			CPC_MOSTUSED,
			CPC_FUZZY,
			CPC_ALL
		};

		CaseSensitive caseSensitive;

		bool enabled; //auto completion enabled (manual completion e.g ctrl+space can always be used)
		bool completeCommonPrefix; //auto tab press
		bool eowCompletes; //if a EOW character key is pressed, the current word is completed and the character added
		bool tooltipHelp; // enable ToolTip-Help during completion
		bool tooltipPreview; // enable ToolTip-Preview during completion
		bool usePlaceholders;
		bool autoInsertMathDelimiters;

		int tabRelFontSizePercent;

		QString startMathDelimiter,stopMathDelimiter;

		PreferedCompletionTab preferedCompletionTab;
		CodeSnippetList words;
		QMultiMap<uint, QPair<int, int> > usage;
		QSet<QString> specialCompletionKeys;

		QList<Macro> userMacros;

		void setFiles(const QStringList & newFiles);

		const QStringList & getLoadedFiles() const;

		QString importedCwlBaseDir;

	private:

		QStringList files;

};


#endif
