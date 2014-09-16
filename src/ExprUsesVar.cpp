#include "ExprUsesVar.h"

namespace Halide {
namespace Internal {

using std::string;

namespace {
class ExprUsesVar : public IRVisitor {
    using IRVisitor::visit;

    const string &var;

    void visit(const Variable *v) {
        if (v->name == var) {
            result = true;
        }
    }

    template <typename LetType>
    void visit_let(const LetType *l) {
        if (l->name != var) {
            // We should ignore uses of lets if they define the var
            // we're looking for.
            l->value.accept(this);
        }
    }

    void visit(const Let *l) {
        visit_let(l);
    }
    void visit(const LetStmt *l) {
        visit_let(l);
    }

public:
    ExprUsesVar(const string &v) : var(v), result(false) {}
    bool result;
};

}

bool expr_uses_var(Expr e, const string &v) {
    ExprUsesVar uses(v);
    e.accept(&uses);
    return uses.result;
}

}
}
