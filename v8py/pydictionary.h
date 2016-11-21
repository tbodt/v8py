#include <Python.h>
#include <v8.h>

using namespace v8;

void py_dictionary_init();
Local<Object> py_dictionary_get_proxy(PyObject *dict, Local<Context> context);

#define DICT_INTERNAL_FIELDS 2

