

#include "LatexReference.hpp"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include "smallUsefulFunctions.h"


using Reference = LatexReference;


Reference::LatexReference(QObject * parent)
    : QObject(parent) {}


/**
 *     \brief
 */

void Reference::setFile(QString path){

	this -> filename = path;

	if(path.isEmpty())
		return;

	QFile file(path);

	if(!file.open(QFile::ReadOnly | QFile::Text))
		return;

	QTextStream stream(& file);

	html = stream.readAll();
	makeIndex();
}


/**
 *      \brief
 */

bool Reference::contains(Command command){
    return anchors.contains(command);
}


/// tries to generate a text of suitable length for display as a tooltip
QString Reference::tooltipText(Command command){

    QString sectionText = getSectionText(command);

    if(sectionText.count('\n') > 30){ // tooltip would be very large: try to get a reasonable smaller string
        if(command.startsWith("\\begin{")){
            return truncateLines(sectionText,30);
        } else {
            QString partialText = getPartialText(command);
            int nr = partialText.count('\n');

            if(nr < 10)
                return truncateLines(sectionText,30);

            if(!partialText.isEmpty())
                return partialText;
        }
    }

    return sectionText;
}


/*! get all the text in the section describing the command
 * it starts with the first heading after the section anchor and ranges down to the next <hr>
 */

QString Reference::getSectionText(Command command){

	Anchor anchor(sectionAnchors[command]);

	auto & name = anchor.name;

	if(name.isNull())
		return QString();

	auto & start = anchor.start_pos;

	if(start < 0){
		start = html.indexOf(QString("<a name=\"%1\">").arg(name));
		start = html.indexOf("<h",start); // skip header div by going to next headline
	}

	if(start < 0)
		return QString();

	auto & end = anchor.end_pos;

	if(end < 0){
		QString tag("<hr>");
		end = html.indexOf(tag,start);
		sectionAnchors.insert(command,anchor); // save positions for a faster lookup next time
	}

	return html.mid(start,end - start);
}


/*! get only a partial description for the command
 *  the serach looks for the following block types (sqare brackets mark the extrated text:
 *    [<dt>(anchor-in-here)</dt><dd></dd>]
 *    </div>[(anchor-in-here)]</a name=
 */

QString Reference::getPartialText(Command command){

	static QRegularExpression startTag("<(dt|/div)>");

	Anchor anchor(anchors[command]);

	auto & name = anchor.name;

	if(name.isNull())
		return QString();

	QString endTag;
	int endOffset = 0;

	auto & start = anchor.start_pos;

	if(start < 0){

		start = html.indexOf(QString("<a name=\"%1\">").arg(name));

		QRegularExpressionMatch match;

		start = html.lastIndexOf(startTag,start,& match);

		if(match.captured(1) == "dt"){
			endTag = "</dd>";
			endOffset = endTag.length();
		} else {
			start += QString("</div>").length();
			endTag = "</p>";
		}
	}

	if(start < 0)
		return QString();

	auto & end = anchor.end_pos;

	if(end < 0){

		end = html.indexOf(endTag,start + 1);

		if(end >= 0)
			end += endOffset;

		int hrEnd = html.indexOf("<hr>",start + 1);

		if(hrEnd >= 0 && hrEnd < end) // don't go further than the next <hr>
			end = hrEnd;

		anchors.insert(command,anchor); // save positions for a faster lookup next time
	}

	return html.mid(start,end - start);
}


/* parses the index of the reference manual and extracts the anchor names for the commands */

void Reference::makeIndex(){

	QString startTag("<table class=\"index-cp\"");
	QString endTag("</table>");

	int start = html.indexOf(startTag);

	if(start < 0)
		return;

	int end = html.indexOf(endTag,start);
	int length = end < 0 ? -1 : end - start + endTag.length();

	QString indexText = html.mid(start, length);

	QRegularExpression rx("<a href=\"#([^\"]+)\"><code>([^\\s<]+)[^\n]*<a href=\"#([^\"]+)\">([^<]+)</a>");

	int pos = 0;

	while(pos >= 0){

        QRegularExpressionMatch match;
        pos = indexText.indexOf(rx,pos,& match);

		if(pos < 0)
			break;

        QString anchorName(match.captured(1));
        QString word(match.captured(2));
        QString sectionAnchorName(match.captured(3));

        if(word.startsWith('\\')){ // a command

			if(word == "\\begin" || word == "\\"){
				// don't add these words to the index because they give a mess in the tooltips
				pos += match.capturedLength();
				continue;
			}

			anchors.insert(word, Anchor(anchorName));
			sectionAnchors.insert(word, Anchor(sectionAnchorName));
		} else
		if (anchorName.contains("environment")) { // an environment
			anchors.insert("\\begin{" + word, Anchor(anchorName));
			sectionAnchors.insert("\\begin{" + word, Anchor(sectionAnchorName));
		} else {
			// TODO: anything useful in the rest?
			//qDebug() << word << anchorName << sectionAnchorName << sectionTitle;
		}

		pos += match.capturedLength();
	}
	//qDebug() << "Found entries in index:" << m_anchors.count();
}
