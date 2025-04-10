# 自定义规则引擎
 
 [返回主页](../../README.md)

 # 原理
 > 基于flex解析词法，基于bison解析语法，在bison解析语法的过程中（在不断规约的过程中，不断构建子树），将条件以及表达式组织为一颗AST二叉树，规则序列化则通过先序的方式进行存储AST树，最终结构为类似为_AND_ left_condition right_condition形式，其中condition则又是一颗子树。

# 概览
> flex与bison互相配合，flex将语言解析为两个栈，一个是词类型，一个是词对应的内容（可通过yytext转换为其它类型，比如int等皆可），bison通过在词类型栈中匹配对应的语法规则，然后通过$$、$1、$2...等方式获取到对应的词内容，然后执行语法规则后定义的c代码块。

# flex
> 通过字符串或者正则匹配，能够获取到对应的匹配内容（yytext），返回值就是该匹配内容对应的词类型（对应bison中的TOKEN），同时还能够通过yylval全局变量返回对应的词内容（对应bison中$1等获取到的内容）。

# bison
> 通过flex解析出的词法流，通过其中的词类型（TOKEN类型）以及字符串等匹配对应的语法，匹配语法后能够通过$1、$2等获取到语法规约式后对应的内容，$$则表示规约后的元素的值，比如condition：statement __AND__ statement，$$表示的是规约后的condition的值，$1和$3表示statement的值，__AND__则为一个TOKEN，当规约后，:后的三个词法元素（词类型以及对应的值）全出栈，condition入栈。

# 例子
```
规则 PART

[Info]
SignID = 131776
Author = xxxxxxx
Time = 20250101

[Scan]
patterns:

@a0:
{
;this is a comment
"this is a pattern string"
}

@a1:
{
; this is another comment
"0xABABAB"
}


condition:
@a0 && @a1

```

```
FLEX PART

int pre_state;
static hex_match_t* hex_match_header;

%x SECTION_INFO
%x SECTION_SCAN
%x SECTION_KILL
%x TEXT_STRING
%x HEX_STRING
%x ATTR_SIGNID
%x ATTR_AUTHOR

HEX_DIGIT [a-fA-F0-9]
DEC_DIGIT [0-9]
SIGN_HEX_INT [+-]?"0x"{HEX_DIGIT}+
PATTERN_NAME "@"[a-zA-Z_]+[a-zA-Z0-9_]*
RULE_ID [+]?{DEC_DIGIT}+

%%

<*>  {
";"    {/*eat comment*/ while(input)...;  }
}

^\[Info\][ \t]*    {
	BEGIN(SECTION_INFO);
	return __SECTION_INFO__;
}

<SECTION_INFO> {
^"SignID"[ \t]*"="   {
			yylval.info_type=BINT_RULEID;
			pre_state=SECTION_INFO;
			BEGIN(ATTR_SIGNID);
			return __INFO_NAME_SIGN_ID__;
		}
^"Author"[ \t]*"="   {
			yylval.info_type=BINT_AUTHOR;
			pre_state=SECTION_INFO;
			BEGIN(ATTR_AUTHOR);
			return __INFO_NAME_AUTHOR__;
		}
}

^\[Scan\][ \t]*    {
	BEGIN(SECTION_SCAN);
	return __SECTION_SCAN__;
}

<SECTION_SCAN> {
"patterns" {return __PATTERN__;}
"condition" {return __CONTIDITON__;}
"&&"  {return __AND__;}
"||"  {return __OR__;}
"!"  {return __NOT__;}
">"  {return __CMP_G__;}
"=="  {return __CMP_E_;}
"+"  {return __PLUS__;}
"*"  {return __MUL__;}
}

<SECTION_SCAN> {
/* functions */
"jmp" {
		yylval.func_call.func_code = FUNC_JUMP;
		yylval.func_call.func_name = 0;
		return __FUNC_CALL__;
	}
"int8" {
		yylval.func_call.func_code = FUNC_INT8;
		yylval.func_call.func_name = 0;
		return __FUNC_CALL__;
	}
/* identifier, int or hex number*/
{SIGN_HEX_INT}  {
		if(str2int(yytext,yyleng,&yylval.s_int32)<0){
				printf("error");
				// lex_terminate();
			}
		return __INT32_NUM__;
	}
{PATTERN_NAME} {
		yylval.comm_str= strdup(yytext);
		return __PATTERN_NAME__
	}
"h\""  {
		pre_state=YY_START;
		BEGIN(HEX_STRING);
	}
\"  {
		pre_state=YY_START;
		BEGIN(TEXT_STRING);
	}
[\n\t\r ] {}
[{}():,] {return yytext[0]; /* 直接返回对应字符*/}
}


<TEXT_STRING>  {
\" {
	BEGIN(pre_state);
	yylval.hex_match_head=hex_match_header; /*支持正则的*/
	return __HEX_DATA__;
	}
\\x{HEX_DIGIT}{2} {
		addhexnormal(hex_match_header,yytext);
	}
}

<HEX_STRING> {
{HEX_DIGIT}{2}  {
		addhexnormal(hex_match_header,yytext);
	}
"??" {
		addhexwild(hex_match_header,yytext); /*通配*/
	}
"?"{HEX_DIGIT}{
		addhexwild(hex_match_header,yytext[0]); /*通配*/
		addhexnormal(hex_match_header,yytext[1]);/*HEX DIGIT*/
	}
\" {
	BEGIN(pre_state);
	yylval.hex_match_head=hex_match_header; /*支持正则的*/
	return __HEX_DATA__;
	}
}

<ATTR_SIGNID>{
{RULE_ID} {
		yylval.ruleid=str2int(yytext);
		BEGIN(pre_state);
		return __INFO_VAL_SIGNID__;
	}
"=" {return yytext[0];}
. {}
}

<ATTR_AUTHOR>{
[A-Za-z]+ {
		yylval.comm_str=yytext;
		BEGIN(pre_state);
		return __INFO_VAL_COMM__;
	}
"=" {return yytext[0];}
. {}
}

. {}

```

```
BISON PART

%union{
	enum INFO_TYPE info_type; // engine define
	char* comm_str;
	int ruleid;
	hex_match_t* hex_match_header; // engine define
	int32_t int32;
	int64_t int64;
	func_call_t func_call; // engine define
	basic_into_t info_sec_node;
	pattern_t* pattern;
	condition_t* condition;
	statement_t* statement;
	func_argument_t* argument;
}

%token <info_type> __INFO_NAME_SIGN_ID__
%token <info_type> __INFO_NAME_AUTHOR__
%token <ruleid> __INFO_VAL_SIGNID__
%token <comm_str> __INFO_VAL_COMM__

%token __CONTIDITON__
%token __PATTERN__
%token <comm_str> __PATTERN_NAME__
%token <hex_match_header> __HEX_DATA__
%token <func_call> __FUNC_CALL__
%token <int32> __INT32_NUM__

%token __SECTION_INFO__ __SECTION_SCAN__

%type <info_sec_node> info infos
%type <pattern> pattern patterns
%type <condition> condition conditions
%type <statement> statement statements number
%type <argument> arg_lists

%left __AND__ __OR__
%right __NOT__
%left __CMP_E_ __CMP_G__
%left __BIT_OR__ __BIT__AND__
%left __PLUS__
%left __MUL__

%%

rules 	: {}
		| __SECTION_INFO__ infos __SECTION_SCAN__ __PATTERN__ ':' patterns __CONTIDITON__ ':' conditions  {
			add_rule($2,$6,$9);
		}
		;

infos	: {$$=0;}
		| infos info  {
				$$ = join_info($1,$2);
			}
info	: __INFO_NAME_SIGN_ID__ '=' __INFO_VAL_SIGNID__  {
				$$ = new_rule_id_info($3);/*$3是token __INFO_VAL_SIGNID__对应的值*/
			}
		| __INFO_NAME_AUTHOR__ '=' __INFO_VAL_COMM__  {
				$$ = new_auth_info($3);
			}
		| __INFO_NAME_RULE_TYPE__ '=' ruletypes  {
				$$ = new_ruletype_info(finnal_ruletype);
			}
		;
ruletypes: {}
		| ruletypes ruletype {}
ruletype : __VAL_TYPE__ {finnal_ruletype=$1}
		| ruletype '|' __VAL_TYPE__ {finnal_ruletype |= $3}
		;

patterns: {$$=0;}
		| pattern patterns {
				$1->next = $2;/*链表串联pattern*/
				$$ = $1;
			}
		;
pattern : __PATTERN_NAME__ ':' '{' statements '}'  {
				$$ = new_pattern($1,$4);/*根据statement创建pattern*/
			}
statements: statement {$$ = $1;}
		| statements statement {/*表达式间无符号的情况*/
				$$ = join_statement($1,$2);/*链表串联在最后*/
				merge_statement($$); /*对于相邻是字符串匹配的statement进行合并，直接拼接，支持的是多行字符串自动拼接*/
			}
statement: __HEX_DATA__  { /*字符串匹配类型*/
				$$ = new_hex_statement($1);/*根据字符串创建statement*/
			}
		| number {$$=$1;}
		| __FUNC_CALL__ '(' arg_lists ')'  {
				$$ = new_func_call_statement($1,$3);
			}
		| statement __PLUS__ statement {
				$$ = new_math_statement($1,$3,'+');
			}
		| statement __MUL__ statement  {
				$$ = new_math_statement($1,$3,'*');
			}
		| __PATTERN_NAME__ {/*支持condition中写规则id@xxx*/
				$$ = new_pattern_id_statement($1);/*PATTERN_ID pattern_name*/
			}
		;
arg_lists: {$$=0;}
		| statements  { /*参数支持表达式*/
				$$ = new_argument_statement($1);
			}
		| arg_lists ',' statements  { /*多参数情况*/
				func_argument_t* t =new_argument_statement($1);
				$$ = join_arg_list($1,t);/*t链接到$1最后，参数顺序变成链表*/
			}
conditions: {$$=0;}
		| condition {$$=$1;}
condition: __TRUE__ {$$=new_bool_condition(1);}
		| statement {$$=new_statement_condition($1);}
		| condition __AND__ condition {$$=new_logic_condition(BCT_AND,$1,$3);}
		| condition __OR__ condition {$$=new_logic_condition(BCT_OR,$1,$3);}
		| __NOT__ condition {$$=new_logic_condition(BCT_NOT,$2,0);}
		| '(' condition ')' {$$=$2;}
		| statement __CMP_E_ statement {$$=new_cmp_condition(BCT_E,$1,$3);}
		| statement __CMP_G_ statement{$$=new_cmp_condition(BCT_G,$1,$3);}
		;
number	: __INT32_NUM__ {$$=new_int32_statement($1);}
		;

```


```
CODE PART
原理：解析规则与条件，整体构建为一颗AST二叉树，序列化时按照先序来序列化
INFO_TYPE  规则内基础信息的类型，如ID、AUTHOR等
basic_into_t 一个链表，保存不同的规则信息，根据info_type区分；
func_argument_t* {
	enum arg_type type;
	statement_t* statement;
	func_argument_t* next;
};
statement_t{
	enum stat_cmd cmd; // NUMBER/FUNC/ADD/SUB/MUL/STRING/PATTERN_ID
	union{
		struct{func_call_t;func_argument_t;}; // used by function
		int32_t int32; // used by number
		char* pattern_name;//used by PATTERN_ID;
		struct{uint8* data;int len;}match_data;// used by __HEX_DATA__ 字符串匹配
		struct{statement_t *l,*r;}; // used by plus mul bitor bitand
	};
	statement_t* next;
}
//condition_t若类型为PATTERN_ID类型的STATEMENT 转为特殊类型的FUNC argument为对应pattern name
condition_t{
	enum condition_type type; // AND OR NOT E GE LESS  BOOL STATEMENT
	union{
		int bool_val;//used by bool 
		statement_t* statment;// used by func
		struct{
			statement_t* l; // 左右为表达式  used by compare condition
			statement_t* r;
		}
	};
	condition_t* l; // 左右为条件 用于logic_condition
	condition_t* r;
}
pattern_t{
	char* name;
	statement_t* statements;
	pattern_t* next;
}
func_call_t{
	int func_code;
	char* funcname;
}

rule_t{
	basic_into_t;
	pattern_t* pattern_list;
	condition_t* tree;
	statement_t* kill_list;
}

规则中的pattern部分数据结构组织结构：

patterns是由pattern串联的链表；
pattern是由statements组成；
statements是由statement串联的链表，对应{}中顺序定义的函数，代表执行顺序；
statement可能是函数调用、字符串、有操作符的表达式；
函数调用由函数名与参数链表组成；
字符串直接保存为statement；
有操作符的表达式则被表示为一颗树，根节点为操作符+-*/；

规则中的condition部分数据结构组织方式：
conditions由condition的树组成，根节点为操作符&& || ! > < 等；


序列化：
condition：
从根开始，先序遍历存储每一个节点；对于父子关系，则记录从condition序列化存储起始位置开始的偏移；

condition段、statement段、match_data段、argument段、pattern段、rules段。

在构建pattern过程中，会构建一个shift表，类似WM算法，提高匹配速度，方便索引到对应的pattern。也就是每个pattern的第一个字符串为key，在匹配过程中，会首先匹配第一个字符串，若匹配上再考虑其中的rule的执行。


bin_condition_head_t{
	uint8_t type;
	offset l;
	offset r;
	offset parent;
}

bin_bool_condition_t{
	head;
	int8 bool_val;
}

bin_logic_condition_t{
	head;
}

bin_statement_condition_t{
	head;
	offset statement; // 对应statement存储于statement块，偏移基于statement块起始位置
	length statement_len;
}

bin_cmp_condition_t{
	head;
	offset lcs;
	length lcs_len;
	offset rcs;
	length rcs_len;
}

bin_arg_head_t{
	int8 type;
}

bin_statement_arg_t{
	arg_head;
	offset statement;
	length statement_len;
}

bin_statement_head_t{
	int8 cmd;
}

bin_match_statement_t{
	st_head;
	length data_len;
	offset data;
}

bin_func_statement_t{
	st_head;
	int func_code;
	offset arg_off;
}

bin_number_statement_t{
	st_head;
	int num;
}

bin_pattern_id_statement_t{
	st_head;
	int pattern_id;
}

bin_math_statement_t{
	st_head;
	length l_st_len;
	offset l_st_off;
	length r_st_len;
	offset r_st_off;
}


```
