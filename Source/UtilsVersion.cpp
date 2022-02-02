
#include "utilsVersion.h"
#include "mostQtHeaders.h"


int gitRevisionToInt(const char *){

    auto revision = QString(TEXSTUDIO_GIT_REVISION)
        .split('-')
        .value(1,"0");
	
    if(revision.endsWith('+'))
		revision = revision.left(revision.length() - 1);
	
    return revision.toInt();
}


const QRegularExpression terminatingChars("[\\s-]");

QList<int> Version::parseVersionNumber(const QString & versionNumber){

	
    int len = versionNumber.indexOf(terminatingChars);
	
    const auto parts = versionNumber
        .left(len)
        .split('.');

	if(parts.isEmpty())
		return QList<int>();
	

	QList<int> result;

    for(const auto & part : parts){

		bool ok;
		
        result << part.toInt(& ok);
		
        if(!ok)
			return QList<int>();
	}

    // 1.0 is extended to 1.0.0

	for (int i = result.count();i<=2;i++)
		result << 0;

	return result;
}


bool Version::versionNumberIsValid(const QString &versionNumber){

    const auto length = parseVersionNumber(versionNumber).length();

    return length == 3 ||
           length == 4;
}


/*!
 *  Return the revision number from a hg revision string, e.g. "1234:asassdasd" -> 1234.
 *  Returns 0 if the input string is not a vailid hg revision string.
 */

int Version::parseGitRevisionNumber(const QString & revision){
    return revision.split('-')[1].toInt();
}


using Result = Version::VersionCompareResult;


// compares two versions strings
// Meaning of result: v1 [result] v2, e.g. v1 Older than v2

Result Version::compareStringVersion(const QString & a,const QString & b){
	return compareIntVersion(parseVersionNumber(a),parseVersionNumber(b));
}

Result Version::compareIntVersion(const QList<int> & a,const QList<int> & b){
    
    if(a.length() < 3)
        return Invalid;
        
    if(b.length() < 3)
		return Invalid;
    
    if(a.length() > 4)
        return Invalid;
        
    if(b.length() > 4)
        return Invalid;

    const auto
        countA = a.count(),
        countB = b.count();

	for(int i = 0;i < countA;i++){

        if(i >= countB)
            return Higher;
		
        if(a.at(i) < b.at(i))
            return Lower;

        if(a.at(i) > b.at(i))
            return Higher;
	}

    if(countA < countB)
        return Lower;

	return Same;
}


Version Version::current(){

	Version version(TXSVERSION);

    const auto revision = QString(TEXSTUDIO_GIT_REVISION);
    
    QString s = revision
        .split('-')
        .value(1,"0");
	
    if(s.endsWith('+'))
		s = s.left(s.length() - 1);
	
    version.revision = s.toInt();
    
    if(revision.contains("RC"))
        version.type = "release candidate";
    
    if(revision.contains("rc"))
        version.type = "release candidate";

    if(revision.contains("beta"))
        version.type = "beta";

    if(version.type.isEmpty())
        version.type = (version.revision >= 2)
            ? "development"
            : "stable";

    #if defined(Q_OS_WIN)
        version.platform = "win";
    #elif defined(Q_OS_MAC)
        version.platform = "mac";
    #elif defined(Q_OS_LINUX)
        version.platform = "linux";
    #endif

	return version;
}


bool Version::operator > (const Version & other) const {
	
    auto result = compareStringVersion(versionNumber,other.versionNumber);
    
    if(result != Same)
        return (result == Higher);
    
    if(type == other.type){
        bool revisionLarger = (revision > 0 && other.revision > 0 && revision > other.revision);
        return revisionLarger;
    }
    
     
    // special treatment 
    // a.b-dev > a.b stable but 
    // a.b-dev < a.b-beta/rc

    const auto lvl = level();

    if(other.inDevelopment() && lvl == 3)
        return false;

    return (lvl > other.level());
}


bool Version::isEmpty() const {
	return versionNumber.isEmpty() && revision == 0;
}

bool Version::isValid() const {
	return versionNumberIsValid(versionNumber);
}

bool Version::inDevelopment() const {
	return type == "development";
}

int Version::level() const {

    if(type == "release candidate")
        return 2;

    if(type == "beta")
        return 1;

    if(type == "development")
        return 0;

    return 3;
}

