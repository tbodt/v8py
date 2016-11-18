#ifndef JSOBJECT_H
#define JSOBJECT_H

#include <Python.h>
#include <v8.h>

using namespace v8;

typedef struct {
    PyObject_HEAD
    Persistent<Object> object;
    Persistent<Context> context;
} js_object;
extern PyTypeObject js_object_type;
int js_object_type_init();

js_object *js_object_new(Local<Object> object, Local<Context> context);
PyObject *js_object_fake_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);
void js_object_dealloc(js_object *self);

PyObject *js_object_getattro(js_object *self, PyObject *name);
int js_object_setattro(js_object *self, PyObject *name, PyObject *value);
Py_ssize_t js_object_length(js_object *self);
PyObject *js_object_dir(js_object *self);
PyObject *js_object_repr(js_object *self);


typedef struct {
    PyObject_HEAD
    Persistent<Object> object;
    Persistent<Context> context;
} js_dictionary;
extern PyTypeObject js_dictionary_type;
int js_dictionary_type_init();

PyObject *js_dictionary_keys(js_dictionary *self);
Py_ssize_t js_dictionary_length(js_dictionary *self);
PyObject *js_dictionary_getitem(js_dictionary *self, PyObject *key);
int js_dictionary_setitem(js_dictionary *self, PyObject *key, PyObject *value);
PyObject *js_dictionary_iter(js_dictionary *self);
PyObject *js_dictionary_repr(js_dictionary *self);

typedef struct {
    PyObject_HEAD
    Persistent<Object> object;
    Persistent<Context> context;
    Persistent<Value> js_this;
} js_function;
extern PyTypeObject js_function_type;
int js_function_type_init();

PyObject *js_function_call(js_function *self, PyObject *args, PyObject *kwargs);
void js_function_dealloc(js_function *self);

#endif
