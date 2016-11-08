import pytest
from v8py import Context

@pytest.fixture
def context():
    return Context()

@pytest.fixture(params=['new', 'old'])
def Test(request):
    if request.param == 'new':
        class Test(object):
            def method(self):
                return 'thing'
        return Test
    else:
        class Test:
            def method(self):
                return 'thing'
        return Test

@pytest.fixture
def class_ctx(Test, context):
    context.glob.Test = Test
    return context
    
