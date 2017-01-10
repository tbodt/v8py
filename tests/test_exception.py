import pytest
from v8py import JSException

class CustomError(Exception):
    def method(self):
        return 'method'

@pytest.fixture(params=[ValueError, CustomError])
def ErrorClass(request):
    return request.param

@pytest.fixture
def context(context, ErrorClass):
    context.TestError = ErrorClass
    def throw_exception():
        raise ErrorClass
    context.throw_exception = throw_exception
    return context

def test_javascript_to_python(context, ErrorClass):
    with pytest.raises(JSException):
        context.eval('throw new Error()')

def test_python_to_javascript(context, ErrorClass):
    context.eval("""
try {
    throw_exception();
} catch (exc) {
    this.exc = exc;
}
    """)
    assert isinstance(context.exc, ErrorClass)
    assert context.eval('exc instanceof TestError')
    
def assert_is_js_frame(frame, script):
    print(frame.f_globals)
    assert type(frame.f_globals['__loader__']).__name__ == 'ScriptLoader'
    assert frame.f_globals['__name__'].startswith('javascript')
    assert frame.f_globals['__loader__'].get_source(frame.f_globals['__name__']) == script
    
def test_tracebacks(context, ErrorClass):
    with pytest.raises(ErrorClass) as exc_info:
        context.eval('throw_exception()')

    js_frame = exc_info.traceback[1].frame
    assert_is_js_frame(js_frame, 'throw_exception()')

def test_conservation(context, ErrorClass):
    context.eval('function f() { throw_exception(); }')
    with pytest.raises(ErrorClass) as exc_info:
        context.eval('f()')

    assert_is_js_frame(exc_info.traceback[1].frame, 'f()')
    assert_is_js_frame(exc_info.traceback[2].frame, 'function f() { throw_exception(); }')

def test_property_error(context, ErrorClass):
    class Test:
        @property
        def foo(self):
            raise ErrorClass
        @foo.setter
        def foo(self, thing):
            raise ErrorClass
    context.test = Test()
    with pytest.raises(ErrorClass):
        context.eval('test.foo')
    with pytest.raises(ErrorClass):
        context.eval('test.foo = "bar"')
