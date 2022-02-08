
#include "symbollistmodel.h"
#include "smallUsefulFunctions.h"
#include "qsvgrenderer.h"

#include <optional>
#include <functional>




/*!
 * A model for providing all the symbols. Specializations can be done using
 * QSortFilterProxyModels. This means that we have to maintain all the relevant
 * information (also usage count and favorites) within the model.
 *
 * This may be a bit heavy and inefficient but does the job so far. It reliefs
 * us from providing extra specialized models for "most used" and "favorites"
 * based on the same data.
 */

SymbolListModel::SymbolListModel(QVariantMap usageCountMap,QStringList favoriteList) 
	: m_darkMode(false) {

	for(const QString & key : usageCountMap.keys())
		usageCount.insert(key,usageCountMap.value(key).toInt());

	favoriteIds = favoriteList;
}


/*!
 * \brief find and load all symbols of a given category
 * \param category
 */

void SymbolListModel::load(QString category){

	QStringList files;
	
	files = findResourceFiles("symbols-ng/" + category, "img*.*");
	
	if(files.isEmpty()) // fallback
		files = findResourceFiles("symbols/" + category, "img*.png");

	QStringList fullNames;

	for(const QString & part : files)
		fullNames << category + "/" + part;

	if(fullNames.length() == 0)
		qDebug() << ("No symbols found for category: " + category);

	loadSymbols(category, fullNames);
}


/*!
 * \brief extract meta data from SVG file and add the file directly as QIcon
 * \param path
 * \return
 */

SymbolItem loadSymbolFromSvg(QString path){

	QFile file(path);

	if(!file.open(QIODevice::ReadOnly)){
		qDebug() << "could not open file";
		return SymbolItem();
	}

	QDomDocument doc("svg");

	QString errorMsg;
	int errorLine, errorColumn;

	if(!doc.setContent(& file,false,& errorMsg,& errorLine,& errorColumn)){
	
		qDebug() << "could not find xml content";
		qDebug() << errorMsg;
		qDebug() << "line is " << errorLine;
		qDebug() << "column is " << errorColumn;
	
		file.close();
	
		return SymbolItem();
	}
	
	file.close();

	// check root element
	auto root = doc.documentElement();
	
	if(root.tagName() != "svg"){
		qDebug() << "wrong format";
		return SymbolItem();
	}


	SymbolItem item;
	item.iconFile = path;
	item.icon = QIcon(path);

	auto nodes = root.elementsByTagName("title");

	if(!nodes.isEmpty()){
		auto node = nodes.at(0);
		item.command = node.toElement().text();
	}

	nodes = root.elementsByTagName("desc");
	
	if(!nodes.isEmpty()){
		auto node = nodes.at(0);
		item.packages = node.toElement().attribute("Packages");
	}

	nodes = root.elementsByTagName("additionalInfo");

	if(!nodes.isEmpty()){
		auto node = nodes.at(0);
		item.unicode = node.toElement().attribute("CommandUnicode");
	}

	return item;
}


/*!
 * \brief load symbols from files
 * png is inverted in dark mode.
 * Symbols are taken from symbols-ng, directory symbols is used as fall-back.
 * meta data is extracted from png/svg files.
 * \param category add symbols to given category
 * \param fileNames list of file names
 */

QString toRealUnicode(const QString fake){

	QString real;

	for(auto & string : fake.split(',')){
		
		bool ok;
		
		const int code = string
			.mid(2) // Remove U+
			.toInt(& ok);

		if(ok)
			real += QChar(code);
	}

	return real;
}

void SymbolListModel::loadSymbols(const QString & category,const QStringList & fileNames){

	for(int i = 0;i < fileNames.size();++i){
		
		QString iconName = fileNames.at(i);
		QString fileName = findResourceFile("symbols-ng/" + iconName);
		
		if(fileName.isEmpty())
			fileName = findResourceFile("symbols/" + iconName);

		SymbolItem symbolItem;

		if(fileName.endsWith("svg")) {
			symbolItem = loadSymbolFromSvg(fileName);
		} else {
			QImage img = QImage(fileName);
			symbolItem.command = img.text("Command");
			symbolItem.packages = img.text("Packages");
			symbolItem.unicode = img.text("CommandUnicode");
			symbolItem.iconFile = fileName;
            symbolItem.icon = QIcon(fileName);
		}

		if(!symbolItem.unicode.isEmpty())
			symbolItem.unicode = toRealUnicode(symbolItem.unicode);

		symbolItem.category = category;
		symbolItem.id = category + '/' + symbolItem.command.mid(1);  // e.g. "greek/alpha"

		symbols.append(symbolItem);
	}

	return;
}


/*!
 * \brief generate a QVariantMap of symbol ids/usage count.
 * Is used for saving info in configuration.
 * \return map
 */

QVariantMap SymbolListModel::usageCountAsQVariantMap() const {

	QVariantMap map;
	
	for(const QString & key : usageCount.keys())
		map.insert(key,usageCount.value(key,0));
	
	return map;
}


/*!
 * \brief return ids of favourites
 * \return
 */

QStringList SymbolListModel::favorites() const {
	return favoriteIds;
}


/*!
 * \brief gives number of symbols
 * \param parent
 * \return number of symbols
 */

int SymbolListModel::rowCount(const QModelIndex & parent) const {
	Q_UNUSED(parent)
	return symbols.count();
}


/*!
 * \brief provide model data for list view
 * \param index
 * \param role
 * \return GUI data
 */

QVariant SymbolListModel::data(const QModelIndex & index,int role) const {

	int r = index.row();
	
	if(r < 0)
		return QVariant();
		
	if(r >= symbols.count())
		return QVariant();

	const auto symbol = symbols[r];

	switch(role){
	case Qt::DisplayRole:
		return QVariant();  // we do only want symbols, so we do not return a text
	case Qt::DecorationRole:
		return getIcon(symbol);
	case Qt::ToolTipRole:
		return getTooltip(symbol);
	}

	switch(role){
	case IdRole:
		return symbol.id;
	case CommandRole:
		return symbol.command;
	case UnicodeRole:
		return symbol.unicode;
	case CategoryRole:
		return symbol.category;
	}

	switch(role){
	case UsageCountRole:
		return usageCount.value(symbol.id, 0);
	case FavoriteRole:
		return QVariant(favoriteIds.contains(symbol.id));
	default:
		return QVariant();
	}
}


/*!
 * \brief increment usage count of symbol id
 * \param id
 */

void SymbolListModel::incrementUsage(const QString & id){

	usageCount.insert(id,usageCount.value(id,0) + 1);
	
	for(int i = 0;i < symbols.count();i++)
		if(symbols[i].id == id){
			emit dataChanged(index(i,0),index(i,0),(QVector<int>() << UsageCountRole));
			break;
		}
}


/*!
 * \brief activate dark mode
 * This mode basically inverts icon colors (black to white)
 * \param active true -> activate mode
 */

void SymbolListModel::setDarkmode(bool active){
    m_darkMode = active;
}


/*!
 * \brief add symbol with id to favourite list
 * \param id  symbol id
 */

void SymbolListModel::addFavorite(const QString & id){
	
	if(favoriteIds.contains(id))
		return;

	favoriteIds.append(id);

	updateFavorites(id);
}


/*!
 * \brief remove symbol with id from favourite list
 * \param id symbol id
 */

void SymbolListModel::removeFavorite(const QString & id){
	if(favoriteIds.removeOne(id))
		updateFavorites(id);
}

void SymbolListModel::updateFavorites(const QString & id){
	for(int i = 0;i < symbols.count();i++)
		if(symbols[i].id == id){
			emit dataChanged(index(i,0),index(i,0),QVector<int>() << FavoriteRole);
			emit favoritesChanged();
			break;
		}
}


QImage svgImageFrom(const QString path){
	
	QSvgRenderer renderer(path);
	auto size = renderer.defaultSize() * 4;

	QImage image(size.width(),size.height(),QImage::Format_ARGB32);
	QPainter painter(& image);

	image.fill(0x000000000);
	renderer.render(& painter);

	return image;
}


/*!
 * \brief return the icon for a symbol
 * This icon is manipulated in darkmode to be inverted (SVG only)
 * \param item
 * \return icon
 */

QIcon SymbolListModel::getIcon(const SymbolItem & item) const {

	// OSX workaround

	#ifdef MXE 
		bool OSX_Fallback = true;
	#else
		bool OSX_Fallback = false;
	#endif

	if(!m_darkMode && !OSX_Fallback)
		return item.icon;

	const auto file = item.iconFile;

	QImage image = (file.endsWith("svg"))
		? svgImageFrom(file)
		: QImage(file);

	if(m_darkMode)
		image.invertPixels(QImage::InvertRgb);

	return QIcon(QPixmap::fromImage(image));
}


/*!
 * \brief get tooltip text for symbolitem item
 * \param item
 * \return tooltip text
 */

const QRegExp regex_Packages("(?:\\[(.*)\\])?\\{(.*)\\}");

QString SymbolListModel::getTooltip(const SymbolItem & item) const {

	QStringList args , packages;

	QString label = item.command;
	label.replace("<", "&lt;");
	label = tr("Command: ") + "<b>" + label + "</b>";

	if(regex_Packages.indexIn(item.packages) != -1){
		args = regex_Packages.cap(1).split(',');
		packages = regex_Packages.cap(2).split(',');
	}

	if(packages.count() > 0){

		label += "<br>";

		label += (packages.count() == 1)
			? tr("Package: ")
			: tr("Packages: ");
	
		for(int j = 0;j < packages.count();j++){

			if(j > 0)
				label += '\n';
			
			if(j < args.count() && ! args.isEmpty())
				label += '[' + args[j] + ']';

			label += packages[j];
		}
	}

	if(!item.unicode.isEmpty())
		label += "<br>" + tr("Unicode Character: ") + item.unicode;
	
	return label;
}



