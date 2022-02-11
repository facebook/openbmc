from ply import lex, yacc


tokens = (
    "SYM",
    "NUM",
    "PLUS",
    "MINUS",
    "MULTIPLY",
    "DIVIDE",
    "PAR_OPEN",
    "PAR_END",
    "LIST_OPEN",
    "LIST_END",
    "LIST_SEP",
    "EQUAL",
    "SEMICOLON",
)

t_SYM = r"[A-Za-z_][:A-Za-z0-9_]*"


def t_NUM(t):
    r"[0-9]+"
    t.value = int(t.value)
    return t


t_PLUS = r"\+"
t_MINUS = r"\-"
t_MULTIPLY = r"\*"
t_DIVIDE = r"\/"
t_PAR_OPEN = r"\("
t_PAR_END = r"\)"
t_LIST_OPEN = r"\["
t_LIST_END = r"\]"
t_LIST_SEP = r","
t_EQUAL = r"="
t_SEMICOLON = r";"

t_ignore = " \t\n"


def t_error(t):
    print(("Illegal character '%s'" % t.value[0]))
    t.lexer.skip(1)


start = "expression"
precedence = (("left", "MULTIPLY", "DIVIDE", "PLUS", "MINUS"),)


def p_list_elements(p):
    "list_elements : list_elements LIST_SEP expression"
    p[0] = p[1] + [p[3]]


def p_list_elements_tail(p):
    "list_elements : expression"
    p[0] = [p[1]]


def p_expression_binding(p):
    "expression : SYM EQUAL expression SEMICOLON expression"
    p[0] = {"type": "bind", "name": p[1], "bound": p[3], "inner": p[5]}


def p_expression_list(p):
    "expression : LIST_OPEN list_elements LIST_END"
    p[0] = {"type": "list", "content": p[2]}


def p_expression_apply(p):
    "expression : SYM PAR_OPEN expression PAR_END"
    p[0] = {"type": "apply", "name": p[1], "inner": p[3]}


def p_expression_plus(p):
    "expression : expression PLUS expression"
    p[0] = {"type": "infix", "op": p[2], "left": p[1], "right": p[3]}


def p_expression_minus(p):
    "expression : expression MINUS expression"
    p[0] = {"type": "infix", "op": p[2], "left": p[1], "right": p[3]}


def p_expression_multiply(p):
    "expression : expression MULTIPLY expression"
    p[0] = {"type": "infix", "op": p[2], "left": p[1], "right": p[3]}


def p_expression_divide(p):
    "expression : expression DIVIDE expression"
    p[0] = {"type": "infix", "op": p[2], "left": p[1], "right": p[3]}


def p_expression_ident(p):
    "expression : SYM"
    p[0] = {"type": "ident", "name": p[1]}


def p_expression_const(p):
    "expression : NUM"
    p[0] = {"type": "const", "value": p[1]}


def p_error(p):
    if p:
        print(("Unexpected token: {}".format(p)))
    else:
        print("Unexpectedly reached end of input")


lexer = lex.lex(errorlog=yacc.NullLogger())
parser = yacc.yacc(errorlog=yacc.NullLogger())


def parse_expr(s):
    return parser.parse(s)
