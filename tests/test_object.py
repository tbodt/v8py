import pytest

@pytest.fixture
def obj(context):
    context.eval('o = {}')
    context.eval('o.__proto__ = {}')
    return context.eval('o')

def test_attrs(obj):
    obj.foo = 'bar'
    assert obj.foo == 'bar'
def test_items(obj):
    obj['foo'] = 'bar'
    assert obj['foo'] == 'bar'

def test_dir(context, obj):
    obj.foo = 'bar'
    assert dir(obj) == ['foo']

def test_iter(context, obj):
    obj.foo = 'bar'
    assert list(obj) == ['foo']

def test_str(obj):
    assert str(obj) == '[object Object]'
    assert repr(obj) == '[object Object]'
