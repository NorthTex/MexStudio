#include "QtGlobal"
#include "QDebug"

#include "BidiExtender.hpp"


//mostly taken from biditexmaker

#if defined( Q_OS_LINUX ) || ( defined( Q_OS_UNIX ) && ! defined( Q_OS_MAC ) )
	#define WS_X11
#endif

#if defined( WS_X11 )
	#include "xkb/XKeyboard.h"
	#include "xkb/X11Exception.h"
	#include <string>
#endif

#if defined( Q_OS_WIN )
	#include "windows.h"
#else
	using HKL = int;
#endif


static bool languagesInitialized = false;
static HKL languageIdRTL, languageIdLTR, oldInputLanguageId;
static InputLanguage oldInputLanguage = IL_UNCERTAIN;



#if defined( Q_OS_WIN )

HKL getCurrentLanguage(){
	return GetKeyboardLayout(0);
}

#elif defined( WS_X11 )

HKL getCurrentLanguage(){

	try {
		kb::XKeyboard keyboard;
		return keyboard.get_group() + 1;
	} catch (X11Exception &) {
		return 0;
	}
}

#else

HKL getCurrentLanguage(){
	return 0;
}

#endif


#if defined( Q_OS_WIN )

bool isProbablyLTRLanguageRaw(int id){
	//checks primary language symbol, e.g. LANG_ENGLISH would be ltr
	return id != LANG_PERSIAN && id != LANG_ARABIC && id != LANG_HEBREW && id != LANG_URDU;
}

#elif defined( WS_X11 )

//e.g. "us" would be ltr
bool isProbablyLTRLanguageRaw(const std::string & type){
	return type != "ir"
		&& type != "ara"
		&& type != "il"
		&& type != "af"
		&& type != "pk";
}

#endif



#if defined( Q_OS_WIN )

bool isProbablyLTRLanguageCode(HKL id){
	return isProbablyLTRLanguageRaw(((int) id)) & 0x000000FF);
}

#elif defined( WS_X11 )

bool isProbablyLTRLanguageCode(HKL id){

	try {

		kb::XKeyboard keyboard;
		kb::string_vector installedLangSymbols = kb::parse3(keyboard.get_kb_string(),kb::nonsyms());

		id--;

		if(id >= 0 && id < static_cast<HKL>(installedLangSymbols.size()))
			return isProbablyLTRLanguageRaw(installedLangSymbols[id]);

	} catch (X11Exception &) {}

	return false;
}

#else

bool isProbablyLTRLanguageCode(HKL id){
	return false;
}

#endif


void initializeLanguages(){

	languageIdLTR = 0;
	languageIdRTL = 0;

	oldInputLanguageId = getCurrentLanguage();

	if (isProbablyLTRLanguageCode(oldInputLanguageId)) {
		oldInputLanguage = IL_LTR;
		languageIdLTR = oldInputLanguageId;
	} else {
		oldInputLanguage = IL_RTL;
		languageIdRTL = oldInputLanguageId;
	}

	#if defined( Q_OS_WIN )
		const int MAXSIZE = 32;
		HKL langs[MAXSIZE];
		int count = GetKeyboardLayoutList(0, NULL);//this doesn't work on Win7 64Bit
		if (count <= 0 || count > MAXSIZE)
			count = GetKeyboardLayoutList(MAXSIZE, langs);//this seems be slow on some systems
		else
			GetKeyboardLayoutList(count, langs);

		HKL bestLTR = 0;
		for (int i = 0; i < count; ++i) {
			int id = (int)langs[i] & 0x000000FF;
			if (id == LANG_ENGLISH) bestLTR = langs[i];
			if (isProbablyLTRLanguageRaw(id)) {
				if (!bestLTR) bestLTR = langs[i];
			} else if (!languageIdRTL) languageIdRTL = langs[i];
		}
		if (!languageIdLTR) languageIdLTR = bestLTR;

	#endif //Q_OS_WIN


	#if defined( WS_X11 )
		try {
			kb::XKeyboard xkb;
			kb::string_vector installedLangSymbols = kb::parse3(xkb.get_kb_string(), kb::nonsyms());
			int bestLTR = -1;
			for (size_t i = 0; i < installedLangSymbols.size(); ++i) {
				std::string symb = installedLangSymbols.at(i);
				if (symb == "us") bestLTR = i;
				if (isProbablyLTRLanguageRaw(symb)) {
					if (bestLTR < 0) bestLTR = i;
				} else if (!languageIdRTL) languageIdRTL = i + 1;
			}
			if (!languageIdLTR) languageIdLTR = bestLTR + 1;
		} catch (X11Exception &) {}
	#endif

	languagesInitialized = true;
}


void setInputLanguage(HKL code){

	if(!code)
		return;

	#if defined( Q_OS_WIN )
		ActivateKeyboardLayout(code,KLF_SETFORPROCESS);
	#endif

	#if defined( WS_X11 )
		try{
			kb::XKeyboard keyboard;
			keyboard.set_group(code - 1);
		} catch (X11Exception &) {}
	#endif
}


void setInputLanguage(InputLanguage lang){

	if(lang == oldInputLanguage)
		return;

	if(!languagesInitialized)
		initializeLanguages();

	HKL current = getCurrentLanguage();
	HKL candidate = 0;

	if(current != oldInputLanguageId)
		switch(oldInputLanguage){
		case IL_LTR:
			languageIdLTR = current;
			break;
		case IL_RTL:
			languageIdRTL = current;
			break;
		case IL_UNCERTAIN:
			switch(lang){
			case IL_LTR:
				languageIdLTR = current;
				break;
			case IL_RTL:
				languageIdRTL = current;
				break;
			default:
				break;
			}
		}


	switch(lang){
	case IL_LTR:
		candidate = languageIdLTR;
		break;
	case IL_RTL:
		candidate = languageIdRTL;
		break;
	default:
		break;
	}


	const bool diffLayout = candidate && (current != candidate);

	if(diffLayout)
		setInputLanguage(candidate);

	oldInputLanguageId = diffLayout ? candidate : current;
	oldInputLanguage = lang;
}
