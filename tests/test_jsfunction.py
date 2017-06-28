from v8py import JSFunction

def test_jsfunction(context):
    f = context.eval('Math.sqrt')
    assert isinstance(f, JSFunction)
