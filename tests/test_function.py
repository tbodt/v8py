def test_function(context):
    def len_args(*args):
        return len(args)
    context.glob.len_args = len_args
    assert context.eval('len_args(1, 2, 3)') == 3

