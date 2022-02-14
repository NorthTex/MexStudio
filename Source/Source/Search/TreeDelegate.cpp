
#include "searchresultwidget.h"
#include "configmanagerinterface.h"
#include "Latex/Document.hpp"


SearchTreeDelegate::SearchTreeDelegate(QString editorFontFamily,QObject * parent)
	: QItemDelegate(parent)
	, m_editorFontFamily(editorFontFamily) {}


void SearchTreeDelegate::paint(
	QPainter * painter,
	const QStyleOptionViewItem & option,
    const QModelIndex & index
) const {

    auto colorGroup = (option.state & QStyle::State_Enabled)
		? QPalette::Normal 
		: QPalette::Disabled;

	painter -> save();

	if(option.state & QStyle::State_Selected){
		painter -> fillRect(option.rect,option.palette.brush(colorGroup,QPalette::Highlight));
		painter -> setPen(option.palette.color(colorGroup,QPalette::HighlightedText));
	} else {
		painter -> setPen(option.palette.color(colorGroup,QPalette::Text));
	}

	// active area. Will be moved during drawing actions

	QRect r = option.rect;

	// draw checkbox

	QSize size;

	if(index.data(Qt::CheckStateRole).isValid()){

		size = doCheck(option,option.rect,Qt::Checked).size();

		QRect checkboxRect(option.rect.x(),option.rect.y(),size.width(),size.height());
        QItemDelegate::drawCheck(painter,option,checkboxRect,static_cast<Qt::CheckState>(index.data(Qt::CheckStateRole).toInt()));
	}

	int spacing = 2;

	r.adjust(size.width() + spacing,0,0,0);

	if(index.data().toString().isEmpty()){
		painter -> restore();
		return;
	}

	// draw filename line
	
	bool isFileName = !index.parent().isValid();
	
	if(isFileName){
		QString text = index.data().toString();
		QFont font = painter -> font();
		font.setBold(true);
		painter -> setFont(font);
		painter -> drawText(r,Qt::AlignLeft | Qt::AlignTop | Qt::TextSingleLine,text);
		painter -> restore();
		return;
	}

	// draw regular line

	bool isSelected = option.state & QStyle::State_Selected;
	
	if(!m_editorFontFamily.isEmpty()){
		QFont font = painter -> font();
		font.setFamily(m_editorFontFamily);
		painter -> setFont(font);
	}

	// draw line number

	QVariant vLineNumber = index.data(SearchResultModel::LineNumberRole);

	if(vLineNumber.isValid()){

		int hPadding = 1;
		int lwidth = UtilsUi::getFmWidth(painter -> fontMetrics(),"00000") + 2 * hPadding;
		
		QRect lineNumberRect = QRect(r.left(),r.top(),lwidth,r.height());
		
		if(!isSelected)
			painter -> fillRect(lineNumberRect,option.palette.window());

		painter -> drawText(lineNumberRect.adjusted(hPadding,0,-hPadding,0),Qt::AlignRight | Qt::AlignTop | Qt::TextSingleLine,vLineNumber.toString());
		r.adjust(lwidth + spacing,0,0,0);
	}

	// draw text

	QString text = index.data().toString();
	QList<SearchMatch> matches = index.data(SearchResultModel::MatchesRole).value<QList<SearchMatch>>();

	int pos = 0;

	for(const auto & match : matches){
		
		// text before match

		QString part = text.mid(pos,match.pos - pos);
		int w = UtilsUi::getFmWidth(painter -> fontMetrics(),part);
		painter -> drawText(r,Qt::AlignLeft | Qt::AlignTop | Qt::TextSingleLine,part);
		r.setLeft(r.left() + w + 1);

		// matched text
		
		part = text.mid(match.pos,match.length);
		w = UtilsUi::getFmWidth(painter -> fontMetrics(),part);
		painter -> save();
        
        painter -> fillRect(QRect(r.left(),r.top(),w,r.height()),QBrush(QColor(255,239,11)));

        const auto penColor = darkMode
            ? Qt::black
            : option.palette.color(colorGroup,QPalette::Text);

		painter -> setPen(penColor);
		painter -> drawText(r,Qt::AlignLeft | Qt::AlignTop | Qt::TextSingleLine,part);
		painter -> restore();

		r.setLeft(r.left() + w + 1);
		
		pos = match.pos + match.length;
	}

	if(pos < text.length()){
		// text after last match
		QString part = text.mid(pos);
		painter -> drawText(r,Qt::AlignLeft | Qt::AlignTop | Qt::TextSingleLine,part);
	}

	painter -> restore();
}


// TODO: the size hint is not exact because the with of the checkbox is missing and
// result lines do not use option.font but m_editorFontFamily

QSize SearchTreeDelegate::sizeHint(
	const QStyleOptionViewItem & option,
    const QModelIndex & index
) const {

	const auto metrics = option.fontMetrics;
	const auto bounds = metrics.boundingRect(index.data().toString());
	
	return QSize(bounds.width(),bounds.height());
}
