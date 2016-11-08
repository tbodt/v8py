def test_glob(context):
    context.eval('foo = "bar"')
    assert context.glob.foo == 'bar'

def test_getattr(context):
    context.foo = 'bar'
    assert context.foo == 'bar'
    assert context.glob.foo == 'bar'
    assert context.eval('foo') == 'bar'

def test_getitem(context):
    context['foo'] = 'bar'
    assert context['foo'] == 'bar'
    assert context.glob['foo'] == 'bar'
    assert context.eval('foo') == 'bar'
