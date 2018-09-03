import pytest
from v8py import JSFunction, JSObject, new

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
    
        test_int = 5;
        test_string = 5;
        test_plain_object = {};
    
        function NewKeywordTest(who)
        {
            this.who = who;
        }
        
        NewKeywordTest.prototype.test = function()
        {
            return "Hello, " + this.who + "!";
        }
        
        function MoreThan16Arguments(a1, a2, a3, a4, b1, b2, b3, b4, c1, c2, c3, c4, d1, d2, d3, d4, e1, e2, e3, e4)
        {
            this.data = [a1, a2, a3, a4, b1, b2, b3, b4, c1, c2, c3, c4, d1, d2, d3, d4, e1, e2, e3, e4];
        }
        
        function MoreThan16Arguments2(a1, a2, a3, a4, b1, b2, b3, b4, c1, c2, c3, c4, d1, d2, d3, d4, e1, e2, e3, e4)
        {
            return [a1, a2, a3, a4, b1, b2, b3, b4, c1, c2, c3, c4, d1, d2, d3, d4, e1, e2, e3, e4];
        }
    """)

    f = context.glob.NewKeywordTest
    assert isinstance(f, JSFunction)

    instance = new(f, "world")
    assert isinstance(f, JSObject)

    with pytest.raises(TypeError):
        new(context.glob.test_int, "world")

    with pytest.raises(TypeError):
        new(context.glob.test_string, "world")

    with pytest.raises(TypeError):
        new(context.glob.test_plain_object, "world")

    with pytest.raises(TypeError):
        new()

    with pytest.raises(TypeError):
        new(None)

    res = instance.test()
    assert res == "Hello, world!"

    args = [2, 4, 6, 23, 54, 2, 8, 43, -1, 100, 200, 300, 3, 4, 5, 23, 5, 38, 10, 29]
    instance = new(context.glob.MoreThan16Arguments, *args)
    assert instance.data == args
    assert context.glob.MoreThan16Arguments2(*args) == args
