<h1>SpellChecker Plugin</h1>
===================

<h2>1. Introduction</h2>
The SpellChecker Plugin is a spellchecker plugin for the Qt Creator IDE. 
This plugin spell checks comments in source files for spelling mistakes and suggestt the correct spelling for misspelled words, if possible. <br>
Currently the plugin only checks C++ files and uses the Hunspell Spell Checker to check words for spelling mistakes. 
The plugin provides an options page in Qt Creator that can be used to configure the parsers as well as the spell checkers available. <br><br>
To build the plugin self, see section 2. <br>
To download a pre-built version of the plugin, refer to section 4. <br><br>
The motivation for this plugin is that my spelling is terrible and I was looking for a plugin that could spell check my Doxygen comments. I feel that good comments are essential to any piece of software. I could not find one suitable to my needs so I decided to challenge myself to see if I can implement one myself. In one of my courses on varsity I investigated the Qt Creator source code and was amazed with the whole plugin system, thus it also inspired me to try and
contribute to such a project. <br><br>
I did look a lot at the code in the TODO plugin as a basis for my own implementation. I do not think that I have violated any licences doing so since I have never just copied code from the plugin. If there are any places that can be problematic regarding any licences, please contact me so that I can resolve the issues. I want to contribute to the Open Source Community, but it is not my intention to step on any toes in the process. <br>
<h2>2. Building The Plugin</h2>
As is, the plugin requires the Qt Creator source files as well as the Hunspell spell checker library. The next section describes how to get and compile Hunspell. <br>
It is assumed that the Qt Creator source files are already downloaded to the system. <br>
For now all steps are done on Windows using MinGW, but as time and testing continues the steps and tests will include other compilers and operating systems. <br>
Qt 5.3.0 was used along with Qt Creator 3.1
<h3>2.1 Setting up Hunspell 1.3.2</h3>
[Hunspell](http://hunspell.sourceforge.net/) is an Open Source spell checker used in many Open Source applications. <br>
<h4>2.1.1 MinGW</h4>
For MinGW the hunspell-mingw repository on github was used to build hunspell. This was the easiest solution to get the lib ready for MinGW. <br>
  1. Get the sources
    - Download the sources using the zip archive or git from https://github.com/zdenop/hunspell-mingw
  2. Build the sources using the steps given on the above page
    - In a command window, navigate to the folder where the sources are located and run qmake and then run mingw32-make

<h4>2.1.2 MSVC</h4>
  1. Get the sources
    - Download the sources for 1.3.2 from http://sourceforge.net/projects/hunspell/files/Hunspell/1.3.2/
	2. Build the sources
    - Open the hunspell.sln file in the folder src\win_api using Visual Studio
    - Build the debug\_dll and release\_dll of the libhunspell project

<h4>2.1.3 GCC (Linux)</h4>
On Linux, make sure that libhunspell-1.3.* and libhunspell-dev is installed. This should be all that is needed regarding Hunspell. 

<h3>2.2 Building The Plugin</h3>
The following steps can be used to build the plugin for a local build of Qt Creator. If the idea is to use the plugin with one of the release versions of Qt Creator, make sure that the compiler and Qt Creator sources match exactly with the setup for the release version of Qt Creator. If this is not possible, the plugin will not be usable with a Qt Creator release version. 
  1. Get the Sources
    - Download the sources from the github project page
  2. Set up the paths to the required locations
    - Rename the spellchecker_local_paths.pri.example to spellchecker_local_paths.pri
    - Update the renamed file and make sure that all of the variables point to the correct locations. Read the comments in the example for more information
  3. Build the plugin
    - Open the pro file of the plugin in Qt Creator
    - Build the plugin. If Hunspell was build correctly and the local paths was set up correctly, the plugin should build correctly.
  4. Run Qt Creator
    - Make sure that the correct Hunspell DLL can be obtained by either adding the path to it to the system path or by copying the correct dll to the Qt Creator folder.
    - Run Qt Creator. If everything was done correctly, Qt Creator should open and load the plugin without any problems.
  5. Verify the plugin has loaded
    - In the running Qt Creator, go to "*Help*" -> "*About Plugins...*". Under Utilities "*SpellChecker*" should be listed and enabled (enable it if it was not enabled).

<h2>Using The Plugin</h2>
After opening Qt Creator and the plugin loaded successfully the following steps must be performed before starting to use the SpellChecker plugin:
  1. Make sure the plugin is enabled
    - Go to "*Help*" -> "*About Plugins...*" and make sure the SpellChecker plugin under Utilities is enabled. 
  2. Set the Spell Checker
    - Go to "*Tools*" -> "*Options...*" 
    - In the Options page, go to the "*Spell Checker*" options page.
    - On the "*SpellChecker*" tab, select the required Spell Checker in the dropdown box. 
      Currently only the Hunspell Spell Checker will be available, but perhaps in future more might be added. 
    - Set the "*Dictionary*" and "*User Dictionary*" paths for the spell checker to use. <br>
      Dictionaries can be downloaded from http://archive.services.openoffice.org/pub/mirror/OpenOffice.org/contrib/dictionaries/ <br>
      The *User Dictionary* is a custom file used to store words added to the dictionary of the spell checker. If such a file does not exist, the plugin will create the file for this purpose. The plugin will attempt to create this file in the User Resource path. On Windows this is in %APPDATA%\QtProject\ and on Linux this is in ~/.config/QtProject/. 
    - After setting the Spell Checker, restart Qt Creator.
  3. Set the Parser Options
    - For the different available parsers there will be tabs with the settings for the parser. 
      Currently there is only one parser available, a C++ Parser. Change its settings according to what is required. For more information
      regarding this parser, refer to section 5. 
  4. Get commenting
    - While typing comments or opening a C++ text editor, the plugin will parse the comments and check words. If a spelling mistake is made the spell checker will add it to the output pane at the bottom of the page. There a user can perform the following actions on a misspelled word:
        * **Give Suggestions**: The spell checker will suggest alternative spellings for the misspelled word. The user can then replace the misspelled word with a suggested word.
        * **Ignore Word**: The word will be ignored for the current Qt Creator instance. Closing and opening Qt Creator will once again flag the word as a spelling mistake
        * **Add Word**: The word will be added to the user dictionary and will be not again be marked as a spelling mistake until the word is manually removed from the user dictionary. 
    - Right clicking on a misspelled word will also allow the user to perform the above actions using the popup menu.
    - Under "*Tools*" -> "*Spell Checker*" the above actions can also be performed.

<h2>Pre-Build Plugins</h2>
I have created a release for the Plugin. The release is listed at version 0.1.1 and can be obtained from the "Releases" page. Read the README.txt file associated with the release for information on how to install the plugin into a Release
version of Qt Creator (Currently 3.2.0). 
<h2>C++ Document Parser</h2>
The C++ document parser that is supplied with the plugin by default has settings that affects how the following types of words will be handled:
- Email Addresses
- Qt Keywords
- Words in CAPS
- Words containing numb3ers
- Words\_with\_underscores
- CamelCase words
- Words that appear in source
- Words.with.dots

Apart from these settings, the plugin also attempts to remove Doxygen Tags in Doxygen comments, in an effort to reduce the number of false positives. 

<h2>TODO</h2>
The following list is a list with a hint into priority of some outstanding tasks I want to do. 
- [x] Get correct Qt Versions to make deployment versions and upload somewhere (Under Releases). 
- [x] Get all spelling mistakes for a active project. The idea was to 1st finish this before releasing, this has changed to start to track the code and get it into a repository. 
- [ ] Underline words that are spelling mistakes (red squiggly lines, did attempt once, but did not work).
- [ ] Parse and ignore website URLs correctly.
- [x] Test in other OS's (Linux, etc.)
  - [x] Make releases for other OS's.
- [ ] Spell check string literals
- [ ] Update to Hunspell 1.3.3

