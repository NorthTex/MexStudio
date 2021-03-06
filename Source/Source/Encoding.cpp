#include "Include/Encoding.hpp"
#include "latexparser/latexparser.h"

#include <vector>

namespace Encoding {
namespace Internal {


/// find the start of a line in the given bytearray before or equal to index
/// \returns the index of the first character of the line

int lineStart(const QByteArray & bytes,int index){
	
	int n = qMax(bytes.lastIndexOf('\n',index),bytes.lastIndexOf('\r',index));
	int o = bytes.lastIndexOf(" )",index);
	
	if(n < 0 && o < 0)
		return 0;
	
	if(n > o)
		n++; // Skip char

	return n + 1; // Skip char

	// if(n > o)
	// 	return n + 1; // skip over character
	// else 
	// 	return n + 2; // skip over both chars
}


/// find the end of a line in the given bytearray after or at index
/// \returns the index+1 of the last character in the line (excluding terminating characters)

int lineEnd(const QByteArray & data,int index){

	std::vector indices = {
		data.indexOf('\n',index),
		data.indexOf('\r',index),
		data.indexOf(" )",index)
	};

	std::sort(indices.begin(),indices.end());

	for(const auto index : indices)
		if(index != -1)
			return index;

	return data.size();

//	int n = data.indexOf('\n',index);
//	int r = data.indexOf('\r',index);

//	if(n < 0)
//		n = r; // prevent non-existing value (-1) to be smaller than existing one in qMin
//	else
//	if(r < 0)
//		r = n;
	
//	n = qMin(n,r);

//	r = data.indexOf(" )",index); // unicode paragraph separator, note this is equivalent to " )" in ASCII but this duplication is ok for the current usecase of lineEnd()
	
//	if(n < 0)
//		n = r;
//	else
//	if(r < 0)
//		r = n;
	
//	n = qMin(n,r);
	
//	if(n >= 0)
//		return n;
	
//	return data.size();
}

#include <optional>
#include <functional>

template<typename Type>
class Optional : std::optional<Type> {


	Optional then(std::function<Type> transform(Type)){
		if(this -> has_value())
			this -> value = transform(this -> value());
	}
};


const std::map<QString,const char *> encodings = {
	{ "windows cyrillic" , "Windows-1251" },
	{ "standard unicode" , "UTF-16"       },
	{ "macosroman"       , "Apple Roman"  },
	{ "isolatin2"        , "ISO 8859-2"   },
	{ "isolatin5"        , "ISO 8859-5"   },
	{ "isolatin9"        , "ISO 8859-9"   },
	{ "sjis_x0213"       , "Shift-JIS"    },
	{ "isolatin"         , "ISO 8859-1"   },
	{ "utf-8 unicode"    , "UTF-8"        },
	{ "gb 18030"         , "GB18030-0"    },
	{ "euc_jp"           , "EUC-JP"       },
	{ "koi8_r"           , "KOI8-R"       }
};

QTextCodec * QTextCodecForTeXShopName(const QByteArray & encoding){
	return (encodings.contains(encoding))
		? QTextCodec::codecForName(encodings.at(encoding))
		: nullptr;

//	//copied and modified from texworks
//	if (encoding == "utf-8 unicode") return QTextCodec::codecForName("UTF-8");
//	if (encoding == "standard unicode") return QTextCodec::codecForName("UTF-16");
//	if (encoding == "windows cyrillic") return QTextCodec::codecForName("Windows-1251");
//	if (encoding == "isolatin") return QTextCodec::codecForName("ISO 8859-1");
//	if (encoding == "isolatin2") return QTextCodec::codecForName("ISO 8859-2");
//	if (encoding == "isolatin5") return QTextCodec::codecForName("ISO 8859-5");
//	if (encoding == "isolatin9") return QTextCodec::codecForName("ISO 8859-9");
//	if (encoding == "macosroman") return QTextCodec::codecForName("Apple Roman");
//	//      "MacJapanese",          "",
//	//      "DOSJapanese",          "",
//	if (encoding == "sjis_x0213") return QTextCodec::codecForName("Shift-JIS");
//	if (encoding == "euc_jp") return QTextCodec::codecForName("EUC-JP");
//	//      "JISJapanese",          "",
//	//      "MacKorean",            "",
//	//      "Mac Cyrillic",         "",
//	//      "DOS Cyrillic",         "",
//	//      "DOS Russian",          "",
//	if (encoding == "koi8_r") return QTextCodec::codecForName("KOI8-R");
//	if (encoding == "gb 18030") return QTextCodec::codecForName("GB18030-0");
//	//      "Mac Chinese Traditional",      "",
//	//      "Mac Chinese Simplified",       "",
//	//      "DOS Chinese Traditional",      "",
//	//      "DOS Chinese Simplified",       "",
//	//      "GBK",                          "",
//	//      "GB 2312",                      "",
//    return nullptr;
}

/*! search for first \usepackage[.*]{<packageName>} outside of a comment
 * returns the string inside the square brackets
 */

QString getEncodingFromPackage(const QByteArray & data,int headerSize,const QString & packageName){

	QByteArray packageEndToken(QString("]{%1}").arg(packageName).toLatin1());
	QByteArray packageStartToken("\\usepackage[");

	int index = data.indexOf(packageEndToken);

	while(index >= 0 && index < headerSize){

		int lStart = lineStart(data,index);
		int lEnd = lineEnd(data,index);

		QByteArray line(data.mid(lStart,lEnd - lStart));

		int encEnd = index - lStart;
		int encStart = line.lastIndexOf(packageStartToken,encEnd);

		if(encStart >= 0){
			encStart += packageStartToken.length();

			int commentStart = line.lastIndexOf('%',encEnd);

			if(commentStart < 0)
				return QString(line.mid(encStart,encEnd - encStart));
		}

		index = data.indexOf(packageEndToken,index + 1);
	}

	return "";
}


} // namespace Internal


using namespace Internal;

/*** public functions ***/

QTextCodec * QTextCodecForLatexName(QString str){

	if(str.contains(',')) //multiple options are allowed
		for(const QString & splitter : str.split(',')){
			
			auto codec = QTextCodecForLatexName(splitter);

			if(codec)
				return codec;
		}
//		foreach (const QString &splitter, str.split(',')) {
//			QTextCodec *codec = QTextCodecForLatexName(splitter);
//			if (codec) return codec;
//		}

	str = str.toLower(); //probably unnecessary

	if(str.startsWith("x-"))
		str = str.mid(2); //needed for inputenx encodings used as parameters for inputenc

	//encodings as defined by inputenc 1.1d (2008/03/30)
	//popular first
	if(str == "utf8" || str == "utf8x")
		return QTextCodec::codecForName("UTF-8");

	if(str.startsWith("latin"))
		return QTextCodec::codecForName(qPrintable(str));

	//as in the docu
	if(str == "ascii")
		return QTextCodec::codecForName("latin1"); //this is wrong (should be latin1 limited to 0x00-0x7f)

	//if (str == "decmulti") return??
	//if (str == "next") ??
	if(
		str.startsWith("cp") && 
		(str.length() == 5 || str.length() == 6) &&
	    (str[2] >= '0') && (str[2] <= '9') &&
	    (str[3] >= '0') && (str[3] <= '9') &&
	    (str[4] >= '0') && (str[4] <= '9') && (
			str.length() == 5 || 
			((str[5] >= '0') && (str[5] <= '9'))
		)
	) return QTextCodec::codecForName(qPrintable(str));

	//if (str == "cp437de") return QTextCodec::codecForName("??");
	if(str == "applemac")
		return QTextCodec::codecForName("macintosh");

	if(str == "macce")
		return QTextCodec::codecForName("macintosh"); //wrong, should be Macintosh Central European code page.

	if(str == "ansinew")
		return QTextCodec::codecForName("cp1252");

	//additional encodings by inputenx
	if (str == "us-ascii" || str == "clean7bit" || str == "ascii-print" || str == "ascii-printable")
		return QTextCodec::codecForName("latin1"); //this is wrong (should be latin1 limited to 0x00-0x7f)

	//if (str == "atari" || )str == "atarist" ||  return QTextCodec::codecForName("???")
	//if (str == "dec-mcs") return??

	if(str == "koi8-r")
		return QTextCodec::codecForName("KOI8-R");

	if(str.startsWith("iso-8859-"))
		return QTextCodec::codecForName(qPrintable(str));

	if(str == "iso88595")
		return QTextCodec::codecForName("ISO-8859-5");

	if(str == "mac-ce" || str == "mac-centeuro")
		return QTextCodec::codecForName("macintosh"); //wrong, should be Macintosh Central European code page.

	if(str == "mac-cyrillic" || str == "maccyr" || str == "mac-ukrainian" || str == "macukr")
		return QTextCodec::codecForName("macintosh"); //wrong, should be Macintosh Cyrillic
	//if (str == "nextstep, next?

	//return QTextCodec::codecForName(str); //try it anyways
    return nullptr;
}


const std::map<int,QStringList> codecNames = {
    {    4 , QStringList() << "latin1"              } , // ISO-8859-1
	{    5 , QStringList() << "latin2"              } , // ISO-8859-2
	{    6 , QStringList() << "latin3"              } , // ISO-8859-3
	{    7 , QStringList() << "latin4"              } , // ISO-8859-4
	{   12 , QStringList() << "latin5"              } , // ISO-8859-9
	{  106 , QStringList() << "utf8" << "utf8x"     } , // UTF-8
	{  111 , QStringList() << "latin9"              } , // ISO-8859-15
	{  112 , QStringList() << "latin10"             } , // ISO-8859-16
	{ 2009 , QStringList() << "cp850"               } , // IBM850
	{ 2086 , QStringList() << "cp866"               } , // IBM866
	{ 2250 , QStringList() << "cp1250"              } , // windows-1250
	{ 2251 , QStringList() << "cp1251"              } , // windows-1251
	{ 2252 , QStringList() << "cp1252" << "ansinew" } , // windows-1252
	{ 2257 , QStringList() << "cp1257"              } , // windows-1257
	{ -168 , QStringList() << "applemac"            }   // Apple Roman
};

QStringList latexNamesForTextCodec(const QTextCodec * codec){
	const auto mib = codec -> mibEnum();

	return (codecNames.contains(mib))
		? codecNames.at(mib)
		: QStringList();

	// switch(codec -> mibEnum()){
	// 	case 4 : return QStringList() << "latin1"; // "ISO-8859-1"
	// 	case 5 : return QStringList() << "latin2"; // "ISO-8859-2"
	// 	case 6 : return QStringList() << "latin3"; // "ISO-8859-3"
	// 	case 7 : return QStringList() << "latin4"; // "ISO-8859-4"
	// 	case 12 : return QStringList() << "latin5"; // "ISO-8859-9"
	// 	case 106 : return QStringList() << "utf8" << "utf8x"; // "UTF-8"
	// 	case 111 : return QStringList() << "latin9"; // "ISO-8859-15"
	// 	case 112 : return QStringList() << "latin10"; // "ISO-8859-16"
	// 	case 2009 : return QStringList() << "cp850"; // "IBM850"
	// 	case 2086 : return QStringList() << "cp866"; // "IBM866"
	// 	case 2250 : return QStringList() << "cp1250"; // "windows-1250"
	// 	case 2251 : return QStringList() << "cp1251"; // "windows-1251"
	// 	case 2252 : return QStringList() << "cp1252" << "ansinew"; // "windows-1252"
	// 	case 2257 : return QStringList() << "cp1257"; // "windows-1257"
	// 	case -168 : return QStringList() << "applemac"; // "Apple Roman"
	// }

	// return QStringList();
}


QTextCodec * guessEncodingBasic(const QByteArray & data,int * outSure){
	
	const char * str = data.data();
	int size = data.size();
    QTextCodec * guess = nullptr;
	int sure = 1;
	
	if(size > 0){

		unsigned char prev = str[0];
	
		int goodUtf8 = 0;
		int badUtf8 = 0;
		int badIso1 = 0;
		int utf16le = 0, utf16be = 0;
		
		if(prev >= 0x80 && prev <= 0x9F)
			badIso1++;
		
		for(int i = 1;i < size;i++){

			unsigned char cur = str[i];

			if(cur >= 0x80 && cur <= 0x9F)
				badIso1++;
			
			if((cur & 0xC0) == 0x80){
				
				if((prev & 0xC0) == 0xC0)
					goodUtf8++;
				else 
				if((prev & 0x80) == 0x00)
					badUtf8++;
			} else {
				
				if((prev & 0xC0) == 0xC0)
					badUtf8++;
				
				if(prev == 0) {
					if((i & 1) == 1)
						utf16be++;
					else
						utf16le++;
				}
			}

			prev = cur;
		}

		// less than 0.1% of the characters can be wrong for utf-16 if at least 1% are valid (for English text)
		
		if(utf16le > utf16be){
			if(utf16be <= size / 1000 && utf16le >= size / 100 && utf16le >= 2){
				guess = QTextCodec::codecForMib(MIB_UTF16LE);
				sure = 2;
			}
		} else {
			if(utf16le <= size / 1000 && utf16be >= size / 100 && utf16be >= 2){
				guess = QTextCodec::codecForMib(MIB_UTF16BE);
				sure = 2;
			}
		}

		if(!guess){
			if(goodUtf8 > 10 * badUtf8){
				guess = QTextCodec::codecForMib(MIB_UTF8);
				sure = 2;
			} else {
				
				if(badIso1 > 0)	
					guess = QTextCodec::codecForMib(MIB_WINDOWS1252);
				else 
					guess = QTextCodec::codecForMib(MIB_LATIN1);
				
				if(badUtf8 == 0)
					sure = 0;
			}
		}
	} else sure = 0;

	if(outSure) 
		* outSure = sure;

	return guess;
}


void guessEncoding(const QByteArray & data,QTextCodec * & guess,int & sure){

	const auto mib = guess -> mibEnum();

	if(guess)
		if(mib == MIB_UTF16LE || mib == MIB_UTF16BE){
			sure = 100;
			return;
		}

	int headerSize = data.indexOf("\\begin{document}");
	
	if(headerSize == -1)
		headerSize = data.size();

	//search for % *!TeX +encoding *= *...\n
	//slow c like search, without encoding we can't get a qstring, and bytearray neither supports
	//regexp nor case insensitive search

	int index = data.indexOf('=');
	
	static const char * searchedLC = "%!tex encoding";
	static const char * searchedUC = "%!TEX ENCODING";
	static const int searchedLast = 13;
	
	Q_ASSERT(searchedLC[searchedLast] == 'g');
	
	while(index >= 0 && index < headerSize){
		
		int temp = index - 1;
		int sp = searchedLast;
		const char * d = data.constData();
		
		for(;temp >= 0 && sp >= 0;temp--){

			if(searchedLC[sp] == d[temp] || searchedUC[sp] == d[temp]){
				sp--;
				continue;
			}

			if(d[temp] != ' ')
				break;
		}

		if(sp == -1){
		
			int end = lineEnd(data,index);
		
			auto encName = data.mid(index + 1,end - index - 1).trimmed();
			auto codec = QTextCodec::codecForName(encName);
		
			if(!codec)
				codec = QTextCodecForTeXShopName(encName.toLower());
			
			if(codec){
				sure = 100;
				guess = codec;
				return;
			}
		}

		index = data.indexOf('=', index + 1);
	}

	QString encoding = getEncodingFromPackage(data,headerSize,"inputenc");

	if(encoding.isEmpty())
		encoding = getEncodingFromPackage(data,headerSize,"inputenx");
	
	if(encoding.isEmpty())
		return;
		
	auto codec = QTextCodecForLatexName(encoding);

	if(codec){
		sure = 100;
		guess = codec;
	}
}

}
