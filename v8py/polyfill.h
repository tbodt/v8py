#ifndef POLYFILL_H
#define POLYFILL_H

#include <Python.h>

// on both, demand trailing semicolon
#define PyErr_PROPAGATE(x) \
    if (x == NULL) { \
        return NULL; \
    } else \
        (void) NULL
#define PyErr_PROPAGATE_(x) \
    if (x == NULL) { \
        return -1; \
    } else \
        (void) NULL

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

#endif
