#include <Python.h>
#include <v8.h>

#include "v8py.h"
#include "script.h"
#include "pyclass.h"
#include "convert.h"

PyTypeObject js_exception_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
};
int js_exception_type_init() {
    js_exception_type.tp_name = "v8py.JSException";
    js_exception_type.tp_base = (PyTypeObject *) PyExc_Exception;
    js_exception_type.tp_basicsize = sizeof(js_exception);
    js_exception_type.tp_flags = Py_TPFLAGS_DEFAULT;
    js_exception_type.tp_doc = "";

    js_exception_type.tp_dealloc = (destructor) js_exception_dealloc;
    return PyType_Ready(&js_exception_type);
}

PyObject *js_exception_new(Local<Value> exception, Local<Message> message) {
    HandleScope hs(isolate);
    Local<Context> no_ctx;
    js_exception *self = (js_exception *) js_exception_type.tp_alloc(&js_exception_type, 0);
    PyErr_PROPAGATE(self);
    self->exception.Reset(isolate, exception);
    self->message.Reset(isolate, message);

    PyObject *py_message = py_from_js(exception->ToString(), no_ctx);
    PyErr_PROPAGATE(py_message);
#if PY_MAJOR_VERSION < 3
    self->base.message = py_message;
    Py_INCREF(self->base.message);
#endif
    self->base.args = PyTuple_New(1);
    PyErr_PROPAGATE(self->base.args);
    PyTuple_SetItem(self->base.args, 0, py_message);
    return (PyObject *) self;
}

void js_exception_dealloc(js_exception *self) {
    self->exception.Reset();
    self->message.Reset();
    Py_TYPE(self)->tp_free((PyObject *) self);
}

PyTypeObject js_terminated_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
};
int js_terminated_type_init() {
    js_terminated_type.tp_name = "v8py.JavaScriptTerminated";
    js_terminated_type.tp_base = (PyTypeObject *) PyExc_Exception;
    js_terminated_type.tp_basicsize = sizeof(PyBaseExceptionObject);
    js_terminated_type.tp_flags = Py_TPFLAGS_DEFAULT;
    js_terminated_type.tp_doc = "";
    return PyType_Ready(&js_terminated_type);
}

// This is the only Python C API function I need that is explicitly not in the
// public API. All I can say is that this function is very important to Python
// and is therefore very unlikely to go away or change any time soon.
extern "C" {
    PyAPI_FUNC(struct _frame *) PyFrame_New(PyThreadState *, PyCodeObject *, PyObject *, PyObject *);
}

void py_throw_js(Local<Value> js_exc, Local<Message> js_message) {
    if (js_exc->IsObject() && js_exc.As<Object>()->InternalFieldCount() == OBJECT_INTERNAL_FIELDS) {
        Local<Object> exc_object = js_exc.As<Object>();
        PyObject *exc_type = (PyObject *) exc_object->GetInternalField(2).As<External>()->Value();
        PyObject *exc_value = (PyObject *) exc_object->GetInternalField(1).As<External>()->Value();
        PyObject *exc_traceback = (PyObject *) exc_object->GetInternalField(3).As<External>()->Value();
        PyErr_Restore(exc_type, exc_value, exc_traceback);
    } else {
        PyObject *exception = js_exception_new(js_exc, js_message);
        if (exception == NULL) {
            return;
        }
        PyErr_SetObject((PyObject *) &js_exception_type, exception);
    }

    Local<StackTrace> stack_trace = js_message->GetStackTrace();
    for (int i = 0; i < stack_trace->GetFrameCount(); i++) {
        Local<StackFrame> stack_frame = stack_trace->GetFrame(i);

        // Not documented, but arguments to PyCode_NewEmpty go through
        // PyUnicode_FromString, so they're UTF-8 encoded
        Local<String> js_func_name = stack_frame->GetFunctionName();
        // whenever I remember the terrible bug introduced by not having
        // space for the null terminator, I get chills
        char *func_name = (char *) malloc(js_func_name->Utf8Length() + 1);
        if (func_name == NULL) {
            // fuck it, plenty of shit is probably already breaking anyway
            func_name = (char *) "help i'm trapped in a computer that's out of memory";
        } else {
            js_func_name->WriteUtf8(func_name);
        }

        PyObject *script_name_unicode = construct_script_name(stack_frame->GetScriptName(), stack_frame->GetScriptId());
        if (script_name_unicode == NULL) return;
        PyObject *script_name_string = PyUnicode_AsUTF8String(script_name_unicode);
        if (script_name_string == NULL) return;
#if PY_MAJOR_VERSION >= 3
        // I can't use the macro form because it uses assert and I
        // redefined assert to not be an expression
        char *script_name = PyBytes_AsString(script_name_string);
#else
        char *script_name = PyString_AS_STRING(script_name_string);
#endif

        PyCodeObject *code = PyCode_NewEmpty(script_name, func_name, stack_frame->GetLineNumber());
        Py_DECREF(script_name_string);
        free(func_name);
        if (code == NULL) return;

        PyObject *globals = PyDict_New();
        if (globals == NULL) return;
        // set the loader and name
        PyDict_SetItemString(globals, "__loader__", script_loader);
        PyDict_SetItemString(globals, "__name__", script_name_unicode);
        Py_DECREF(script_name_unicode);

        struct _frame *frame = PyFrame_New(PyThreadState_GET(), code, globals, NULL);
        if (frame == NULL) return;

        Py_DECREF(globals);
        Py_DECREF(code);
        PyTraceBack_Here(frame);
    }
}

void js_throw_py() {
    Local<Context> context = isolate->GetCurrentContext();
    PyObject *exc_type, *exc_value, *exc_traceback;
    PyErr_Fetch(&exc_type, &exc_value, &exc_traceback);
    PyErr_NormalizeException(&exc_type, &exc_value, &exc_traceback);
    Local<Object> exception;
    if (PyObject_TypeCheck(exc_value, &js_exception_type)) {
        exception = ((js_exception *) exc_value)->exception.Get(isolate).As<Object>();
    } else {
        exception = js_from_py(exc_value, context).As<Object>();
        exception->SetInternalField(2, External::New(isolate, exc_type));
        exception->SetInternalField(3, External::New(isolate, exc_traceback));
    }
    isolate->ThrowException(exception);
}
