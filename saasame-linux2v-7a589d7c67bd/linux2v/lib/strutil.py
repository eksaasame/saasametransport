import re
re_boost_format_string_argument = re.compile(r"%\d+%")


def replace_boost_arg_with_py_arg(term):
    arg_number = int(term.split("%")[1]) - 1
    return "{" + str(arg_number) + "}"


def format_booststr(msg_fmt, msg_args):
    args_terms = re_boost_format_string_argument.findall(msg_fmt)
    if not args_terms or not msg_args:
        return msg_fmt

    left = re_boost_format_string_argument.split(msg_fmt)
    py_args_terms = list(map(replace_boost_arg_with_py_arg, args_terms))
    python_string = r"".join(a + b for a, b in zip(left, py_args_terms + [""]))
    return python_string.format(*msg_args)
