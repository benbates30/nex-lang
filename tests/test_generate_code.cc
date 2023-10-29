


#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include <iostream>

#include "code_gen.h"

#include "lang.h"
#include "parsing.h"


#include "assembly.h"
#include "beq_label.h"
#include "bin_op.h"
#include "block.h"
#include "bne_label.h"
#include "chunk.h"
#include "comparators.h"
#include "define_label.h"
#include "elim_calls.h"
#include "elim_if_stmts.h"
#include "elim_labels.h"
#include "elim_scopes.h"
#include "elim_vars_proc.h"
#include "entry_exit.h"
#include "flatten.h"
#include "if_stmt.h"
#include "label.h"
#include "operators.h"
#include "print.h"
#include "procedure.h"
#include "pseudo_assembly.h"
#include "reg.h"
#include "stack.h"
#include "use_label.h"
#include "utils.h"
#include "var_access.h"
#include "variable.h"
#include "while_loop.h"
#include "word.h"
#include "write_file.h"



static uint32_t TERMINATION_PC = 0b11111110111000011101111010101101;
static std::string file_name("test_max.bin");

TEST_CASE("Test code gen", "[codegen]") {
    Grammar grammar = make_grammar();

    /*
    std::string input =
        "fn max(x: i32, y: i32) -> i32 {"
        "   let result: i32 = 0;"
        "   if (x > y) {"
        "       result = x;"
        "   } else {"
        "       result = y;"
        "   }"
        "   return result;"
        "}"
        ""
        "fn main() -> i32 {"
        "   let z: i32 = max(5, 12);"
        "}";*/
    std::string input = 
        "fn main(x: i32, y: i32) -> i32 {"
        "   let result: i32 = 0;"
        "   result = x + y;"
        "}";
    auto tokens = scan(input);
    auto ast_node = parse_cyk(tokens, grammar);

    std::cout << ast_node->to_string(0) << std::endl;

    auto procedures = generate(ast_node.value());
    
    std::shared_ptr<Procedure> main_proc;
    for (auto proc : procedures) {
        if (proc->name == "main") {
            main_proc = proc;
        }
    }
    
    std::map<std::shared_ptr<Procedure>, std::shared_ptr<Chunk>> param_chunks;
    for (auto proc : procedures) {
        param_chunks[proc] = std::make_shared<Chunk>(proc->parameters);
    }

    auto start_proc = std::make_shared<Procedure>(
        "start_proc",
        std::vector<std::shared_ptr<Variable>> {}
    );
    start_proc->code = make_block(
        {make_call(main_proc, {to_expr(Reg::Input1), to_expr(Reg::Input2)}),
         make_lis(Reg::TargetPC),
         make_word(TERMINATION_PC),
         make_jr(Reg::TargetPC)}
    );
    procedures.insert(procedures.begin(), start_proc);

    for (auto proc : procedures) {
        ElimCalls elim_calls {proc, param_chunks};
        proc->code = proc->code->accept(elim_calls);

        ElimIfStmts elim_if_stmts;
        proc->code = proc->code->accept(elim_if_stmts);

        ElimScopes elim_scopes;
        proc->code = proc->code->accept(elim_scopes);
        auto local_vars = elim_scopes.get();

        std::vector<std::shared_ptr<Variable>> all_local_vars = {
            proc->param_ptr,
            proc->dynamic_link,
            proc->saved_pc};
        all_local_vars
            .insert(all_local_vars.end(), local_vars.begin(), local_vars.end());
        std::shared_ptr<Chunk> local_vars_chunk =
            std::make_shared<Chunk>(all_local_vars);

        proc->code = add_entry_exit(proc, local_vars_chunk);

        auto param_chunk = std::make_shared<Chunk>(proc->parameters);
        ElimVarsProc elim_vars_proc(
            local_vars_chunk,
            param_chunk,
            proc->param_ptr
        );
        proc->code = proc->code->accept(elim_vars_proc);
    }

    std::vector<std::shared_ptr<Code>> all_code;
    for (auto proc : procedures) {
        all_code.push_back(proc->code);
    }
    auto program = make_block(all_code);

    Flatten flatten;
    program->accept(flatten);

    auto program2 = flatten.get();

    auto program3 = elim_labels(program2);

    write_file(file_name, program3);

    //for (auto input : {0, 1, 2, 3, 5, 6, 7, 8, 9}) {
    //    REQUIRE(stoi(emulate(file_name, input, 0)) == sample_main(input, 0));
    //}


}
