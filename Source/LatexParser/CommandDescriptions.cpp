#include "commanddescription.h"


CommandDescription::CommandDescription()
    : optionalArgs(0)
    , bracketArgs(0)
    , overlayArgs(0)
    , args(0)
    , level(0)
    , bracketCommand(false)
    , verbatimAfterOptionalArg(false) {}


QString tokenTypesToString(const QList<Token::TokenType> & types){
	
    QStringList strings;

    for(const Token::TokenType & type : types)
        strings << QString("%1").arg(type);
	
    return strings.join(" ");
}


QString CommandDescription::toDebugString() const {
    return QString("%1:%2:%3").arg(
        tokenTypesToString(optTypes),
        tokenTypesToString(argTypes),
        tokenTypesToString(bracketTypes)
    );
}


bool CommandDescription::operator == (const CommandDescription & other) const {
    return (
        this -> optionalCommandName == other.optionalCommandName && 
        this -> optionalArgs == other.optionalArgs &&
        this -> bracketTypes == other.bracketTypes &&
        this -> bracketArgs == other.bracketArgs && 
        this -> argTypes == other.argTypes &&
        this -> optTypes == other.optTypes &&
        this -> level == other.level &&
        this -> args == other.args
    );
}


bool CommandDescriptionHash::isRelevant(const CommandDescriptionHash & other,const QString key) const {

    if(!contains(key))
        return true;

    auto 
        a = value(key),
        b = other.value(key);

    if(b.args > a.args)
        return true;

    if(b.args < a.args)
        return false;

    if(b.optionalArgs > a.optionalArgs)
        return true;

    if(b.optionalArgs < a.optionalArgs)
        return false;

    if(a.args < 1)
        return false;

    if(b.overlayArgs > a.overlayArgs)
        return true;

    if(b.overlayArgs < a.overlayArgs)
        return false;

    for(int i = 0;i < a.args;i++)
        if(b.argTypes.at(i) == Token::generalArg)
            return false;

    for(int i = 0;i < a.optionalArgs;i++)
        if(b.optTypes.at(i) == Token::generalArg)
            return false;

    return true;
}


//QHash<QString,CommandDescription>::unite(other);
    // checks whether cd is already present
    // it is only overwritten, if the new definition contain *more* arguments
    // thus automatically generated definitions like \section#S are not used to disturb present functionality

void CommandDescriptionHash::unite(const CommandDescriptionHash & other){

    for(const QString & key : other.keys())
        if(isRelevant(other,key))
            insert(key,other.value(key));
}

