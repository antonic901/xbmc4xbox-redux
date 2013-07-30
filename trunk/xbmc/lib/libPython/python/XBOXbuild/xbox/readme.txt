changes to python

two configurations:

Release and Debug-XBMC.
For each project in the solution the following libraries must be added to 'Ignore Specific Library' in the Linker properties page
	- libc
	- msvcr71
	- libcmt

Release:
	- is the normal python dll
Debug-XBMC:
	- is the python.dll that can be used for debugging in xbmc.
	  It is a copy from release with the following modifications
	  	- no optimizations
	  	- debugging information format 'Program Database /Zi'
	  	- runtime library 'Multi-threaded /MT'
	  	- no function level linking /Gy
	  	- generate debug info 'yes /DEBUG'
	  	
include\Python.h
	- extra include 'xbox_dll.h', this files renames a lot of functions so that we can be imported in a diferent way in xbmc
	  it is also needed so that visual.net doesn't link its own functions within the dll
	  (an example is getenv, it is not imported in the dll at runtime, but linked staticly in the dll which is useless for us)
	  
		  	/* Include nearly all Python header files */
	
			#include "xbox_dll.h"
			
			#include "patchlevel.h"
			#include "pyconfig.h"
	- and copy xbox_dll.h to PC\xbox_dll.h
	
Modules\posixmodule.h
	- lots of changes, check file itself for _XBOX
	
	chdir
	rename
	rmdir
	unlink
	fileno
	
PC\config.c
	- comment the next lines, they are not needed
		- {"audioop", initaudioop},
		- {"imageop", initimageop},
		- {"rgbimg", initrgbimg},
		- {"msvcrt", initmsvcrt},
		- {"_subprocess", init_subprocess},
		- {"mmap", initmmap},
		- {"_winreg", init_winreg},
	- and delete the associated files from the solution (saves dll size)
		
PC\pyconfig.h
	- change: #define Py_WIN_WIDE_FILENAMES
	  int: #undef Py_WIN_WIDE_FILENAMES

Python\ceval.c
	- change in Py_MakePendingCalls:
	
			#ifdef WITH_THREAD
				if (main_thread && PyThread_get_thread_ident() != main_thread)
					return 0;
			#endif
				if (busy)
					return 0;
	  to:

			// !!!!
			// XBOX
			// since we want to send messages to threads we disable the next check for now
			// !!!!
			#ifdef WITH_THREAD
				//if (main_thread && PyThread_get_thread_ident() != main_thread)
				//	return 0;
			#endif
				/* busy check disabled for xbmc,
			
				code example :
				-----------------------------
				class MainWin(xbmcgui.Window):
					def onAction(self, action):
						if action == ACTION_PREVIOUS_MENU:
							self.close()
						elif action == ACTION_SELECT_ITEM:
							ChildWin().doModal()
			
				class ChildWin(xbmcgui.Window):
					def onAction(self, action):
						if action == ACTION_PREVIOUS_MENU:
							self.close()
			
				parent = MainWin()
				parent.doModal()
				-------------------------------
			
				For each xbmc onAction call, xbmc will call Pendingcalls, execute the
				python onAction object and waits for it's return.
			
				But because ChildWin().doModal() is executed in an onAction method, the
				call to a python onAction object will never return until Child().close
				is called.
			
				With busy checking on this method will just return instead of executing the
				close() command. So for now we just disabled it and hope it won't give any
				trouble when executing code
			*/
				//if (busy)
				//	return 0;

Python\import.c
	- in import.c change:
	
				fd = open(filename, O_EXCL|O_CREAT|O_WRONLY|O_TRUNC
			#ifdef O_BINARY
							|O_BINARY   /* necessary for Windows */
			#endif
			#ifdef __VMS
			                        , 0666, "ctxt=bin", "shr=nil");
			#else
			                        , 0666);
			#endif
	
	  to:
	  
	  		fd = open(filename, O_EXCL|O_CREAT|O_WRONLY|O_TRUNC|O_BINARY, 0666); // XBOX

PC\dl_nt.c
	- change:
		
		LoadString(hInst, 1000, dllVersionBuffer, sizeof(dllVersionBuffer));
	
	  in:
	  
	  	//LoadString(hInst, 1000, dllVersionBuffer, sizeof(dllVersionBuffer));
		strcpy(dllVersionBuffer, "2.4.1"); // _XBOX
		
Python\dynload_win.c
	- change:
	
							} else {
							char buffer[256];
				
				#ifdef _DEBUG
							PyOS_snprintf(buffer, sizeof(buffer), "python%d%d_d.dll",
				#else
							PyOS_snprintf(buffer, sizeof(buffer), "python%d%d.dll",
				#endif
								      PY_MAJOR_VERSION,PY_MINOR_VERSION);
							import_python = GetPythonImport(hDLL);
				
							if (import_python &&
							    strcasecmp(buffer,import_python)) {
								PyOS_snprintf(buffer, sizeof(buffer),
									      "Module use of %.150s conflicts "
									      "with this version of Python.",
									      import_python);
								PyErr_SetString(PyExc_ImportError,buffer);
								FreeLibrary(hDLL);
								return NULL;
							}
						}
						p = GetProcAddress(hDLL, funcname);
						
	  to:
		  
							} else {
				// for now we disable version checking
				#ifndef _XBOX
							char buffer[256];
				
				#ifdef _DEBUG
							PyOS_snprintf(buffer, sizeof(buffer), "python%d%d_d.dll",
				#else
							PyOS_snprintf(buffer, sizeof(buffer), "python%d%d.dll",
				#endif
								      PY_MAJOR_VERSION,PY_MINOR_VERSION);
							import_python = GetPythonImport(hDLL);
				
							if (import_python &&
							    strcasecmp(buffer,import_python)) {
								PyOS_snprintf(buffer, sizeof(buffer),
									      "Module use of %.150s conflicts "
									      "with this version of Python.",
									      import_python);
								PyErr_SetString(PyExc_ImportError,buffer);
								FreeLibrary(hDLL);
								return NULL;
							}
				#endif // _XBOX
						}
						p = GetProcAddress(hDLL, funcname);
						
Objects\fileobject.c
	- change:
		#define fileno _fileno
	  to:
	  	//#define fileno _fileno // XBOX
	
	- change:

		/* From here on we need access to the real fgets and fread */
		#undef fgets
		#undef fread
	
	  to:
	  
		/* From here on we need access to the real fgets and fread */
		//#undef fgets // _XBOX
		//#undef fread // _XBOX