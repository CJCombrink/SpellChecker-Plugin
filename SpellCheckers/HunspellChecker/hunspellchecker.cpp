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
#include "HunspellConstants.h"
#include "hunspelloptionswidget.h"

#include "../../spellcheckerconstants.h"

#include <hunspell/hunspell.hxx>

#include <coreplugin/icore.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSharedPointer>
#include <QMutex>
#include <QTextCodec>

typedef QSharedPointer< ::Hunspell> HunspellPtr;

class SpellChecker::Checker::Hunspell::HunspellCheckerPrivate
{
public:
  HunspellPtr hunspell;
  QString dictionary;
  QString userDictionary;
  QMutex  mutex;
  QTextCodec* codec;

  HunspellCheckerPrivate()
    : hunspell( nullptr )
    , dictionary()
    , userDictionary()
    , codec()
  {}
  ~HunspellCheckerPrivate() {}

  /*! \brief Encode a word into the encoding of the selected dictionary.
   *
   * If the selected dictionary uses a different encoding than the one
   * that Qt Creator uses (UTF-8) then this function will encode the
   * word to the encoding of the dictionary, before it is spell checked by the
   * hunspell library.
   *
   * If the codec is not set or valid the word is converted to
   * its Latin-1 representation. */
  QByteArray encode( const QString& word )
  {
    if( codec != nullptr ) {
      return codec->fromUnicode( word );
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
  QString decode( const QByteArray& word )
  {
    if( codec != nullptr ) {
      return codec->toUnicode( word );
    }
    return QLatin1String( word );
  }
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
  /* Get the affix dictionary path */
  QString affPath = QString( d->dictionary ).replace( QRegExp( QLatin1String( "\\.dic$" ) ), QLatin1String( ".aff" ) );
  d->hunspell = HunspellPtr( new ::Hunspell( affPath.toLatin1(), d->dictionary.toLatin1() ) );
  d->codec = QTextCodec::codecForName( d->hunspell->get_dic_encoding() );
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
  d->dictionary = settings->value( QLatin1String( SpellCheckers::HunspellChecker::Constants::SETTING_DICTIONARY ), QLatin1String( "" ) ).toString();
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
  QMutexLocker lock( &d->mutex );
  HunspellPtr  hunspell = d->hunspell;
  bool recognised = hunspell->spell( d->encode( word ) );
  return ( recognised == false );
}
// --------------------------------------------------

void HunspellChecker::getSuggestionsForWord( const QString& word, QStringList& suggestionsList ) const
{
  QMutexLocker lock( &d->mutex );
  HunspellPtr  hunspell = d->hunspell;
  char** suggestions;
  int numSuggestions = hunspell->suggest( &suggestions, d->encode( word ) );
  suggestionsList.reserve( numSuggestions );
  for( int i = 0; i < numSuggestions; ++i ) {
    suggestionsList << d->decode( suggestions[i] );
  }
  hunspell->free_list( &suggestions, numSuggestions );
  return;
}
// --------------------------------------------------

bool HunspellChecker::addWord( const QString& word )
{
  QMutexLocker lock( &d->mutex );
  HunspellPtr  hunspell = d->hunspell;
  /* Save the word to the user dictionary */
  if( d->userDictionary.isEmpty() == true ) {
    qDebug() << "User dictionary name empty";
    return false;
  }

  QFileInfo( d->userDictionary ).dir().mkpath( "." );
  QFile dictionary( d->userDictionary );
  if( dictionary.open( QIODevice::Append ) == false ) {
    qDebug() << "Could not open user dictionary file: " << d->userDictionary;
    return false;
  }
  /* Only add the word to the spellchecker if the previous checkers passed. */
  hunspell->add( d->encode( word ).constData() );

  QTextStream stream( &dictionary );
  stream << word << endl;
  dictionary.close();
  return true;
}
// --------------------------------------------------

bool HunspellChecker::ignoreWord( const QString& word )
{
  QMutexLocker lock( &d->mutex );
  HunspellPtr  hunspell = d->hunspell;
  hunspell->add( d->encode( word ).constData() );
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
