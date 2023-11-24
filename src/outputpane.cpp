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

#include "outputpane.h"
#include "spellcheckerconstants.h"
#include "spellcheckercore.h"
#include "spellingmistakesmodel.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <utils/utilsicons.h>
#include <utils/qtcsettings.h>

#include <QApplication>
#include <QCheckBox>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QSpacerItem>
#include <QToolButton>
#include <QTreeView>

class SpellChecker::Internal::OutputPanePrivate
{
public:
  SpellingMistakesModel* model;
  QTreeView* treeView;
  QList<QWidget*> toolbarWidgets;
  QToolButton* buttonIgnore;
  QToolButton* buttonAdd;
  QToolButton* buttonSuggest;
  QToolButton* buttonLucky;
  QToolButton* buttonLiterals;
  OutputPaneDelegate* delegate;
  QModelIndex indexAfterReplace;

  OutputPanePrivate() {}
};
// --------------------------------------------------
// --------------------------------------------------
// --------------------------------------------------

using namespace SpellChecker;
using namespace SpellChecker::Internal;

OutputPane::OutputPane( SpellingMistakesModel* model, QObject* parent )
  : IOutputPane( parent )
  , d( new OutputPanePrivate() )
{
  setId("SpellingMistakes");
  setDisplayName(tr( Constants::OUTPUT_PANE_TITLE ));
  setPriorityInStatusBar(1);

  Q_ASSERT( model != nullptr );
  d->model = model;

  /* Create the tree view for the model */
  d->treeView = new QTreeView();
  d->treeView->setRootIsDecorated( false );
  d->treeView->setFrameStyle( QFrame::NoFrame );
  d->treeView->setSortingEnabled( true );
  d->treeView->setModel( d->model );
  d->treeView->setAttribute( Qt::WA_MacShowFocusRect, false );

  d->delegate = new OutputPaneDelegate( d->treeView );
  d->treeView->setItemDelegate( d->delegate );
  d->treeView->setUniformRowHeights( true );
  connect( this,        &OutputPane::selectionChanged,    d->delegate, &OutputPaneDelegate::rowSelected );
  connect( d->delegate, &OutputPaneDelegate::replaceWord, this,        &OutputPane::replaceWord );

  QHeaderView* header                = d->treeView->header();
  QHeaderView::ResizeMode resizeMode = QHeaderView::Interactive;
  header->setSectionHidden( Constants::MISTAKE_COLUMN_IDX, true );
  header->setSectionResizeMode( Constants::MISTAKE_COLUMN_WORD,        resizeMode );
  header->setSectionResizeMode( Constants::MISTAKE_COLUMN_SUGGESTIONS, QHeaderView::Stretch );
  header->setSectionResizeMode( Constants::MISTAKE_COLUMN_LITERAL,     resizeMode );
  header->setSectionResizeMode( Constants::MISTAKE_COLUMN_LINE,        resizeMode );
  header->setSectionResizeMode( Constants::MISTAKE_COLUMN_COLUMN,      resizeMode );
  header->setStretchLastSection( false );
  header->setSectionsMovable( false );
  loadColumnSizes();

  /* Create the toolbar buttons */
  d->buttonSuggest = new QToolButton();
  /* Create the icon for the suggestion ToolButton.
   * This code was copied directly from {QtC}/src/plugins/texteditor/refactoroverlay.cpp
   * An icon is created with the 2 parts with specific colors for each. */
  d->buttonSuggest->setIcon(
    Utils::Icon( { { ":/utils/images/lightbulbcap.png", Utils::Theme::PanelTextColorMid }, { ":/utils/images/lightbulb.png", Utils::Theme::IconsWarningColor } }
                 , Utils::Icon::Tint ).icon() );
  d->buttonSuggest->setText( tr( "Give Suggestions" ) );
  d->buttonSuggest->setToolTip( tr( "Give suggestions for the word" ) );
  d->toolbarWidgets.push_back( d->buttonSuggest );
  connect( d->buttonSuggest, &QAbstractButton::clicked, SpellCheckerCore::instance(), &SpellCheckerCore::giveSuggestionsForWordUnderCursor );

  d->buttonIgnore = new QToolButton();
  d->buttonIgnore->setIcon( Utils::Icons::MINUS_TOOLBAR.icon() );
  d->buttonIgnore->setToolTip( tr( "Ignore the word" ) );
  d->toolbarWidgets.push_back( d->buttonIgnore );
  connect( d->buttonIgnore, &QAbstractButton::clicked, SpellCheckerCore::instance(), &SpellCheckerCore::ignoreWordUnderCursor );

  d->buttonAdd = new QToolButton();
  d->buttonAdd->setIcon( Utils::Icons::PLUS_TOOLBAR.icon() );
  d->buttonAdd->setToolTip( tr( "Add the word to the user dictionary" ) );
  d->toolbarWidgets.push_back( d->buttonAdd );
  connect( d->buttonAdd, &QAbstractButton::clicked, SpellCheckerCore::instance(), &SpellCheckerCore::addWordUnderCursor );

  d->buttonLucky = new QToolButton();
  d->buttonLucky->setIcon( QIcon( QLatin1String( Constants::ICON_OUTPUTPANE_LUCKY_BUTTON ) ) );
  d->buttonLucky->setText( tr( "Feeling Lucky" ) );
  d->buttonLucky->setToolTip( tr( "Replace the word with the first suggestion" ) );
  d->toolbarWidgets.push_back( d->buttonLucky );
  connect( d->buttonLucky, &QAbstractButton::clicked, SpellCheckerCore::instance(), &SpellCheckerCore::replaceWordUnderCursorFirstSuggestion );

  SpellCheckerCore* core = SpellCheckerCore::instance();
  connect( core, &SpellCheckerCore::wordUnderCursorMistake, this, &OutputPane::wordUnderCursorMistake, Qt::DirectConnection );

  connect( d->model,    &SpellingMistakesModel::mistakesUpdated, this, &OutputPane::modelMistakesUpdated );
  connect( d->treeView, &QAbstractItemView::clicked,             this, &OutputPane::mistakeSelected );
}
// --------------------------------------------------

OutputPane::~OutputPane()
{
  saveColumnSizes();

  delete d->treeView;
  for( QWidget* widget: d->toolbarWidgets ) {
    delete widget;
  }

  d->model = nullptr;
  delete d;
}
// --------------------------------------------------

QWidget* OutputPane::outputWidget( QWidget* parent )
{
  Q_UNUSED( parent );
  return d->treeView;
}
// --------------------------------------------------

QList<QWidget*> OutputPane::toolBarWidgets() const
{
  return d->toolbarWidgets;
}
// --------------------------------------------------

void OutputPane::clearContents()
{}
// --------------------------------------------------

void OutputPane::visibilityChanged( bool visible )
{
  Q_UNUSED( visible );
}
// --------------------------------------------------

void OutputPane::setFocus()
{
  d->treeView->setFocus();
}
// --------------------------------------------------

bool OutputPane::hasFocus() const
{
  return d->treeView->hasFocus();
}
// --------------------------------------------------

bool OutputPane::canFocus() const
{
  return true;
}
// --------------------------------------------------

bool OutputPane::canNavigate() const
{
  return true;
}
// --------------------------------------------------

bool OutputPane::canNext() const
{
  return d->treeView->model()->rowCount() > 1;
}
// --------------------------------------------------

bool OutputPane::canPrevious() const
{
  return d->treeView->model()->rowCount() > 1;
}
// --------------------------------------------------

void OutputPane::goToNext()
{
  QModelIndex currentIndex;
  QModelIndexList currentSelectedList = d->treeView->selectionModel()->selectedIndexes();
  if( currentSelectedList.isEmpty() == false ) {
    currentIndex = currentSelectedList.first();
  }
  QModelIndex nextIndex = d->treeView->indexBelow( currentIndex );
  if( nextIndex.isValid() == false ) {
    nextIndex = d->treeView->model()->index( 0, 0 );
  }

  d->treeView->selectionModel()->select( nextIndex, QItemSelectionModel::SelectCurrent );
  mistakeSelected( nextIndex );
}
// --------------------------------------------------

void OutputPane::goToPrev()
{
  QModelIndex currentIndex;
  QModelIndexList currentSelectedList = d->treeView->selectionModel()->selectedIndexes();
  if( currentSelectedList.isEmpty() == false ) {
    currentIndex = currentSelectedList.first();
  }

  QModelIndex previousIndex = d->treeView->indexAbove( currentIndex );
  if( previousIndex.isValid() == false ) {
    previousIndex = d->treeView->model()->index( d->treeView->model()->rowCount() - 1, 0 );
  }

  d->treeView->selectionModel()->select( previousIndex, QItemSelectionModel::SelectCurrent );
  mistakeSelected( previousIndex );
}
// ----------------------------------------------------------------------------

void OutputPane::loadColumnSizes()
{
  /* Sensible default values for the column sizes. */
  int colWord    = 200;
  int colLiteral = 45;
  int colLine    = 40;
  int colColumn  = 40;

  Utils::QtcSettings* settings = Core::ICore::settings();
  settings->beginGroup( Constants::CORE_SETTINGS_GROUP );
  settings->beginGroup( Constants::CORE_SETTINGS_OP_GROUP );
  colWord    = settings->value( Constants::SETTINGS_OUTPUT_PANE_COL_WORD, colWord ).toInt();
  colLiteral = settings->value( Constants::SETTINGS_OUTPUT_PANE_COL_LITERAL, colLiteral ).toInt();
  colLine    = settings->value( Constants::SETTINGS_OUTPUT_PANE_COL_LINE, colLine ).toInt();
  colColumn  = settings->value( Constants::SETTINGS_OUTPUT_PANE_COL_COLUMN, colColumn ).toInt();
  settings->endGroup(); /* CORE_SETTINGS_OP_GROUP */
  settings->endGroup(); /* CORE_SETTINGS_GROUP */

  d->treeView->setColumnWidth( Constants::MISTAKE_COLUMN_WORD,    colWord );
  d->treeView->setColumnWidth( Constants::MISTAKE_COLUMN_LITERAL, colLiteral );
  d->treeView->setColumnWidth( Constants::MISTAKE_COLUMN_LINE,    colLine );
  d->treeView->setColumnWidth( Constants::MISTAKE_COLUMN_COLUMN,  colColumn );
}
// ----------------------------------------------------------------------------

void OutputPane::saveColumnSizes()
{
  /* Save the table sizes to the settings file. */
  Utils::QtcSettings* settings = Core::ICore::settings();
  settings->beginGroup( Constants::CORE_SETTINGS_GROUP );
  settings->beginGroup( Constants::CORE_SETTINGS_OP_GROUP );
  settings->setValue( Constants::SETTINGS_OUTPUT_PANE_COL_WORD,    d->treeView->columnWidth( Constants::MISTAKE_COLUMN_WORD ) );
  settings->setValue( Constants::SETTINGS_OUTPUT_PANE_COL_LITERAL, d->treeView->columnWidth( Constants::MISTAKE_COLUMN_LITERAL ) );
  settings->setValue( Constants::SETTINGS_OUTPUT_PANE_COL_LINE,    d->treeView->columnWidth( Constants::MISTAKE_COLUMN_LINE ) );
  settings->setValue( Constants::SETTINGS_OUTPUT_PANE_COL_COLUMN,  d->treeView->columnWidth( Constants::MISTAKE_COLUMN_COLUMN ) );
  settings->endGroup(); /* CORE_SETTINGS_OP_GROUP */
  settings->endGroup(); /* CORE_SETTINGS_GROUP */
  settings->sync();
}
// --------------------------------------------------

void OutputPane::replaceWord( const QModelIndex& index, const Word& word, const QString& suggestion )
{
  /* Keep track of the index that was selected before the replace is made.
   * This index is then used when the model is updated to select that index.
   * This is needed to implement the feature to go to the next mistake in the
   * list when one is replaced.
   * If the last item is replaced, the next index must go to the first item. */
  if( index.row() == ( d->treeView->model()->rowCount() - 1 ) ) {
    d->indexAfterReplace = d->treeView->model()->index( 0, index.column() );
  } else {
    d->indexAfterReplace = index;
  }
  WordList words;
  words.append( word );
  /* Replace the word which will result in the model getting updated/ */
  SpellCheckerCore::instance()->replaceWordsInCurrentEditor( words, suggestion );
}
// --------------------------------------------------

void OutputPane::modelMistakesUpdated()
{
  IOutputPane::navigateStateUpdate();
  setBadgeNumber( d->model->rowCount() );
  /* Check if the index is valid so that that index can be selected in the view. */
  if( d->indexAfterReplace.isValid() == true ) {
    mistakeSelected( d->indexAfterReplace );
    /* Reset/invalidate the index */
    d->indexAfterReplace = QModelIndex();
  }
}
// --------------------------------------------------

void OutputPane::mistakeSelected( const QModelIndex& index )
{
  int row               = index.row();
  int line              = index.sibling( row, Constants::MISTAKE_COLUMN_LINE ).data().toInt();
  int column            = index.sibling( row, Constants::MISTAKE_COLUMN_COLUMN ).data().toInt();
  Core::IEditor* editor = Core::EditorManager::currentEditor();
  if( editor == nullptr ) {
    Q_ASSERT( editor != nullptr );
    return;
  }
  editor->gotoLine( line, column - 1 );
  Core::EditorManager::activateEditor( editor );
}
// --------------------------------------------------

void OutputPane::wordUnderCursorMistake( bool isMistake, const SpellChecker::Word& word )
{
  d->buttonSuggest->setEnabled( isMistake );
  d->buttonIgnore->setEnabled( isMistake );
  d->buttonAdd->setEnabled( isMistake );
  /* Do not set the button for Lucky enabled if there is no suggestions for the word. */
  d->buttonLucky->setEnabled( isMistake && ( word.suggestions.isEmpty() == false ) );
  /* Select the Word in the Output pane */
  QModelIndex index;
  if( isMistake == true ) {
    index = d->model->indexOfWord( word );
  }
  /* Select the index on the pane. If the word is not a spelling mistake,
   * the selection on the pane is cleared.
   * mistakeSelected() is not called since the cursor is already
   * at the selection and there is no need to jump to the word in the
   * editor, just update the selection on the pane. */
  d->treeView->selectionModel()->setCurrentIndex( index, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows );
  emit selectionChanged( index, word );
}
// --------------------------------------------------
// --------------------------------------------------
// --------------------------------------------------

class SpellChecker::Internal::OutputPaneDelegatePrivate
{
public:
  QTreeView* treeView;
  bool buttonsShown = false;
  int  selectedRow  = -1;
  QModelIndex editIndex;
  int  created = 0;
  Word wordSelected;
  const QString buttonStylesheet = QLatin1String( "QPushButton { "
                                                  "  border-radius: 3px;"
                                                  "  padding: 0px 2px;"
                                                  "  background-color: palette(button);"
                                                  "}"
                                                  "QPushButton:hover {"
                                                  "  border-style: solid; "
                                                  "  background-color: palette(dark);"
                                                  "}" );

  OutputPaneDelegatePrivate() {}
};
// --------------------------------------------------
// --------------------------------------------------
// --------------------------------------------------

OutputPaneDelegate::OutputPaneDelegate( QTreeView* parent )
  : QItemDelegate( parent )
  , d( new OutputPaneDelegatePrivate() )
{
  d->treeView = parent;
}
// --------------------------------------------------

OutputPaneDelegate::~OutputPaneDelegate()
{
  delete d;
}
// --------------------------------------------------

void OutputPaneDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
  if( ( index.column() == Constants::MISTAKE_COLUMN_SUGGESTIONS )
      && ( index.row() == d->selectedRow ) ) {
    /* Open the buttons for this row if it is the selected row and the column gets repainted. */
    d->editIndex = index;
    d->treeView->openPersistentEditor( index );
    d->buttonsShown = true;
    /* Paint invalid item, to ensure there is no text */
    QItemDelegate::paint( painter, option, QModelIndex() );
  } else {
    QItemDelegate::paint( painter, option, index );
  }
}
// --------------------------------------------------

QWidget* OutputPaneDelegate::createEditor( QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
  if( index.column() == Constants::MISTAKE_COLUMN_SUGGESTIONS ) {
    /* Create an editor widget that contains each suggestion as a button to replace the
     * misspelled word with the clicked button. */
    QWidget* widget     = new QWidget( parent );
    QHBoxLayout* layout = new QHBoxLayout();
    widget->setLayout( layout );
    layout->setContentsMargins(1, 1, 1, 1);
    layout->setSpacing( 3 );
    Word word               = d->wordSelected;
    QStringList suggestions = index.data().toString().split( QStringLiteral( ", " ) );
    for( const QString& suggestion: suggestions ) {
      QPushButton* button = new QPushButton();
      button->setStyleSheet( d->buttonStylesheet );
      button->setText( suggestion );
      button->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Minimum );
      button->setContentsMargins( 0, 0, 0, 0 );
      connect( button, &QPushButton::clicked, [this, word, suggestion, index]() {
        emit replaceWord( index, word, suggestion );
      } );
      layout->addWidget( button );
    }
    layout->addSpacerItem( new QSpacerItem( 0, 0, QSizePolicy::MinimumExpanding ) );
    return widget;
  } else {
    return QItemDelegate::createEditor( parent, option, index );
  }
}
// --------------------------------------------------

void OutputPaneDelegate::updateEditorGeometry( QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
  if( index.column() == Constants::MISTAKE_COLUMN_SUGGESTIONS ) {
    QRect rect = option.rect;
    rect.setWidth( qMax( rect.width(), editor->layout()->minimumSize().width() ) );
    editor->setGeometry( rect );
  } else {
    return QItemDelegate::updateEditorGeometry( editor, option, index );
  }
}
// --------------------------------------------------

void OutputPaneDelegate::setEditorData( QWidget* editor, const QModelIndex& index ) const
{
  Q_UNUSED( editor );
  Q_UNUSED( index );
}
// --------------------------------------------------

void OutputPaneDelegate::setModelData( QWidget* editor, QAbstractItemModel* model, const QModelIndex& index ) const
{
  Q_UNUSED( editor );
  Q_UNUSED( model );
  Q_UNUSED( index );
}
// --------------------------------------------------

void OutputPaneDelegate::rowSelected( const QModelIndex& index, const Word& word )
{
  /* First close the editor/buttons if they are shown on a previous row. */
  if( d->buttonsShown == true ) {
    d->buttonsShown = false;
    QModelIndex prevIndex = d->editIndex;
    d->editIndex   = QModelIndex();
    d->selectedRow = -1;
    d->treeView->closePersistentEditor( prevIndex );
  }
  if( index.isValid() == false ) {
    return;
  }
  d->selectedRow  = index.row();
  d->wordSelected = word;
}
// --------------------------------------------------
