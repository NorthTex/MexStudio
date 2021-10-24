#ifndef Header_BibTex_Type
#define Header_BibTex_Type


#include <QString>
#include <QStringList>


namespace BibTex { struct Type; }


struct BibTex::Type {

	private:

		using string = const QString &;
		using strings = const QStringList &;

	public:

		QStringList required , optional;
		QString name , description;

	public:

		Type(const Type & other)
			: required(other.required)
			, optional(other.optional)
			, name(other.name)
			, description(other.description) {}

		Type(string name,string description,strings required,strings optional)
			: required(required)
			, optional(optional)
			, name(name)
			, description(description) {}

	public:

		Type & operator = (const Type &) = default;

};


#endif
