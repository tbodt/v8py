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
