#include "ReorderStores.h"
#include "Debug.h"
#include "IRMutator.h"
#include "IRPrinter.h"
#include "IROperator.h"
#include "IREquality.h"
#include "Substitute.h"
#include "ExprUsesVar.h"
#include "RemoveUndef.h"

#include <list>

using namespace std;

namespace Halide {
namespace Internal {

class StmtInfo : public IRVisitor {
public:
    Stmt stmt;
    std::vector<const Load *> loads;
    std::vector<const Store *> stores;
    std::vector<const Free *> frees;

    StmtInfo(Stmt stmt) : stmt(stmt) {
        stmt.accept(this);
    }

private:
    using IRVisitor::visit;

    void visit(const Load *op) {
        loads.push_back(op);
        IRVisitor::visit(op);
    }

    void visit(const Store *op) {
        stores.push_back(op);
        IRVisitor::visit(op);
    }

    void visit(const Free *op) {
        frees.push_back(op);
        IRVisitor::visit(op);
    }
};

void flatten_stmts(Stmt from, list<StmtInfo> &to) {
    if (!from.defined()) {
        return;
    }

    if (const Block *block = from.as<Block>()) {
        flatten_stmts(block->first, to);
        flatten_stmts(block->rest, to);
    } else {
        to.push_back(from);
    }
}

// Check if 'from' might load memory stored by 'to'.
bool loads_from_stores(const Load *from, const StmtInfo &to) {
    for (size_t j = 0; j < to.stores.size(); j++) {
        const Store *store = to.stores[j];
        if (from->name == store->name && equal(from->index, store->index)) {
            return true;
        }
    }
    return false;
}

bool stmt_loads_from_stores(const StmtInfo &from, const StmtInfo &to) {
    for (size_t i = 0; i < from.loads.size(); i++) {
        if (loads_from_stores(from.loads[i], to)) {
            return true;
        }
    }
    return false;
}

template <typename It>
bool stmt_loads_from_stores(const StmtInfo &loads, It begin, It end) {
    for (It i = begin; i != end; i++) {
        if (stmt_loads_from_stores(loads, *i)) {
            return true;
        }
    }
    return false;
}

Stmt substitute_trivial_stores(const StmtInfo &to, const StmtInfo &from) {
    Stmt result = to.stmt;
    for (size_t i = 0; i < to.stores.size(); i++) {
        const Store *store = to.stores[i];

        // If the store is storing a load, try to just store the value
        if (const Load *store_value = store->value.as<Load>()) {
            for (size_t j = 0; j < from.stores.size(); j++) {
                const Store *from_store = from.stores[j];

                // If the store in the to stmt is storing a load from a store in the from stmt, just store the value in the from stmt.
                if (from_store->name == store_value->name && equal(from_store->index, store_value->index)) {
                    result = substitute(store_value, from_store->value, result);
                    break;
                }
            }
        }
    }
    return result;
}

class ReorderPipelines : public IRMutator {
public:
    ReorderPipelines() {}

private:
    using IRMutator::visit;

    void visit(const Pipeline *op) {
        Stmt produce = mutate(op->produce);
        Stmt update = mutate(op->update);
        Stmt consume = mutate(op->consume);

        // Flatten the pipeline into a list of Stmts that can be
        // reordered. In the process, we're going to gather the
        // information we need about the loads and stores in those
        // Stmts (buffer name, index).
        list<StmtInfo> stmts;
        flatten_stmts(produce, stmts);
        flatten_stmts(update, stmts);
        flatten_stmts(consume, stmts);

        // Now, try to reorder the stmts such that loads that load
        // data stored from a previous statement are as close together
        // as possible.

        // Build the list of output stmts.
        vector<Stmt> reordered;

        while (!stmts.empty()) {
            // Just grab the first one to start with.
            StmtInfo target(stmts.front());
            reordered.push_back(target.stmt);
            stmts.pop_front();

            // Now, try to find any other stmts left that load from
            // stores in target.
            for (list<StmtInfo>::iterator i = stmts.begin(); i != stmts.end();) {
                // We need to ensure that reordering this stmt
                // won't skip a store that this stmt loads from.
                if (stmt_loads_from_stores(*i, target) &&
                    !stmt_loads_from_stores(*i, stmts.begin(), i)) {
                    Stmt s = substitute_trivial_stores(*i, target);
                    reordered.push_back(s);
                    i = stmts.erase(i);
                } else {
                    i++;
                }
            }
        }

        Stmt result = reordered[0];
        for (size_t i = 1; i < reordered.size(); i++) {
            result = Block::make(result, reordered[i]);
        }
        stmt = result;
    }
};

Stmt reorder_stores(Stmt s) {
    ReorderPipelines ro;
    return ro.mutate(s);
}

}
}
