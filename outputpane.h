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

#include "Word.h"

#include <coreplugin/ioutputpane.h>

#include <QItemDelegate>
#include <QStyledItemDelegate>
#include <QTreeView>

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

namespace SpellChecker {
namespace Internal {

class SpellingMistakesModel;

class OutputPanePrivate;
class OutputPane
  : public Core::IOutputPane
{
  Q_OBJECT
public:
  OutputPane( SpellingMistakesModel* model, QObject* parent = 0 );
  ~OutputPane();

  QWidget* outputWidget( QWidget* parent );
  QList<QWidget*> toolBarWidgets() const;
  QString displayName() const;
  int priorityInStatusBar() const;
  void clearContents();
  void visibilityChanged( bool visible );
  void setFocus();
  bool hasFocus() const;
  bool canFocus() const;
  bool canNavigate() const;
  bool canNext() const;
  bool canPrevious() const;
  void goToNext();
  void goToPrev();
private:
  /*! \brief Load the column sizes from the application settings.
   *
   * If there are no current values saved in the settings file, default values
   * will be used. */
  void loadColumnSizes();
  /*! \brief Save the current column sizes of the table to the settings page.
  *
  * This is done so that the column sizes are kept between runs of the application.
  * The settings are not managed by the normal core settings page since they
  * are not configurable and does not need a page. Instead the current state is
  * saved when the application is closed and loaded again on the next run. */
  void saveColumnSizes();
signals:
  void selectionChanged( const QModelIndex& index, const SpellChecker::Word& word );
private slots:
  /*! \brief Slot called by the OutputPaneDelegate when a suggestion button is pressed
   * for the current misspelled word.
   *
   * \param[in] index The index is used to update the selection to go to the next index in the model. This
   * allows the user to replace words one after the other without much interaction with
   * other buttons or the editor.
   * \param[in] word The word that must be replaced.
   * \param[in] suggestion Suggestion that was selected by the user. */
  void replaceWord( const QModelIndex& index, const SpellChecker::Word& word, const QString& suggestion );
  /*! \brief Slot called when the mistakes in the model gets updated.
   *
   * This slot is used to update the badge number to indicate the number of mistakes and to
   * reset the selection if a replacement was made from the output pane. */
  void modelMistakesUpdated();
  /*! \brief Slot called when a mistake in the output pane is selected.
   *
   * This slot will cause the cursor in the current editor to jump to the location
   * of the selected spelling mistake.
   *
   * This function will also get called when the user presses the next and previous buttons. */
  void mistakeSelected( const QModelIndex& index );
  /*! \brief Slot called when the cursor moves over a spelling mistake.
   *
   * If the cursor in the editor is over a mistake, then the item in the
   * output pane will be selected. */
  void wordUnderCursorMistake( bool isMistake, const SpellChecker::Word& word );
private:
  OutputPanePrivate* const d;
};

class OutputPaneDelegatePrivate;
/*! \brief The output pane delegate.
 *
 * This class is responsible for drawing a list of buttons in the tree view of the output
 * pane for a selected spelling mistake. Each button contains a possible suggestion for the
 * current misspelled word under the cursor. When a user presses the button, the word under the
 * cursor will be replaced with the selected suggestion. */
class OutputPaneDelegate
  : public QItemDelegate
{
  Q_OBJECT
public:
  OutputPaneDelegate( QTreeView* parent );
  virtual ~OutputPaneDelegate();
private:
  void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
  QWidget* createEditor( QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index ) const Q_DECL_OVERRIDE;
  void updateEditorGeometry( QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
  void setEditorData( QWidget* editor, const QModelIndex& index ) const Q_DECL_OVERRIDE;
  void setModelData( QWidget* editor, QAbstractItemModel* model, const QModelIndex& index ) const Q_DECL_OVERRIDE;
signals:
  /*! \brief Signal emitted when the user clicks on one of the suggestions buttons.
   *
   * \param index Current index in the model that the word is in.
   * \param word The misspelled word.
   * \param suggestion The selected suggestion that the \a word must be replaced with. */
  void replaceWord( const QModelIndex& index, const SpellChecker::Word& word, const QString suggestion ) const;
public slots:
  /*! \brief Slot called when a row is selected.
   *
   * This slot will cause the row to be updated and the buttons to be drawn. If buttons are
   * drawn for the previous selected row, those buttons will be hidden as well. */
  void rowSelected( const QModelIndex& index, const SpellChecker::Word& word );
private:
  OutputPaneDelegatePrivate* const d;
};

} // namespace Internal
} // namespace SpellChecker
