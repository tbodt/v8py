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
