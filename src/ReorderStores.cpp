#include "ReorderStores.h"
#include "Debug.h"
#include "IRMutator.h"
#include "IRPrinter.h"
#include "IROperator.h"
#include "IREquality.h"
#include "Substitute.h"

#include <list>

namespace Halide {
namespace Internal {
class TrivialLoadStores : public IRVisitor {
public:
    std::list<Stmt> trivial;

private:
    using IRVisitor::visit;

    void visit(const Store *op) {
        // This Load/Store is trivial, try moving forwarding the store to where the load is originally stored to.
        const Load *value = op->value.as<Load>();
        if (value) {
            trivial.push_back(Stmt(op));
        } else {
            op->value.accept(this);
        }
    }
};

class ForwardTrivialLoadStores : public IRMutator {
public:
    ForwardTrivialLoadStores(std::list<Stmt> &trivial) : trivial(trivial) {}

private:
    using IRMutator::visit;

    std::list<Stmt> &trivial;
    std::list<Stmt> dead;

    void visit(const Store *op) {
        for (std::list<Stmt>::iterator i = dead.begin(); i != dead.end(); i++) {
            if (equal(*i, Stmt(op))) {
                stmt = Evaluate::make(0);
                return;
            }
        }
        for (std::list<Stmt>::iterator i = trivial.begin(); i != trivial.end(); ++i) {
            // If this trivial load loads from where op is storing, just use the trivial store instead.
            const Store *store = i->as<Store>();
            assert(store);
            const Load *load = store->value.as<Load>();
            assert(load);

            if (load->name == op->name && equal(op->index, load->index)) {
                stmt = Store::make(store->name, op->value, store->index);
                dead.push_back(*i);
                trivial.erase(i);
                return;
            }
        }
        stmt = Stmt(op);
    }
};

Stmt reorder_stores(Stmt stmt) {
    TrivialLoadStores trivial;
    stmt.accept(&trivial);

    ForwardTrivialLoadStores forward(trivial.trivial);
    return forward.mutate(stmt);
}

}
}
