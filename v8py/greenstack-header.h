/* vim:set noet ts=8 sw=8 : */

/* Stack greenstack object interface */

#ifndef Py_GREENSTACK_H
#define Py_GREENSTACK_H

#include <Python.h>

#ifdef GREENSTACK_MODULE
#include "libcoro/coro.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define GREENSTACK_VERSION "0.6"

typedef struct _greenstack {
	PyObject_HEAD
	void *stack;
	size_t stack_size;
	struct _greenstack* parent;
	PyObject* run_info;
	struct _frame* top_frame;
	PyObject* weakreflist;
	PyObject* dict;
#ifdef GREENSTACK_MODULE
	/* This field is hidden from the C API because its contents are
	 * platform-dependent and require a bunch of macros to be defined,
	 * which I don't want to make anyone do. */
	coro_context context;
#endif
} PyGreenstack;

#define PyGreenstack_Check(op)      PyObject_TypeCheck(op, &PyGreenstack_Type)
#define PyGreenstack_MAIN(op)       (((PyGreenstack*)(op))->stack == (char *) 1)
#define PyGreenstack_STARTED(op)    (((PyGreenstack*)(op))->stack_size != 0)
#define PyGreenstack_ACTIVE(op)     (((PyGreenstack*)(op))->stack != NULL)
#define PyGreenstack_GET_PARENT(op) (((PyGreenstack*)(op))->parent)

#if (PY_MAJOR_VERSION == 2 && PY_MINOR_VERSION >= 7) || (PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 1) || PY_MAJOR_VERSION > 3
#define GREENSTACK_USE_PYCAPSULE
#endif

typedef void (*switchwrapperfunc)(void *);
typedef void (*stateinitfunc)(void);

struct _statehandler {
	switchwrapperfunc wrapper;
	stateinitfunc stateinit;
	struct _statehandler *next;
};

#define PyGreenstack_CALL_SWITCH(next_void) { \
	struct _statehandler *next = (struct _statehandler *) next_void; \
	next->wrapper(next->next); \
}

/* C API functions */

/* Total number of symbols that are exported */
#define PyGreenstack_API_pointers  10

#define PyGreenstack_Type_NUM       0
#define PyExc_GreenstackError_NUM   1
#define PyExc_GreenstackExit_NUM    2

#define PyGreenstack_New_NUM        3
#define PyGreenstack_GetCurrent_NUM 4
#define PyGreenstack_Throw_NUM      5
#define PyGreenstack_Switch_NUM     6
#define PyGreenstack_SetParent_NUM  7
#define PyGreenstack_AddStateHandler_NUM 8

#ifndef GREENSTACK_MODULE
/* This section is used by modules that uses the greenstack C API */
static void **_PyGreenstack_API = NULL;

#define PyGreenstack_Type (*(PyTypeObject *) _PyGreenstack_API[PyGreenstack_Type_NUM])

#define PyExc_GreenstackError \
	((PyObject *) _PyGreenstack_API[PyExc_GreenstackError_NUM])

#define PyExc_GreenstackExit \
	((PyObject *) _PyGreenstack_API[PyExc_GreenstackExit_NUM])

/*
 * PyGreenstack_New(PyObject *args)
 *
 * greenstack.greenstack(run, parent=None)
 */
#define PyGreenstack_New \
	(* (PyGreenstack * (*)(PyObject *run, PyGreenstack *parent)) \
	 _PyGreenstack_API[PyGreenstack_New_NUM])

/*
 * PyGreenstack_GetCurrent(void)
 *
 * greenstack.getcurrent()
 */
#define PyGreenstack_GetCurrent \
	(* (PyGreenstack * (*)(void)) _PyGreenstack_API[PyGreenstack_GetCurrent_NUM])

/*
 * PyGreenstack_Throw(
 *         PyGreenstack *greenstack,
 *         PyObject *typ,
 *         PyObject *val,
 *         PyObject *tb)
 *
 * g.throw(...)
 */
#define PyGreenstack_Throw \
	(* (PyObject * (*) \
	    (PyGreenstack *self, PyObject *typ, PyObject *val, PyObject *tb)) \
	 _PyGreenstack_API[PyGreenstack_Throw_NUM])

/*
 * PyGreenstack_Switch(PyGreenstack *greenstack, PyObject *args)
 *
 * g.switch(*args, **kwargs)
 */
#define PyGreenstack_Switch \
	(* (PyObject * (*)(PyGreenstack *greenstack, PyObject *args, PyObject *kwargs)) \
	 _PyGreenstack_API[PyGreenstack_Switch_NUM])

/*
 * PyGreenstack_SetParent(PyObject *greenstack, PyObject *new_parent)
 *
 * g.parent = new_parent
 */
#define PyGreenstack_SetParent \
	(* (int (*)(PyGreenstack *greenstack, PyGreenstack *nparent)) \
	 _PyGreenstack_API[PyGreenstack_SetParent_NUM])

/*
 * PyGreenstack_AddStateHandler(switchwrapperfunc wrapper, stateinitfunc stateinit)
 */
#define PyGreenstack_AddStateHandler \
	(* (int (*)(switchwrapperfunc wrapper, stateinitfunc stateinit)) \
	_PyGreenstack_API[PyGreenstack_AddStateHandler_NUM])

/* Macro that imports greenstack and initializes C API */
#ifdef GREENSTACK_USE_PYCAPSULE
#define PyGreenstack_Import() \
{ \
	_PyGreenstack_API = (void**)PyCapsule_Import("greenstack._C_API", 0); \
}
#else
#define PyGreenstack_Import() \
{ \
	PyObject *module = PyImport_ImportModule("greenstack"); \
	if (module != NULL) { \
		PyObject *c_api_object = PyObject_GetAttrString( \
			module, "_C_API"); \
		if (c_api_object != NULL && PyCObject_Check(c_api_object)) { \
			_PyGreenstack_API = \
				(void **) PyCObject_AsVoidPtr(c_api_object); \
			Py_DECREF(c_api_object); \
		} \
		Py_DECREF(module); \
	} \
}
#endif

#endif /* GREENSTACK_MODULE */

#ifdef __cplusplus
}
#endif
#endif /* !Py_GREENSTACK_H */
