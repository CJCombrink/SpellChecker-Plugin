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

#include "hunspellchecker.h"
#include "hunspelloptionswidget.h"
#include "HunspellConstants.h"

#include "../../spellcheckerconstants.h"

#include <hunspell/hunspell.hxx>

#include <coreplugin/icore.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QSharedPointer>
#include <QTextCodec>

#include <memory>

namespace {
/*! \brief Wrapper around Hunspell object */
class HunspellWrapper
{
public:
  /*! \brief Construct the wrapper and set up the hunspell object.
   *
   * The dictionary name (full path and name) is needed to set up the
   * hunspell object. From the supplied dictionary file, the associated
   * .aff file is derived, which is also needed by Hunspell and must be
   * co-located with the dictionary file. */
  HunspellWrapper( const QString& dictionary )
  {
    /* Get the affix dictionary path */
    QString affPath = QString( dictionary ).replace( QRegExp( QLatin1String( "\\.dic$" ) ), QLatin1String( ".aff" ) );
    d_hunspell = HunspellPtr( new ::Hunspell( affPath.toLatin1(), dictionary.toLatin1() ) );
    d_codec    = QTextCodec::codecForName( d_hunspell->get_dic_encoding() );
  }
  /*! \brief Check if the supplied \a word is a spelling mistake or not.
   *
   * A spelling mistake is a word that is not recognised by the Hunspell
   * object. */
  bool isSpellingMistake( const QString& word ) const
  {
    QMutexLocker lock( &d_mutex );
    HunspellPtr  hunspell = d_hunspell;
    bool recognised       = hunspell->spell( encode( word ) );
    return ( recognised == false );
  }
  /*! \brief Get the list of suggestions for the given word.
   *
   * It is assumed that the \a word is a spelling mistake, thus
   * this is not checked again. */
  QStringList getSuggestionsForWord( const QString& word ) const
  {
    QStringList suggestionsList;
    QMutexLocker lock( &d_mutex );
    HunspellPtr  hunspell = d_hunspell;
    char** suggestions;
    int numSuggestions = d_hunspell->suggest( &suggestions, encode( word ) );
    suggestionsList.reserve( numSuggestions );
    for( int i = 0; i < numSuggestions; ++i ) {
      suggestionsList << decode( suggestions[i] );
    }
    hunspell->free_list( &suggestions, numSuggestions );
    return suggestionsList;
  }
  /*! \brief Add the given word to the Hunspell object.
   *
   * A word that is added will not be considered a spelling mistake.
   * Words added to the object will only be remembered for the lifetime
   * of the object. To remember a word between runs, external functionality
   * must be used. */
  void addWord( const QString& word )
  {
    QMutexLocker lock( &d_mutex );
    HunspellPtr  hunspell = d_hunspell;
    d_hunspell->add( encode( word ).constData() );
  }

private:
  /*! \brief Encode a word into the encoding of the selected dictionary.
   *
   * If the selected dictionary uses a different encoding than the one
   * that Qt Creator uses (UTF-8) then this function will encode the
   * word to the encoding of the dictionary, before it is spell checked by the
   * hunspell library.
   *
   * If the codec is not set or valid the word is converted to
   * its Latin-1 representation. */
  QByteArray encode( const QString& word ) const
  {
    if( d_codec != nullptr ) {
      return d_codec->fromUnicode( word );
    }
    return word.toLatin1();
  }

  /*! \brief Decode a word from the encoding of the selected dictionary.
   *
   * If the selected dictionary uses a different encoding than the one
   * that Qt Creator uses (UTF-8) then this function will decode the
   * word returned by the hunspell library to Unicode.
   *
   * If the codec is not set or invalid the word is converted to
   * its Latin-1 representation. */
  QString decode( const QByteArray& word ) const
  {
    if( d_codec != nullptr ) {
      return d_codec->toUnicode( word );
    }
    return QLatin1String( word );
  }

private:
  using HunspellPtr = QSharedPointer< ::Hunspell>;
  HunspellPtr d_hunspell;
  QTextCodec* d_codec;
  mutable QMutex d_mutex;
};

} // namespace


class SpellChecker::Checker::Hunspell::HunspellCheckerPrivate
{
  using HunspellWrapperPtr = std::unique_ptr<HunspellWrapper>;
public:
  QString dictionary;
  QString userDictionary;
  QMutex  fileMutex;
  HunspellWrapperPtr hunspell;

  HunspellCheckerPrivate()
    : dictionary()
    , userDictionary()
  {}
  ~HunspellCheckerPrivate() {}
};
// --------------------------------------------------
// --------------------------------------------------
// --------------------------------------------------

using namespace SpellChecker::Checker::Hunspell;

HunspellChecker::HunspellChecker()
  : ISpellChecker()
  , d( new HunspellCheckerPrivate() )
{
  loadSettings();
  d->hunspell = std::make_unique<HunspellWrapper>( d->dictionary );
  loadUserAddedWords();
}
// --------------------------------------------------

HunspellChecker::~HunspellChecker()
{
  /* Codec not deleted since the destructor of QTextCodec is private */
  // delete d->codec;
  saveSettings();
  delete d;
}
// --------------------------------------------------

QString HunspellChecker::name() const
{
  return tr( "Hunspell" );
}
// --------------------------------------------------

void HunspellChecker::loadSettings()
{
  QSettings* settings = Core::ICore::settings();
  settings->beginGroup( QLatin1String( Constants::CORE_SETTINGS_GROUP ) );
  settings->beginGroup( QLatin1String( Constants::CORE_SPELLCHECKERS_GROUP ) );
  settings->beginGroup( QLatin1String( SpellCheckers::HunspellChecker::Constants::SETTINGS_GROUP ) );
  d->dictionary     = settings->value( QLatin1String( SpellCheckers::HunspellChecker::Constants::SETTING_DICTIONARY ), QLatin1String( "" ) ).toString();
  d->userDictionary = settings->value( QLatin1String( SpellCheckers::HunspellChecker::Constants::SETTING_USER_DICTIONARY ), QLatin1String( "" ) ).toString();
  settings->endGroup();
  settings->endGroup();
  settings->endGroup();
}
// --------------------------------------------------

void HunspellChecker::loadUserAddedWords()
{
  /* Save the word to the user dictionary */
  if( d->userDictionary.isEmpty() == true ) {
    qDebug() << "loadUserAddedWords: User dictionary name empty";
    return;
  }

  QFile dictionary( d->userDictionary );
  if( dictionary.open( QIODevice::ReadOnly ) == false ) {
    qDebug() << "loadUserAddedWords: Could not open user dictionary file: " << d->userDictionary;
    return;
  }

  QTextStream stream( &dictionary );
  QString word;
  while( stream.atEnd() != true ) {
    word = stream.readLine();
    ignoreWord( word );
  }
  dictionary.close();
}
// --------------------------------------------------

void HunspellChecker::saveSettings() const
{
  QSettings* settings = Core::ICore::settings();
  settings->beginGroup( QLatin1String( Constants::CORE_SETTINGS_GROUP ) );
  settings->beginGroup( QLatin1String( Constants::CORE_SPELLCHECKERS_GROUP ) );
  settings->beginGroup( QLatin1String( SpellCheckers::HunspellChecker::Constants::SETTINGS_GROUP ) );
  settings->setValue( QLatin1String( SpellCheckers::HunspellChecker::Constants::SETTING_DICTIONARY ),      d->dictionary );
  settings->setValue( QLatin1String( SpellCheckers::HunspellChecker::Constants::SETTING_USER_DICTIONARY ), d->userDictionary );
  settings->endGroup();
  settings->endGroup();
  settings->endGroup();
  settings->sync();
}
// --------------------------------------------------

bool HunspellChecker::isSpellingMistake( const QString& word ) const
{
  return d->hunspell->isSpellingMistake( word );
}
// --------------------------------------------------

void HunspellChecker::getSuggestionsForWord( const QString& word, QStringList& suggestionsList ) const
{
  suggestionsList = d->hunspell->getSuggestionsForWord( word );
}
// --------------------------------------------------

bool HunspellChecker::addWord( const QString& word )
{
  // HunspellPtr  hunspell = d->hunspell;*/
  /* Save the word to the user dictionary */
  if( d->userDictionary.isEmpty() == true ) {
    qDebug() << "User dictionary name empty";
    return false;
  }

  QMutexLocker lock( &d->fileMutex );
  QFileInfo( d->userDictionary ).dir().mkpath( "." );
  QFile dictionary( d->userDictionary );
  if( dictionary.open( QIODevice::Append ) == false ) {
    qDebug() << "Could not open user dictionary file: " << d->userDictionary;
    return false;
  }
  /* Only add the word to the spellchecker if the previous checks passed. */
  d->hunspell->addWord( word );

  QTextStream stream( &dictionary );
  stream << word << endl;
  dictionary.close();
  return true;
}
// --------------------------------------------------

bool HunspellChecker::ignoreWord( const QString& word )
{
  /* The word is only added for this run of the IDE.
   * For this reason it is not added to the file. */
  d->hunspell->addWord( word );
  return true;
}
// --------------------------------------------------

QWidget* HunspellChecker::optionsWidget()
{
  HunspellOptionsWidget* widget = new HunspellOptionsWidget( d->dictionary, d->userDictionary );
  connect( this,   &HunspellChecker::dictionaryChanged,           widget, &HunspellOptionsWidget::updateDictionary );
  connect( this,   &HunspellChecker::userDictionaryChanged,       widget, &HunspellOptionsWidget::updateUserDictionary );
  connect( widget, &HunspellOptionsWidget::dictionaryChanged,     this,   &HunspellChecker::updateDictionary );
  connect( widget, &HunspellOptionsWidget::userDictionaryChanged, this,   &HunspellChecker::updateUserDictionary );
  return widget;
}
// --------------------------------------------------

void HunspellChecker::updateDictionary( const QString& dictionary )
{
  if( d->dictionary != dictionary ) {
    d->dictionary = dictionary;
    emit dictionaryChanged( d->dictionary );
  }
}
// --------------------------------------------------

void HunspellChecker::updateUserDictionary( const QString& userDictionary )
{
  if( d->userDictionary != userDictionary ) {
    d->userDictionary = userDictionary;
    emit userDictionaryChanged( d->userDictionary );
  }
}
// --------------------------------------------------
