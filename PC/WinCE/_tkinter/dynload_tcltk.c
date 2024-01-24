/*
 *	David Kashtan, Validus Medical Systems Inc.
 *
 *	Dynamic loading interface for _tkinter's use of Tcl/Tk
 */

#include "Python.h"
#include "tcl.h"
#include "tk.h"

#include <windows.h>

/*
 *	Possible paths for the TCL and TK dlls
 */

static TCHAR Celib_Dll_Local[MAX_PATH+1] = TEXT("celib.dll");
static TCHAR TCL_Dll_Local[MAX_PATH+1] = TEXT("tcl84.dll");
static TCHAR TK_Dll_Local[MAX_PATH+1] = TEXT("tk84.dll");

static TCHAR *Celib_Dll[] = {Celib_Dll_Local, TEXT("\\Windows\\celib.dll"), 0};
static TCHAR *TCL_Dll[] = {TCL_Dll_Local, TEXT("\\Windows\\tcl84.dll"), 0};
static TCHAR *TK_Dll[] = {TK_Dll_Local, TEXT("\\Windows\\tk84.dll"), 0};

/*
 *	The dynamic link functions for celib
 */
struct _Celib_Dynload {void (*_xceinit)(const wchar_t *);};
#define	_CELIB_FUNCTIONS \
 _FUNCTION(xceinit) \

/*
 *	The dynamic link functions for Tcl
 */
#define _TCL_FUNCTIONS \
_FUNCTION(Tcl_Alloc) \
_FUNCTION(Tcl_Free) \
_FUNCTION(Tcl_SplitList) \
_FUNCTION(Tcl_GetStringResult) \
_FUNCTION(Tcl_Init) \
_FUNCTION(Tcl_GetString) \
_FUNCTION(Tcl_GetStringFromObj) \
_FUNCTION(Tcl_GetThreadData) \
_FUNCTION(Tcl_GetCurrentThread) \
_FUNCTION(Tcl_MutexLock) \
_FUNCTION(Tcl_MutexUnlock) \
_FUNCTION(Tcl_ConditionWait) \
_FUNCTION(Tcl_ThreadAlert) \
_FUNCTION(Tcl_ThreadQueueEvent) \
_FUNCTION(Tcl_NewStringObj) \
_FUNCTION(Tcl_NewLongObj) \
_FUNCTION(Tcl_NewDoubleObj) \
_FUNCTION(Tcl_NewListObj) \
_FUNCTION(Tcl_NewBooleanObj) \
_FUNCTION(Tcl_NewUnicodeObj) \
_FUNCTION(Tcl_NewWideIntObj) \
_FUNCTION(Tcl_GetObjResult) \
_FUNCTION(Tcl_GetUnicode) \
_FUNCTION(Tcl_GetCharLength) \
_FUNCTION(Tcl_ListObjIndex) \
_FUNCTION(Tcl_ListObjLength) \
_FUNCTION(Tcl_GetByteArrayFromObj) \
_FUNCTION(Tcl_ConditionNotify) \
_FUNCTION(Tcl_GlobalEval) \
_FUNCTION(Tcl_Merge) \
_FUNCTION(Tcl_Eval) \
_FUNCTION(Tcl_EvalFile) \
_FUNCTION(Tcl_RecordAndEval) \
_FUNCTION(Tcl_AddErrorInfo) \
_FUNCTION(Tcl_SetVar2Ex) \
_FUNCTION(Tcl_GetVar2Ex) \
_FUNCTION(Tcl_UnsetVar2) \
_FUNCTION(Tcl_GetInt) \
_FUNCTION(Tcl_GetDouble) \
_FUNCTION(Tcl_GetBoolean) \
_FUNCTION(Tcl_ExprString) \
_FUNCTION(Tcl_ExprLong) \
_FUNCTION(Tcl_ExprDouble) \
_FUNCTION(Tcl_ExprBoolean) \
_FUNCTION(Tcl_CreateCommand) \
_FUNCTION(Tcl_DeleteCommand) \
_FUNCTION(Tcl_DeleteTimerHandler) \
_FUNCTION(Tcl_CreateTimerHandler) \
_FUNCTION(Tcl_DoOneEvent) \
_FUNCTION(Tcl_DeleteInterp) \
_FUNCTION(Tcl_SetVar) \
_FUNCTION(Tcl_GetVar) \
_FUNCTION(Tcl_SetVar2) \
_FUNCTION(Tcl_GetObjType) \
_FUNCTION(Tcl_CreateInterp) \
_FUNCTION(Tcl_SetResult) \
_FUNCTION(Tcl_EvalObjv) \
_FUNCTION(TclFreeObj) \
_FUNCTION(Tcl_FindExecutable) \

/*
 *	The dynamic link functions for Tk
 */
#define _TK_FUNCTIONS \
_FUNCTION(Tk_MainWindow) \
_FUNCTION(Tk_Init) \
_FUNCTION(Tk_GetNumMainWindows)

/*
 *	The lists of function names and offsets
 */
struct _Function {
	TCHAR *Name;
	int Offset;
	};

#define _FUNCTION(name) {TEXT(#name), (int)&((struct _Celib_Dynload *)0)->_##name},
static struct _Function Celib_Functions[] = {_CELIB_FUNCTIONS {0, 0}};
#undef	_FUNCTION

#define _FUNCTION(name) {TEXT(#name), (int)&((struct _Tcl_Dynload *)0)->_##name},
static struct _Function Tcl_Functions[] = {_TCL_FUNCTIONS {0, 0}};
#undef	_FUNCTION

#define _FUNCTION(name) {TEXT(#name), (int)&((struct _Tk_Dynload *)0)->_##name},
static struct _Function Tk_Functions[] = {_TK_FUNCTIONS {0, 0}};
#undef	_FUNCTION

/*
 *	Storage for the dynamically loaded functions
 */
struct _Celib_Dynload _Celib_Dynload = {0};
struct _Tcl_Dynload _Tcl_Dynload = {0};
struct _Tk_Dynload _Tk_Dynload = {0};

/*
 *	Do the dynamic linking
 */
static int Do_Dynamic_Link(void)
{
	int i;
	HANDLE Handle;
	TCHAR **Dll;
	struct _Function *Function;
	void *Dynload_Structure;
	char Error_String[128];

    static int Local_Ready = 0;
    static wchar_t Prog_Dir[MAX_PATH+1];
	wchar_t *c;

	static struct {
		char *Name;
		TCHAR **Dll;
		struct _Function *Functions;
		void *Dynload_Structure;
		} Load[] = {{"CeLib", Celib_Dll, Celib_Functions, (void *)&_Celib_Dynload},
			    {"TCL",   TCL_Dll,   Tcl_Functions,   (void *)&_Tcl_Dynload},
			    {"TK",     TK_Dll,   Tk_Functions,    (void *)&_Tk_Dynload}};

	if (Local_Ready == 0) {
		if (!GetModuleFileName(NULL, Prog_Dir, MAX_PATH+1))
			Local_Ready = -1;
		else {
			c = wcsrchr(Prog_Dir, L'\\');
			if (c != NULL)
				*c = L'\0';
			else
				Prog_Dir[0] = L'\0';
			Local_Ready = 1;
			for(i = 0; i < (sizeof(Load)/sizeof(Load[0])); i++) {
				if (PathCchCombineEx(*(Load[i].Dll), MAX_PATH+1, Prog_Dir, *(Load[i].Dll), 0) != S_OK) {
					Local_Ready = -1;
					break;
				}
			}
		}
	}
	/*
	 *	Do the dynamic loading
	 */
	for(i = 0; i < (sizeof(Load)/sizeof(Load[0])); i++) {
		/*
		 *	Get the dll
		 */
		Dll = Load[i].Dll;
		Handle = 0;
        if (Local_Ready <= 0)
            Dll++;
		while(*Dll) {
#ifdef	SEM_NOOPENFILEERRORBOX
			int Saved_Error_Mode = SetErrorMode(SEM_NOOPENFILEERRORBOX);
#endif	/* SEM_NOOPENFILEERRORBOX */

			/*
			 *	Attempt to load the DLL
			 */
			Handle = LoadLibrary(*Dll);
#ifdef	SEM_NOOPENFILEERRORBOX
			SetErrorMode(Saved_Error_Mode);
#endif	/* SEM_NOOPENFILEERRORBOX */
			if (Handle) {
			}
			/*
			 *	Failed: Try the next path
			 */
			Dll++;
		}
		/*
		 *	Did we find the DLL?
		 */
		if (!Handle) {
			/*
			 *	No: Generate a python error
			 */
			PyOS_snprintf(Error_String, sizeof(Error_String), "Could not find %s DLL", Load[i].Name);
			PyErr_SetString(PyExc_RuntimeError, Error_String);
			return(-1);
		}
		/*
		 *	Yes: Fill in its functions
		 */
		Dynload_Structure = Load[i].Dynload_Structure;
		for(Function = Load[i].Functions; Function->Name; Function++) {
			void (**Function_Pointer)(void);

			/*
			 *	Get the function pointer pointer (offset into the dynload structure)
			 */
			Function_Pointer = (void (**)(void))((char *)Dynload_Structure + Function->Offset);
			/*
			 *	Get the function pointer from the DLL
			 */
			*Function_Pointer = (void (*)(void))GetProcAddress(Handle, Function->Name);
			if (!*Function_Pointer) {
				TCHAR *cp; char *cp1;

				/*
				 *	Not found: Get the function name in ascii (for the error string)
				 */
				cp = Function->Name;
				while(*cp) cp++;
				cp1 = Error_String + sizeof(Error_String);
				*--cp1 = '\0';
				while(cp > Function->Name) *--cp1 = (char)*--cp;
				/*
				 *	Generate the Python error
				 */
				PyOS_snprintf(Error_String, cp1 - Error_String, "Function \"%s\" not found in %s DLL", cp1, Load[i].Name);
				PyErr_SetString(PyExc_RuntimeError, Error_String);
				return(-1);
			}
		}
	}
	/*
	 *	Success
	 */
	return(0);
}

/*
 *	Dynamic linking trigger function:  We are called out of init_tkinter
 */
void Tcl_FindExecutable(const char * argv0)
{
	/*
	 *	See if we are already initialized
	*/
	if (!_Celib_Dynload._xceinit) {
		/*
		 *	No: Dynamically load the modules
		 */
		if (Do_Dynamic_Link() != 0) return;
		/*
		 *	Initialize Celib
		 */
		(*_Celib_Dynload._xceinit)(0);
	}
	/*
	 *	Call Tcl_FindExecutable
	 */
	(*_Tcl_Dynload._Tcl_FindExecutable)(argv0);
}
