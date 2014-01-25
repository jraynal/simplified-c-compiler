#include "sem_actions.h"

#define LLVM( string ) fprintf(stdout,##string); fprintf(stdout,"\n");

#define CHK(truc) do{if(truc == NULL) {fprintf(stderr,"error in "#truc" at %s in %s line %d\n",__FILE__,__FUNCTION__,__LINE__);exit(EXIT_FAILURE);}}while(0)
#define INVALID_OP  do{fprintf(stderr,"uncommon execution at %s in %s line %d\n",__FILE__,__FUNCTION__,__LINE__);	exit(1);}while(0)


const char *itoa(int i) {
	char *c=NULL;
	int len=0;
	len=snprintf(c,len,"%d",i);
	c=(char*)malloc(len+1);
	snprintf(c,len+1,"%d",i);
	return c;
}

const char *new_reg(){
	static int i =0;
	i++;
	return itoa(i);
}

const char *new_label(){
	static int i =0;
	return itoa(i++);
}
char* strOfNametype(enum _type *t){
	switch(*t){
		case INT_TYPE :
		return "i32";
		break;
		case FLOAT_TYPE:
		return "float";
		break;
		default:
		return "";
	}
	INVALID_OP;
}

const char* officialName(const char* name){
	CHK(name);
	if (strcmp(name, "$accel")== 0)
		return "%accel";
	else return name;
}

void deleteAttribute(struct _attribute* a) {
	free(a);
}

struct _attribute *newAttribute(const char * id){
	struct _attribute *a=malloc(sizeof(struct _attribute));
	a->reg = new_reg(); // registre de la donnée
	a->addr= NULL; // ctxtesse de la donnée (identifiants uniquement)
	a->type=malloc(sizeof(enum _type));
	*a->type = UNKNOWN;
	a->code = initCode();
	a->identifier= id;
	a->arguments = NULL;
	CHK(a);
	return a;
}

struct _variable * varCreate(enum _type *type, const char *addr){
	struct _variable* var = malloc(sizeof(struct _variable));
	if(var){
		var->type = type;
		var->addr = addr;
	}
	else
		fprintf(stderr, "No created variable\n");
	CHK(var);
	return var;
}

struct _attribute *get_attr_from_context(struct _layer* ctxt,const char* name){
	CHK(ctxt);
	CHK(name);
	char dest [100];
	sprintf(dest,"/%s",name);
	/* Obtention de l'indentifiant */
	struct _variable *var = get_var_layer(ctxt,dest);
	CHK (var);

	/* Creation de la liste d'attributs */
	struct _attribute *a = newAttribute(name);

	/* Chargement de l'identifiant */
	a->addr = var->addr;												// sauvegarde de l'ctxtresse pour tableaux par exemple
	a->type = var->type;
	char * str_type = strOfNametype(a->type);
	addCode(a->code,"%%%s =load %s* %s \n",a->reg,str_type,var->addr);	// chargement en mémoire pour identifiant de variable
	CHK(a);
	return a;														// ecriture
}


struct _attribute *getVar(const char* name,struct _layer *ctxt) {
	struct _attribute *a = get_attr_from_context(ctxt,name);
	CHK(a);
	return a;
}

struct _attribute *varIncr(const char * name,struct _layer* ctxt){
	struct _attribute *a = get_attr_from_context(ctxt,name);
	char * str_type = strOfNametype(a->type);
	
	/* Creation du registre de sortie du calcul */
	const char *reg = new_reg();
	switch(*a->type){
		/* Addition d'entiers */
		case INT_TYPE :
			addCode(a->code,"%%%s =add %s %%%s, i32 1\n",reg,str_type,a->reg);
			break;
		/* Addition de flottants */
		case FLOAT_TYPE :
			addCode(a->code,"%%%s = fadd %s %%%s, float 1.0\n",reg,str_type,a->reg);
			break; 
		default:
			break;

	}
	/* Sauvegarde dans l'identifiant */
	addCode(a->code,"store %s %%%s, %s %s\n",str_type,reg,str_type,a->addr);
	a->reg=reg;
	CHK(a);	
	return a;
}

struct _attribute *varDecr(const char * name,struct _layer* ctxt) {
	struct _attribute *a = get_attr_from_context(ctxt,name);
	char * str_type = strOfNametype(a->type);
	/* Creation du registre de sortie du calcul */
	const char *reg = new_reg();
	switch(*a->type){
		/* decrementation d'entiers */
		case INT_TYPE :
			addCode(a->code,"%%%s =sub %s %%%s, i32 1\n",reg,str_type,a->reg);
			break;
		/* decrementation de flottants */
		case FLOAT_TYPE :
			addCode(a->code,"%%%s = fsub %s %%%s, float 1.0\n",reg,str_type,a->reg);
			break; 
		default:
			break;
	}
	addCode(a->code,"store %s %%%s, %s %s\n",str_type,reg,str_type,a->addr);
	return a;
}

struct _attribute *simpleFuncall(struct _layer* ctxt,const char * funName){
	struct _attribute *a = get_attr_from_context(ctxt,funName);
	addCode(a->code,"call  %s @%s ()\n",strOfNametype(a->type), a->identifier);
	CHK(a);
	return a;

}


struct _attribute *multipleFuncall(struct _layer* ctxt,const char * funName,struct _list * list){
	CHK(ctxt);
	CHK(funName);
	CHK(list);
	struct _attribute *a = get_attr_from_context(ctxt,funName);
	CHK(a);
	struct _attribute *  argument;
	addCode(a->code,"call  %s @%s (",strOfNametype(a->type), a->identifier);
		while(!is_empty(list)){
			argument = (struct _attribute *) list->tail->value;
			addCode(a->code,", %s %%%d",strOfNametype(argument->type),argument->reg);// LLVM
			removeElmnt(argument,list);
		}
		addCode(a->code,"\n)");
		del_list(list);
	CHK(a);
	return a;
}

struct _attribute *newInt(int i){
	struct _attribute *a = newAttribute("/");
	*a->type = INT_TYPE;
	
	addCode(a->code,"%%%s  = add i32 %d, 0\n",a->reg,i);
	CHK(a);
	return a;
}

struct _attribute *newFloat(float f){
	struct _attribute *a = newAttribute("/");
	*a->type = FLOAT_TYPE;
	addCode(a->code,"%%%s  = fadd float %g, 0.0\n",a->reg,f);
	CHK(a);
	return a;
}



struct _attribute *getValArray(struct _attribute *array, struct _attribute *i){
	struct _attribute *a = newAttribute("/");
	// Ne pas oublier le code des autres...
	a->code=fusionCode(array->code,i->code);
	a->type = array->type;
	/* retourne l'élément situé à i.reg * array.type de l'ctxtesse de base, donc le ième */
	addCode(a->code,"%%%s = getelementptr %%%s* %%%s, %%%s %%%s\n",a->reg,strOfNametype(array->type),array->addr,strOfNametype(array->type),i->reg);
	CHK(a);
	deleteAttribute(i);
	deleteAttribute(array);
	return a;
}


/* TODO: gestion propre des listes d'attributs */
struct _list * expression_list(struct _attribute *a){
	struct _list * list = init_list();
	insertElmnt(a,list);
	CHK(list);
	return list;
}


struct _list * insert_expr_list(struct _attribute *a ,struct _list * list){
	insertElmnt(a,list);
	CHK(list);
	return list;
}


/* Je pense que la grammaire gère le cas où l'incrémentation doit se faire avant... */
struct _attribute *prefixedVarIncr(struct _attribute *a){
	char * str_type = strOfNametype(a->type);
	
	/* Creation du registre de sortie du calcul */
	const char *reg = new_reg();
	switch(*a->type){
		/* Addition d'entiers */
		case INT_TYPE :
			addCode(a->code,"%%%s =add %s %%%s, i32 1\n",reg,str_type,a->reg);
			break;
		/* Addition de flottants */
		case FLOAT_TYPE :
			addCode(a->code,"%%%s = fadd %s %%%s, float 1.0\n",reg,str_type,a->reg);
			break; 
		default:
			break;

	}
	/* Sauvegarde dans l'identifiant */
	addCode(a->code,"store %s %%%s, %s %s\n",str_type,reg,str_type,a->addr);
	a->reg=reg;
	CHK(a);
	return a;
}


struct _attribute *prefixedVarDecr(struct _attribute *a){
	char * str_type = strOfNametype(a->type);
	/* Creation du registre de sortie du calcul */
	const char *reg = new_reg();
	switch(*a->type){
		/* decrementation d'entiers */
		case INT_TYPE :
			addCode(a->code,"%%%s =sub %s %%%s, i32 1\n",reg,str_type,a->reg);
			break;
		/* decrementation de flottants */
		case FLOAT_TYPE :
			addCode(a->code,"%%%s = fsub %s %%%s, float 1.0\n",reg,str_type,a->reg);
			break; 
		default:
			break;
	}
	addCode(a->code,"store %s %%%s, %s %s ",str_type,reg,str_type,a->addr);
	CHK(a);
	return a;
}

struct _attribute *binOp(struct _attribute *a1,struct _attribute *a2,char* intOp, char * floatOp){
	CHK(a1);
	CHK(a2);
	if (*a1->type != *a2->type){
		INVALID_OP;
	}
	// struct _attribute *a = newAttribute(a1->identifier);
	// Je qu'il faut un nouvel identifiant...
	struct _attribute *a = newAttribute("/");

	// ne pas oublier de récupérer le code qui remonte...
	a->code=fusionCode(a1->code,a2->code);

	switch(*a1->type){
		case INT_TYPE : 
		*a->type = INT_TYPE;
		addCode(a->code,"%%%s = %s i32 %%%s, i32 %%%s\n",a->reg,intOp,a1->reg,a2->reg);	
		break;
		case FLOAT_TYPE:
		*a->type = FLOAT_TYPE;
		addCode(a->code,"%%%s = %s float %%%s, float %%%s\n",a->reg,floatOp,a1->reg,a2->reg);	
		break;
		default: 
		INVALID_OP;

	}
	deleteAttribute(a1);
	deleteAttribute(a2);
	CHK(a);
	return	a;
}

/* ça c'est super malin, de vrai de vrai !! */
struct _attribute *multiply(struct _attribute *a1,struct _attribute *a2){
	return binOp(a1,a2,"mul","fmul");
}

struct _attribute *divide(struct _attribute *a1,struct _attribute *a2){
	return binOp(a1,a2,"sdiv","fdiv");
}

struct _attribute *add(struct _attribute *a1,struct _attribute *a2){
	return binOp(a1,a2,"add","fadd");
}

struct _attribute *sub(struct _attribute *a1 ,struct _attribute *a2){
	return binOp(a1,a2,"sub","fsub");
}

struct _attribute *neg(struct _attribute *a){
	CHK(a);
	struct _attribute *na=newAttribute(a->identifier);
	na->type=a->type;
	switch(*a->type){
		case INT_TYPE : 
		addCode(a->code,"%%%s = sub i32 0 , %%%s\n",na->reg,a->reg) ;
		break;
		case FLOAT_TYPE:
		addCode(a->code , "%%%s = fsub float 0.0 , %%%s\n",na->reg,a->reg) ;
		break;
		default:
		INVALID_OP;
	}
	deleteAttribute(a);
	CHK(na);
	return na;
}


struct _attribute *cmp(struct _attribute *a1 ,struct _attribute *a2 , char* intConditionCode,  char* floatConditionCode ){
	CHK(a1);
	CHK(a2);
	if (*a1->type != *a2->type){
		INVALID_OP;
	}
	// struct _attribute *a = newAttribute(a1->identifier);
	// Idem binOp...
	struct _attribute *a = newAttribute("/");
	// Encore une fois, y a du code qui remonte, on le stocke:
	a->code=fusionCode(a1->code,a2->code);
	// les affectation de TYPE sont commentées car la comparaison renvoie un i1 et que ça doit rester transparent....
	switch(*a1->type){
		case INT_TYPE : 
	//	*a->type = INT_TYPE;
		addCode(a->code,"%%%s = icmp %s i32 %%%s, i32 %%%s\n",a->reg,intConditionCode, a1->reg, a2->reg);	
		break;
		case FLOAT_TYPE:
	//	*a->type = FLOAT_TYPE;
		addCode (a->code,"%%%s = fcmp %s float %%%s, float %%%s\n",a->reg,floatConditionCode,a1->reg,a2->reg);	
		break;
		default:
		INVALID_OP;
	}
	CHK(a);
	deleteAttribute(a1);
	deleteAttribute(a2);
	return	a;
}

struct _attribute *l_op (struct _attribute *a1 ,struct _attribute *a2 ){
	return cmp (a1,a2,"slt","ult");
}

struct _attribute *g_op (struct _attribute *a1 ,struct _attribute *a2 ){
	return cmp (a1,a2,"sgt","ugt");
}

struct _attribute *le_op (struct _attribute *a1 ,struct _attribute *a2 ){
	return cmp (a1,a2,"sle","ule");
}

struct _attribute *ge_op (struct _attribute *a1 ,struct _attribute *a2 ){
	return cmp (a1,a2,"sge","uge");
}

struct _attribute *ne_op (struct _attribute *a1 ,struct _attribute *a2 ){
	return cmp (a1,a2,"ne","une");

}

struct _attribute *eq_op (struct _attribute *a1 ,struct _attribute *a2 ){
	return cmp (a1,a2,"eq","ueq");
}

struct _attribute *declareVar(char* nom,struct _layer* ctxt){
	//vérification de l'existence de l'ARBRE DE RECHERCHE
	CHK(ctxt);
	CHK(nom);
	char dest[strlen(nom)+2];
	sprintf(dest,"/%s",nom);
	struct _attribute *a=newAttribute(nom);
	// Modification du type de la variable et de l'ctxtese du pointeur par effet de bord.
	// On ne pourra alour la mémoire qu'en connaisant le type!
	struct _variable * var = varCreate(a->type,a->addr);

	// Ajout de la clef dans l'arbre
	set_var_layer(ctxt,dest,var);

	/* DEBUG SPOTTED */
	// Vérifiction de la présence de la clef
	// var = get_layer(ctxt,dest);
	// if(var==NULL)
	// 	fprintf(stderr, "Variable non set : %s\n",dest);
	// fprintf(stderr, "declaration de %s\n",nom);

	// on remonte à la règle du dessus qui devra faire l'allocation!!
	CHK(a);
	return a;
}

struct _attribute * declare_array(struct _attribute* array, int size){
	CHK(array);
	addCode(array->code,"%%%s = alloca %s,%s %d",array->addr,strOfNametype(array->type),strOfNametype(array->type),size);
	return array;
}

struct _attribute *simple_declare_function(struct _attribute * func){
	CHK(func);
	CHK(my_ctxt);
	// my_ctxt=add_layer(my_ctxt);
	*(func->type) = UNKNOWN_FUNC;
	func->arguments = init_list();
return NULL;
}
struct _attribute *multiple_declare_function(struct _attribute * func , struct _list * args){
	CHK(func);
	CHK(args);
	CHK(my_ctxt);
	// my_ctxt=add_layer(my_ctxt);
	*(func->type) = UNKNOWN_FUNC;
	func->arguments = args;
return NULL;
}

struct _attribute *allocate_id(struct _attribute *a, enum _type t) {
	CHK(a);
	setType(a,t);
	addCode(a->code,"%%%s = alloca %s",a->addr,strOfNametype(&t));
	CHK(a);
	return a;	
}

void setType(struct _attribute *a, enum _type t){
	CHK(a);
	CHK(a->type);
	//selon le type d'objet remonte : variable, fonction ou tableau
	switch(*a->type){
		case UNKNOWN:
		*a->type = t;
		break;
		case UNKNOWN_FUNC:
		switch(t){
			//selon le type a affecter (int float void...)
			case INT_TYPE :
			*a->type = INT_FUNC;		
			break;
			case FLOAT_TYPE: 
			*a->type = FLOAT_FUNC;
			break;
			case VOID_TYPE: 
			*a->type = VOID_FUNC;
			break;
			default : 
			INVALID_OP;
		}
		break;
			//selon le type a affecter (int float ...)
		case UNKNOWN_ARRAY: 
		switch(t){
			case INT_TYPE :
			*a->type = INT_ARRAY;		
			break;
			case FLOAT_TYPE: 
			*a->type = FLOAT_ARRAY;
			break;
			default : 
			INVALID_OP;
		}
		break;
		default:
		INVALID_OP;
	}
	// Modification dans l'arbre par effet de bord
	// TODO : code llvm  //LLVM
	return;
}

struct _attribute * setTypeList(struct _list * list, enum _type t){
	CHK(list);
	struct _attribute* attr;
	struct _attribute* ret;
	ret = newAttribute("/");
	while(! is_empty(list)){
		attr = (struct _attribute *) (list->head->value);
		setType(attr,t);
		ret= concat(ret,attr);
		removeElmnt(attr,list);
	}
	
	del_list(list);
	CHK(ret);
	return ret;
}

struct _attribute *make_function(enum _type t , struct _attribute * name, struct _attribute * content){
	CHK(name);
	CHK(content);
	//TODO checker le type si existe deja  
	//sinon
	struct _attribute* attr_funct = name;
	setType(attr_funct,t);
	char * str_type =  strOfNametype(attr_funct->type);
	addCode(ret,"define %s @%s(",str_type,attr_funct->identifier,)
	//TODO inserer le nom des arguments
		//TODO inserer les arguments au context de la fonction
	//TODO supprimer la list
	addCode(ret,"){");
	ret2 = fusionCode(ret,content);
	addCode(ret2,"ret %s}",str_type_ret);
	CHK(ret);
	return ret;
}


void print(struct _attribute *a) {
	CHK(a);
	printCode(STDOUT_FILENO,a->code);
	deleteCode(a->code);
	deleteAttribute(a);
}

struct _attribute * emptyExpr(){
	struct _attribute * ret = newAttribute("/");
	CHK(ret);
	return ret;
}


struct _attribute *selection(struct _attribute *cond, struct _attribute *then, struct _attribute *other) {
	CHK(cond); CHK(then); 
	struct _attribute *a= newAttribute("/");
	const char *label1=new_label();
	const char *label2=new_label();
	// On ajoute le code de la condition et on teste
	a->code=cond->code;
	a->code=addCode(cond->code,"br i1 %%%s, label %%%s label %%%s\n",cond->reg,label1,label2);
	// On ajoute le label et le then
	a->code=fusionCode(addCode(a->code,"label %%%s\n",label1),then->code);
	// On ajoute le label de other et si il y en a le code de
	addCode(a->code,"label %%%s\n",label2);
	if(other) {
		a->code=fusionCode(a->code,other->code);
		deleteAttribute(other);
	}
	deleteAttribute(then);
	deleteAttribute(cond);
	return a;
}

struct _attribute *loop(struct _attribute *init, struct _attribute *cond, struct _attribute *ite, struct _attribute *body) {
	struct _attribute *a=newAttribute("/");
	const char *loop_label=new_label();
	if(init)
		a->code=init->code;
	a->code=fusionCode(addCode((init)?a->code:initCode(),"label %%%s\n",loop_label),cond->code);
	body->code=addCode(fusionCode(body->code,(ite)?ite->code:initCode()),"br label %%%s",loop_label);
	selection(cond,body,NULL); // On teste et on fait au besoin
	if(init)
		deleteAttribute(init);
	deleteAttribute(cond);
	if(ite)
		deleteAttribute(ite);
	deleteAttribute(body);
	return a;
}

struct _attribute *concat(struct _attribute *a1,struct _attribute *a2) {
	CHK(a1);
	CHK(a2);
	struct _attribute *a=newAttribute("/");
	a->code=fusionCode(a1->code,a2->code);
	CHK(a);
	deleteAttribute(a1);
	deleteAttribute(a2);
	return a;
}

struct _attribute *assignment(struct _attribute *tgt, enum _affectation eg ,struct _attribute *ori){
	struct _attribute *a;
	CHK(tgt);
	CHK(ori);
	switch(eg){
		case ADD:
			a=add(tgt,ori);
			break;
		case MUL:
			a=multiply(tgt,ori);
			break;
		case SUB:
			a=sub(tgt,ori);
			break;
		default:
			a=newAttribute("/");
			a->reg=ori->reg;
			a->code= ori->code;
			a->type = ori->type;
			a->identifier = ori->identifier;
			break;
	}
	char *type = strOfNametype(a->type);
	addCode(a->code,"store %s %%%s, %s* %%%s\n",type,a->reg,type,tgt->addr);
	deleteAttribute(tgt);
	deleteAttribute(ori);
	return a;
}

