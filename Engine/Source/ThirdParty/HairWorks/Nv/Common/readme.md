NvCommon
========

NvCommon is a small stand alone C++ library that provides the following facilities

* Logging (Logger)
* Abstraction of memory management (MemoryAllocator)
* A very simple object model
* Smart pointers (UniquePtr, and ComPtr)
* String handling (String, SubString)
* Simple containers (Array, PodBuffer)
* Platform specific helper facilities 
	
This document is written in 'markdown' format and can be read in a normal text editor, but may be easier to read in with a nice 'reader'. Some examples

* Markdown plugin for firefox https://addons.mozilla.org/en-US/firefox/addon/markdown-viewer/
	
Releases
--------

* 1.0 - Original Release
	
Integration
-----------

To integrate the appropriate include files need to be included. The main include is 

```
#include <Nv/Common/NvCoCommon.h>
```

All of the files in the NvCommon library are in the Nv/Common directory. All of the files are prefixed with NvCo (short for NvCommon), and are in the namespace nvidia::Common. Typing nvidia::Common can get kind of dull, so there is an alias NvCo. You will see this often used in headers to access NvCommon types explicitly. In cpp files it is not uncommon to just use 'using namespace NvCo' if this doesn't cause significant clashes.

Files are always included by the full path. 

The files in Nv/Common/Platform hold platform specific headers and code. Your project should compiler/include those that are applicable to your platform. All the other files/directories outside of Nv/Common/Platform in Nv/Common are platform independent. To be clear to achieve their platform independence they may be dependant on implementations in the Nv/Common/Platform.

The NvCommon code base depends on the NvCore/1.0 code base. This header only library provides compiler/machine abstraction as well as support for some simple types and error checking. The Nv core library has to be includable along the path Nv/Core/1.0 to work correctly with NvCommon/1.0.

Platforms
---------

Whilst much of the code base in platform independent, the the code in Nv/Common/Platform is currently Windows centric. The codebase is mainly tested and used with Visual Studio 2015 on Windows 7 and highter.
