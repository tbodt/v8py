#ifndef CLASS_TEMPLATE_H
#define CLASS_TEMPLATE_H

using namespace v8;

typedef struct {
    PyObject_HEAD
    PyObject *cls;
    PyObject *cls_name;
    Persistent<FunctionTemplate> *templ;
} py_class_template;
int py_class_template_type_init();

void py_class_template_dealloc(py_class_template *self);
PyObject *py_class_template_new(PyObject *cls);
PyObject *py_class_to_template(PyObject *cls);
Local<Function> py_class_get_constructor(py_class_template *self, Local<Context> context);

extern PyTypeObject py_class_template_type;

void py_class_construct_callback(const FunctionCallbackInfo<Value> &info);
void py_class_method_callback(const FunctionCallbackInfo<Value> &info);

#define OBJ_MAGIC ((void *) 0xDaC1a550)

#endif
