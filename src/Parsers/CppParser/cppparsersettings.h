/**************************************************************************
 *
 * Copyright (c) 2014 Carel Combrink
 *
 * This file is part of the SpellChecker Plugin, a Qt Creator plugin.
 *
 * The SpellChecker Plugin is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * The SpellChecker Plugin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the SpellChecker Plugin.  If not, see <http://www.gnu.org/licenses/>.
 ***************************************************************************/

#pragma once

#include <utils/qtcsettings.h>

#include <QObject>

namespace SpellChecker {
namespace CppSpellChecker {
namespace Internal {

class CppParserSettings
  : public QObject
{
  Q_OBJECT
  Q_ENUMS( WhatToCheck CommentsToCheck WordsWithNumbersOption WordsWithUnderscoresOption CamelCaseWordOption WordsWithDotsOption )
  Q_FLAGS( WhatToCheckOptions CommentsToCheckOptions )
public:
  CppParserSettings();
  CppParserSettings( const CppParserSettings& settings );
  ~CppParserSettings();

  enum WhatToCheck {
    Tokens_NONE         = 0,        /*!< Invalid option needed for QFLAGS. */
    CheckComments       = 1 << 0,   /*!< Check Comments. */
    CheckStringLiterals = 1 << 1,   /*!< Check String Literals */
  };
  Q_DECLARE_FLAGS( WhatToCheckOptions, WhatToCheck )

  enum CommentsToCheck {
    Comments_NONE = 0,              /*!< Invalid option needed for QFLAGS. */
    CommentsC     = 1 << 0,         /*!< Check C Style comments. */
    CommentsCpp   = 1 << 1,         /*!< Check C++ Style comments. */
    CommentsBoth  = CommentsC | CommentsCpp
  };
  Q_DECLARE_FLAGS( CommentsToCheckOptions, CommentsToCheck )

  enum WordsWithNumbersOption {
    RemoveWordsWithNumbers = 0,     /*!< Words containing numbers will not be checked and will be
                                     * removed from words that should be checked, most spell
                                     * checkers discard words with numbers in. */
    SplitWordsOnNumbers    = 1,     /*!< Split words on numbers. Thus a word like 'some9word' will
                                     * result in 2 words, 'some' and 'word' that will get added to
                                     * the list of words to be checked. */
    LeaveWordsWithNumbers  = 2      /*!< Leave words with numbers as they are.
                                     * \note Some spell checkers will discard words with numbers in
                                     * without checking them. */
  };

  enum WordsWithUnderscoresOption {
    RemoveWordsWithUnderscores = 0, /*!< Words that contains underscores will be removed and will
                                     * not be checked by the spell checker. */
    SplitWordsOnUnderscores    = 1, /*!< Split words on underscores. A word like 'some_word' will
                                     * result in 2 words, 'some' and 'word' that will be added to
                                     * the words that will be checked. */
    LeaveWordsWithUnderscores  = 2  /*!< Leave words with underscores so that the spell checker can
                                     * check them. This option does not make much sense since there
                                     * are no words with underscores in the English language. This
                                     * will be for perhaps some custom words added to a dictionary.
                                     * */
  };

  enum CamelCaseWordOption {
    RemoveWordsInCamelCase = 0,     /*!< Remove and do not check words in CamelCase. Since most
                                     * words in camelCase would refer to functions or data in the
                                     * code this can reduce the checks against such words. This
                                     * option takes precedence over the option to check words that
                                     * occur in the source. */
    SplitWordsOnCamelCase  = 1,     /*!< Words are split on camelCase. Thus a word like 'camelCase'
                                     * will result in 2 words, 'camel' and 'Case' that will get
                                     * added to the the list of words that will be checked. */
    LeaveWordsInCamelCase  = 2      /*!< Leave words in camelCase so that the spell checked can
                                     * check them. This option will in most cases only make sense if
                                     * the option to check words against those in source is is set.
                                     * */
  };

  enum WordsWithDotsOption {
    RemoveWordsWithDots = 0,        /*!< Remove and do not check words with dots. */
    SplitWordsOnDots    = 1,        /*!< Words are split on dots. */
    LeaveWordsWithDots  = 2         /*!< Leave words that contain dots. */
  };

  WhatToCheckOptions whatToCheck;         /*!< Flag for the setting about what to check in the
                                           * document. This includes Comments, String Literals or
                                           * both. */
  CommentsToCheckOptions commentsToCheck; /*!< Flag for the settings about what comments to check if
                                           * the \a whatToCheck setting includes comments. If
                                           * comments are not getting checked, this setting does not
                                           * apply. Doxygen comments will always be checked no
                                           * matter what the setting.
                                           * \todo Change the options so that all combinations are
                                           * selectable, C, Cpp and Doxygen. */
  bool checkQtKeywords;                   /*!< Qt keywords are words in the form where the first
                                           * letter is a caps 'Q' followed by a capital letter.
                                           * Keywords can also start with 'Q_' or words like 'emit',
                                           * 'slot', etc. If this is false, such words will be
                                           * removed from the words to be checked. */
  bool checkAllCapsWords;                 /*!< All caps words are words where the following is
                                           * true:
                                           *   word == word.toUpper().
                                           * If this is false, such words will be removed from the
                                           * words to be checked. This setting takes precedence over
                                           * words with underscores. Thus a word in all caps that
                                           * contains numbers or underscores will be removed if this
                                           * setting is false, even if the \a wordsWithNumberOption
                                           * or \a wordsWithUnderscoresOption would cause such words
                                           * to be checked. */
  WordsWithNumbersOption wordsWithNumberOption;
  WordsWithUnderscoresOption wordsWithUnderscoresOption;
  CamelCaseWordOption camelCaseWordOption;
  bool removeWordsThatAppearInSource;
  bool removeEmailAddresses;
  WordsWithDotsOption wordsWithDotsOption;
  bool removeWebsites;                    /*!< The words with websites uses a very zealous regular
                                           * expression that can remove a lot of actual errors in
                                           * the code.
                                           * For this reason it will be false by default. */
  bool removeFirstComment;                /*!< Remove the first comment in a file. This option is
                                           * useful to prevent parsing the first comment in a file.
                                           * Most often the very first comment in the file is a
                                           * license header, and should be the same for each file.
                                           * If this comment is ignored, a lot of processing is
                                           * skipped.
                                           * Also this can remove more false positives like names
                                           * and websites.
                                           * Doxygen comments that are the first comment in a file
                                           * will not be ignored. This is to handle pure doxygen
                                           * docs files that might start without a file header. */

  void loadFromSettings(Utils::QtcSettings* settings);
  void saveToSetting(Utils::QtcSettings* settings) const;

  CppParserSettings& operator=( const CppParserSettings& other );
  bool operator==( const CppParserSettings& other ) const;

signals:
  void settingsChanged();

public slots:

protected:
  void setDefaults();
};

} // namespace Internal
} // namespace CppSpellChecker
} // namespace SpellChecker

Q_DECLARE_OPERATORS_FOR_FLAGS( SpellChecker::CppSpellChecker::Internal::CppParserSettings::WhatToCheckOptions )
Q_DECLARE_OPERATORS_FOR_FLAGS( SpellChecker::CppSpellChecker::Internal::CppParserSettings::CommentsToCheckOptions )
