/*
 *	Minimal stuff we need to interface with TCL
 */
#ifndef _TCL_H
#define _TCL_H

/*
 *  Define Tcl_WideInt
 */
#define TCL_WIDE_INT_TYPE __int64

typedef TCL_WIDE_INT_TYPE Tcl_WideInt;

#define Tcl_WideAsLong(val) ((long)((Tcl_WideInt)(val)))
#define Tcl_LongAsWide(val) ((Tcl_WideInt)((long)(val)))
#define Tcl_WideAsDouble(val) ((double)((Tcl_WideInt)(val)))
#define Tcl_DoubleAsWide(val) ((Tcl_WideInt)((double)(val)))

/*
 *	Define the Tcl Object
 */
typedef struct Tcl_ObjType {
	char *name;
	void *freeIntRepProc;
	void *dupIntRepProc;
	void *updateStringProc;
	void *setFromAnyProc;
	} Tcl_ObjType;
typedef struct Tcl_Obj {
	int refCount;
	char *bytes;
	int length;
	Tcl_ObjType *typePtr;
	union {long longValue; double doubleValue;void *otherValuePtr; Tcl_WideInt wideValue; struct {void *ptr1; void *ptr2;} twoPtrValue;} internalRep;
	} Tcl_Obj;

/*
 *	Define Tcl Events
 */
typedef struct Tcl_Event Tcl_Event;
typedef int (Tcl_EventProc) (Tcl_Event *evPtr, int flags);
struct Tcl_Event {
	Tcl_EventProc *proc;
	struct Tcl_Event *nextPtr;
	};

/*
 *	Tcl Object Reference count manipulation macros
 */
#define Tcl_IncrRefCount(objPtr) ++((objPtr)->refCount)
#define Tcl_DecrRefCount(objPtr) if (--(objPtr)->refCount <= 0) TclFreeObj(objPtr)

/*
 *	Define TCL constants
 */
#define TCL_ALPHA_RELEASE   0
#define TCL_BETA_RELEASE    1
#define TCL_FINAL_RELEASE   2

#define TCL_VERSION	    "8.4"
#define TCL_PATCH_LEVEL	"8.4.3"
#define TCL_OK		    0
#define TCL_ERROR	    1
#define TCL_UTF_MAX	    3
#define TCL_GLOBAL_ONLY     1
#define TCL_LEAVE_ERR_MSG   0x200
#define TCL_DONT_WAIT	    (1<<1)
#define TCL_WINDOW_EVENTS   (1<<2)
#define TCL_FILE_EVENTS	    (1<<3)
#define TCL_TIMER_EVENTS    (1<<4)
#define TCL_IDLE_EVENTS	    (1<<5)	/* WAS 0x10 ???? */
#define TCL_ALL_EVENTS	    (~TCL_DONT_WAIT)
#define TCL_READABLE	    (1<<1)
#define TCL_WRITABLE	    (1<<2)
#define TCL_EXCEPTION	    (1<<3)
#define TCL_NO_EVAL	    0x10000
#define TCL_EVAL_GLOBAL 0x20000
#define TCL_EVAL_DIRECT	0x40000
#define TCL_EVAL_INVOKE 0x80000
typedef enum {TCL_QUEUE_TAIL, TCL_QUEUE_HEAD, TCL_QUEUE_MARK} Tcl_QueuePosition;
#define TCL_VOLATILE	((Tcl_FreeProc *) 1)
#define TCL_STATIC      ((Tcl_FreeProc *) 0)
#define TCL_DYNAMIC     ((Tcl_FreeProc *) 3)


/*
 *	Define macros required for Tcl
 */
#define attemptckalloc(x) Tcl_AttemptAlloc(x)
#define ckalloc(x) Tcl_Alloc(x)
#define ckfree(x) Tcl_Free(x)
#define TCL_DECLARE_MUTEX(name) static Tcl_Mutex name;
//#define	_kbhit() WinCEShell_kbhit()

/*
 *	Define the Tcl Unicode Character
 */
typedef unsigned short Tcl_UniChar;

/*
 *	Define the opaque typedefs required for the Tcl functions
 */
typedef void *Tcl_Interp;
typedef void *Tcl_ThreadDataKey;
typedef void *Tcl_ThreadId;
typedef void *Tcl_Mutex;
typedef void *Tcl_Condition;
typedef void *Tcl_Time;
typedef void *Tcl_Command;
typedef void *Tcl_TimerToken;
typedef void *ClientData;

/*
 *	Define procedure typedefs required for the Tcl functions
 */
typedef int (Tcl_CmdProc)(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]);
typedef void (Tcl_CmdDeleteProc) (ClientData clientData);
typedef void (Tcl_TimerProc)(ClientData clientData);
typedef void (Tcl_FreeProc)(char *blockPtr);

/*
 *	Define the dynamically loaded Tcl functions
 */
extern struct _Tcl_Dynload {
#define Tcl_Alloc (*_Tcl_Dynload._Tcl_Alloc)
char * (*_Tcl_Alloc)(unsigned int size);

#define Tcl_Free (*_Tcl_Dynload._Tcl_Free)
void (*_Tcl_Free)(char * ptr);

#define Tcl_SplitList (*_Tcl_Dynload._Tcl_SplitList)
int (*_Tcl_SplitList)(Tcl_Interp * interp, const char * listStr, int * argcPtr, char *** argvPtr);

#define Tcl_GetStringResult (*_Tcl_Dynload._Tcl_GetStringResult)
char * (*_Tcl_GetStringResult)(Tcl_Interp * interp);

#define Tcl_Init (*_Tcl_Dynload._Tcl_Init)
int (*_Tcl_Init)(Tcl_Interp * interp);

#define Tcl_GetString (*_Tcl_Dynload._Tcl_GetString)
char * (*_Tcl_GetString)(Tcl_Obj * objPtr);

#define Tcl_GetStringFromObj (*_Tcl_Dynload._Tcl_GetStringFromObj)
char * (*_Tcl_GetStringFromObj)(Tcl_Obj * objPtr, int * lengthPtr);

#define Tcl_GetThreadData (*_Tcl_Dynload._Tcl_GetThreadData)
void * (*_Tcl_GetThreadData) (Tcl_ThreadDataKey * keyPtr, int size);

#define Tcl_GetCurrentThread (*_Tcl_Dynload._Tcl_GetCurrentThread)
Tcl_ThreadId (*_Tcl_GetCurrentThread) (void);

#define Tcl_MutexLock (*_Tcl_Dynload._Tcl_MutexLock)
void (*_Tcl_MutexLock) (Tcl_Mutex * mutexPtr);

#define Tcl_MutexUnlock (*_Tcl_Dynload._Tcl_MutexUnlock)
void (*_Tcl_MutexUnlock) (Tcl_Mutex * mutexPtr);

#define Tcl_ConditionWait (*_Tcl_Dynload._Tcl_ConditionWait)
void (*_Tcl_ConditionWait)(Tcl_Condition * condPtr, Tcl_Mutex * mutexPtr, Tcl_Time * timePtr);

#define Tcl_ThreadAlert (*_Tcl_Dynload._Tcl_ThreadAlert)
void (*_Tcl_ThreadAlert) (Tcl_ThreadId threadId);

#define Tcl_ThreadQueueEvent (*_Tcl_Dynload._Tcl_ThreadQueueEvent)
void (*_Tcl_ThreadQueueEvent) (Tcl_ThreadId threadId, Tcl_Event* evPtr, Tcl_QueuePosition position);

#define Tcl_NewStringObj (*_Tcl_Dynload._Tcl_NewStringObj)
Tcl_Obj * (*_Tcl_NewStringObj) (const char * bytes, int length);

#define Tcl_NewLongObj (*_Tcl_Dynload._Tcl_NewLongObj)
Tcl_Obj * (*_Tcl_NewLongObj) (long longValue);

#define Tcl_NewDoubleObj (*_Tcl_Dynload._Tcl_NewDoubleObj)
Tcl_Obj * (*_Tcl_NewDoubleObj) (double doubleValue);

#define Tcl_NewListObj (*_Tcl_Dynload._Tcl_NewListObj)
Tcl_Obj * (*_Tcl_NewListObj) (int objc, Tcl_Obj *const objv[]);

#define Tcl_NewBooleanObj (*_Tcl_Dynload._Tcl_NewBooleanObj)
Tcl_Obj * (*_Tcl_NewBooleanObj) (int boolValue);

#define Tcl_NewUnicodeObj (*_Tcl_Dynload._Tcl_NewUnicodeObj)
Tcl_Obj * (*_Tcl_NewUnicodeObj) (Tcl_UniChar * unicode, int numChars);

#define Tcl_NewWideIntObj (*_Tcl_Dynload._Tcl_NewWideIntObj)
Tcl_Obj * (*_Tcl_NewWideIntObj) (Tcl_WideInt * unicode);

#define Tcl_GetObjResult (*_Tcl_Dynload._Tcl_GetObjResult)
Tcl_Obj * (*_Tcl_GetObjResult) (Tcl_Interp * interp);

#define Tcl_GetUnicode (*_Tcl_Dynload._Tcl_GetUnicode)
Tcl_UniChar * (*_Tcl_GetUnicode) (Tcl_Obj * objPtr);

#define Tcl_GetCharLength (*_Tcl_Dynload._Tcl_GetCharLength)
int (*_Tcl_GetCharLength) (Tcl_Obj * objPtr);

#define Tcl_ListObjIndex (*_Tcl_Dynload._Tcl_ListObjIndex)
int (*_Tcl_ListObjIndex) (Tcl_Interp * interp, Tcl_Obj * listPtr, int index, Tcl_Obj ** objPtrPtr);

#define Tcl_ListObjLength (*_Tcl_Dynload._Tcl_ListObjLength)
int (*_Tcl_ListObjLength) (Tcl_Interp * interp, Tcl_Obj * listPtr, int * intPtr);

#define Tcl_GetByteArrayFromObj (*_Tcl_Dynload._Tcl_GetByteArrayFromObj)
unsigned char * (*_Tcl_GetByteArrayFromObj) (Tcl_Obj * objPtr, int * lengthPtr);

#define Tcl_ConditionNotify (*_Tcl_Dynload._Tcl_ConditionNotify)
void (*_Tcl_ConditionNotify) (Tcl_Condition * condPtr);

#define Tcl_GlobalEval (*_Tcl_Dynload._Tcl_GlobalEval)
int (*_Tcl_GlobalEval) (Tcl_Interp * interp, char * command);

#define Tcl_Merge (*_Tcl_Dynload._Tcl_Merge)
char * (*_Tcl_Merge) (int argc, char ** argv);

#define Tcl_Eval (*_Tcl_Dynload._Tcl_Eval)
int (*_Tcl_Eval) (Tcl_Interp * interp, char * string);

#define Tcl_EvalFile (*_Tcl_Dynload._Tcl_EvalFile)
int (*_Tcl_EvalFile) (Tcl_Interp * interp, char * fileName);

#define Tcl_RecordAndEval (*_Tcl_Dynload._Tcl_RecordAndEval)
int (*_Tcl_RecordAndEval) (Tcl_Interp * interp, char * cmd, int flags);

#define Tcl_AddErrorInfo (*_Tcl_Dynload._Tcl_AddErrorInfo)
void (*_Tcl_AddErrorInfo) (Tcl_Interp * interp, const char * message);

#define Tcl_SetVar2Ex (*_Tcl_Dynload._Tcl_SetVar2Ex)
Tcl_Obj * (*_Tcl_SetVar2Ex) (Tcl_Interp * interp, char * part1, char * part2, Tcl_Obj * newValuePtr, int flags);

#define Tcl_GetVar2Ex (*_Tcl_Dynload._Tcl_GetVar2Ex)
Tcl_Obj * (*_Tcl_GetVar2Ex) (Tcl_Interp * interp, char * part1, char * part2, int flags);

#define Tcl_UnsetVar2 (*_Tcl_Dynload._Tcl_UnsetVar2)
int (*_Tcl_UnsetVar2) (Tcl_Interp * interp, char * part1, char * part2, int flags);

#define Tcl_GetInt (*_Tcl_Dynload._Tcl_GetInt)
int (*_Tcl_GetInt) (Tcl_Interp * interp, char * str, int * intPtr);

#define Tcl_GetDouble (*_Tcl_Dynload._Tcl_GetDouble)
int (*_Tcl_GetDouble) (Tcl_Interp * interp, char * str, double * doublePtr);

#define Tcl_GetBoolean (*_Tcl_Dynload._Tcl_GetBoolean)
int (*_Tcl_GetBoolean) (Tcl_Interp * interp, char * str, int * boolPtr);

#define Tcl_ExprString (*_Tcl_Dynload._Tcl_ExprString)
int (*_Tcl_ExprString) (Tcl_Interp * interp, char * string);

#define Tcl_ExprLong (*_Tcl_Dynload._Tcl_ExprLong)
int (*_Tcl_ExprLong) (Tcl_Interp * interp, char * str, long * ptr);

#define Tcl_ExprDouble (*_Tcl_Dynload._Tcl_ExprDouble)
int (*_Tcl_ExprDouble) (Tcl_Interp * interp, char * str, double * ptr);

#define Tcl_ExprBoolean (*_Tcl_Dynload._Tcl_ExprBoolean)
int (*_Tcl_ExprBoolean) (Tcl_Interp * interp, char * str, int * ptr);

#define Tcl_CreateCommand (*_Tcl_Dynload._Tcl_CreateCommand)
Tcl_Command (*_Tcl_CreateCommand) (Tcl_Interp * interp, char * cmdName, Tcl_CmdProc * proc, ClientData clientData, Tcl_CmdDeleteProc * deleteProc);

#define Tcl_DeleteCommand (*_Tcl_Dynload._Tcl_DeleteCommand)
int (*_Tcl_DeleteCommand) (Tcl_Interp * interp, char * cmdName);

#define Tcl_DeleteTimerHandler (*_Tcl_Dynload._Tcl_DeleteTimerHandler)
void (*_Tcl_DeleteTimerHandler) (Tcl_TimerToken token);

#define Tcl_CreateTimerHandler (*_Tcl_Dynload._Tcl_CreateTimerHandler)
Tcl_TimerToken (*_Tcl_CreateTimerHandler) (int milliseconds, Tcl_TimerProc * proc, ClientData clientData);

#define Tcl_DoOneEvent (*_Tcl_Dynload._Tcl_DoOneEvent)
int (*_Tcl_DoOneEvent) (int flags);

#define Tcl_DeleteInterp (*_Tcl_Dynload._Tcl_DeleteInterp)
void (*_Tcl_DeleteInterp) (Tcl_Interp * interp);

#define Tcl_SetVar (*_Tcl_Dynload._Tcl_SetVar)
char * (*_Tcl_SetVar) (Tcl_Interp * interp, char * varName, char * newValue, int flags);

#define Tcl_GetVar (*_Tcl_Dynload._Tcl_GetVar)
char * (*_Tcl_GetVar) (Tcl_Interp * interp, char * varName, int flags);

#define Tcl_SetVar2 (*_Tcl_Dynload._Tcl_SetVar2)
char * (*_Tcl_SetVar2) (Tcl_Interp * interp, char * part1, char * part2, char * newValue, int flags);

#define Tcl_GetObjType (*_Tcl_Dynload._Tcl_GetObjType)
Tcl_ObjType * (*_Tcl_GetObjType) (char * typeName);

#define Tcl_CreateInterp (*_Tcl_Dynload._Tcl_CreateInterp)
Tcl_Interp * (*_Tcl_CreateInterp) (void);

#define	Tcl_SetResult (*_Tcl_Dynload._Tcl_SetResult)
void (*_Tcl_SetResult) (Tcl_Interp * interp, char * str, Tcl_FreeProc * freeProc);

#define	Tcl_EvalObjv (*_Tcl_Dynload._Tcl_EvalObjv)
int (*_Tcl_EvalObjv) (Tcl_Interp * interp, int objc, Tcl_Obj *const objv[], int flags);

#define	TclFreeObj (*_Tcl_Dynload._TclFreeObj)
void (*_TclFreeObj) (Tcl_Obj * objPtr);

void (*_Tcl_FindExecutable)(const char * argv0);
} _Tcl_Dynload;

/*
 *	This is the first TCL function that is ever called (out of init_tkintr):
 *
 *	We use it to trigger the dynamic loading of Tk/Tcl
 */
extern void Tcl_FindExecutable(const char * argv0);

/*
 *	Do this so we don't invoke the Python "ctype" hack
 */
#undef	isupper
#define	isupper(Character)  (((Character) >= 'A') && ((Character) <= 'Z'))
#endif /* _TCL */
