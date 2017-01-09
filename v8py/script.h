#include <Python.h>
#include <v8.h>

#include "v8py.h"

typedef struct {
    PyObject_HEAD
    Persistent<UnboundScript> script;
    PyObject *source;
} script_c;
extern PyTypeObject script_type;

int script_type_init();
PyObject *script_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);
void script_dealloc(script_c *self);

PyObject *construct_script_name(Local<Value> js_name, int id);
extern PyObject *script_loader;
