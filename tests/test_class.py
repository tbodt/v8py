import pytest

@pytest.fixture(params=['new', 'old'])
def Test(request):
    # Sadly, I have to have two blocks of almost identical code to test both
    # old and new style classes.
    if request.param == 'new':
        class Test(object):
            def method(self):
                return 'thing'
            def __getitem__(self, name):
                if name == 'getitem':
                    return 'value'
            @staticmethod
            def static_method():
                return 'static'
            @classmethod
            def class_method(cls):
                return 'class'
        return Test
    else:
        class Test:
            def method(self):
                return 'thing'
            def __getitem__(self, name):
                if name == 'getitem':
                    return 'value'
            @staticmethod
            def static_method():
                return 'static'
            @classmethod
            def class_method(cls):
                return 'class'
        return Test

@pytest.fixture
def context(Test, context):
    context.glob.Test = Test
    return context
    
def test_call_method(context):
    assert context.eval('new Test().method()') == 'thing'

def test_call_method_on_wrong_object(context):
    with pytest.raises(Exception):
        context.eval('new Test().method.call({})')

def test_class_name(context):
    assert context.eval('Test.name') == 'Test'
    assert context.eval('new Test().toString()') == '[object Test]'

def test_calling_methods_from_python(context):
    context.eval('test = new Test()')
    assert context.eval('test.method()') == 'thing'

def test_object_passthrough(context):
    test = context.eval('test = new Test()')
    assert test is context.test

def test_getter(context):
    test = context.eval('test = new Test()')
    test.foo = 'bar'
    assert context.eval('test.foo') == 'bar'
    assert context.eval('test.getitem') == 'value'

def test_static_method(context):
    assert context.eval('Test.static_method()') == 'static'
    assert context.eval('Test.class_method()') == 'class'

