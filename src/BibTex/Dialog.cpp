 

#include "BibTex/Dialog.hpp"
#include "ui_bibtexdialog.h"
#include "utilsUI.h"




using BibTex::Dialog;
using TexType = BibTex::Type;
using StringMap = QMap<QString,QString>;

void Dialog::generateBibtexEntryTypes(bool forceRecreate){

	if(!forceRecreate && !bibtex.isEmpty())
		return;

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

}

void Dialog::generateBiblatexEntryTypes(bool forceRecreate){

	if(!forceRecreate && !biblatex.isEmpty())
		return;

	if(forceRecreate)
		biblatex = {};


	// An article in a journal, magazine,
	// newspaper, or other periodical which forms
	// a self-contained unit with its own
	// title. The title of the periodical is given
	// in the journaltitle field. If the issue has
	// its own title in addition to the main title
	// of the periodical, it goes in the
	// issuetitle field. Note that "editor" and
	// related fields refer to the journal while
	// translator and related fields refer to the
	// article.

	biblatex << TexType(tr("Article in &Journal"),"@article",
		QStringList() << "author" << "title" << "journaltitle" << "date",
		QStringList() << "translator" << "annotator" << "commentator" << "subtitle"
		<< "titleaddon" << "editor" << "editora" << "editorb" << "editorc"
		<< "journalsubtitle" << "issuetitle" << "issuesubtitle" << "language"
		<< "origlanguage" << "series" << "volume" << "number" << "eid" << "issue"
		<< "month" << "pages" << "version" << "note" << "issn" << "addendum"
		<< "pubstate" << "doi" << "eprint" << "eprintclass" << "eprinttype"
		<< "url" << "urldate");

	biblatex << TexType(tr("&Book"), "@book",
									 // A single-volume book with one or more
									 // authors where the authors share credit for
									 // the work as a whole. This entry type also
									 // covers the function of the @inbook type of
									 // traditional BibTeX, see � 2.3.1 for
									 // details.
									 QStringList() << "author" << "title" << "date",
									 QStringList() << "editor" << "editora" << "editorb" << "editorc" << "translator" << "annotator" << "commentator" << "introduction" << "foreword" << "afterword" << "subtitle" << "titleaddon" << "maintitle" << "mainsubtitle" << "maintitleaddon" << "language" << "origlanguage" << "volume" << "part" << "edition" << "volumes" << "series" << "number" << "note" << "publisher" << "location" << "isbn" << "chapter" << "pages" << "pagetotal" << "addendum" << "pubstate" << "doi" << "eprint" << "eprintclass" << "eprinttype" << "url" << "urldate");
	biblatex << TexType(tr("&Multi-volume Book"), "@mvbook",
									 // A multi-volume @book. For backwards
									 // compatibility, multi-volume books are also
									 // supported by the entry type @book. However,
									 // it is advisable to make use of the
									 // dedicated entry type @mvbook.
									 QStringList() << "author" << "title" << "date",
									 QStringList() << "editor" << "editora" << "editorb" << "editorc" << "translator" << "annotator" << "commentator" << "introduction" << "foreword" << "afterword" << "subtitle" << "titleaddon" << "language" << "origlanguage" << "edition" << "volumes" << "series" << "number" << "note" << "publisher" << "location" << "isbn" << "pagetotal" << "addendum" << "pubstate" << "doi" << "eprint" << "eprintclass" << "eprinttype" << "url" << "urldate");
	biblatex << TexType(tr("Part of a Book With Its Own Title"), "@inbook",
									 // A part of a book which forms a
									 // self-contained unit with its own
									 // title. Note that the profile of this entry
									 // type is di|erent from standard BibTeX, see
									 // � 2.3.1.
									 QStringList() << "author" << "title" << "booktitle" << "date",
									 QStringList() << "bookauthor" << "editor" << "editora" << "editorb" << "editorc" << "translator" << "annotator" << "commentator" << "introduction" << "foreword" << "afterword" << "subtitle" << "titleaddon" << "maintitle" << "mainsubtitle" << "maintitleaddon" << "booksubtitle" << "booktitleaddon" << "language" << "origlanguage" << "volume" << "part" << "edition" << "volumes" << "series" << "number" << "note" << "publisher" << "location" << "isbn" << "chapter" << "pages" << "addendum" << "pubstate" << "doi" << "eprint" << "eprintclass" << "eprinttype" << "url" << "urldate");
	biblatex << TexType(tr("Book in Book"), "@bookinbook",
									 // This type is similar to @inbook but
									 // intended for works originally published as
									 // a stand-alone book. A typical example are
									 // books reprinted in the collected works of
									 // an author.
									 QStringList() << "author" << "title" << "booktitle" << "date",
									 QStringList() << "bookauthor" << "editor" << "editora" << "editorb" << "editorc" << "translator" << "annotator" << "commentator" << "introduction" << "foreword" << "afterword" << "subtitle" << "titleaddon" << "maintitle" << "mainsubtitle" << "maintitleaddon" << "booksubtitle" << "booktitleaddon" << "language" << "origlanguage" << "volume" << "part" << "edition" << "volumes" << "series" << "number" << "note" << "publisher" << "location" << "isbn" << "chapter" << "pages" << "addendum" << "pubstate" << "doi" << "eprint" << "eprintclass" << "eprinttype" << "url" << "urldate");
	biblatex << TexType(tr("Supplemental Material in a Book"), "@suppbook",
									 // Supplemental material in a @book. This type
									 // is closely related to the @inbook entry
									 // type. While @inbook is primarily intended
									 // for a part of a book with its own title
									 // (e. g., a single essay in a collection of
									 // essays by the same author), this type is
									 // provided for elements such as prefaces,
									 // introductions, forewords, afterwords, etc.
									 // which often have a generic title
									 // only. Style guides may require such items
									 // to be formatted di|erently from other
									 // @inbook items. The standard styles will
									 // treat this entry type as an alias for
									 // @inbook.
									 QStringList() << "author" << "title" << "booktitle" << "date",
									 QStringList() << "bookauthor" << "editor" << "editora" << "editorb" << "editorc" << "translator" << "annotator" << "commentator" << "introduction" << "foreword" << "afterword" << "subtitle" << "titleaddon" << "maintitle" << "mainsubtitle" << "maintitleaddon" << "booksubtitle" << "booktitleaddon" << "language" << "origlanguage" << "volume" << "part" << "edition" << "volumes" << "series" << "number" << "note" << "publisher" << "location" << "isbn" << "chapter" << "pages" << "addendum" << "pubstate" << "doi" << "eprint" << "eprintclass" << "eprinttype" << "url" << "urldate");
	biblatex << TexType(tr("Book&let"), "@booklet",
									 // A book-like work without a formal publisher
									 // or sponsoring institution. Use the field
									 // howpublished to supply publishing
									 // information in free format, if applicable.
									 // The field type may be useful as well.
									 QStringList() << "author/editor" << "title" << "date",
									 QStringList() << "subtitle" << "titleaddon" << "language" << "howpublished" << "type" << "note" << "location" << "chapter" << "pages" << "pagetotal" << "addendum" << "pubstate" << "doi" << "eprint" << "eprintclass" << "eprinttype" << "url" << "urldate");
	biblatex << TexType(tr("Single-volume Collection"), "@collection",
									 // A single-volume collection with multiple,
									 // self-contained contributions by distinct
									 // authors which have their own title. The
									 // work as a whole has no overall author but
									 // it will usually have an editor.
									 QStringList() << "editor" << "title" << "date",
									 QStringList() << "editora" << "editorb" << "editorc" << "translator" << "annotator" << "commentator" << "introduction" << "foreword" << "afterword" << "subtitle" << "titleaddon" << "maintitle" << "mainsubtitle" << "maintitleaddon" << "language" << "origlanguage" << "volume" << "part" << "edition" << "volumes" << "series" << "number" << "note" << "publisher" << "location" << "isbn" << "chapter" << "pages" << "pagetotal" << "addendum" << "pubstate" << "doi" << "eprint" << "eprintclass" << "eprinttype" << "url" << "urldate");
	biblatex << TexType(tr("Multi-volume Collection"), "@mvcollection",
									 // A multi-volume @collection. For backwards
									 // compatibility, multi-volume collections are
									 // also supported by the entry type
									 // @collection. However, it is advisable to
									 // make use of the dedicated entry type
									 // @mvcollection.
									 QStringList() << "editor" << "title" << "date",
									 QStringList() << "editora" << "editorb" << "editorc" << "translator" << "annotator" << "commentator" << "introduction" << "foreword" << "afterword" << "subtitle" << "titleaddon" << "language" << "origlanguage" << "edition" << "volumes" << "series" << "number" << "note" << "publisher" << "location" << "isbn" << "pagetotal" << "addendum" << "pubstate" << "doi" << "eprint" << "eprintclass" << "eprinttype" << "url" << "urldate");
	biblatex << TexType(tr("Article in a &Collection"), "@incollection",
									 // A contribution to a collection which forms
									 // a self-contained unit with a distinct
									 // author and title. The author refers to the
									 // title, the editor to the booktitle, i. e.,
									 // the title of the collection.
									 QStringList() << "author" << "editor" << "title" << "booktitle" << "date",
									 QStringList() << "editora" << "editorb" << "editorc" << "translator" << "annotator" << "commentator" << "introduction" << "foreword" << "afterword" << "subtitle" << "titleaddon" << "maintitle" << "mainsubtitle" << "maintitleaddon" << "booksubtitle" << "booktitleaddon" << "language" << "origlanguage" << "volume" << "part" << "edition" << "volumes" << "series" << "number" << "note" << "publisher" << "location" << "isbn" << "chapter" << "pages" << "addendum" << "pubstate" << "doi" << "eprint" << "eprintclass" << "eprinttype" << "url" << "urldate");
	biblatex << TexType(tr("Supplemental Material in a Collection"), "@suppcollection",
									 // Supplemental material in a
									 // @collection. This type is similar to
									 // @suppbook but related to the @collection
									 // entry type. The standard styles will treat
									 // this entry type as an alias for
									 // @incollection.
									 QStringList() << "author" << "title" << "booktitle" << "date",
									 QStringList() << "bookauthor" << "editor" << "editora" << "editorb" << "editorc" << "translator" << "annotator" << "commentator" << "introduction" << "foreword" << "afterword" << "subtitle" << "titleaddon" << "maintitle" << "mainsubtitle" << "maintitleaddon" << "booksubtitle" << "booktitleaddon" << "language" << "origlanguage" << "volume" << "part" << "edition" << "volumes" << "series" << "number" << "note" << "publisher" << "location" << "isbn" << "chapter" << "pages" << "addendum" << "pubstate" << "doi" << "eprint" << "eprintclass" << "eprinttype" << "url" << "urldate");
	biblatex << TexType(tr("Technical Ma&nual"), "@manual",
									 // Technical or other documentation, not
									 // necessarily in printed form. The author or
									 // editor is omissible in terms of � 2.3.2.
									 QStringList() << "author/editor" << "title" << "date",
									 QStringList() << "subtitle" << "titleaddon" << "language" << "edition" << "type" << "series" << "number" << "version" << "note" << "organization" << "publisher" << "location" << "isbn" << "chapter" << "pages" << "pagetotal" << "addendum" << "pubstate" << "doi" << "eprint" << "eprintclass" << "eprinttype" << "url" << "urldate");
	biblatex << TexType(tr("Miscellan&eous"), "@misc",
									 // A fallback type for entries which do not
									 // fit into any other category. Use the field
									 // howpublished to supply publishing
									 // information in free format, if
									 // applicable. The field type may be useful as
									 // well. author, editor, and year are
									 // omissible in terms of � 2.3.2.
									 QStringList() << "author/editor" << "title" << "date",
									 QStringList() << "subtitle" << "titleaddon" << "language" << "howpublished" << "type" << "version" << "note" << "organization" << "location" << "date" << "month" << "year" << "addendum" << "pubstate" << "doi" << "eprint" << "eprintclass" << "eprinttype" << "url" << "urldate");
	biblatex << TexType(tr("Online Resource"), "@online",
									 // An online resource. author, editor, and
									 // year are omissible in terms of � 2.3.2.
									 // This entry type is intended for sources
									 // such as web sites which are intrinsicly
									 // online resources. Note that all entry types
									 // support the url field. For example, when
									 // adding an article from an online journal,
									 // it may be preferable to use the @article
									 // type and its url field.
									 QStringList() << "author/editor" << "title" << "date" << "url",
									 QStringList() << "subtitle" << "titleaddon" << "language" << "version" << "note" << "organization" << "date" << "month" << "year" << "addendum" << "pubstate" << "urldate");
	biblatex << TexType(tr("Patent"), "@patent",
									 // A patent or patent request. The number or
									 // record token is given in the number
									 // field. Use the type field to specify the
									 // type and the location field to indicate the
									 // scope of the patent, if di|erent from the
									 // scope implied by the type. Note that the
									 // location field is treated as a key list
									 // with this entry type, see � 2.2.1 for
									 // details.
									 QStringList() << "author" << "title" << "number" << "date",
									 QStringList() << "holder" << "subtitle" << "titleaddon" << "type" << "version" << "location" << "note" << "date" << "month" << "year" << "addendum" << "pubstate" << "doi" << "eprint" << "eprintclass" << "eprinttype" << "url" << "urldate");
	biblatex << TexType(tr("Complete Issue of a Periodical"), "@periodical",
									 // An complete issue of a periodical, such as
									 // a special issue of a journal. The title of
									 // the periodical is given in the title
									 // field. If the issue has its own title in
									 // addition to the main title of the
									 // periodical, it goes in the issuetitle
									 // field. The editor is omissible in terms of
									 // � 2.3.2.
									 QStringList() << "editor" << "title" << "date",
									 QStringList() << "editora" << "editorb" << "editorc" << "subtitle" << "issuetitle" << "issuesubtitle" << "language" << "series" << "volume" << "number" << "issue" << "date" << "month" << "year" << "note" << "issn" << "addendum" << "pubstate" << "doi" << "eprint" << "eprintclass" << "eprinttype" << "url" << "urldate");
	biblatex << TexType(tr("Supplemental Material in a Periodical"), "@suppperiodical",
									 // Supplemental material in a
									 // @periodical. This type is similar to
									 // @suppbook but related to the @periodical
									 // entry type. The role of this entry type may
									 // be more obvious if you bear in mind that
									 // the @article type could also be called
									 // @inperiodical.  This type may be useful
									 // when referring to items such as regular
									 // columns, obituaries, letters to the editor,
									 // etc. which only have a generic title. Style
									 // guides may require such items to be
									 // formatted di|erently from articles in the
									 // strict sense of the word. The standard
									 // styles will treat this entry type as an
									 // alias for @article.
									 QStringList() << "author" << "title" << "booktitle" << "date",
									 QStringList() << "bookauthor" << "editor" << "editora" << "editorb" << "editorc" << "translator" << "annotator" << "commentator" << "introduction" << "foreword" << "afterword" << "subtitle" << "titleaddon" << "maintitle" << "mainsubtitle" << "maintitleaddon" << "booksubtitle" << "booktitleaddon" << "language" << "origlanguage" << "volume" << "part" << "edition" << "volumes" << "series" << "number" << "note" << "publisher" << "location" << "isbn" << "chapter" << "pages" << "addendum" << "pubstate" << "doi" << "eprint" << "eprintclass" << "eprinttype" << "url" << "urldate");
	biblatex << TexType(tr("Conference Pr&oceedings"), "@proceedings",
									 // A single-volu conference
									 // proceedings. This type is very similar to
									 // @collection.  It supports an optional
									 // organization field which holds the
									 // sponsoring institution.  The editor is
									 // omissible in terms of � 2.3.2.
									 QStringList() << "editor" << "title" << "date",
									 QStringList() << "subtitle" << "titleaddon" << "maintitle" << "mainsubtitle" << "maintitleaddon" << "eventtitle" << "eventdate" << "venue" << "language" << "volume" << "part" << "volumes" << "series" << "number" << "note" << "organization" << "publisher" << "location" << "month" << "isbn" << "chapter" << "pages" << "pagetotal" << "addendum" << "pubstate" << "doi" << "eprint" << "eprintclass" << "eprinttype" << "url" << "urldate");
	biblatex << TexType(tr("Multi-volume Proceedings Entry"), "@mvproceedings",
									 // A multi-volume @proceedings entry. For
									 // backwards compatibility, multi-volume
									 // proceedings are also supported by the entry
									 // type @proceedings. However, it is advisable
									 // to make use of the dedicated entry type
									 // @mvproceedings
									 QStringList() << "editor" << "title" << "date",
									 QStringList() << "subtitle" << "titleaddon" << "eventtitle" << "eventdate" << "venue" << "language" << "volumes" << "series" << "number" << "note" << "organization" << "publisher" << "location" << "month" << "isbn" << "pagetotal" << "addendum" << "pubstate" << "doi" << "eprint" << "eprintclass" << "eprinttype" << "url" << "urldate");
	biblatex << TexType(tr("Article in Conference &Proceedings"), "@inproceedings",
									 // An article in a conference
									 // proceedings. This type is similar to
									 // @incollection. It supports an optional
									 // organization field.
									 QStringList() << "author" << "editor" << "title" << "booktitle" << "date",
									 QStringList() << "subtitle" << "titleaddon" << "maintitle" << "mainsubtitle" << "maintitleaddon" << "booksubtitle" << "booktitleaddon" << "eventtitle" << "eventdate" << "venue" << "language" << "volume" << "part" << "volumes" << "series" << "number" << "note" << "organization" << "publisher" << "location" << "month" << "isbn" << "chapter" << "pages" << "addendum" << "pubstate" << "doi" << "eprint" << "eprintclass" << "eprinttype" << "url" << "urldate");
	biblatex << TexType(tr("Reference"), "@reference",
									 // A single-volume work of reference such as
									 // an encyclopedia or a dictionary. This is a
									 // more specific variant of the generic
									 // @collection entry type. The standard styles
									 // will treat this entry type as an alias for
									 // @collection.
									 QStringList() << "editor" << "title" << "date",
									 QStringList() << "editora" << "editorb" << "editorc" << "translator" << "annotator" << "commentator" << "introduction" << "foreword" << "afterword" << "subtitle" << "titleaddon" << "maintitle" << "mainsubtitle" << "maintitleaddon" << "language" << "origlanguage" << "volume" << "part" << "edition" << "volumes" << "series" << "number" << "note" << "publisher" << "location" << "isbn" << "chapter" << "pages" << "pagetotal" << "addendum" << "pubstate" << "doi" << "eprint" << "eprintclass" << "eprinttype" << "url" << "urldate");
	biblatex << TexType(tr("Multi-volume Reference Entry"), "@mvreference",
									 // A multi-volume @reference entry. The
									 // standard styles will treat this entry type
									 // as an alias for @mvcollection. For
									 // backwards compatibility, multi-volume
									 // references are also supported by the entry
									 // type @reference. However, it is advisable
									 // to make use of the dedicated entry type
									 // @mvreference.
									 QStringList() << "editor" << "title" << "date",
									 QStringList() << "editora" << "editorb" << "editorc" << "translator" << "annotator" << "commentator" << "introduction" << "foreword" << "afterword" << "subtitle" << "titleaddon" << "language" << "origlanguage" << "edition" << "volumes" << "series" << "number" << "note" << "publisher" << "location" << "isbn" << "pagetotal" << "addendum" << "pubstate" << "doi" << "eprint" << "eprintclass" << "eprinttype" << "url" << "urldate");
	biblatex << TexType(tr("Article in a Reference"), "@inreference",
									 // An article in a work of reference. This is
									 // a more specific variantof the generic
									 // @incollection entry type. The standard
									 // styles will treat this entry type as an
									 // alias for @incollection.
									 QStringList() << "author" << "editor" << "title" << "booktitle" << "date",
									 QStringList() << "editora" << "editorb" << "editorc" << "translator" << "annotator" << "commentator" << "introduction" << "foreword" << "afterword" << "subtitle" << "titleaddon" << "maintitle" << "mainsubtitle" << "maintitleaddon" << "booksubtitle" << "booktitleaddon" << "language" << "origlanguage" << "volume" << "part" << "edition" << "volumes" << "series" << "number" << "note" << "publisher" << "location" << "isbn" << "chapter" << "pages" << "addendum" << "pubstate" << "doi" << "eprint" << "eprintclass" << "eprinttype" << "url" << "urldate");
	biblatex << TexType(tr("&Report"), "@report",
									 // A technical report, research report, or
									 // white paper published by a university or
									 // some other institution. Use the type field
									 // to specify the type of report ("techreport"
									 // or "resreport"). The sponsoring institution
									 // goes in the institution field.
									 QStringList() << "author" << "title" << "type" << "institution" << "date",
									 QStringList() << "subtitle" << "titleaddon" << "language" << "number" << "version" << "note" << "location" << "month" << "isrn" << "chapter" << "pages" << "pagetotal" << "addendum" << "pubstate" << "doi" << "eprint" << "eprintclass" << "eprinttype" << "url" << "urldate");
	biblatex << TexType(tr("&Thesis"), "@thesis",
									 // A thesis written for an educational
									 // institution to satisfy the requirements for
									 // a degree. Use the type field to specify the
									 // type of thesis ("mathesis" or "phdthesis").
									 QStringList() << "author" << "title" << "type" << "institution" << "date",
									 QStringList() << "subtitle" << "titleaddon" << "language" << "note" << "location" << "month" << "isbn" << "chapter" << "pages" << "pagetotal" << "addendum" << "pubstate" << "doi" << "eprint" << "eprintclass" << "eprinttype" << "url" << "urldate");
	biblatex << TexType(tr("&Unpublished"), "@unpublished",
									 // A work with an author and a title which has
									 // not been formally published, such as a
									 // manuscript or the script of a talk. Use the
									 // fields howpublished and note to supply
									 // additional information in free format, if
									 // applicable.
									 QStringList() << "author" << "title" << "date",
									 QStringList() << "subtitle" << "titleaddon" << "language" << "howpublished" << "note" << "location" << "isbn" << "date" << "month" << "year" << "addendum" << "pubstate" << "url" << "urldate");
}


Dialog::Dialog(QWidget * parent,strings files,int currentFile,string id)
	: QDialog(parent)
	, ui(new UI)
	, fileId(-2){

	ui -> setup(this);

	UtilsUi::resizeInFontHeight(this,59,36);

	ui -> files -> addItem(tr("<New File>"));

	for(auto && file : files)
		ui -> files -> addItem(file);

	ui -> files -> setCurrentRow(currentFile + 1);

	needEntryTypes();

	if(!id.isEmpty()){

		ui -> fields -> setRowCount(1);

		auto * item = new QTableWidgetItem("ID");

		QFont font = QApplication::font();
		font.setBold(true);
		item -> setFont(font);

		ui -> fields -> setItem(0,0,item);
		ui -> fields -> setItem(0,1,new QTableWidgetItem(id));
	}

	for(auto && type : * entries){
		QString description = type.description;
		auto * item = new QListWidgetItem(description.remove('&'),ui -> types);
		item -> setToolTip(type.name);
	}

	connect(ui -> types,SIGNAL(currentRowChanged(int)),SLOT(typeSelectionChanged()));

	if(type == BibLatex){
		setWindowTitle(tr("New BibLaTeX Entry"));
	} else {
		setWindowTitle(tr("New BibTeX Entry"));
	}
}

Dialog::~Dialog(){
	delete ui;
}

void Dialog::setType(Type t){
	type = t;
	needEntryTypes();
}


//generate an entry in the bibtex format out of the given parameters
QString Dialog::textToInsert(const TexType & entry,bool keepOptional,const QMap<QString,QString> & fields){

	QString result = entry.name + "{" + fields.value("ID", "%<ID%>") + ",\n";
	QMap<QString, QString> remainingFields = fields;

	remainingFields.remove("ID");

	for(auto entry : entry.required){
		if(entry.contains('/')){

			// split alternative values and use first set value,
			// if non is set use all prepended with ALT like in texmaker

			strings list = entry.split('/');
			bool inserted = false;

			for(auto value : list)
				if(remainingFields.contains(value)){
					result += value + " = {" + remainingFields.value(value, "%<" + value + "%>") + "},\n";
					inserted = true;
					remainingFields.remove(value);
					break;
				}

			if(!inserted)
				for(auto value : list)
					result += "ALT" + value + " = {%<" + value + "%>},\n";
		} else {
			result += entry + " = {" + remainingFields.value(entry,"%<" + entry + "%>") + "},\n";
			remainingFields.remove(entry);
		}
	}

	for(auto entry : entry.optional){
		QMap<QString,QString>::iterator iterator = remainingFields.find(entry);

		if(iterator == remainingFields.end()){
			if(keepOptional)
				result += "OPT" + entry + " = {%<" + entry + "%>},\n";
		} else {
			result += entry + " = {" + remainingFields.value(entry,"%<" + entry + "%>") + "},\n";
			remainingFields.erase(iterator);
		}
	}


	for(auto const & [ key , value ] : remainingFields.toStdMap())
		result += key + " = {" + value + "},\n";


	result += "}\n";

	return result;
}


QString Dialog::textToInsert(const QString & entryName){

	needEntryTypes();

	for(auto type : * entries)
		if(type.name == entryName)
			return textToInsert(type,true,QMap<QString,QString>());

	return "";
}


QList<TexType> Dialog::entryTypesFor(Type type){
	if(type == BibTex){
		generateBibtexEntryTypes(true);
		return bibtex;
	} else {
		generateBiblatexEntryTypes(true);
		return biblatex;
	}
}


void Dialog::changeEvent(QEvent * event){
	switch(event -> type()){
	case QEvent::LanguageChange:
		ui -> translate();
		break;
	default:
		break;
	}
}


void Dialog::accept(){

	result = "";

	if(ui -> types -> currentRow() < 0)
		return;

	if(ui -> types -> currentRow() >= entries -> count())
		return;

	QMap<QString,QString> curFields;

	for(int i = 0;i < ui -> fields -> rowCount();i++){

		if(! ui -> fields -> item(i,0))
			continue;

		if(! ui -> fields -> item(i,1))
			continue;

		QString k = ui -> fields -> item(i,0) -> text();

		if(k.contains("/"))
			k = k.split("/").first();

		QString v = ui -> fields -> item(i, 1) -> text();

		if(v != "")
			curFields.insert(k, v);
	}

	result = textToInsert(entries -> at(ui -> types -> currentRow()),ui -> checkbox -> isChecked(),curFields);
	fileId = ui -> files -> currentRow() - 1;

	QDialog::accept();
}


void Dialog::typeSelectionChanged(){

	if(ui -> types -> currentRow() < 0)
		return;

	if(ui -> types -> currentRow() >= entries -> count())
		return;

	//save current values
	QMap<QString,QString> curFields;

	for(int i = 0;i < ui -> fields -> rowCount();i++){

		if(! ui -> fields -> item(i,0))
			continue;

		if(! ui -> fields -> item(i,1))
			continue;

		curFields.insert(
			ui -> fields -> item(i,0) -> text(),
			ui -> fields -> item(i,1) -> text()
		);
	}

	//update
	ui -> fields -> clearContents();

	const TexType & bt = entries -> at(ui -> types -> currentRow());

	ui -> fields -> setRowCount(bt.required.count() + bt.optional.count() + 10);

	QFont font = QApplication::font();
	font.setBold(true);

	//mandatory fields
	int row = 0;
	QStringList fields = bt.required;
	fields.prepend("ID");

	for(int i = 0;i < fields.count();i++,row++){

		auto * item = new QTableWidgetItem(fields[i]);
		item -> setFont(font);
		item -> setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled); // no edit

		ui -> fields -> setItem(row,0,item);
		ui -> fields -> setItem(row,1,new QTableWidgetItem(curFields.value(fields[i],"")));
	}

	//optional fields
	for(int i = 0;i < bt.optional.count();i++,row++){

		auto * item = new QTableWidgetItem(bt.optional[i]);
		item -> setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled); // no edit

		ui -> fields -> setItem(row,0,item);
		ui -> fields -> setItem(row,1,new QTableWidgetItem(curFields.value(bt.optional[i],"")));
	}

	ui -> fields -> setCurrentCell(0,1);
	ui -> fields -> resizeRowsToContents();
}


void Dialog::needEntryTypes(){
	if(type == BibTex){
		generateBibtexEntryTypes();
		entries = & bibtex;
	} else {
		generateBiblatexEntryTypes();
		entries = & biblatex;
	}
}
