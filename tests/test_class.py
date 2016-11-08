import pytest

@pytest.fixture(params=['new', 'old'])
def class_ctx(request, context):
    if request.param == 'new':
        class Test(object):
            def method(self):
                return 'thing'
        context.glob.Test = Test
    else:
        class Test:
            def method(self):
                return 'thing'
        context.glob.Test = Test
    return context
    
def test_call_method(class_ctx):
    assert class_ctx.eval('new Test().method()') == 'thing'

def test_call_method_on_wrong_object(class_ctx):
    with pytest.raises(Exception):
        class_ctx.eval('new Test().method.call({})')

def test_construct_without_new(class_ctx):
    assert class_ctx.eval('Test()') == None

def test_class_name(class_ctx):
    assert class_ctx.eval('Test.name') == 'Test'
    assert class_ctx.eval('new Test().toString()') == '[object Test]'

def test_calling_methods_from_python(class_ctx):
    class_ctx.eval('test = new Test()')
    assert class_ctx.glob.test.method() == 'thing'
