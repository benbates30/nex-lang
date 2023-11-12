
#pragma once

#include "ast_node.h"
#include "symbol_table.h"

void extract_fns(ASTNode root, SymbolTable& symbol_table);
void extract_fn(ASTNode root, SymbolTable& symbol_table);
