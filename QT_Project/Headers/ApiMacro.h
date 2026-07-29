#ifdef QTRECORDPARSER_USE_DLL
#ifdef QTRECORDPARSER_BUILDING_PROJECT
#define QTRECORDPARSER_API __declspec(dllexport)
#else
#define QTRECORDPARSER_API __declspec(dllimport)
#endif
#else
#define QTRECORDPARSER_API
#endif
