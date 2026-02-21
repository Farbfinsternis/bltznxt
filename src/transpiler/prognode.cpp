
#include "nodes.h"
#include "std.h"
#include "visitor.h"

//////////////////
// The program! //
//////////////////
BCEnviron *ProgNode::semant(BCEnviron *e) {

  file_lab = genLabel();

  StmtSeqNode::reset(stmts->file, file_lab);

  a_ptr<BCEnviron> env(d_new<BCEnviron>(genLabel(), Type::int_type, 0, e));

  consts->proto(env->decls, env);
  structs->proto(env->typeDecls, env);
  structs->semant(env);
  funcs->proto(env->funcDecls, env);
  stmts->semant(env);
  funcs->semant(env);
  datas->proto(env->decls, env);
  datas->semant(env);

  sem_env = env.release();
  return sem_env;
}

void ProgNode::accept(Visitor *v) { v->visit(this); }
