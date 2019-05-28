import fsc_parser


class InfixNode:
    def __init__(self, op, lhs, rhs):
        self.op = op
        self.lhs = lhs
        self.rhs = rhs

    def eval(self, ctx):
        return self.op.apply(self.lhs.eval(ctx), self.rhs.eval(ctx))

    def dbgeval(self, ctx):
        (lhv, lht) = self.lhs.dbgeval(ctx)
        (rhv, rht) = self.rhs.dbgeval(ctx)
        fv = self.op.apply(lhv, rhv)
        return (fv, "%s %s %s" % (lht, str(self.op), rht))

    def __str__(self):
        return str(self.lhs) + " " + str(self.op) + " " + str(self.rhs)


class ListNode:
    def __init__(self, inners):
        self.inners = inners

    def eval(self, ctx):
        return [i.eval(ctx) for i in self.inners]

    def dbgeval(self, ctx):
        evals = [i.dbgeval(ctx) for i in self.inners]
        fvs = [fv for (fv, dt) in evals]
        dts = [dt for (fv, dt) in evals]
        return (fvs, "[\n " + ",\n ".join(dts) + "]")

    def __str__(self):
        return "[" + ", ".join([str(i) for i in self.inners]) + "]"


class BindNode:
    def __init__(self, name, bindnode, innernode):
        self.name = name
        self.bindnode = bindnode
        self.innernode = innernode

    def eval(self, ctx):
        innerctx = ctx.copy()
        innerctx[self.name] = self.bindnode.eval(ctx)
        return self.innernode.eval(innerctx)

    def dbgeval(self, ctx):
        innerctx = ctx.copy()
        (bfv, bdt) = self.bindnode.dbgeval(ctx)
        innerctx[self.name] = bfv
        (ifv, idt) = self.innernode.dbgeval(innerctx)
        return (ifv, "{}[{}] = {};\n{}".format(self.name, bfv, bdt, idt))

    def __str__(self):
        return "{} = {};\n{}".format(self.name, str(self.bindnode), str(self.innernode))


class IdentNode:
    def __init__(self, name):
        self.name = name

    def eval(self, ctx):
        return ctx.get(self.name, None)

    def dbgeval(self, ctx):
        fv = ctx.get(self.name, None)
        return (fv, "{}={}".format(self.name, fv))

    def __str__(self):
        return self.name


class ConstNode:
    def __init__(self, value):
        self.value = value

    def eval(self, ctx):
        return self.value

    def dbgeval(self, ctx):
        return self.value

    def __str__(self):
        return str(self.value)


class ApplyNode:
    def __init__(self, name, op, inner):
        self.name = name
        self.op = op
        self.inner = inner

    def eval(self, ctx):
        return self.op.apply(self.inner.eval(ctx), ctx)

    def dbgeval(self, ctx):
        (iv, it) = self.inner.dbgeval(ctx)
        ft = self.name
        if hasattr(self.op, "dbgapply"):
            (fv, ft) = self.op.dbgapply(iv, ctx)
        else:
            fv = self.op.apply(iv, ctx)
        return (fv, "{}[{}]({})".format(ft, fv, it))

    def __str__(self):
        return self.name + "(" + str(self.inner) + ")"


def make_infix_node(ast_node, info, profiles):
    op = None
    if ast_node["op"] == "+":
        op = Sum()

    if ast_node["op"] == "-":
        op = Sub()

    if ast_node["op"] == "*":
        op = Mul()

    if ast_node["op"] == "/":
        op = Div()

    if not op:
        raise InvalidExpression("Bad infix operator: %s" % (ast_node["op"],))
    lhs_e = make_eval_node(ast_node["left"], info, profiles)
    rhs_e = make_eval_node(ast_node["right"], info, profiles)
    node = InfixNode(op, lhs_e, rhs_e)
    return node


def make_apply_node(ast_node, info, profiles):
    name = ast_node["name"]
    inner_e = make_eval_node(ast_node["inner"], info, profiles)
    applies = {"hold": Hold, "max": Max}
    constructor = applies.get(name)
    if constructor:
        op = constructor()
    else:
        # check for profile?
        if name not in profiles:
            raise InvalidExpression(
                "Not a built-in function or profile name: '%s'" % (ast_node["name"],)
            )
        controller = profiles[name]()
        op = ApplyProfile(name, controller)
    return ApplyNode(name, op, inner_e)


def make_ident_node(ast_node, info, profiles):
    info["ext_vars"].add(ast_node["name"])
    return IdentNode(ast_node["name"])


def make_bind_node(ast_node, info, profiles):
    name = ast_node["name"]
    bindnode = make_eval_node(ast_node["bound"], info, profiles)
    innernode = make_eval_node(ast_node["inner"], info, profiles)
    if name in info["ext_vars"]:
        info["ext_vars"].remove(name)
    return BindNode(name, bindnode, innernode)


def make_const_node(ast_node, info, profiles):
    return ConstNode(ast_node["value"])


def make_list_node(ast_node, info, profiles):
    return ListNode([make_eval_node(n, info, profiles) for n in ast_node["content"]])


def make_eval_node(ast_node, info, profiles):
    makers = {
        "infix": make_infix_node,
        "apply": make_apply_node,
        "ident": make_ident_node,
        "bind": make_bind_node,
        "list": make_list_node,
        "const": make_const_node,
    }
    maker = makers.get(ast_node["type"])
    return maker(ast_node, info, profiles)


def make_eval_tree(source, profiles):
    root_ast_node = fsc_parser.parse_expr(source)
    info = {"profiles": set(), "ext_vars": set()}
    eval_root = make_eval_node(root_ast_node, info, profiles)
    return (eval_root, info)


class InvalidExpression(Exception):
    pass


class Hold:
    """If no data is available, returns last known sample"""

    def __init__(self):
        self.last = None

    def dbgapply(self, inp, ctx):
        return (self.apply(inp), "hold[held={}]".format(self.last))

    def apply(self, inp, ctx):
        if inp is not None:
            self.last = inp
            return inp
        else:
            return self.last


class Max:
    identity = 0

    def apply(self, inp, ctx):
        m = None
        for i in inp:
            if not i:
                continue
            if not m or i > m:
                m = i
        return m


class ApplyProfile:
    def __init__(self, profile, controller):
        self.profile = profile
        self.controller = controller

    def apply(self, inp, ctx):
        if inp is not None:
            out = self.controller.run(inp, ctx["dt"])
            return out


class Sum:
    identity = 0

    def apply(self, in_l, in_r):
        if in_l is not None and in_r is not None:
            return in_l + in_r
        if in_l is None and in_r is not None:
            return in_r
        if in_r is None and in_l is not None:
            return in_l

    def __str__(self):
        return "+"


class Sub:
    identity = 0

    def apply(self, in_l, in_r):
        if in_l is not None and in_r is not None:
            return in_l - in_r
        if in_l is None and in_r is not None:
            return in_r
        if in_r is None and in_l is not None:
            return in_l

    def __str__(self):
        return "-"


class Mul:
    identity = 0

    def apply(self, in_l, in_r):
        if in_l is not None and in_r is not None:
            return in_l * in_r
        if in_l is None and in_r is not None:
            return in_r
        if in_r is None and in_l is not None:
            return in_l

    def __str__(self):
        return "*"


class Div:
    identity = 0

    def apply(self, in_l, in_r):
        if in_l is not None and in_r is not None:
            if in_r != 0:
                return in_l / in_r
            else:
                return in_l
        if in_l is None and in_r is not None:
            return in_r
        if in_r is None and in_l is not None:
            return in_l

    def __str__(self):
        return "/"
