#ifndef POLYFILL_H
#define POLYFILL_H

#include <Python.h>

inline extern int PyObject_GenericHasAttr(PyObject *obj, PyObject *name) {
    PyObject *value = PyObject_GenericGetAttr(obj, name);
    if (value == NULL) {
        PyErr_Clear();
        return 0;
    }
    Py_DECREF(value);
    return 1;
}

#endif
