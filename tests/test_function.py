from v8py import JSFunction

def test_function(context):
    def len_args(*args):
        return len(args)
    context.glob.len_args = len_args
    assert context.eval('len_args(1, 2, 3)') == 3
    assert context.eval('len_args.name') == 'len_args'

def test_jsfunction(context):
    f = context.eval('Math.sqrt')
    assert isinstance(f, JSFunction)
