/**************************************************************************
**
** Copyright (c) 2014 Carel Combrink
**
** This file is part of the SpellChecker Plugin, a Qt Creator plugin.
**
** The SpellChecker Plugin is free software: you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public License as
** published by the Free Software Foundation, either version 3 of the
** License, or (at your option) any later version.
**
** The SpellChecker Plugin is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with the SpellChecker Plugin.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

#pragma once

#include "ProjectMistakesModel.h"

#include <coreplugin/inavigationwidgetfactory.h>
#include <utils/itemviews.h>

#include <QStyledItemDelegate>
#include <QTableView>

namespace Core {
class IEditor;
} // namespace Core

namespace SpellChecker {
namespace Internal {

class NavigationWidgetPrivate;
class NavigationWidgetFactoryPrivate;
/*! \brief The Navigation Widget class.
 *
 * The navigation widget shows all files in the project that has spelling
 * mistakes, as well as the number of mistakes for the given file.
 *
 * Clicking on the file will open the file in the editor. The Widget will
 * also show the current editor as the selected one in the list of files.
 *
 * For the design and implementation of this functionality the "Bookmarks"
 * and "Open Document" Navigation widgets was referenced.
 */
class NavigationWidget
  : public Utils::ListView
{
  Q_OBJECT
public:
  explicit NavigationWidget( ProjectMistakesModel* model );
  ~NavigationWidget();
public slots:
  void updateCurrentItem( Core::IEditor* editor );
private:
  NavigationWidgetPrivate* const d;
};
// --------------------------------------------------

/*! \brief The Navigation Widget Factory class.
 *
 * Factory to add the Spellchecker navigation widget to the list
 * of navigation widgets in Qt Creator.
 *
 * The factory also manages the actions to sort the files
 * in the navigation widgets created.
 */
class NavigationWidgetFactory
  : public Core::INavigationWidgetFactory
{
  Q_OBJECT
public:
  NavigationWidgetFactory( ProjectMistakesModel* model );
  ~NavigationWidgetFactory();
private slots:
  void sortingActionActivated( QAction* action );
private:
  Core::NavigationView createWidget();
  NavigationWidgetFactoryPrivate* const d;
};
// --------------------------------------------------

/*! \brief The Spelling Mistake Delegate class.
 *
 * Delegate to format the items in the Widget list.
 */
class SpellingMistakeDelegate
  : public QStyledItemDelegate
{
  Q_OBJECT
public:
  SpellingMistakeDelegate( QObject* parent = 0 );
private:
  void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
};
// --------------------------------------------------

} // namespace Internal
} // namespace SpellChecker
