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

#include "NavigationWidget.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/idocument.h>
#include <utils/utilsicons.h>

#include <QHeaderView>
#include <QPainter>
#include <QToolButton>
#include <QActionGroup>
#include <QMenu>

using namespace SpellChecker::Internal;

#define ENUM_VAL_PROPERTY "enum.Value"

enum SortBy {
  SortByFileName = 0,
  SortByMistakes,
  SortByLiterals,
  SortByFileType
};
// --------------------------------------------------
// --------------------------------------------------
// --------------------------------------------------

SpellingMistakeDelegate::SpellingMistakeDelegate( QObject* parent )
  : QStyledItemDelegate( parent )
{}
// --------------------------------------------------

void SpellingMistakeDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
  QStyleOptionViewItem opt = option;
  initStyleOption( &opt, index );
  QStyledItemDelegate::paint( painter, option, index );
  QFontMetrics fm( opt.font );
  painter->save();

  int colLit   = opt.rect.right() - 4;
  int colMist  = colLit - 4 - fm.width( QStringLiteral( "\"000\"" ) );
  int textArea = colMist - 4 - fm.width( QStringLiteral( "000" ) );

  QString fileName = index.data( ProjectMistakesModel::COLUMN_FILE ).toString();
  /* Elide the text to make it fit into the available space. */
  fileName = fm.elidedText( fileName, Qt::ElideMiddle, textArea );
  QString nrMistakes = index.data( ProjectMistakesModel::COLUMN_MISTAKES_TOTAL ).toString();
  bool inStartupProject = index.data( ProjectMistakesModel::COLUMN_FILE_IN_STARTUP ).toBool();
  QString nrLiterals = index.data( ProjectMistakesModel::COLUMN_LITERAL_COUNT ).toString();
  nrLiterals.append( QLatin1String( "\"" ) );
  nrLiterals.prepend( QLatin1String( "\"" ) );
  if( inStartupProject == false ) {
    painter->setPen( Qt::lightGray );
  }

  /* Write the File Name */
  painter->drawText( 6, 2 + opt.rect.top() + fm.ascent(), fileName );
  /* Write the number of mistakes */
  painter->drawText( colMist - fm.width( nrMistakes ), 2 + opt.rect.top() + fm.ascent(), nrMistakes );
  /* Draw the number of String Literal Mistakes. */
  painter->setPen( Qt::darkGreen );
  painter->drawText( colLit - fm.width( nrLiterals ), 2 + opt.rect.top() + fm.ascent(), nrLiterals );
  painter->restore();
}
// --------------------------------------------------
// --------------------------------------------------
// --------------------------------------------------
// --------------------------------------------------

class SpellChecker::Internal::NavigationWidgetPrivate
{
public:
  NavigationWidgetPrivate() {}
  ProjectMistakesModel* model;
};
// --------------------------------------------------
// --------------------------------------------------
// --------------------------------------------------
NavigationWidget::NavigationWidget( ProjectMistakesModel* model )
  : d( new SpellChecker::Internal::NavigationWidgetPrivate() )
{
  d->model = model;
  setModel( model );
  setItemDelegate( new SpellingMistakeDelegate( this ) );
  setFrameStyle( QFrame::NoFrame );
  setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
  setFocusPolicy( Qt::NoFocus );
  setSelectionMode( QAbstractItemView::SingleSelection );
  setSelectionBehavior( QAbstractItemView::SelectRows );

  connect( Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged, this, &NavigationWidget::updateCurrentItem );
}
// --------------------------------------------------

NavigationWidget::~NavigationWidget()
{
  delete d;
}
// --------------------------------------------------

void NavigationWidget::updateCurrentItem( Core::IEditor* editor )
{
  if( editor == nullptr ) {
    clearSelection();
    return;
  }
  QString fileName = editor->document()->filePath().toString();
  int idx = d->model->indexOfFile( fileName );
  QModelIndex modelIndex = model()->index( idx, 0 );
  if( modelIndex.isValid() == false ) {
    clearSelection();
    return;
  }
  setCurrentIndex( modelIndex );
  selectionModel()->select( currentIndex(), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows );
  scrollTo( currentIndex() );
}
// --------------------------------------------------
// --------------------------------------------------
// --------------------------------------------------

class SpellChecker::Internal::NavigationWidgetFactoryPrivate
{
public:
  NavigationWidgetFactoryPrivate() {}
  ProjectMistakesModel* model;
  QAction* sortActionFileName;
  QAction* sortActionFileType;
  QAction* sortActionMistakes;
  QAction* sortActionLiterals;
};
// --------------------------------------------------
// --------------------------------------------------

NavigationWidgetFactory::NavigationWidgetFactory( ProjectMistakesModel* model )
  : INavigationWidgetFactory()
  , d( new NavigationWidgetFactoryPrivate() )
{
  setDisplayName( NavigationWidget::tr( "Spelling Mistakes" ) );
  setPriority( 600 );
  setId( "SpellingMistakes" );
  d->model = model;

  d->sortActionFileName = new QAction( tr( "Sort by File Name" ), this );
  d->sortActionFileName->setCheckable( true );
  d->sortActionFileName->setChecked( false );
  d->sortActionFileName->setProperty( ENUM_VAL_PROPERTY, SortByFileName );

  d->sortActionMistakes = new QAction( tr( "Sort by Total Mistakes" ), this );
  d->sortActionMistakes->setCheckable( true );
  d->sortActionMistakes->setChecked( false );
  d->sortActionMistakes->setProperty( ENUM_VAL_PROPERTY, SortByMistakes );

  d->sortActionLiterals = new QAction( tr( "Sort by String Literal Mistakes" ), this );
  d->sortActionLiterals->setCheckable( true );
  d->sortActionLiterals->setChecked( false );
  d->sortActionLiterals->setProperty( ENUM_VAL_PROPERTY, SortByLiterals );

  d->sortActionFileType = new QAction( tr( "Sort by File Type" ), this );
  d->sortActionFileType->setCheckable( true );
  d->sortActionFileType->setChecked( false );
  d->sortActionFileType->setProperty( ENUM_VAL_PROPERTY, SortByFileType );

  QActionGroup* actionGroup = new QActionGroup( this );
  actionGroup->setExclusive( true );
  actionGroup->addAction( d->sortActionFileName );
  actionGroup->addAction( d->sortActionMistakes );
  actionGroup->addAction( d->sortActionLiterals );
  actionGroup->addAction( d->sortActionFileType );
  connect( actionGroup, &QActionGroup::triggered, this, &NavigationWidgetFactory::sortingActionActivated );
  d->sortActionFileName->activate( QAction::Trigger );
}
// --------------------------------------------------

NavigationWidgetFactory::~NavigationWidgetFactory()
{
  delete d;
}
// --------------------------------------------------

void NavigationWidgetFactory::sortingActionActivated( QAction* action )
{
  SortBy sortOption = static_cast<SortBy>( action->property( ENUM_VAL_PROPERTY ).toInt() );
  /* Sort always the most logic way, the user does not have control over the order. */
  switch( sortOption ) {
    case SortByFileName:
      d->model->sort( ProjectMistakesModel::COLUMN_FILE, Qt::AscendingOrder );
      break;
    case SortByMistakes:
      d->model->sort( ProjectMistakesModel::COLUMN_MISTAKES_TOTAL, Qt::DescendingOrder );
      break;
    case SortByLiterals:
      d->model->sort( ProjectMistakesModel::COLUMN_LITERAL_COUNT, Qt::DescendingOrder );
      break;
    case SortByFileType:
      d->model->sort( ProjectMistakesModel::COLUMN_FILE_TYPE, Qt::AscendingOrder );
      break;
  }
}
// --------------------------------------------------

Core::NavigationView NavigationWidgetFactory::createWidget()
{
  NavigationWidget* widget = new NavigationWidget( d->model );
  connect( widget, &QAbstractItemView::clicked, d->model, &ProjectMistakesModel::fileSelected );
  Core::NavigationView view;
  view.widget = widget;

  QToolButton* sortButton = new QToolButton( widget );
  sortButton->setIcon( Utils::Icons::ARROW_DOWN.icon() );
  sortButton->setToolTip( tr( "Sort" ) );
  sortButton->setPopupMode( QToolButton::InstantPopup );

  QMenu* sortMenu = new QMenu( sortButton );
  sortMenu->addAction( d->sortActionFileName );
  sortMenu->addAction( d->sortActionMistakes );
  sortMenu->addAction( d->sortActionLiterals );
  sortMenu->addAction( d->sortActionFileType );
  sortButton->setMenu( sortMenu );

  view.dockToolBarWidgets << sortButton;
  return view;
}
// --------------------------------------------------
