import pytest

def test_call_method(class_ctx):
    assert class_ctx.eval('new Test().method()') == 'thing'

def test_call_method_on_wrong_object(class_ctx):
    with pytest.raises(Exception):
        class_ctx.eval('new Test().method.call({})')

def test_class_name(class_ctx):
    assert class_ctx.eval('Test.name') == 'Test'
    assert class_ctx.eval('new Test().toString()') == '[object Test]'

def test_calling_methods_from_python(class_ctx):
    class_ctx.eval('test = new Test()')
    assert class_ctx.eval('test.method()') == 'thing'

def test_magic_getter(class_ctx):
    test = class_ctx.eval('test = new Test()')
    test.foo = 'bar'
    assert class_ctx.eval('test.foo') == 'bar'
