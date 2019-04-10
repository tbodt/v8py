from v8py import Null
import sys


def test_convert_to_py(context):
    assert context.eval('"Hello, world!"') == 'Hello, world!'
    assert context.eval('124987234789413') == 124987234789413
    assert context.eval('189329854.18894627') == 189329854.18894627
    assert context.eval('true') == True
    assert context.eval('false') == False
    assert context.eval('null') == Null
    assert repr(Null) == 'Null'
    assert bool(Null) is False
    assert not Null
    assert context.eval('undefined') == None
    assert context.eval('[1, 2, 3]') == [1, 2, 3]
    assert context.eval('({foo: "bar"})') == {'foo': 'bar'}
    assert context.eval('"Hello, world!"') == 'Hello, world!'
    long_text = "The missile knows where it is at all times. It knows this because it knows where it isn't. " \
                "By subtracting where it is from where it isn't, or where it isn't from where it is - whichever is " \
                "greater - it obtains a difference or deviation. The guidance subsystem uses deviations to generate" \
                " corrective commands to drive the missile from a position where it is to a position where it isn't," \
                " and arriving at a position that it wasn't, it now is. Consequently, the position where it is is" \
                " now the position that it wasn't, and if follows that the position that it was is now the position" \
                " that it isn't. In the event that the position that it is in is not the position that it wasn't," \
                " the system has acquired a variation. The variation being the difference between where the missile" \
                " is and where it wasn't. If variation is considered to be a significant factor, it too may be" \
                " corrected by the GEA. However, the missile must also know where it was. The missile guidance" \
                " computer scenario works as follows: Because a variation has modified some of the information" \
                " that the missile has obtained, it is not sure just where it is. However, it is sure where it" \
                " isn't, within reason, and it know where it was. It now subtracts where it should be from where" \
                " it wasn't, or vice versa. And by differentiating this from the algebraic sum of where it" \
                " shouldn't be and where it was, it is able to obtain the deviation and its variation, which" \
                " is called error."
    assert context.eval('"' + long_text + '"') == long_text

    if sys.version_info.major >= 3:
        assert context.eval('new Uint8Array([1, 2, 3])') == bytes([1, 2, 3])


def test_convert_to_js(context):
    context.glob.foo = 'Hello, world!'
    assert context.eval('foo == "Hello, world!"')
    context.glob.foo = 13324235
    assert context.eval('foo == 13324235')
    context.glob.foo = 189329854.18894627
    assert context.eval('foo == 189329854.18894627')
    context.glob.foo = True
    assert context.eval('foo == true')
    context.glob.foo = False
    assert context.eval('foo == false')
    context.glob.foo = None
    assert context.eval('foo == null')
    context.glob.foo = None
    assert context.eval('foo == undefined')
    context.glob.foo = [1, 2, 3]
    # In JavaScript, [1,2,3] == [1,2,3] is false. Don't ask.
    assert context.eval('foo.length == 3')
    assert context.eval('foo[0] == 1')
    assert context.eval('foo[1] == 2')
    assert context.eval('foo[2] == 3')
    context.glob.foo = {'foo': 'bar'}
    assert context.eval('foo.foo == "bar"')
