
#pragma once

#include "ast_node.h"
#include "module_table.h"

struct ASTNode;

void extract_s(ASTNode root, ModuleTable& module_table);