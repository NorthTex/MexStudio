
#include "BibTex/Dialog.hpp"


using BibTex::Dialog;


// TODO: Replace with .yaml


QList<BibTex::Type> Dialog::generateBibTex(bool forceRecreate){

	if(!forceRecreate && !bibtex.isEmpty())
		return bibtex;

	if(forceRecreate)
		bibtex = {};

	bibtex << TexType(tr("Article in &Journal"),"@Article",
								   QStringList() << "author" << "title" << "journal" << "year",
								   QStringList() << "key" << "volume" << "number" << "pages" << "month" << "note" << "annote");
	bibtex << TexType(tr("Article in Conference &Proceedings"), "@InProceedings",
								   QStringList() << "author" << "title" << "booktitle",
								   QStringList() << "crossref" << "key" << "pages" << "year" << "editor" << "volume" << "number" << "series" << "address" << "month" << "organization" << "publisher" << "note" << "annote");
	bibtex << TexType(tr("Article in a &Collection"), "@InCollection",
								   QStringList() << "author" << "title" << "booktitle",
								   QStringList() << "crossref" << "key" << "pages" << "publisher" << "year" <<  "editor" << "volume" << "number" << "series" << "type" << "chapter" << "address" << "edition" << "month" << "note" << "annote");
	bibtex << TexType(tr("Chapter or &Pages in a Book"), "@InBook",
								   QStringList() << "author/editor" << "title" << "chapter" << "publisher" << "year",
								   QStringList() << "key" << "volume" << "number" << "series" << "type" << "address" << "edition" << "month" << "pages" << "note" <<  "annote");
	bibtex << TexType(tr("Conference Pr&oceedings"), "@Proceedings",
								   QStringList() << "title" << "year",
								   QStringList() << "key" << "editor" << "volume" << "number" <<
								   "series" << "address" << "month" << "organization" << "publisher" << "note" <<
								   "annote");
	bibtex << TexType(tr("&Book"), "@Book",
								   QStringList() << "author/editor" << "title" << "publisher" << "year",
								   QStringList() <<  "key" << "volume" << "number" << "series" << "address" <<
								   "edition" << "month" << "note" << "annote");
	bibtex << TexType(tr("Book&let"), "@Booklet",
								   QStringList() << "title",
								   QStringList() << "key" << "author" << "howpublished" << "address" << "month" << "year" << "note" << "annote");
	bibtex << TexType(tr("PhD. &Thesis"), "@PhdThesis",
								   QStringList() << "author" << "title" << "school" << "year",
								   QStringList() << "key" << "type" << "address" << "month" << "note" <<
								   "annote");
	bibtex << TexType(tr("&Master's Thesis"), "@MastersThesis",
								   QStringList() << "author" << "title" << "school" << "year",
								   QStringList() <<  "key" << "type" << "address" << "month" <<  "note" <<
								   "annote");
	bibtex << TexType(tr("Technical &Report"), "@TechReport",
								   QStringList() << "author" << "title" << "institution" << "year",
								   QStringList() <<  "key" << "type" << "number" << "address" << "month" <<
								   "note" << "annote");
	bibtex << TexType(tr("Technical Ma&nual"), "@Manual",
								   QStringList() << "title",
								   QStringList() << "key" << "author" << "organization" << "address" << "edition" << "month" << "year" << "note" << "annote");

	bibtex << TexType(tr("&Unpublished"), "@Unpublished",
								   QStringList() << "author" << "title" << "note",
								   QStringList() << "key" << "month" << "year" << "annote");
	bibtex << TexType(tr("Miscellan&eous"), "@Misc",
								   QStringList(),
								   QStringList() << "key" << "author" << "title" << "howpublished" << "month" << "year" << "note" << "annote");

	return bibtex;
}
