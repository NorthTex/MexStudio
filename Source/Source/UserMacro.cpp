
#include "mostQtHeaders.h"
#include "usermacro.h"
#include "smallUsefulFunctions.h"
#include "qformatscheme.h"
#include "qlanguagefactory.h"
#include "qdocument.h"


Macro::Macro()
	: type(Snippet)
	, triggerLookBehind(false)
	, document(nullptr) {}


Macro::Macro(
	const QString & name,
	const QString & typedTag,
	const QString & abbreviation,
	const QString & trigger
) : triggerLookBehind(false)
  , document(nullptr) {

	Macro::Type type;
	auto tag = parseTypedTag(typedTag,type);

	init(name,type,tag,abbreviation,trigger);
}


Macro::Macro(
	const QString & name,
	Macro::Type type,
	const QString & tag,
	const QString & abbreviation,
	const QString & trigger
) : triggerLookBehind(false)
  , document(nullptr) {

	init(name,type,tag,abbreviation,trigger);
}


Macro::Macro(const QStringList & fields)
	: type(Snippet)
	, triggerLookBehind(false)
	, document(nullptr) {

	if(fields.count() >= 4){

		Macro::Type type;
		QString tag = parseTypedTag(fields[1],type);

		init(fields[0],type,tag,fields[2],fields[3]);
	}
}


Macro Macro::fromTypedTag(const QString & typedTag){
	return Macro("unnamed",typedTag);
}

void Macro::init(
	const QString & nname,
	Macro::Type ntype,
	const QString & ntag,
	const QString & nabbrev,
	const QString & ntrigger
){

	name = nname;
	type = ntype;
	tag = ntag;
	abbrev = nabbrev;
	trigger = ntrigger;
	triggerLookBehind = false;

	QString realtrigger = trigger;

	triggers = SpecialTriggers();

	if(realtrigger.trimmed().startsWith("?")){

		QStringList sl = realtrigger.split("|");
		realtrigger.clear();

		foreach (const QString & x,sl){

			QString t = x.trimmed();

			if (t == "?txs-start") triggers |= ST_TXS_START;
			else if (t == "?new-file") triggers |= ST_NEW_FILE;
			else if (t == "?new-from-template") triggers |= ST_NEW_FROM_TEMPLATE;
			else if (t == "?load-file") triggers |= ST_LOAD_FILE;
			else if (t == "?load-this-file") triggers |= ST_LOAD_THIS_FILE;
			else if (t == "?save-file") triggers |= ST_FILE_SAVED;
			else if (t == "?close-file") triggers |= ST_FILE_CLOSED;
			else if (t == "?master-changed") triggers |= ST_MASTER_CHANGED;
			else if (t == "?after-typeset") triggers |= ST_AFTER_TYPESET;
			else if (t == "?after-command-run") triggers |= ST_AFTER_COMMAND_RUN;
			else if (realtrigger.isEmpty()) realtrigger = t;
			else realtrigger = realtrigger + "|" + t;
		}
	}

	if(realtrigger.isEmpty())
		return;

	triggers |= ST_REGEX;

	int lastLen;

	do {
		lastLen = realtrigger.length();

		if(realtrigger.startsWith("(?language:")){

			const int langlen = strlen("(?language:");
			int paren = 1, bracket = 0, i = langlen;
			
			for(;i < realtrigger.length() && paren;i++)
				switch(realtrigger[i].unicode()){
				case '(':
					if(!bracket)
						paren++;
					break;
				case ')':
					if(!bracket)
						paren--;
					break;
				case '[':
					bracket = 1;
					break;
				case ']':
					bracket = 0;
					break;
				case '\\':
					i++;
					break;
				}

			triggerLanguage = realtrigger.mid(langlen,i - langlen - 1);
			triggerLanguage.replace("latex","\\(La\\)TeX");
			realtrigger.remove(0,i);
		}

		if(realtrigger.startsWith("(?highlighted-as:")){

			int 
				start = realtrigger.indexOf(':') + 1,
				closing = realtrigger.indexOf(")");

			//handle later, when the formats are loaded

			triggerFormatsUnprocessed = realtrigger
				.mid(start,closing - start)
				.replace(',','|')
				.replace(" ","");

			realtrigger.remove(0,closing + 1);
		}

		if(realtrigger.startsWith("(?not-highlighted-as:")){

			int 
				start = realtrigger.indexOf(':') + 1,
				closing = realtrigger.indexOf(")");

			//handle later, when the formats are loaded

			triggerFormatExcludesUnprocessed = realtrigger
				.mid(start,closing - start)
				.replace(',','|')
				.replace(" ","");

			realtrigger.remove(0,closing + 1);
		}

	} while (lastLen != realtrigger.length());

	if(realtrigger.startsWith("(?<=")){
		triggerLookBehind = true;

		//qregexp doesn't support look behind, but we can emulate it by removing the first capture

		realtrigger.remove(1,3);
	}

	// (?: non capturing)

	triggerRegex = QRegExp("(?:" + realtrigger + ")$"); 
}


void Macro::initTriggerFormats(){

	auto scheme = QDocument::defaultFormatScheme();

	REQUIRE(scheme);

	for(const auto & name : triggerFormatsUnprocessed.split('|'))
		if(scheme -> exists(name))
			triggerFormats << scheme -> id(name);

	triggerFormatsUnprocessed.clear();

	for(const auto & name :	triggerFormatExcludesUnprocessed.split('|'))
		if(scheme -> exists(name))
			triggerFormatExcludes << scheme -> id(name);

	triggerFormatExcludesUnprocessed.clear();
}


QStringList Macro::toStringList() const {
	return QStringList() 
		<< name 
		<< typedTag() 
		<< abbrev 
		<< trigger;
}


QString Macro::snippet() const {
	switch(type){
	case Snippet: 
		return tag;
	case Environment: 
		return "\\begin{" + tag + "}";
	default: 
		return "";
	}
}


QString Macro::script() const {
	return (type == Script) 
		? tag 
		: "";
}


QString Macro::shortcut() const {
	return m_shortcut;
}


bool Macro::isEmpty() const {
    return name.isEmpty() && 
		tag.isEmpty() && 
		trigger.isEmpty();
}

void Macro::setShortcut(const QString & shortcut){
    m_shortcut = shortcut;
}


void Macro::setTrigger(const QString & trigger){
    init(name,type,tag,abbrev,trigger);
}


QString Macro::typedTag() const {
    switch(type){
    case Snippet:
		return tag;
    case Environment:
		return "%" + tag;
    case Script:
		return "%SCRIPT\n" + tag;
	default:
		qDebug() << "unknown macro type" << type;
		return "";
	}
}


void Macro::setTypedTag(const QString & typedTag){
	tag = parseTypedTag(typedTag,type);
}


QString Macro::parseTypedTag(QString typedTag,Macro::Type & retType){

	if(typedTag.startsWith("%SCRIPT\n")){
		retType = Script;
		return typedTag.mid(8);
	}
	
	// Note: while % is an empty environemnt, reserved sequences like %%, %<, %| are snippets.

	if(typedTag.startsWith('%') && (typedTag.length() == 1 || typedTag.at(1).isLetter())){
		retType = Environment;
		return typedTag.mid(1);
	}

	retType = Snippet;
	
	return typedTag;
}


void Macro::parseTriggerLanguage(QLanguageFactory * langFactory){

    if(!langFactory)
        return;

    if(triggerLanguage.isEmpty())
        return;

    triggerLanguages.clear();

    QRegExp matchLanguage(triggerLanguage,Qt::CaseInsensitive);

	for(const auto & lang : langFactory -> languages())
		if(matchLanguage.exactMatch(lang))
			triggerLanguages << langFactory -> languageData(lang).d;
}


bool Macro::isActiveForTrigger(Macro::SpecialTrigger trigger) const {
	return triggers & trigger;
}


// If no trigger language is specified, the macro is active for all languages.

bool Macro::isActiveForLanguage(QLanguageDefinition * language) const {
	return triggerLanguage.isEmpty() || 
		triggerLanguages.contains(language);
}


bool Macro::isActiveForFormat(int format) const {

	if(
		!triggerFormatsUnprocessed.isEmpty() || 
		!triggerFormatExcludesUnprocessed.isEmpty()
	)	(const_cast<Macro *>(this)) -> initTriggerFormats();

	// If no trigger format is specified, the macro is active for all formats.
	
	if(triggerFormatExcludes.contains(format))
		return false;

	return triggerFormats.contains(format);
}


bool Macro::save(const QString & path) const {

    QFile file(path);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QJsonObject json;
    json.insert("formatVersion",1);
    json.insert("name",name);

    QString tag = typedTag();
    json.insert("tag",QJsonArray::fromStringList(tag.split("\n")));

    json.insert("description",QJsonArray::fromStringList(description.split("\n")));
    json.insert("abbrev",abbrev);
    json.insert("trigger",trigger);
    json.insert("menu",menu);
    json.insert("shortcut",m_shortcut);

    QJsonDocument document(json);
    file.write(document.toJson());

    return true;
}


bool Macro::load(const QString & fileName){

    QFile file(fileName);

    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream in(& file);


    QString text = in.readAll();

    return loadFromText(text);
}


bool Macro::loadFromText(const QString & text){

    QHash<QString,QString>rawData;
    QJsonParseError parseError;

    auto document = QJsonDocument::fromJson(text.toUtf8(),& parseError);

    // parser could not read input

    if(parseError.error != QJsonParseError::NoError){
        QMessageBox msgBox;
        msgBox.setText(QObject::tr("Macro read-in failed\nError: ") + parseError.errorString());
        msgBox.exec();
        return false;
    }

    auto json = document.object();

    if(json.contains("formatVersion")){
        for(const auto & key : json.keys()){

            if(json[key].isString())
                rawData.insert(key,json[key].toString());

            if(json[key].isArray()){

                auto items = json[key].toArray();
                QString text;

                for(const auto & item : items){

                    if(!item.isString())
                        continue;

                    if(!text.isEmpty())
                        text.append('\n');

                    text.append(item.toString());
                }

                rawData.insert(key,text);
            }
        }
    } else {
        //old format
        qDebug() << "support for old macro format was removed!";
        return false;
    }

    // Distrbute data on internal structure

    Macro::Type typ;
    QString typedTag = parseTypedTag(rawData.value("tag"),typ);

    init(rawData.value("name"),typ,typedTag,rawData.value("abbrev"),rawData.value("trigger"));

    description = rawData.value("description");
    m_shortcut = rawData.value("shortcut");
    menu = rawData.value("menu");

    return true;
}


