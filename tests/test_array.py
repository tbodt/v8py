import pytest

@pytest.fixture
def context(context):
    context.eval('a = [1, 2, 3]')
    return context

@pytest.fixture
def a(context):
    return context.a

def test_length(a):
    assert len(a) == 3
    
def test_getitem(a):
    assert a[0] == 1
    assert a[1] == 2
    assert a[2] == 3
    with pytest.raises(IndexError):
        a[3]

def test_listify(a):
    assert list(a) == [1, 2, 3]

def test_concat(a):
    assert a + [4] == [1, 2, 3, 4]

def test_repeat(a):
    assert a * 2 == [1, 2, 3, 1, 2, 3]

def test_setitem(context, a):
    a[0] = 2
    assert context.eval('a[0]') == 2
    with pytest.raises(IndexError):
        a[4] = 1
