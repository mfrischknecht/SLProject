//#############################################################################
//  File:      SL/SL.h
//  Author:    Marcus Hudritsch
//  Date:      October 2015
//  Codestyle: https://github.com/cpvrlab/SLProject/wiki/Coding-Style-Guidelines
//  Copyright: Marcus Hudritsch
//             This software is provide under the GNU General Public License
//             Please visit: http://opensource.org/licenses/GPL-3.0
//#############################################################################

#ifndef SL_H
#define SL_H

#include <SLEnums.h>
//-----------------------------------------------------------------------------
// Include standard C++ libraries
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <queue>
#include <typeinfo>
#include <string>
#include <algorithm>
#include <map>
#include <chrono>
#include <thread>
#include <atomic>
#include <functional>
#include <random>
#include <cstdarg>

//-----------------------------------------------------------------------------
// Include standard C libraries
#include <stdio.h>               // for the old ANSI C IO functions
#include <stdlib.h>              // srand, rand
#include <float.h>               // for defines like FLT_MAX & DBL_MAX
#include <limits.h>              // for defines like UINT_MAX
#include <assert.h>              // for debug asserts
#include <time.h>                // for clock()
#include <sys/stat.h>            // for file info used in SLUtils
#include <math.h>                // for math functions
#include <string.h>              // for string functions
//-----------------------------------------------------------------------------

//////////////////////////////////////////////////////////
// Preprocessor constant definitions used in the SLProject
//////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
/* Determine one of the following operating systems:
SL_OS_MACOS    :Apple Mac OSX
SL_OS_MACIOS   :Apple iOS
SL_OS_WINDOWS  :Microsoft desktop Windows XP, 7, 8, ...
SL_OS_ANDROID  :Goggle Android
SL_OS_LINUX    :Linux desktop OS

With the OS definition the following constants are defined:
SL_GLES : Any version of OpenGL ES
SL_GLES2: Supports only OpenGL ES2
SL_GLES3: Supports only OpenGL ES3
SL_MEMLEAKDETECT: The memory leak detector NVWA is used
SL_USE_DISCARD_STEREOMODES: The discard stereo modes can be used (SLCamera)
*/

#ifdef __APPLE__
    #include <TargetConditionals.h>
    #if TARGET_OS_IOS
        #define SL_OS_MACIOS
        #define SL_GLES
        #define SL_GLES3
    #else
        #define SL_OS_MACOS
        #if defined(_DEBUG)
            #define _GLDEBUG
            //#define SL_MEMLEAKDETECT  // nvwa doesn't work under OSX/clang
        #endif
    #endif
#elif defined(ANDROID) || defined(ANDROID_NDK)
    #define SL_OS_ANDROID
    #define SL_GLES
    #define SL_GLES3
#elif defined(_WIN32)
    #define SL_OS_WINDOWS
    #define SL_USE_DISCARD_STEREOMODES
    #ifdef _DEBUG
        #define _GLDEBUG
        //#define SL_MEMLEAKDETECT
        //#define _NO_DEBUG_HEAP 1
    #endif
    #define STDCALL __stdcall
#elif defined(linux) || defined(__linux) || defined(__linux__)
    #define SL_OS_LINUX
    #define SL_USE_DISCARD_STEREOMODES
    #ifdef _DEBUG
        //#define SL_MEMLEAKDETECT  // nvwa doesn't work under OSX/clang
    #endif
#else
    #error "SL has not been ported to this OS"
#endif

//-----------------------------------------------------------------------------
/* With one of the following constants the GUI system must be defined. This
has to be done in the project settings (pro files for QtCreator or in the
Visual Studio project settings):

SL_GUI_QT   :Qt on OSX, Windows, Linux or Android
SL_GUI_OBJC :ObjectiveC on iOS
SL_GUI_GLFW :GLFW on OSX, Windows or Linux
SL_GUI_JAVA :Java on Android (with the VS-Android project)
*/

//-----------------------------------------------------------------------------
#if defined(SL_OS_MACIOS)
    #include <sys/time.h>
    #include <OpenGLES/ES3/gl.h>
    #include <OpenGLES/ES3/glext.h>
    #include <zlib.h>
    #include <functional>
    #include <thread>
    #include <chrono>
    #include <random>
#elif defined(SL_OS_MACOS)
    #include <sys/time.h>
    #include <functional>
    #include <thread>
    #include <chrono>
    #include <random>
    #include <GL/glew.h>
#elif defined(SL_OS_ANDROID)
    #include <sys/time.h>
    #ifdef SL_GLES2
        #include <GLES2/gl2.h>
        #include <GLES2/gl2ext.h>
    #endif
    #ifdef SL_GLES3
        #include <GLES3/gl3.h>
        #include <GLES3/gl3ext.h>
    #endif
    #include <android/log.h>
    #include <functional>
    #include <thread>
    #include <chrono>
    #include <random>
#elif defined(SL_OS_WINDOWS)
    #include <functional>
    #include <thread>
    #include <chrono>
    #include <random>
    #include <windows.h>
    #include <GL/glew.h>
#elif defined(SL_OS_LINUX)
    #include <sys/time.h>
    #include <functional>
    #include <thread>
    #include <chrono>
    #include <random>
    #include <GL/glew.h>
#else
    #error "SL has not been ported to this OS"
#endif

//-----------------------------------------------------------------------------
using namespace std;



//-----------------------------------------------------------------------------
// Determine compiler
#if defined(__GNUC__)
    #undef _MSC_VER
#endif

#if defined(_MSC_VER)
    #define SL_COMP_MSVC
    #define SL_STDCALL __stdcall
    #define SL_DEPRECATED __declspec(deprecated)
    #define _CRT_SECURE_NO_DEPRECATE // visual 8 secure crt warning
#elif defined(__BORLANDC__)
    #define SL_COMP_BORLANDC
    #define SL_STDCALL Stdcall
    #define SL_DEPRECATED   // @todo Does this compiler support deprecated attributes
#elif defined(__INTEL_COMPILER)
    #define SL_COMP_INTEL
    #define SL_STDCALL Stdcall
    #define SL_DEPRECATED   // @todo does this compiler support deprecated attributes
#elif defined(__GNUC__)
    #define SL_COMP_GNUC
    #define SL_STDCALL
    #define SL_DEPRECATED __attribute__((deprecated))
#else 
    #error "SL has not been ported to this compiler"
#endif

//-----------------------------------------------------------------------------
// Redefinition of standard types for platform independence
typedef std::string     SLstring;
#ifndef SL_OS_ANDROID
    typedef std::wstring    SLwstring;
#endif
typedef GLchar          SLchar;  // char is signed [-128 ... 127]!
typedef unsigned char   SLuchar;
typedef signed long     SLlong;
typedef unsigned long   SLulong;
typedef GLbyte          SLbyte;  // byte is signed [-128 ... 127]!
typedef GLubyte         SLubyte;
typedef GLshort         SLshort;
typedef GLushort        SLushort; 
typedef GLint           SLint;
typedef GLuint          SLuint;
typedef int64_t         SLint64;
typedef uint64_t        SLuint64;
typedef GLsizei         SLsizei;
typedef GLfloat         SLfloat;
#ifdef SL_HAS_DOUBLE
typedef GLdouble        SLdouble;
typedef GLfloat         SLreal;
#else
typedef GLfloat         SLreal;
#endif
typedef bool            SLbool; 
typedef GLenum          SLenum;
typedef GLbitfield      SLbitfield;
typedef GLfloat         SLfloat;

// All 1D vectors begin with SLV*
typedef std::vector<SLbool>   SLVbool;
typedef std::vector<SLbyte>   SLVbyte;
typedef std::vector<SLubyte>  SLVubyte;
typedef std::vector<SLchar>   SLVchar;
typedef std::vector<SLuchar>  SLVuchar;
typedef std::vector<SLshort>  SLVshort;
typedef std::vector<SLushort> SLVushort;
typedef std::vector<SLint>    SLVint;
typedef std::vector<SLuint>   SLVuint;
typedef std::vector<SLlong>   SLVlong;
typedef std::vector<SLulong>  SLVulong;
typedef std::vector<SLfloat>  SLVfloat;
typedef std::vector<SLstring> SLVstring;

// All 2D vectors begin with SLVV*
typedef std::vector<vector<SLfloat>>    SLVVfloat;
typedef std::vector<vector<SLuchar>>    SLVVuchar;
typedef std::vector<vector<SLushort>>   SLVVushort;
typedef std::vector<vector<SLuint>>     SLVVuint;
typedef std::vector<vector<SLchar>>     SLVVchar;
typedef std::vector<vector<SLshort>>    SLVVshort;
typedef std::vector<vector<SLint>>      SLVVint;

//-----------------------------------------------------------------------------
// Shortcut for size of a vector
template<class T> inline SLint SL_sizeOfVector(const T &vector)
{
    return (SLint)(vector.capacity()*sizeof(typename T::value_type));
}
//-----------------------------------------------------------------------------
// Bit manipulation macros for ones that forget it always
#define SL_GETBIT(VAR, BITVAL) VAR&BITVAL
#define SL_SETBIT(VAR, BITVAL) VAR|=BITVAL
#define SL_DELBIT(VAR, BITVAL) VAR&=~BITVAL
#define SL_TOGBIT(VAR, BITVAL) if (VAR&BITVAL) VAR &=~BITVAL; else VAR|=BITVAL

//-----------------------------------------------------------------------------
// Prevention for warnings in XCode
#define UNUSED_PARAMETER(r)  ((void)(x))

//-----------------------------------------------------------------------------
// Some debugging and error handling functions and macros
#define SL_LOG(...)     SL::log(__VA_ARGS__)
#define SL_EXIT_MSG(M)  SL::exitMsg((M), __LINE__, __FILE__)
#define SL_WARN_MSG(M)  SL::warnMsg((M), __LINE__, __FILE__)
//-----------------------------------------------------------------------------
//! Class SL with some global static functions and members.
class SLSceneView;
class SL
{
    public:
    static void             log                 (const char* format, ...);
    static void             exitMsg             (const SLchar* msg, 
                                                 const SLint line, 
                                                 const SLchar* file);
    static void             warnMsg             (const SLchar* msg, 
                                                 const SLint line, 
                                                 const SLchar* file);
    static SLuint           maxThreads          ();
    static SLstring         getCWD              ();
    static void             parseCmdLineArgs    (SLVstring& cmdLineArgs);
    static SLbool           noTestIsRunning     (){return (SLint)testScene == -1;}
    static SLbool           singleTestIsRunning (){return testScene > C_sceneAll && 
                                                          testScene <= C_sceneRTTest;}
    static SLbool           allTestIsRunning    (){return testScene > C_sceneAll && 
                                                          testScene <= C_sceneRTTest;}
    static void             loadConfig          (SLSceneView* sv);
    static void             saveConfig          (SLSceneView* sv);
    static SLfloat          dpmm                () {return (float)dpi/25.4f;}

    static SLCommand        testScene;          //!< Test scene command id (-1 for no test)
    static SLCommand        testSceneAll;       //!< Test scene command id for all tests
    static SLint            testDurationSec;    //!< Test time in seconds
    static SLint            testFactor;         //!< Test factor for scene construction
    static SLLogVerbosity   testLogVerbosity;   //!< Test logging verbosity
    static SLuint           testFrameCounter;   //!< Test frame counters
    static const SLVstring  testSceneNames;     //!< Vector with scene names
    
    static SLstring         configPath;         //!< Default path for calibration files
    static SLstring         configTime;         //!< Time of stored configuration 
    static SLint            dpi;                //!< Current UI dot per inch resolution
    static SLCommand        currentSceneID;     //!< ID of last loaded scene
};
//-----------------------------------------------------------------------------
#endif
