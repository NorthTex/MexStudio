#ifndef Header_UserMacro
#define Header_UserMacro

#include "mostQtHeaders.h"

#include <QJsonDocument>
#include <QJsonArray>

#include "Latex/Document.hpp"

class QLanguageDefinition;
class QLanguageFactory;
class LatexDocument;


class Macro {

	public:

		enum Type { Snippet, Environment, Script };

		enum SpecialTrigger {
			ST_NO_TRIGGER = 0,
			ST_REGEX = 1,
			ST_TXS_START = 2,
			ST_NEW_FILE = 4,
			ST_NEW_FROM_TEMPLATE = 8,
			ST_LOAD_FILE = 0x10,
			ST_LOAD_THIS_FILE = 0x20,
			ST_FILE_SAVED = 0x40,
			ST_FILE_CLOSED = 0x80,
			ST_MASTER_CHANGED = 0x100,
			ST_AFTER_TYPESET = 0x200,
			ST_AFTER_COMMAND_RUN = 0x400
		};

		Q_DECLARE_FLAGS(SpecialTriggers, SpecialTrigger)

		Macro(const QString & nname,const QString & typedTag,const QString & nabbrev = QString(),const QString & ntrigger = QString());
		Macro(const QString & nname,const Macro::Type ntype,const QString & ntag,const QString & nabbrev,const QString & ntrigger);
		Macro(const QStringList & fieldList);
		Macro();

		static Macro fromTypedTag(const QString & typedTag);


		QString name, abbrev , description , menu , trigger;
		QRegExp triggerRegex;
		Type type;

		bool triggerLookBehind;


		QStringList toStringList() const;

		QString shortcut() const;
		QString snippet() const;
		QString script() const;

		bool isEmpty() const;

		void setShortcut(const QString & shortcut);
		void setTypedTag(const QString & typedTag);
		void setTrigger(const QString & trigger);

		QString typedTag() const;

		static QString parseTypedTag(QString typedTag, Macro::Type &retType);

		void parseTriggerLanguage(QLanguageFactory * langFactory);

		bool isActiveForLanguage(QLanguageDefinition * lang) const;
		bool isActiveForTrigger(SpecialTrigger trigger) const;
		bool isActiveForFormat(int format) const;

		bool loadFromText(const QString & text);
		bool load(const QString & fileName);
		bool save(const QString & fileName) const;

		LatexDocument * document;

	private:

		void init(const QString &nname, Macro::Type ntype, const QString &ntag, const QString &nabbrev, const QString &ntrigger);
		void initTriggerFormats();

		QString tag;
		QString triggerLanguage;
		QString triggerFormatsUnprocessed;
		QString triggerFormatExcludesUnprocessed;

		SpecialTriggers triggers;

		QList<QLanguageDefinition *> triggerLanguages;

		QList<int> triggerFormats;
		QList<int> triggerFormatExcludes;

		QString m_shortcut;
};


Q_DECLARE_METATYPE(Macro);


class MacroExecContext {

	public:

		int triggerId;
		QStringList triggerMatches;


		MacroExecContext()
			: triggerId(Macro::ST_NO_TRIGGER) {};

		MacroExecContext(int triggerType)
			: triggerId(triggerType) {};

		MacroExecContext(int triggerType,QStringList triggerMatches)
			: triggerId(triggerType)
			, triggerMatches(triggerMatches) {};
};

#endif
