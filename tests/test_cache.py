def test_cache(context):
    class Test(object):
        def get_self(self): return self
    context.expose(Test)
    context.eval('t = new Test()')
    assert context.eval('t === t.get_self()')
