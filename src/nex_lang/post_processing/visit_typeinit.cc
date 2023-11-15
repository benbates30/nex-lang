
#include "visit_typeinit.h"

#include <cassert>
#include <iostream>

#include "ast_node.h"
#include "call.h"
#include "nl_type_i32.h"
#include "nl_type_ptr.h"
#include "operators.h"
#include "pseudo_assembly.h"
#include "type_mismatch_error.h"
#include "visit_expr.h"
#include "visit_type.h"
#include "bin_op.h"

TypedExpr visit_typeinit(
    ASTNode root,
    bool read_address,
    SymbolTable& symbol_table,
    ModuleTable& module_table,
    std::vector<std::shared_ptr<Code>>& static_data
) {
    assert(std::get<NonTerminal>(root.state) == NonTerminal::typeinit);
    TypedExpr result = TypedExpr {nullptr, nullptr};

    std::vector<State> prod = root.get_production();
    if (prod == std::vector<State> {NonTerminal::typeinit, NonTerminal::type}) {
        ASTNode type_node = root.children.at(0);
        std::shared_ptr<NLType> nl_type = visit_type(type_node);

        std::shared_ptr<TypedProcedure> typed_proc = std::dynamic_pointer_cast<TypedProcedure>(module_table.at("heap").at("heap_allocate"));
        assert(typed_proc);

        result = TypedExpr {
            make_call(typed_proc->procedure, {int_literal(nl_type->bytes())}),
            std::make_shared<NLTypePtr>(nl_type)
        };
    } else if (prod == std::vector<State> {NonTerminal::typeinit, NonTerminal::type, Terminal::LBRACKET, NonTerminal::expr, Terminal::RBRACKET}) {
        ASTNode type_node = root.children.at(0); 
        std::shared_ptr<NLType> nl_type = visit_type(type_node);

        ASTNode expr_node = root.children.at(2);
        TypedExpr expr = visit_expr(expr_node, read_address, symbol_table, module_table, static_data);
        std::shared_ptr<TypedProcedure> typed_proc = std::dynamic_pointer_cast<TypedProcedure>(module_table.at("heap").at("heap_allocate"));
        assert(typed_proc);

        if ((*expr.nl_type) == NLTypeI32{}) {
            result = TypedExpr {
                make_call(typed_proc->procedure, {make_block({bin_op(expr.code, op::times(), int_literal(nl_type->bytes()))})}),
                std::make_shared<NLTypePtr>(nl_type)
            };
        }
        else {
            throw TypeMismatchError("Expression between square brackets must be of type i32.", root.children.at(1).line_no);
        }
    } else {
        std::cerr << "Invalid production found while processing expr."
                  << std::endl;
        exit(1);
    }

    assert(result.code);
    assert(result.nl_type);
    return result;
}
