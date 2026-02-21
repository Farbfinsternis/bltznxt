
#ifndef NODES_H
#define NODES_H

// Must include in this exact order to resolve the circular
// dependency chain: node.h -> exprnode.h -> varnode.h,
// stmtnode.h -> exprnode.h + declnode.h, declnode.h -> exprnode.h + stmtnode.h.
// The include guards in each file prevent re-inclusion.

#include "prognode.h"
#include "stmtnode.h"


#endif
