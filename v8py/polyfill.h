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

#endif
