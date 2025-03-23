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

#include "ProjectMistakesModel.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <QFileInfo>

using namespace SpellChecker::Internal;
using namespace SpellChecker;

using WordListInternalPair = QPair<SpellChecker::WordList, bool /* In Startup Project */>;
using FileMistakes         = QMap<QString, WordListInternalPair>;

class SpellChecker::Internal::ProjectMistakesModelPrivate
{
public:
  FileMistakes spellingMistakes;
  QList<QString> sortedKeys; /*!< This list contains the keys of the
                              * spellingMistakes map but sorted according
                              * to the selected \a column and \a order.
                              * This list is then used as the key for
                              * retrieving the data from the map to make sure
                              * that the views connected to the model gets the
                              * items in a sorted manner. */
  ProjectMistakesModel::Columns sortedColumn;
  Qt::SortOrder sortOrder;

  ProjectMistakesModelPrivate()
    : sortedColumn( ProjectMistakesModel::COLUMN_FILE )
    , sortOrder( Qt::AscendingOrder ) {}
};
// --------------------------------------------------
// --------------------------------------------------
// --------------------------------------------------

ProjectMistakesModel::ProjectMistakesModel()
  : d( new ProjectMistakesModelPrivate() )
{}
// --------------------------------------------------

ProjectMistakesModel::~ProjectMistakesModel()
{
  delete d;
}
// --------------------------------------------------

void ProjectMistakesModel::insertSpellingMistakes( const QString& fileName, const SpellChecker::WordList& words, bool inStartupProject )
{
  /* Check if the model already contains the file */
  FileMistakes::iterator file = d->spellingMistakes.find( fileName );

  if( ( words.isEmpty() == true )
      && ( file == d->spellingMistakes.end() ) ) {
    /* The file was never added, no need to do anything */
    return;
  }

  /* If there are no mistakes in the list of words, remove the file
   * from the model if it was found in the list of files. */
  if( ( words.isEmpty() == true )
      && ( file != d->spellingMistakes.end() ) ) {
    int idx = indexOfFile( fileName );
    Q_ASSERT( idx != -1 );
    beginRemoveRows( QModelIndex(), idx, idx );
    d->spellingMistakes.remove( fileName );
    d->sortedKeys.removeAll( fileName );
    endRemoveRows();
    return;
  }

  /* So there are misspelled words */
  if( file != d->spellingMistakes.end() ) {
    /* The file was added with mistakes before, check if there are a change in the
     * total number of items. */
    bool changed = ( file.value().first.count() != words.count() );
    /* Assign the words to the file */
    file.value().first = words;
    /* Notify of the change if there was one */
    if( changed == true ) {
      int idx = indexOfFile( fileName );
      Q_ASSERT( idx != -1 );
      emit dataChanged( index( idx, 0, QModelIndex() ), index( idx, columnCount( QModelIndex() ), QModelIndex() ) );
    }
  } else {
    /* Insert the mistakes for the file. The model is not notified of the
     * change here since the sort() function will in any case invalidate
     * all of the views and they will get refreshed after that. */
    d->spellingMistakes.insert( fileName, qMakePair( words, inStartupProject ) );
    d->sortedKeys.append( fileName );
    sort( static_cast<int>( d->sortedColumn ), d->sortOrder );
  }
}
// --------------------------------------------------

void ProjectMistakesModel::clearAllSpellingMistakes()
{
  beginResetModel();
  d->spellingMistakes.clear();
  d->sortedKeys.clear();
  endResetModel();
}
// --------------------------------------------------

SpellChecker::WordList ProjectMistakesModel::mistakesForFile( const QString& fileName ) const
{
  return d->spellingMistakes.value( fileName ).first;
}
// --------------------------------------------------

void ProjectMistakesModel::removeAllOccurrences( const QString& wordText )
{
  beginResetModel();
  FileMistakes::Iterator iter = d->spellingMistakes.begin();
  while( iter != d->spellingMistakes.end() ) {
    iter.value().first.remove( wordText );
    /* If there are no more words for the file, remove the file from the list */
    if( iter.value().first.isEmpty() == true ) {
      d->sortedKeys.removeAll( iter.key() );
      iter = d->spellingMistakes.erase( iter );
    } else {
      ++iter;
    }
  }
  endResetModel();
}
// --------------------------------------------------

int ProjectMistakesModel::countStringLiterals( const SpellChecker::WordList& words ) const
{
  /* Count how many of the words for the given file are in String Literals. */
  int count = std::count_if( words.constBegin(),
                             words.constEnd(),
                             []( const Word& word ) { return ( word.inComment == false ); } );
  return count;
}
// --------------------------------------------------

void ProjectMistakesModel::projectFilesChanged( QStringSet filesAdded, QStringSet filesRemoved )
{
  const auto mistakesEnd = d->spellingMistakes.end();

  {
    const QStringSet::const_iterator addedEnd = filesAdded.cend();
    for( auto addedIter = filesAdded.cbegin(); addedIter != addedEnd; ++addedIter ) {
      auto wordIter = d->spellingMistakes.find( *addedIter );
      if( wordIter != mistakesEnd ) {
        /* Found one */
        wordIter.value().second = true;
        const int32_t row       = indexOfFile( *addedIter );
        const QModelIndex idx   = index( row, COLUMN_FILE_IN_STARTUP );
        emit dataChanged( idx, idx );
      }
    }
  }

  {
    const QStringSet::const_iterator removedEnd = filesRemoved.cend();
    for( auto removedIter = filesRemoved.cbegin(); removedIter != removedEnd; ++removedIter ) {
      auto wordIter = d->spellingMistakes.find( *removedIter );
      if( wordIter != mistakesEnd ) {
        /* Found one */
        wordIter.value().second = false;
        const int32_t row       = indexOfFile( *removedIter );
        const QModelIndex idx   = index( row, COLUMN_FILE_IN_STARTUP );
        emit dataChanged( idx, idx );
      }
    }
  }
}
// --------------------------------------------------

void ProjectMistakesModel::fileSelected( const QModelIndex& index )
{
  QString fileName = index.data( COLUMN_FILEPATH ).toString();
  if( QFileInfo::exists( fileName ) == true ) {
    Core::IEditor* editor = Core::EditorManager::openEditor( Utils::FilePath::fromString(fileName) );
    emit editorOpened();
    Q_ASSERT( editor != nullptr );
    Q_ASSERT( d->spellingMistakes.value( fileName ).first.isEmpty() == false );
    /* Go to the first misspelled word in the editor. */
    const SpellChecker::WordList words = d->spellingMistakes.value( fileName ).first;
    Q_ASSERT( words.empty() == false );
    /* Get a word on the first line with a spelling mistake and
     * go to that line. This is to ensure that the highest up spelling
     * mistake is selected instead of a random mistake randomly in the
     * file. This seemed strange. */
    const Word word = *std::min_element( words.begin(), words.end(), []( const Word& lhs, const Word& rhs ) {
      return lhs.lineNumber < rhs.lineNumber;
    } );
    editor->gotoLine( int32_t( word.lineNumber ), int32_t( word.columnNumber - 1 ) );
  }
}
// --------------------------------------------------

QModelIndex ProjectMistakesModel::index( int row, int column, const QModelIndex& parent ) const
{
  if( parent.isValid() == true ) {
    return QModelIndex();
  } else {
    return createIndex( row, column );
  }
}
// --------------------------------------------------

QModelIndex ProjectMistakesModel::parent( const QModelIndex& child ) const
{
  Q_UNUSED( child );
  return QModelIndex();
}
// --------------------------------------------------

int ProjectMistakesModel::rowCount( const QModelIndex& parent ) const
{
  if( parent.isValid() == true ) {
    return 0;
  } else {
    return d->spellingMistakes.count();
  }
}
// --------------------------------------------------

int ProjectMistakesModel::columnCount( const QModelIndex& parent ) const
{
  if( parent.isValid() == true ) {
    return 0;
  } else {
    return COLUMN_COUNT - Qt::UserRole;
  }
}
// --------------------------------------------------

QVariant ProjectMistakesModel::data( const QModelIndex& index, int role ) const
{
  if( ( index.isValid() == false )
      || ( index.row() < 0 )
      || ( index.row() >= rowCount( QModelIndex() ) )
      || ( role <= Qt::DisplayRole ) ) {
    return QVariant();
  }

  QString file                     = d->sortedKeys.at( index.row() );
  FileMistakes::ConstIterator iter = d->spellingMistakes.find( file );
  if( iter == d->spellingMistakes.constEnd() ) {
    return QVariant();
  }

  switch( role ) {
    case COLUMN_FILE:
      return QFileInfo( iter.key() ).fileName();
    case COLUMN_MISTAKES_TOTAL:
      return ( iter.value().first.count() );
    case COLUMN_FILEPATH:
      return iter.key();
    case COLUMN_FILE_IN_STARTUP:
      return ( iter.value().second );
    case COLUMN_LITERAL_COUNT:
      return countStringLiterals( iter.value().first );
    case COLUMN_FILE_TYPE:
      return QFileInfo( iter.key() ).suffix();
    default:
      return QVariant();
  }
}
// --------------------------------------------------

int ProjectMistakesModel::indexOfFile( const QString& fileName ) const
{
  return d->sortedKeys.indexOf( fileName );
}
// --------------------------------------------------

void ProjectMistakesModel::sort( int column, Qt::SortOrder order )
{
  beginResetModel();
  d->sortedColumn = static_cast<Columns>( column );
  d->sortOrder    = order;
  std::sort( d->sortedKeys.begin(), d->sortedKeys.end(), [=]( const QString& stringLhs, const QString& strinRhs ) -> bool {
    FileMistakes::ConstIterator iterLhs = d->spellingMistakes.find( stringLhs );
    FileMistakes::ConstIterator iterRhs = d->spellingMistakes.find( strinRhs );
    if( ( iterLhs == d->spellingMistakes.constEnd() )
        || ( iterRhs == d->spellingMistakes.constEnd() ) ) {
      /* This should not be possible */
      Q_ASSERT( false );
      return false;
    }

    /* Check to see if both files are internal or external to the current project.
     * If the LHS is internal and RHS not, the internal one is always greater than
     * the external one. If they are both either internal or external, then sort
     * them on the requested column.
     * This ensures that the files are grouped together based on internal or external
     * and then sorted according to name. The external files will always be listed last. */
    if( iterLhs.value().second != iterRhs.value().second ) {
      return iterLhs.value().second;
    }
    bool greaterThan = false;

    switch( d->sortedColumn ) {
      case SpellChecker::Internal::ProjectMistakesModel::COLUMN_COUNT:
        break;
      case COLUMN_FILE:
        greaterThan = ( QFileInfo( iterLhs.key() ).fileName().toUpper() < QFileInfo( iterRhs.key() ).fileName().toUpper() );
        break;
      case COLUMN_MISTAKES_TOTAL:
        greaterThan = ( iterLhs.value().first.count() < iterRhs.value().first.count() );
        break;
      case COLUMN_FILEPATH:
        greaterThan = ( iterLhs.key().toUpper() < iterRhs.key().toUpper() );
        break;
      case COLUMN_FILE_IN_STARTUP:
        greaterThan = iterLhs.value().second;
        break;
      case COLUMN_LITERAL_COUNT: {
        int countLhs = countStringLiterals( iterLhs.value().first );
        int countRhs = countStringLiterals( iterRhs.value().first );
        greaterThan  = ( countLhs < countRhs );
        break;
      }
      case COLUMN_FILE_TYPE: {
        const QFileInfo infoRhs = QFileInfo( iterRhs.key() );
        const QFileInfo infoLhs = QFileInfo( iterLhs.key() );
        if( infoRhs.suffix().toUpper() == infoLhs.suffix().toUpper() ) {
          greaterThan = ( QFileInfo( iterLhs.key() ).fileName().toUpper() < QFileInfo( iterRhs.key() ).fileName().toUpper() );
        } else {
          greaterThan = ( infoLhs.suffix().toUpper() < infoRhs.suffix().toUpper() );
        }
        break;
      }
    }
    return ( order == Qt::AscendingOrder )
    ? greaterThan
    : !greaterThan;
  } );
  endResetModel();
}
// --------------------------------------------------
// --------------------------------------------------
// --------------------------------------------------
// --------------------------------------------------
