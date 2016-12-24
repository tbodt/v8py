import sys
import pytest
import types

if sys.version_info.major < 3:
    params = ['new', 'old']
else:
    params = ['new']
@pytest.fixture(params=params)
def Test(request):
    # Sadly, I have to have two blocks of almost identical code to test both
    # old and new style classes.
    if request.param == 'new':
        class Test(object):
            def __init__(self):
                self.sets = 0
            def __len__(self):
                return 3
            def __getitem__(self, i):
                return len(self) - i
            def __setitem__(self, key, value):
                self.sets += 1
            def __delitem__(self, key):
                print('delitem called')
                self.sets = 0
        return Test
    else:
        class Test:
            def __init__(self):
                self.sets = 0
            def __len__(self):
                return 3
            def __getitem__(self, i):
                return len(self) - i
            def __setitem__(self, key, value):
                self.sets += 1
            def __delitem__(self, key):
                print('delitem called')
                self.sets = 0
        return Test

@pytest.fixture
def context(Test, context):
    context.Test = Test
    context.test = Test()
    return context
    
def test_getitem(context):
    assert context.eval('test[1]') == 2

def test_setitem(context, Test):
    test = context.eval('test')
    assert test.sets == 0
    context.eval('test[1] = 2')
    assert test.sets == 1

def test_delitem(context):
    test = context.eval('test')
    test.sets = 3
    context.eval('delete test[2]')
    assert test.sets == 0

def test_query(context):
    descriptor = context.eval('Object.getOwnPropertyDescriptor(test, 0)')
    assert descriptor.writable
    assert not descriptor.enumerable
    assert descriptor.configurable
    assert descriptor.value == 3

def test_enumerate(context, Test):
    name_list = context.eval('Object.getOwnPropertyNames(test)')
    assert "2" in name_list
    assert len(name_list) == 3
