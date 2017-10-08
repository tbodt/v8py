from v8py import JSFunction, JSObject

def test_function(context):
    def len_args(*args):
        return len(args)
    context.glob.len_args = len_args
    assert context.eval('len_args(1, 2, 3)') == 3
    assert context.eval('len_args.name') == 'len_args'

def test_jsfunction(context):
    f = context.eval('Math.sqrt')
    assert isinstance(f, JSFunction)


def test_new_keyword(context):
    context.eval("""
        function NewKeywordTest(who)
        {
            this.who = who;
        }
        
        NewKeywordTest.prototype.test = function()
        {
            return "Hello, " + this.who + "!";
        }
    """)

    f = context.glob.NewKeywordTest
    assert isinstance(f, JSFunction)

    instance = f.new("world")
    assert isinstance(f, JSObject)

    res = instance.test()
    assert res == "Hello, world!"
