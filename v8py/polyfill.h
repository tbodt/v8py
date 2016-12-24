#ifndef POLYFILL_H
#define POLYFILL_H

#include <Python.h>

// on both, demand trailing semicolon
#define PyErr_PROPAGATE_RET(x, retval) \
    if (x == NULL) { \
        return retval; \
    } else \
        (void) NULL
#define PyErr_PROPAGATE(x) PyErr_PROPAGATE_RET(x, NULL)
#define PyErr_PROPAGATE_(x) PyErr_PROPAGATE_RET(x, -1)

inline extern int PyObject_GenericHasAttr(PyObject *obj, PyObject *name) {
    PyObject *value = PyObject_GenericGetAttr(obj, name);
    if (value == NULL) {
        PyErr_Clear();
        return 0;
    }
    Py_DECREF(value);
    return 1;
}

#define PyClass_GET_BASES(cls) (((PyClassObject *) cls)->cl_bases)

inline extern int PyString_StartsWithString(PyObject *str, const char *prefix) {
    PyObject *result = PyObject_CallMethod(str, (char *) "startswith", (char *) "s", prefix);
    int retval = (result == Py_True);
    Py_DECREF(result);
    return retval;
}

#endif

#if PY_MAJOR_VERSION >= 3

#define PyInstance_Check(x) false
#define PyClass_Check(x) false

#define PyString_Check PyUnicode_Check
#define PyString_InternFromString PyUnicode_InternFromString

#define nb_nonzero nb_bool

#endif
