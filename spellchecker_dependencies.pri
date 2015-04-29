QTC_PLUGIN_NAME = SpellChecker
QTC_LIB_DEPENDS += \
    cplusplus \
    extensionsystem

QTC_PLUGIN_DEPENDS += \
    coreplugin \
    texteditor \
    projectexplorer \
    cppeditor \
    cpptools

QTC_PLUGIN_RECOMMENDS += \
    # optional plugin dependencies. nothing here at this time
