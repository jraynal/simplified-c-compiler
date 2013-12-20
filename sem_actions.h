#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "structs.h"

 //for dynamic allocation

struct _variable * varCreate(enum _type type,	union _value *value);
int varFree(struct _variable *); 

//affiche la ligne d'operation en llvm
void binOp(struct _variable*,struct _variable* , char* iOp, char* fOp, char* vOp);

//calcule le resultat de l'operation sur les variables
struct _variable *mul(struct _variable *,struct _variable *);
struct _variable *divide(struct _variable *,struct _variable *);
struct _variable *add(struct _variable *,struct _variable *);
struct _variable *sub(struct _variable *,struct _variable *);


struct _variable *incr(struct _variable *);
struct _variable *decr(struct _variable *);
struct _variable *neg(struct _variable *);

struct _variable * l_op (struct _variable *,struct _variable *);
struct _variable * g_op (struct _variable *,struct _variable *);
struct _variable * le_op (struct _variable *,struct _variable *);
struct _variable * ge_op (struct _variable *,struct _variable *);
struct _variable * ne_op (struct _variable *,struct _variable *);
struct _variable * eq_op (struct _variable *,struct _variable *);

int initLayer();
int initNode();



#endif //FUNCTIONS_H