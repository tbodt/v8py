#ifndef CLASS_TEMPLATE_H
#define CLASS_TEMPLATE_H

using namespace v8;

typedef struct {
    PyObject_HEAD
    PyObject *cls;
    PyObject *cls_name;
    Persistent<FunctionTemplate> *templ;
} py_class;
int py_class_type_init();

void py_class_dealloc(py_class *self);
PyObject *py_class_new(PyObject *cls);
PyObject *py_class_to_template(PyObject *cls);
Local<Function> py_class_get_constructor(py_class *self, Local<Context> context);
Local<Object> py_class_create_js_object(py_class *self, PyObject *py_object, Local<Context> context);

extern PyTypeObject py_class_type;

void py_class_construct_callback(const FunctionCallbackInfo<Value> &info);
void py_class_method_callback(const FunctionCallbackInfo<Value> &info);

#define OBJ_MAGIC ((void *) 0xDaC1a550)

#endif
