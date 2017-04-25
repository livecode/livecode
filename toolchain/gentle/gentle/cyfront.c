/* --PATCH-- */ #include <stdlib.h>
/* --PATCH-- */ #include <stdio.h>
/* --PATCH-- */ #include <string.h>
/*
   GENTLE 97 CAMPUS EDITION

   COPYRIGHT (C) 1992, 1997. All rights reserved.

   Metarga GmbH, Joachim-Friedrich-Str. 54, D-10711 Berlin

   gentle-97-v-4-1-0
*/


typedef long * yy;
#define yyu (-2147483647L)
static yy yynull;
extern yy yyh;
extern yy yyhx;
static yyErr(n,l)
{
yyAbort(n,"cyfront", l);
}
yyeq_IDENT(t1, t2) yy t1, t2;
{
return t1 == t2;
}
yyPrint_IDENT(t) yy t;
{
yyPrintOpaque(t);
}
yybroadcast_IDENT(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_IDENT, t, In, Out)) {
*Out = In;}
}
yyeq_UNIT(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_IDENT((yy)t1[1], (yy)t2[1])
&& yyeq_EXPORT((yy)t1[2], (yy)t2[2])
&& yyeq_DECLARATIONLIST((yy)t1[3], (yy)t2[3])
&& (t1[4] == t2[4])
;
}
}
yyPrint_UNIT(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("unit");
yyFirstArg();
yyPrint_IDENT((yy)t[1]);
yyNextArg();
yyPrint_EXPORT((yy)t[2]);
yyNextArg();
yyPrint_DECLARATIONLIST((yy)t[3]);
yyNextArg();
yyPrint_POS((yy)t[4]);
yyEndArgs();
break;
}
}
yybroadcast_UNIT(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_UNIT, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_IDENT((yy)t[1], A, &B, Handler);
yybroadcast_EXPORT((yy)t[2], B, &A, Handler);
yybroadcast_DECLARATIONLIST((yy)t[3], A, &B, Handler);
*Out = B;
break;
}
}
}
yyeq_EXPORT(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_IDENTLIST((yy)t1[1], (yy)t2[1])
&& (t1[2] == t2[2])
;
}
}
yyPrint_EXPORT(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("export");
yyFirstArg();
yyPrint_IDENTLIST((yy)t[1]);
yyNextArg();
yyPrint_POS((yy)t[2]);
yyEndArgs();
break;
}
}
yybroadcast_EXPORT(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_EXPORT, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_IDENTLIST((yy)t[1], A, &B, Handler);
*Out = B;
break;
}
}
}
yyeq_DECLARATION(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& (t1[1] == t2[1])
&& yyeq_MEMBERS((yy)t1[2], (yy)t2[2])
;
case 2: return (t2[0] == 2)
&& yyeq_IDENT((yy)t1[1], (yy)t2[1])
&& (t1[2] == t2[2])
&& yyeq_ASSOC((yy)t1[3], (yy)t2[3])
;
}
}
yyPrint_DECLARATION(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("root");
yyFirstArg();
yyPrint_POS((yy)t[1]);
yyNextArg();
yyPrint_MEMBERS((yy)t[2]);
yyEndArgs();
break;
case 2: 
yyTerm("declaration");
yyFirstArg();
yyPrint_IDENT((yy)t[1]);
yyNextArg();
yyPrint_POS((yy)t[2]);
yyNextArg();
yyPrint_ASSOC((yy)t[3]);
yyEndArgs();
break;
}
}
yybroadcast_DECLARATION(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_DECLARATION, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_MEMBERS((yy)t[2], A, &B, Handler);
*Out = B;
break;
case 2: 
yybroadcast_IDENT((yy)t[1], A, &B, Handler);
yybroadcast_ASSOC((yy)t[3], B, &A, Handler);
*Out = A;
break;
}
}
}
yyeq_ASSOC(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_TYPEKIND((yy)t1[1], (yy)t2[1])
&& yyeq_FUNCTORSPECLIST((yy)t1[2], (yy)t2[2])
;
case 2: return (t2[0] == 2)
&& yyeq_IDENT((yy)t1[1], (yy)t2[1])
&& (t1[2] == t2[2])
;
case 3: return (t2[0] == 3)
&& yyeq_TABLEFIELDLIST((yy)t1[1], (yy)t2[1])
;
case 4: return (t2[0] == 4)
&& yyeq_CODENAME((yy)t1[1], (yy)t2[1])
&& yyeq_CLASS((yy)t1[2], (yy)t2[2])
&& yyeq_FORMALS((yy)t1[3], (yy)t2[3])
&& yyeq_RULELIST((yy)t1[4], (yy)t2[4])
;
case 5: return (t2[0] == 5)
;
}
}
yyPrint_ASSOC(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("type");
yyFirstArg();
yyPrint_TYPEKIND((yy)t[1]);
yyNextArg();
yyPrint_FUNCTORSPECLIST((yy)t[2]);
yyEndArgs();
break;
case 2: 
yyTerm("var");
yyFirstArg();
yyPrint_IDENT((yy)t[1]);
yyNextArg();
yyPrint_POS((yy)t[2]);
yyEndArgs();
break;
case 3: 
yyTerm("table");
yyFirstArg();
yyPrint_TABLEFIELDLIST((yy)t[1]);
yyEndArgs();
break;
case 4: 
yyTerm("predicate");
yyFirstArg();
yyPrint_CODENAME((yy)t[1]);
yyNextArg();
yyPrint_CLASS((yy)t[2]);
yyNextArg();
yyPrint_FORMALS((yy)t[3]);
yyNextArg();
yyPrint_RULELIST((yy)t[4]);
yyEndArgs();
break;
case 5: 
yyTerm("predefined");
yyNoArgs();
break;
}
}
yybroadcast_ASSOC(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_ASSOC, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_TYPEKIND((yy)t[1], A, &B, Handler);
yybroadcast_FUNCTORSPECLIST((yy)t[2], B, &A, Handler);
*Out = A;
break;
case 2: 
yybroadcast_IDENT((yy)t[1], A, &B, Handler);
*Out = B;
break;
case 3: 
yybroadcast_TABLEFIELDLIST((yy)t[1], A, &B, Handler);
*Out = B;
break;
case 4: 
yybroadcast_CODENAME((yy)t[1], A, &B, Handler);
yybroadcast_CLASS((yy)t[2], B, &A, Handler);
yybroadcast_FORMALS((yy)t[3], A, &B, Handler);
yybroadcast_RULELIST((yy)t[4], B, &A, Handler);
*Out = A;
break;
case 5: 
*Out = A;
break;
}
}
}
yyeq_TYPEKIND(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
;
case 2: return (t2[0] == 2)
;
}
}
yyPrint_TYPEKIND(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("domain");
yyNoArgs();
break;
case 2: 
yyTerm("class");
yyNoArgs();
break;
}
}
yybroadcast_TYPEKIND(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_TYPEKIND, t, In, Out)) {
switch(t[0]) {
case 1: 
*Out = A;
break;
case 2: 
*Out = A;
break;
}
}
}
yyeq_CODENAME(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_IDENT((yy)t1[1], (yy)t2[1])
;
case 2: return (t2[0] == 2)
;
case 3: return (t2[0] == 3)
;
}
}
yyPrint_CODENAME(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("extern");
yyFirstArg();
yyPrint_IDENT((yy)t[1]);
yyEndArgs();
break;
case 2: 
yyTerm("yygoal");
yyNoArgs();
break;
case 3: 
yyTerm("noname");
yyNoArgs();
break;
}
}
yybroadcast_CODENAME(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_CODENAME, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_IDENT((yy)t[1], A, &B, Handler);
*Out = B;
break;
case 2: 
*Out = A;
break;
case 3: 
*Out = A;
break;
}
}
}
yyeq_BASECLASS(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
;
}
}
yyPrint_BASECLASS(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("none");
yyNoArgs();
break;
}
}
yybroadcast_BASECLASS(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_BASECLASS, t, In, Out)) {
switch(t[0]) {
case 1: 
*Out = A;
break;
}
}
}
yyeq_TABLEFIELDLIST(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_TABLEFIELD((yy)t1[1], (yy)t2[1])
&& yyeq_TABLEFIELDLIST((yy)t1[2], (yy)t2[2])
;
case 2: return (t2[0] == 2)
;
}
}
yyPrint_TABLEFIELDLIST(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("tablefieldlist");
yyFirstArg();
yyPrint_TABLEFIELD((yy)t[1]);
yyNextArg();
yyPrint_TABLEFIELDLIST((yy)t[2]);
yyEndArgs();
break;
case 2: 
yyTerm("nil");
yyNoArgs();
break;
}
}
yybroadcast_TABLEFIELDLIST(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_TABLEFIELDLIST, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_TABLEFIELD((yy)t[1], A, &B, Handler);
yybroadcast_TABLEFIELDLIST((yy)t[2], B, &A, Handler);
*Out = A;
break;
case 2: 
*Out = A;
break;
}
}
}
yyeq_TABLEFIELD(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_IDENT((yy)t1[1], (yy)t2[1])
&& yyeq_IDENT((yy)t1[2], (yy)t2[2])
&& (t1[3] == t2[3])
;
}
}
yyPrint_TABLEFIELD(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("tablefield");
yyFirstArg();
yyPrint_IDENT((yy)t[1]);
yyNextArg();
yyPrint_IDENT((yy)t[2]);
yyNextArg();
yyPrint_POS((yy)t[3]);
yyEndArgs();
break;
}
}
yybroadcast_TABLEFIELD(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_TABLEFIELD, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_IDENT((yy)t[1], A, &B, Handler);
yybroadcast_IDENT((yy)t[2], B, &A, Handler);
*Out = A;
break;
}
}
}
yyeq_FUNCTORSPEC(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_IDENT((yy)t1[1], (yy)t2[1])
&& (t1[2] == t2[2])
&& yyeq_ARGSPECLIST((yy)t1[3], (yy)t2[3])
;
}
}
yyPrint_FUNCTORSPEC(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("functorspec");
yyFirstArg();
yyPrint_IDENT((yy)t[1]);
yyNextArg();
yyPrint_POS((yy)t[2]);
yyNextArg();
yyPrint_ARGSPECLIST((yy)t[3]);
yyEndArgs();
break;
}
}
yybroadcast_FUNCTORSPEC(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_FUNCTORSPEC, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_IDENT((yy)t[1], A, &B, Handler);
yybroadcast_ARGSPECLIST((yy)t[3], B, &A, Handler);
*Out = A;
break;
}
}
}
yyeq_ARGSPEC(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_FORMALNAME((yy)t1[1], (yy)t2[1])
&& yyeq_IDENT((yy)t1[2], (yy)t2[2])
&& yyeq_ACCESS((yy)t1[3], (yy)t2[3])
;
}
}
yyPrint_ARGSPEC(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("argspec");
yyFirstArg();
yyPrint_FORMALNAME((yy)t[1]);
yyNextArg();
yyPrint_IDENT((yy)t[2]);
yyNextArg();
yyPrint_ACCESS((yy)t[3]);
yyEndArgs();
break;
}
}
yybroadcast_ARGSPEC(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_ARGSPEC, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_FORMALNAME((yy)t[1], A, &B, Handler);
yybroadcast_IDENT((yy)t[2], B, &A, Handler);
yybroadcast_ACCESS((yy)t[3], A, &B, Handler);
*Out = B;
break;
}
}
}
yyeq_FORMALNAME(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_IDENT((yy)t1[1], (yy)t2[1])
;
case 2: return (t2[0] == 2)
;
}
}
yyPrint_FORMALNAME(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("name");
yyFirstArg();
yyPrint_IDENT((yy)t[1]);
yyEndArgs();
break;
case 2: 
yyTerm("noname");
yyNoArgs();
break;
}
}
yybroadcast_FORMALNAME(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_FORMALNAME, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_IDENT((yy)t[1], A, &B, Handler);
*Out = B;
break;
case 2: 
*Out = A;
break;
}
}
}
yyeq_ACCESS(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
;
case 2: return (t2[0] == 2)
;
}
}
yyPrint_ACCESS(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("var");
yyNoArgs();
break;
case 2: 
yyTerm("const");
yyNoArgs();
break;
}
}
yybroadcast_ACCESS(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_ACCESS, t, In, Out)) {
switch(t[0]) {
case 1: 
*Out = A;
break;
case 2: 
*Out = A;
break;
}
}
}
yyeq_FORMALS(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_ARGSPECLIST((yy)t1[1], (yy)t2[1])
&& yyeq_ARGSPECLIST((yy)t1[2], (yy)t2[2])
;
}
}
yyPrint_FORMALS(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("formals");
yyFirstArg();
yyPrint_ARGSPECLIST((yy)t[1]);
yyNextArg();
yyPrint_ARGSPECLIST((yy)t[2]);
yyEndArgs();
break;
}
}
yybroadcast_FORMALS(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_FORMALS, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_ARGSPECLIST((yy)t[1], A, &B, Handler);
yybroadcast_ARGSPECLIST((yy)t[2], B, &A, Handler);
*Out = A;
break;
}
}
}
yyeq_CLASS(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
;
case 2: return (t2[0] == 2)
;
case 3: return (t2[0] == 3)
;
case 4: return (t2[0] == 4)
;
case 5: return (t2[0] == 5)
;
case 6: return (t2[0] == 6)
;
}
}
yyPrint_CLASS(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("proc");
yyNoArgs();
break;
case 2: 
yyTerm("cond");
yyNoArgs();
break;
case 3: 
yyTerm("nonterm");
yyNoArgs();
break;
case 4: 
yyTerm("token");
yyNoArgs();
break;
case 5: 
yyTerm("choice");
yyNoArgs();
break;
case 6: 
yyTerm("sweep");
yyNoArgs();
break;
}
}
yybroadcast_CLASS(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_CLASS, t, In, Out)) {
switch(t[0]) {
case 1: 
*Out = A;
break;
case 2: 
*Out = A;
break;
case 3: 
*Out = A;
break;
case 4: 
*Out = A;
break;
case 5: 
*Out = A;
break;
case 6: 
*Out = A;
break;
}
}
}
yyeq_RULE(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_LHS((yy)t1[1], (yy)t2[1])
&& yyeq_MEMBERS((yy)t1[2], (yy)t2[2])
&& (t1[3] == t2[3])
;
}
}
yyPrint_RULE(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("rule");
yyFirstArg();
yyPrint_LHS((yy)t[1]);
yyNextArg();
yyPrint_MEMBERS((yy)t[2]);
yyNextArg();
yyPrint_INT((yy)t[3]);
yyEndArgs();
break;
}
}
yybroadcast_RULE(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_RULE, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_LHS((yy)t[1], A, &B, Handler);
yybroadcast_MEMBERS((yy)t[2], B, &A, Handler);
*Out = A;
break;
}
}
}
yyeq_LHS(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_IDENT((yy)t1[1], (yy)t2[1])
&& (t1[2] == t2[2])
&& yyeq_DEFARGLIST((yy)t1[3], (yy)t2[3])
&& yyeq_USEARGLIST((yy)t1[4], (yy)t2[4])
;
}
}
yyPrint_LHS(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("lhs");
yyFirstArg();
yyPrint_IDENT((yy)t[1]);
yyNextArg();
yyPrint_POS((yy)t[2]);
yyNextArg();
yyPrint_DEFARGLIST((yy)t[3]);
yyNextArg();
yyPrint_USEARGLIST((yy)t[4]);
yyEndArgs();
break;
}
}
yybroadcast_LHS(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_LHS, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_IDENT((yy)t[1], A, &B, Handler);
yybroadcast_DEFARGLIST((yy)t[3], B, &A, Handler);
yybroadcast_USEARGLIST((yy)t[4], A, &B, Handler);
*Out = B;
break;
}
}
}
yyeq_MEMBER(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_DEFARG((yy)t1[1], (yy)t2[1])
&& yyeq_USEARG((yy)t1[2], (yy)t2[2])
&& (t1[3] == t2[3])
;
case 2: return (t2[0] == 2)
&& yyeq_IDENT((yy)t1[1], (yy)t2[1])
&& (t1[2] == t2[2])
&& yyeq_USEARG((yy)t1[3], (yy)t2[3])
;
case 3: return (t2[0] == 3)
&& yyeq_IDENT((yy)t1[1], (yy)t2[1])
&& (t1[2] == t2[2])
&& yyeq_DEFARG((yy)t1[3], (yy)t2[3])
;
case 4: return (t2[0] == 4)
&& yyeq_IDENT((yy)t1[1], (yy)t2[1])
&& yyeq_IDENT((yy)t1[2], (yy)t2[2])
&& (t1[3] == t2[3])
&& yyeq_REF_INT((yy)t1[4], (yy)t2[4])
;
case 5: return (t2[0] == 5)
&& yyeq_IDENT((yy)t1[1], (yy)t2[1])
&& yyeq_IDENT((yy)t1[2], (yy)t2[2])
&& yyeq_USEARG((yy)t1[3], (yy)t2[3])
&& (t1[4] == t2[4])
;
case 6: return (t2[0] == 6)
&& yyeq_IDENT((yy)t1[1], (yy)t2[1])
&& yyeq_IDENT((yy)t1[2], (yy)t2[2])
&& yyeq_DEFARG((yy)t1[3], (yy)t2[3])
&& (t1[4] == t2[4])
;
case 7: return (t2[0] == 7)
&& yyeq_IDENT((yy)t1[1], (yy)t2[1])
&& (t1[2] == t2[2])
&& yyeq_USEARGLIST((yy)t1[3], (yy)t2[3])
&& yyeq_DEFARGLIST((yy)t1[4], (yy)t2[4])
&& yyeq_REF_INT((yy)t1[5], (yy)t2[5])
;
case 8: return (t2[0] == 8)
&&(strcmp((char *) t1[1], (char *) t2[1]) == 0)
&& (t1[2] == t2[2])
;
case 9: return (t2[0] == 9)
&& yyeq_DEFARG((yy)t1[1], (yy)t2[1])
&& (t1[2] == t2[2])
;
case 10: return (t2[0] == 10)
&& yyeq_ALTERNATIVELIST((yy)t1[1], (yy)t2[1])
&& yyeq_REF_IDENTLIST((yy)t1[2], (yy)t2[2])
&& (t1[3] == t2[3])
;
case 11: return (t2[0] == 11)
&& yyeq_MEMBERS((yy)t1[1], (yy)t2[1])
&& (t1[2] == t2[2])
;
}
}
yyPrint_MEMBER(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("localasg");
yyFirstArg();
yyPrint_DEFARG((yy)t[1]);
yyNextArg();
yyPrint_USEARG((yy)t[2]);
yyNextArg();
yyPrint_POS((yy)t[3]);
yyEndArgs();
break;
case 2: 
yyTerm("update");
yyFirstArg();
yyPrint_IDENT((yy)t[1]);
yyNextArg();
yyPrint_POS((yy)t[2]);
yyNextArg();
yyPrint_USEARG((yy)t[3]);
yyEndArgs();
break;
case 3: 
yyTerm("query");
yyFirstArg();
yyPrint_IDENT((yy)t[1]);
yyNextArg();
yyPrint_POS((yy)t[2]);
yyNextArg();
yyPrint_DEFARG((yy)t[3]);
yyEndArgs();
break;
case 4: 
yyTerm("newkey");
yyFirstArg();
yyPrint_IDENT((yy)t[1]);
yyNextArg();
yyPrint_IDENT((yy)t[2]);
yyNextArg();
yyPrint_POS((yy)t[3]);
yyNextArg();
yyPrint_REF_INT((yy)t[4]);
yyEndArgs();
break;
case 5: 
yyTerm("table_update");
yyFirstArg();
yyPrint_IDENT((yy)t[1]);
yyNextArg();
yyPrint_IDENT((yy)t[2]);
yyNextArg();
yyPrint_USEARG((yy)t[3]);
yyNextArg();
yyPrint_POS((yy)t[4]);
yyEndArgs();
break;
case 6: 
yyTerm("table_query");
yyFirstArg();
yyPrint_IDENT((yy)t[1]);
yyNextArg();
yyPrint_IDENT((yy)t[2]);
yyNextArg();
yyPrint_DEFARG((yy)t[3]);
yyNextArg();
yyPrint_POS((yy)t[4]);
yyEndArgs();
break;
case 7: 
yyTerm("call");
yyFirstArg();
yyPrint_IDENT((yy)t[1]);
yyNextArg();
yyPrint_POS((yy)t[2]);
yyNextArg();
yyPrint_USEARGLIST((yy)t[3]);
yyNextArg();
yyPrint_DEFARGLIST((yy)t[4]);
yyNextArg();
yyPrint_REF_INT((yy)t[5]);
yyEndArgs();
break;
case 8: 
yyTerm("literal");
yyFirstArg();
yyPrint_STRING((yy)t[1]);
yyNextArg();
yyPrint_POS((yy)t[2]);
yyEndArgs();
break;
case 9: 
yyTerm("posmark");
yyFirstArg();
yyPrint_DEFARG((yy)t[1]);
yyNextArg();
yyPrint_POS((yy)t[2]);
yyEndArgs();
break;
case 10: 
yyTerm("disjunction");
yyFirstArg();
yyPrint_ALTERNATIVELIST((yy)t[1]);
yyNextArg();
yyPrint_REF_IDENTLIST((yy)t[2]);
yyNextArg();
yyPrint_POS((yy)t[3]);
yyEndArgs();
break;
case 11: 
yyTerm("loop");
yyFirstArg();
yyPrint_MEMBERS((yy)t[1]);
yyNextArg();
yyPrint_POS((yy)t[2]);
yyEndArgs();
break;
}
}
yybroadcast_MEMBER(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_MEMBER, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_DEFARG((yy)t[1], A, &B, Handler);
yybroadcast_USEARG((yy)t[2], B, &A, Handler);
*Out = A;
break;
case 2: 
yybroadcast_IDENT((yy)t[1], A, &B, Handler);
yybroadcast_USEARG((yy)t[3], B, &A, Handler);
*Out = A;
break;
case 3: 
yybroadcast_IDENT((yy)t[1], A, &B, Handler);
yybroadcast_DEFARG((yy)t[3], B, &A, Handler);
*Out = A;
break;
case 4: 
yybroadcast_IDENT((yy)t[1], A, &B, Handler);
yybroadcast_IDENT((yy)t[2], B, &A, Handler);
*Out = A;
break;
case 5: 
yybroadcast_IDENT((yy)t[1], A, &B, Handler);
yybroadcast_IDENT((yy)t[2], B, &A, Handler);
yybroadcast_USEARG((yy)t[3], A, &B, Handler);
*Out = B;
break;
case 6: 
yybroadcast_IDENT((yy)t[1], A, &B, Handler);
yybroadcast_IDENT((yy)t[2], B, &A, Handler);
yybroadcast_DEFARG((yy)t[3], A, &B, Handler);
*Out = B;
break;
case 7: 
yybroadcast_IDENT((yy)t[1], A, &B, Handler);
yybroadcast_USEARGLIST((yy)t[3], B, &A, Handler);
yybroadcast_DEFARGLIST((yy)t[4], A, &B, Handler);
*Out = B;
break;
case 8: 
*Out = A;
break;
case 9: 
yybroadcast_DEFARG((yy)t[1], A, &B, Handler);
*Out = B;
break;
case 10: 
yybroadcast_ALTERNATIVELIST((yy)t[1], A, &B, Handler);
*Out = B;
break;
case 11: 
yybroadcast_MEMBERS((yy)t[1], A, &B, Handler);
*Out = B;
break;
}
}
}
yyeq_MEMBERS(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_MEMBERLIST((yy)t1[1], (yy)t2[1])
&& yyeq_REF_INT((yy)t1[2], (yy)t2[2])
;
}
}
yyPrint_MEMBERS(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("members");
yyFirstArg();
yyPrint_MEMBERLIST((yy)t[1]);
yyNextArg();
yyPrint_REF_INT((yy)t[2]);
yyEndArgs();
break;
}
}
yybroadcast_MEMBERS(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_MEMBERS, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_MEMBERLIST((yy)t[1], A, &B, Handler);
*Out = B;
break;
}
}
}
yyeq_DEFARG(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_REF_TEMPO((yy)t1[1], (yy)t2[1])
&& yyeq_IDENT((yy)t1[2], (yy)t2[2])
&& (t1[3] == t2[3])
&& yyeq_DEFARGLIST((yy)t1[4], (yy)t2[4])
;
case 2: return (t2[0] == 2)
&& yyeq_IDENT((yy)t1[1], (yy)t2[1])
&& yyeq_REF_TEMPO((yy)t1[2], (yy)t2[2])
&& yyeq_IDENT((yy)t1[3], (yy)t2[3])
&& (t1[4] == t2[4])
&& yyeq_DEFARGLIST((yy)t1[5], (yy)t2[5])
;
case 3: return (t2[0] == 3)
&& yyeq_REF_TEMPO((yy)t1[1], (yy)t2[1])
&& yyeq_IDENT((yy)t1[2], (yy)t2[2])
&& (t1[3] == t2[3])
;
case 4: return (t2[0] == 4)
&& yyeq_REF_TEMPO((yy)t1[1], (yy)t2[1])
&& yyeq_IDENT((yy)t1[2], (yy)t2[2])
&& (t1[3] == t2[3])
&& yyeq_DEFARG((yy)t1[4], (yy)t2[4])
;
case 5: return (t2[0] == 5)
&& yyeq_REF_TEMPO((yy)t1[1], (yy)t2[1])
&& (t1[2] == t2[2])
;
case 6: return (t2[0] == 6)
;
}
}
yyPrint_DEFARG(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("defterm");
yyFirstArg();
yyPrint_REF_TEMPO((yy)t[1]);
yyNextArg();
yyPrint_IDENT((yy)t[2]);
yyNextArg();
yyPrint_POS((yy)t[3]);
yyNextArg();
yyPrint_DEFARGLIST((yy)t[4]);
yyEndArgs();
break;
case 2: 
yyTerm("deftypedterm");
yyFirstArg();
yyPrint_IDENT((yy)t[1]);
yyNextArg();
yyPrint_REF_TEMPO((yy)t[2]);
yyNextArg();
yyPrint_IDENT((yy)t[3]);
yyNextArg();
yyPrint_POS((yy)t[4]);
yyNextArg();
yyPrint_DEFARGLIST((yy)t[5]);
yyEndArgs();
break;
case 3: 
yyTerm("defvar");
yyFirstArg();
yyPrint_REF_TEMPO((yy)t[1]);
yyNextArg();
yyPrint_IDENT((yy)t[2]);
yyNextArg();
yyPrint_POS((yy)t[3]);
yyEndArgs();
break;
case 4: 
yyTerm("defnamed");
yyFirstArg();
yyPrint_REF_TEMPO((yy)t[1]);
yyNextArg();
yyPrint_IDENT((yy)t[2]);
yyNextArg();
yyPrint_POS((yy)t[3]);
yyNextArg();
yyPrint_DEFARG((yy)t[4]);
yyEndArgs();
break;
case 5: 
yyTerm("defskip");
yyFirstArg();
yyPrint_REF_TEMPO((yy)t[1]);
yyNextArg();
yyPrint_POS((yy)t[2]);
yyEndArgs();
break;
case 6: 
yyTerm("deferror");
yyNoArgs();
break;
}
}
yybroadcast_DEFARG(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_DEFARG, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_IDENT((yy)t[2], A, &B, Handler);
yybroadcast_DEFARGLIST((yy)t[4], B, &A, Handler);
*Out = A;
break;
case 2: 
yybroadcast_IDENT((yy)t[1], A, &B, Handler);
yybroadcast_IDENT((yy)t[3], B, &A, Handler);
yybroadcast_DEFARGLIST((yy)t[5], A, &B, Handler);
*Out = B;
break;
case 3: 
yybroadcast_IDENT((yy)t[2], A, &B, Handler);
*Out = B;
break;
case 4: 
yybroadcast_IDENT((yy)t[2], A, &B, Handler);
yybroadcast_DEFARG((yy)t[4], B, &A, Handler);
*Out = A;
break;
case 5: 
*Out = A;
break;
case 6: 
*Out = A;
break;
}
}
}
yyeq_USEARG(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_REF_TEMPO((yy)t1[1], (yy)t2[1])
&& yyeq_BINOP((yy)t1[2], (yy)t2[2])
&& (t1[3] == t2[3])
&& yyeq_USEARG((yy)t1[4], (yy)t2[4])
&& yyeq_USEARG((yy)t1[5], (yy)t2[5])
;
case 2: return (t2[0] == 2)
&& yyeq_REF_TEMPO((yy)t1[1], (yy)t2[1])
&& yyeq_MONOP((yy)t1[2], (yy)t2[2])
&& (t1[3] == t2[3])
&& yyeq_USEARG((yy)t1[4], (yy)t2[4])
;
case 3: return (t2[0] == 3)
&& yyeq_REF_TEMPO((yy)t1[1], (yy)t2[1])
&& yyeq_REF_INT((yy)t1[2], (yy)t2[2])
&& yyeq_IDENT((yy)t1[3], (yy)t2[3])
&& (t1[4] == t2[4])
&& yyeq_USEARGLIST((yy)t1[5], (yy)t2[5])
;
case 4: return (t2[0] == 4)
&& yyeq_IDENT((yy)t1[1], (yy)t2[1])
&& yyeq_USEARG((yy)t1[2], (yy)t2[2])
&& (t1[3] == t2[3])
;
case 5: return (t2[0] == 5)
&& yyeq_REF_TEMPO((yy)t1[1], (yy)t2[1])
&& yyeq_IDENT((yy)t1[2], (yy)t2[2])
&& (t1[3] == t2[3])
;
case 6: return (t2[0] == 6)
&& yyeq_REF_TEMPO((yy)t1[1], (yy)t2[1])
&& (t1[2] == t2[2])
&& (t1[3] == t2[3])
;
case 7: return (t2[0] == 7)
&& yyeq_REF_TEMPO((yy)t1[1], (yy)t2[1])
&&(strcmp((char *) t1[2], (char *) t2[2]) == 0)
&& (t1[3] == t2[3])
;
}
}
yyPrint_USEARG(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("usebinary");
yyFirstArg();
yyPrint_REF_TEMPO((yy)t[1]);
yyNextArg();
yyPrint_BINOP((yy)t[2]);
yyNextArg();
yyPrint_POS((yy)t[3]);
yyNextArg();
yyPrint_USEARG((yy)t[4]);
yyNextArg();
yyPrint_USEARG((yy)t[5]);
yyEndArgs();
break;
case 2: 
yyTerm("useunary");
yyFirstArg();
yyPrint_REF_TEMPO((yy)t[1]);
yyNextArg();
yyPrint_MONOP((yy)t[2]);
yyNextArg();
yyPrint_POS((yy)t[3]);
yyNextArg();
yyPrint_USEARG((yy)t[4]);
yyEndArgs();
break;
case 3: 
yyTerm("useterm");
yyFirstArg();
yyPrint_REF_TEMPO((yy)t[1]);
yyNextArg();
yyPrint_REF_INT((yy)t[2]);
yyNextArg();
yyPrint_IDENT((yy)t[3]);
yyNextArg();
yyPrint_POS((yy)t[4]);
yyNextArg();
yyPrint_USEARGLIST((yy)t[5]);
yyEndArgs();
break;
case 4: 
yyTerm("usetypedterm");
yyFirstArg();
yyPrint_IDENT((yy)t[1]);
yyNextArg();
yyPrint_USEARG((yy)t[2]);
yyNextArg();
yyPrint_POS((yy)t[3]);
yyEndArgs();
break;
case 5: 
yyTerm("usevar");
yyFirstArg();
yyPrint_REF_TEMPO((yy)t[1]);
yyNextArg();
yyPrint_IDENT((yy)t[2]);
yyNextArg();
yyPrint_POS((yy)t[3]);
yyEndArgs();
break;
case 6: 
yyTerm("usenumber");
yyFirstArg();
yyPrint_REF_TEMPO((yy)t[1]);
yyNextArg();
yyPrint_INT((yy)t[2]);
yyNextArg();
yyPrint_POS((yy)t[3]);
yyEndArgs();
break;
case 7: 
yyTerm("usestring");
yyFirstArg();
yyPrint_REF_TEMPO((yy)t[1]);
yyNextArg();
yyPrint_STRING((yy)t[2]);
yyNextArg();
yyPrint_POS((yy)t[3]);
yyEndArgs();
break;
}
}
yybroadcast_USEARG(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_USEARG, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_BINOP((yy)t[2], A, &B, Handler);
yybroadcast_USEARG((yy)t[4], B, &A, Handler);
yybroadcast_USEARG((yy)t[5], A, &B, Handler);
*Out = B;
break;
case 2: 
yybroadcast_MONOP((yy)t[2], A, &B, Handler);
yybroadcast_USEARG((yy)t[4], B, &A, Handler);
*Out = A;
break;
case 3: 
yybroadcast_IDENT((yy)t[3], A, &B, Handler);
yybroadcast_USEARGLIST((yy)t[5], B, &A, Handler);
*Out = A;
break;
case 4: 
yybroadcast_IDENT((yy)t[1], A, &B, Handler);
yybroadcast_USEARG((yy)t[2], B, &A, Handler);
*Out = A;
break;
case 5: 
yybroadcast_IDENT((yy)t[2], A, &B, Handler);
*Out = B;
break;
case 6: 
*Out = A;
break;
case 7: 
*Out = A;
break;
}
}
}
yyeq_TEMPO(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& (t1[1] == t2[1])
&& yyeq_TEMPO((yy)t1[2], (yy)t2[2])
;
case 2: return (t2[0] == 2)
;
}
}
yyPrint_TEMPO(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("indexed");
yyFirstArg();
yyPrint_INT((yy)t[1]);
yyNextArg();
yyPrint_TEMPO((yy)t[2]);
yyEndArgs();
break;
case 2: 
yyTerm("nil");
yyNoArgs();
break;
}
}
yybroadcast_TEMPO(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_TEMPO, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_TEMPO((yy)t[2], A, &B, Handler);
*Out = B;
break;
case 2: 
*Out = A;
break;
}
}
}
yyeq_BINOP(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
;
case 2: return (t2[0] == 2)
;
case 3: return (t2[0] == 3)
;
case 4: return (t2[0] == 4)
;
}
}
yyPrint_BINOP(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("plus");
yyNoArgs();
break;
case 2: 
yyTerm("minus");
yyNoArgs();
break;
case 3: 
yyTerm("mult");
yyNoArgs();
break;
case 4: 
yyTerm("div");
yyNoArgs();
break;
}
}
yybroadcast_BINOP(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_BINOP, t, In, Out)) {
switch(t[0]) {
case 1: 
*Out = A;
break;
case 2: 
*Out = A;
break;
case 3: 
*Out = A;
break;
case 4: 
*Out = A;
break;
}
}
}
yyeq_MONOP(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
;
case 2: return (t2[0] == 2)
;
}
}
yyPrint_MONOP(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("plus");
yyNoArgs();
break;
case 2: 
yyTerm("minus");
yyNoArgs();
break;
}
}
yybroadcast_MONOP(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_MONOP, t, In, Out)) {
switch(t[0]) {
case 1: 
*Out = A;
break;
case 2: 
*Out = A;
break;
}
}
}
yyeq_REF_IDENTLIST(t1, t2) yy t1, t2;
{
return t1 == t2;
}
yyPrint_REF_IDENTLIST(t) yy t;
{
yyPrintIndex(t);
}
yyeq_UNITLIST(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_UNIT((yy)t1[1], (yy)t2[1])
&& yyeq_UNITLIST((yy)t1[2], (yy)t2[2])
;
case 2: return (t2[0] == 2)
;
}
}
yyPrint_UNITLIST(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("unitlist");
yyFirstArg();
yyPrint_UNIT((yy)t[1]);
yyNextArg();
yyPrint_UNITLIST((yy)t[2]);
yyEndArgs();
break;
case 2: 
yyTerm("nil");
yyNoArgs();
break;
}
}
yybroadcast_UNITLIST(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_UNITLIST, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_UNIT((yy)t[1], A, &B, Handler);
yybroadcast_UNITLIST((yy)t[2], B, &A, Handler);
*Out = A;
break;
case 2: 
*Out = A;
break;
}
}
}
yyeq_IDENTLIST(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_IDENT((yy)t1[1], (yy)t2[1])
&& yyeq_IDENTLIST((yy)t1[2], (yy)t2[2])
;
case 2: return (t2[0] == 2)
;
}
}
yyPrint_IDENTLIST(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("identlist");
yyFirstArg();
yyPrint_IDENT((yy)t[1]);
yyNextArg();
yyPrint_IDENTLIST((yy)t[2]);
yyEndArgs();
break;
case 2: 
yyTerm("nil");
yyNoArgs();
break;
}
}
yybroadcast_IDENTLIST(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_IDENTLIST, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_IDENT((yy)t[1], A, &B, Handler);
yybroadcast_IDENTLIST((yy)t[2], B, &A, Handler);
*Out = A;
break;
case 2: 
*Out = A;
break;
}
}
}
yyeq_DECLARATIONLIST(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_DECLARATION((yy)t1[1], (yy)t2[1])
&& yyeq_DECLARATIONLIST((yy)t1[2], (yy)t2[2])
;
case 2: return (t2[0] == 2)
;
}
}
yyPrint_DECLARATIONLIST(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("declarationlist");
yyFirstArg();
yyPrint_DECLARATION((yy)t[1]);
yyNextArg();
yyPrint_DECLARATIONLIST((yy)t[2]);
yyEndArgs();
break;
case 2: 
yyTerm("nil");
yyNoArgs();
break;
}
}
yybroadcast_DECLARATIONLIST(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_DECLARATIONLIST, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_DECLARATION((yy)t[1], A, &B, Handler);
yybroadcast_DECLARATIONLIST((yy)t[2], B, &A, Handler);
*Out = A;
break;
case 2: 
*Out = A;
break;
}
}
}
yyeq_FUNCTORSPECLIST(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_FUNCTORSPEC((yy)t1[1], (yy)t2[1])
&& yyeq_FUNCTORSPECLIST((yy)t1[2], (yy)t2[2])
;
case 2: return (t2[0] == 2)
;
}
}
yyPrint_FUNCTORSPECLIST(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("functorspeclist");
yyFirstArg();
yyPrint_FUNCTORSPEC((yy)t[1]);
yyNextArg();
yyPrint_FUNCTORSPECLIST((yy)t[2]);
yyEndArgs();
break;
case 2: 
yyTerm("nil");
yyNoArgs();
break;
}
}
yybroadcast_FUNCTORSPECLIST(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_FUNCTORSPECLIST, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_FUNCTORSPEC((yy)t[1], A, &B, Handler);
yybroadcast_FUNCTORSPECLIST((yy)t[2], B, &A, Handler);
*Out = A;
break;
case 2: 
*Out = A;
break;
}
}
}
yyeq_ARGSPECLIST(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_ARGSPEC((yy)t1[1], (yy)t2[1])
&& (t1[2] == t2[2])
&& yyeq_ARGSPECLIST((yy)t1[3], (yy)t2[3])
;
case 2: return (t2[0] == 2)
;
}
}
yyPrint_ARGSPECLIST(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("argspeclist");
yyFirstArg();
yyPrint_ARGSPEC((yy)t[1]);
yyNextArg();
yyPrint_POS((yy)t[2]);
yyNextArg();
yyPrint_ARGSPECLIST((yy)t[3]);
yyEndArgs();
break;
case 2: 
yyTerm("nil");
yyNoArgs();
break;
}
}
yybroadcast_ARGSPECLIST(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_ARGSPECLIST, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_ARGSPEC((yy)t[1], A, &B, Handler);
yybroadcast_ARGSPECLIST((yy)t[3], B, &A, Handler);
*Out = A;
break;
case 2: 
*Out = A;
break;
}
}
}
yyeq_RULELIST(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_RULE((yy)t1[1], (yy)t2[1])
&& yyeq_RULELIST((yy)t1[2], (yy)t2[2])
;
case 2: return (t2[0] == 2)
;
}
}
yyPrint_RULELIST(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("rulelist");
yyFirstArg();
yyPrint_RULE((yy)t[1]);
yyNextArg();
yyPrint_RULELIST((yy)t[2]);
yyEndArgs();
break;
case 2: 
yyTerm("nil");
yyNoArgs();
break;
}
}
yybroadcast_RULELIST(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_RULELIST, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_RULE((yy)t[1], A, &B, Handler);
yybroadcast_RULELIST((yy)t[2], B, &A, Handler);
*Out = A;
break;
case 2: 
*Out = A;
break;
}
}
}
yyeq_MEMBERLIST(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_MEMBER((yy)t1[1], (yy)t2[1])
&& yyeq_MEMBERLIST((yy)t1[2], (yy)t2[2])
;
case 2: return (t2[0] == 2)
;
}
}
yyPrint_MEMBERLIST(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("memberlist");
yyFirstArg();
yyPrint_MEMBER((yy)t[1]);
yyNextArg();
yyPrint_MEMBERLIST((yy)t[2]);
yyEndArgs();
break;
case 2: 
yyTerm("nil");
yyNoArgs();
break;
}
}
yybroadcast_MEMBERLIST(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_MEMBERLIST, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_MEMBER((yy)t[1], A, &B, Handler);
yybroadcast_MEMBERLIST((yy)t[2], B, &A, Handler);
*Out = A;
break;
case 2: 
*Out = A;
break;
}
}
}
yyeq_ALTERNATIVELIST(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_MEMBERS((yy)t1[1], (yy)t2[1])
&& (t1[2] == t2[2])
&& yyeq_ALTERNATIVELIST((yy)t1[3], (yy)t2[3])
;
case 2: return (t2[0] == 2)
;
}
}
yyPrint_ALTERNATIVELIST(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("alternativelist");
yyFirstArg();
yyPrint_MEMBERS((yy)t[1]);
yyNextArg();
yyPrint_POS((yy)t[2]);
yyNextArg();
yyPrint_ALTERNATIVELIST((yy)t[3]);
yyEndArgs();
break;
case 2: 
yyTerm("nil");
yyNoArgs();
break;
}
}
yybroadcast_ALTERNATIVELIST(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_ALTERNATIVELIST, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_MEMBERS((yy)t[1], A, &B, Handler);
yybroadcast_ALTERNATIVELIST((yy)t[3], B, &A, Handler);
*Out = A;
break;
case 2: 
*Out = A;
break;
}
}
}
yyeq_USEARGLIST(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_USEARG((yy)t1[1], (yy)t2[1])
&& (t1[2] == t2[2])
&& yyeq_USEARGLIST((yy)t1[3], (yy)t2[3])
;
case 2: return (t2[0] == 2)
&& (t1[1] == t2[1])
;
}
}
yyPrint_USEARGLIST(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("usearglist");
yyFirstArg();
yyPrint_USEARG((yy)t[1]);
yyNextArg();
yyPrint_POS((yy)t[2]);
yyNextArg();
yyPrint_USEARGLIST((yy)t[3]);
yyEndArgs();
break;
case 2: 
yyTerm("nil");
yyFirstArg();
yyPrint_POS((yy)t[1]);
yyEndArgs();
break;
}
}
yybroadcast_USEARGLIST(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_USEARGLIST, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_USEARG((yy)t[1], A, &B, Handler);
yybroadcast_USEARGLIST((yy)t[3], B, &A, Handler);
*Out = A;
break;
case 2: 
*Out = A;
break;
}
}
}
yyeq_DEFARGLIST(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_DEFARG((yy)t1[1], (yy)t2[1])
&& (t1[2] == t2[2])
&& yyeq_DEFARGLIST((yy)t1[3], (yy)t2[3])
;
case 2: return (t2[0] == 2)
&& (t1[1] == t2[1])
;
}
}
yyPrint_DEFARGLIST(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("defarglist");
yyFirstArg();
yyPrint_DEFARG((yy)t[1]);
yyNextArg();
yyPrint_POS((yy)t[2]);
yyNextArg();
yyPrint_DEFARGLIST((yy)t[3]);
yyEndArgs();
break;
case 2: 
yyTerm("nil");
yyFirstArg();
yyPrint_POS((yy)t[1]);
yyEndArgs();
break;
}
}
yybroadcast_DEFARGLIST(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_DEFARGLIST, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_DEFARG((yy)t[1], A, &B, Handler);
yybroadcast_DEFARGLIST((yy)t[3], B, &A, Handler);
*Out = A;
break;
case 2: 
*Out = A;
break;
}
}
}
yy yyglov_currentGroupIdent = (yy) yyu;
yyeq_BOOL(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
;
case 2: return (t2[0] == 2)
;
}
}
yyPrint_BOOL(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("yes");
yyNoArgs();
break;
case 2: 
yyTerm("no");
yyNoArgs();
break;
}
}
yybroadcast_BOOL(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_BOOL, t, In, Out)) {
switch(t[0]) {
case 1: 
*Out = A;
break;
case 2: 
*Out = A;
break;
}
}
}
yy yyglov_containsGrammar = (yy) yyu;
yy yyglov_containsRoot = (yy) yyu;
yy yyglov_maxAttr = (yy) yyu;
yy yyglov_tokenList = (yy) yyu;
yyeq_TOKENLIST(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_TokenIndex((yy)t1[1], (yy)t2[1])
&& yyeq_TOKENLIST((yy)t1[2], (yy)t2[2])
;
case 2: return (t2[0] == 2)
;
}
}
yyPrint_TOKENLIST(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("list");
yyFirstArg();
yyPrint_TokenIndex((yy)t[1]);
yyNextArg();
yyPrint_TOKENLIST((yy)t[2]);
yyEndArgs();
break;
case 2: 
yyTerm("nil");
yyNoArgs();
break;
}
}
yybroadcast_TOKENLIST(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_TOKENLIST, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_TOKENLIST((yy)t[2], A, &B, Handler);
*Out = B;
break;
case 2: 
*Out = A;
break;
}
}
}
yyeq_TokenIndex(t1, t2) yy t1, t2;
{
return t1 == t2;
}
yyPrint_TokenIndex(t) yy t;
{
yyPrintIndex(t);
}
yy yyglov_errorId = (yy) yyu;
yy yyglov_id_INT = (yy) yyu;
yy yyglov_id_STRING = (yy) yyu;
yy yyglov_id_POS = (yy) yyu;
yy yyglov_importedDeclarations = (yy) yyu;
yy yyglov_declaredVars = (yy) yyu;
yyeq_TYPEDIDLIST(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_TYPEDID((yy)t1[1], (yy)t2[1])
&& yyeq_TYPEDIDLIST((yy)t1[2], (yy)t2[2])
;
case 2: return (t2[0] == 2)
;
}
}
yyPrint_TYPEDIDLIST(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("typedidlist");
yyFirstArg();
yyPrint_TYPEDID((yy)t[1]);
yyNextArg();
yyPrint_TYPEDIDLIST((yy)t[2]);
yyEndArgs();
break;
case 2: 
yyTerm("nil");
yyNoArgs();
break;
}
}
yybroadcast_TYPEDIDLIST(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_TYPEDIDLIST, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_TYPEDID((yy)t[1], A, &B, Handler);
yybroadcast_TYPEDIDLIST((yy)t[2], B, &A, Handler);
*Out = A;
break;
case 2: 
*Out = A;
break;
}
}
}
yyeq_TYPEDID(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_IDENT((yy)t1[1], (yy)t2[1])
&& yyeq_IDENT((yy)t1[2], (yy)t2[2])
;
}
}
yyPrint_TYPEDID(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("typedid");
yyFirstArg();
yyPrint_IDENT((yy)t[1]);
yyNextArg();
yyPrint_IDENT((yy)t[2]);
yyEndArgs();
break;
}
}
yybroadcast_TYPEDID(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_TYPEDID, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_IDENT((yy)t[1], A, &B, Handler);
yybroadcast_IDENT((yy)t[2], B, &A, Handler);
*Out = A;
break;
}
}
}
yy yyglov_spaceRequired = (yy) yyu;
yy yyglov_currentTokenCode = (yy) yyu;
yy yyglov_currentProcNumber = (yy) yyu;
yy yyglov_currentRuleNumber = (yy) yyu;
yyeq_CODEKIND(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
;
case 2: return (t2[0] == 2)
;
}
}
yyPrint_CODEKIND(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("grammar_code");
yyNoArgs();
break;
case 2: 
yyTerm("predicate_code");
yyNoArgs();
break;
}
}
yybroadcast_CODEKIND(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_CODEKIND, t, In, Out)) {
switch(t[0]) {
case 1: 
*Out = A;
break;
case 2: 
*Out = A;
break;
}
}
}
yyeq_RULEGROUPLIST(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_RULEGROUP((yy)t1[1], (yy)t2[1])
&& yyeq_RULEGROUPLIST((yy)t1[2], (yy)t2[2])
;
case 2: return (t2[0] == 2)
;
}
}
yyPrint_RULEGROUPLIST(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("list");
yyFirstArg();
yyPrint_RULEGROUP((yy)t[1]);
yyNextArg();
yyPrint_RULEGROUPLIST((yy)t[2]);
yyEndArgs();
break;
case 2: 
yyTerm("nil");
yyNoArgs();
break;
}
}
yybroadcast_RULEGROUPLIST(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_RULEGROUPLIST, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_RULEGROUP((yy)t[1], A, &B, Handler);
yybroadcast_RULEGROUPLIST((yy)t[2], B, &A, Handler);
*Out = A;
break;
case 2: 
*Out = A;
break;
}
}
}
yyeq_RULEGROUP(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_IDENT((yy)t1[1], (yy)t2[1])
&& yyeq_RULE_ENTRY((yy)t1[2], (yy)t2[2])
;
}
}
yyPrint_RULEGROUP(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("rulegroup");
yyFirstArg();
yyPrint_IDENT((yy)t[1]);
yyNextArg();
yyPrint_RULE_ENTRY((yy)t[2]);
yyEndArgs();
break;
}
}
yybroadcast_RULEGROUP(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_RULEGROUP, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_IDENT((yy)t[1], A, &B, Handler);
*Out = B;
break;
}
}
}
yyeq_RULE_ENTRY(t1, t2) yy t1, t2;
{
return t1 == t2;
}
yyPrint_RULE_ENTRY(t) yy t;
{
yyPrintIndex(t);
}
yy yyglov_localVars = (yy) yyu;
yy yyglov_env = (yy) yyu;
yyeq_ENV(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_IDENTLIST((yy)t1[1], (yy)t2[1])
&& yyeq_ENV((yy)t1[2], (yy)t2[2])
;
case 2: return (t2[0] == 2)
;
}
}
yyPrint_ENV(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("env");
yyFirstArg();
yyPrint_IDENTLIST((yy)t[1]);
yyNextArg();
yyPrint_ENV((yy)t[2]);
yyEndArgs();
break;
case 2: 
yyTerm("nil");
yyNoArgs();
break;
}
}
yybroadcast_ENV(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_ENV, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_IDENTLIST((yy)t[1], A, &B, Handler);
yybroadcast_ENV((yy)t[2], B, &A, Handler);
*Out = A;
break;
case 2: 
*Out = A;
break;
}
}
}
yy yyglov_currentFailLabel = (yy) yyu;
yy yyglov_currentFailLabelUsed = (yy) yyu;
yy yyglov_currentSuccessLabel = (yy) yyu;
yyeq_TYPECHECKFLAG(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
;
case 2: return (t2[0] == 2)
;
}
}
yyPrint_TYPECHECKFLAG(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("typecheck");
yyNoArgs();
break;
case 2: 
yyTerm("no_typecheck");
yyNoArgs();
break;
}
}
yybroadcast_TYPECHECKFLAG(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_TYPECHECKFLAG, t, In, Out)) {
switch(t[0]) {
case 1: 
*Out = A;
break;
case 2: 
*Out = A;
break;
}
}
}
yyeq_REF_TEMPO(t1, t2) yy t1, t2;
{
return t1 == t2;
}
yyPrint_REF_TEMPO(t) yy t;
{
yyPrintIndex(t);
}
yyeq_REF_INT(t1, t2) yy t1, t2;
{
return t1 == t2;
}
yyPrint_REF_INT(t) yy t;
{
yyPrintIndex(t);
}
yy yyglov_choice_Declarations = (yy) yyu;
yy yyglov_choice_Types = (yy) yyu;
yy yyglov_insideChoiceRule = (yy) yyu;
yyeq_CHOICETYPELIST(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_IDENT((yy)t1[1], (yy)t2[1])
&& yyeq_CHOICETYPE((yy)t1[2], (yy)t2[2])
&& yyeq_CHOICETYPELIST((yy)t1[3], (yy)t2[3])
;
case 2: return (t2[0] == 2)
;
}
}
yyPrint_CHOICETYPELIST(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("list");
yyFirstArg();
yyPrint_IDENT((yy)t[1]);
yyNextArg();
yyPrint_CHOICETYPE((yy)t[2]);
yyNextArg();
yyPrint_CHOICETYPELIST((yy)t[3]);
yyEndArgs();
break;
case 2: 
yyTerm("nil");
yyNoArgs();
break;
}
}
yybroadcast_CHOICETYPELIST(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_CHOICETYPELIST, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_IDENT((yy)t[1], A, &B, Handler);
yybroadcast_CHOICETYPELIST((yy)t[3], B, &A, Handler);
*Out = A;
break;
case 2: 
*Out = A;
break;
}
}
}
yyeq_CHOICEFUNCTORLIST(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_IDENT((yy)t1[1], (yy)t2[1])
&& yyeq_CHOICEFUNCTOR((yy)t1[2], (yy)t2[2])
&& yyeq_CHOICEFUNCTORLIST((yy)t1[3], (yy)t2[3])
;
case 2: return (t2[0] == 2)
;
}
}
yyPrint_CHOICEFUNCTORLIST(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("list");
yyFirstArg();
yyPrint_IDENT((yy)t[1]);
yyNextArg();
yyPrint_CHOICEFUNCTOR((yy)t[2]);
yyNextArg();
yyPrint_CHOICEFUNCTORLIST((yy)t[3]);
yyEndArgs();
break;
case 2: 
yyTerm("nil");
yyNoArgs();
break;
}
}
yybroadcast_CHOICEFUNCTORLIST(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_CHOICEFUNCTORLIST, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_IDENT((yy)t[1], A, &B, Handler);
yybroadcast_CHOICEFUNCTORLIST((yy)t[3], B, &A, Handler);
*Out = A;
break;
case 2: 
*Out = A;
break;
}
}
}
yyeq_CHOICETYPE(t1, t2) yy t1, t2;
{
return t1 == t2;
}
yyPrint_CHOICETYPE(t) yy t;
{
yyPrintIndex(t);
}
yyeq_CHOICEFUNCTOR(t1, t2) yy t1, t2;
{
return t1 == t2;
}
yyPrint_CHOICEFUNCTOR(t) yy t;
{
yyPrintIndex(t);
}
yy yyglov_current_choice_type = (yy) yyu;
yyeq_PREDICATELIST(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_IDENT((yy)t1[1], (yy)t2[1])
&& yyeq_PREDICATELIST((yy)t1[2], (yy)t2[2])
;
case 2: return (t2[0] == 2)
;
}
}
yyPrint_PREDICATELIST(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("list");
yyFirstArg();
yyPrint_IDENT((yy)t[1]);
yyNextArg();
yyPrint_PREDICATELIST((yy)t[2]);
yyEndArgs();
break;
case 2: 
yyTerm("nil");
yyNoArgs();
break;
}
}
yybroadcast_PREDICATELIST(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_PREDICATELIST, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_IDENT((yy)t[1], A, &B, Handler);
yybroadcast_PREDICATELIST((yy)t[2], B, &A, Handler);
*Out = A;
break;
case 2: 
*Out = A;
break;
}
}
}
yy yyglov_tryLabel = (yy) yyu;
yyeq_PATH(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_PATH((yy)t1[1], (yy)t2[1])
&& (t1[2] == t2[2])
;
case 2: return (t2[0] == 2)
;
}
}
yyPrint_PATH(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("path");
yyFirstArg();
yyPrint_PATH((yy)t[1]);
yyNextArg();
yyPrint_INT((yy)t[2]);
yyEndArgs();
break;
case 2: 
yyTerm("nil");
yyNoArgs();
break;
}
}
yybroadcast_PATH(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_PATH, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_PATH((yy)t[1], A, &B, Handler);
*Out = B;
break;
case 2: 
*Out = A;
break;
}
}
}
yyeq_VARPATHES(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_IDENT((yy)t1[1], (yy)t2[1])
&& yyeq_PATH((yy)t1[2], (yy)t2[2])
&& yyeq_VARPATHES((yy)t1[3], (yy)t2[3])
;
case 2: return (t2[0] == 2)
;
}
}
yyPrint_VARPATHES(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("varpathes");
yyFirstArg();
yyPrint_IDENT((yy)t[1]);
yyNextArg();
yyPrint_PATH((yy)t[2]);
yyNextArg();
yyPrint_VARPATHES((yy)t[3]);
yyEndArgs();
break;
case 2: 
yyTerm("nil");
yyNoArgs();
break;
}
}
yybroadcast_VARPATHES(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_VARPATHES, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_IDENT((yy)t[1], A, &B, Handler);
yybroadcast_PATH((yy)t[2], B, &A, Handler);
yybroadcast_VARPATHES((yy)t[3], A, &B, Handler);
*Out = B;
break;
case 2: 
*Out = A;
break;
}
}
}
yyeq_CHOICEMEMBERLIST(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
&& yyeq_IDENT((yy)t1[1], (yy)t2[1])
&& yyeq_IDENT((yy)t1[2], (yy)t2[2])
&& yyeq_PATH((yy)t1[3], (yy)t2[3])
&& yyeq_CHOICETYPE((yy)t1[4], (yy)t2[4])
&& yyeq_CHOICEMEMBERLIST((yy)t1[5], (yy)t2[5])
;
case 2: return (t2[0] == 2)
;
}
}
yyPrint_CHOICEMEMBERLIST(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("list");
yyFirstArg();
yyPrint_IDENT((yy)t[1]);
yyNextArg();
yyPrint_IDENT((yy)t[2]);
yyNextArg();
yyPrint_PATH((yy)t[3]);
yyNextArg();
yyPrint_CHOICETYPE((yy)t[4]);
yyNextArg();
yyPrint_CHOICEMEMBERLIST((yy)t[5]);
yyEndArgs();
break;
case 2: 
yyTerm("nil");
yyNoArgs();
break;
}
}
yybroadcast_CHOICEMEMBERLIST(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_CHOICEMEMBERLIST, t, In, Out)) {
switch(t[0]) {
case 1: 
yybroadcast_IDENT((yy)t[1], A, &B, Handler);
yybroadcast_IDENT((yy)t[2], B, &A, Handler);
yybroadcast_PATH((yy)t[3], A, &B, Handler);
yybroadcast_CHOICEMEMBERLIST((yy)t[5], B, &A, Handler);
*Out = A;
break;
case 2: 
*Out = A;
break;
}
}
}
yy yyglov_pathes = (yy) yyu;
yyeq_EXPORTABLE(t1, t2) yy t1, t2;
{
switch(t1[0]) {
case 1: return (t2[0] == 1)
;
case 2: return (t2[0] == 2)
;
}
}
yyPrint_EXPORTABLE(t) yy t;
{
switch(t[0]) {
case 1: 
yyTerm("exportable");
yyNoArgs();
break;
case 2: 
yyTerm("not_exportable");
yyNoArgs();
break;
}
}
yybroadcast_EXPORTABLE(t,In,Out,Handler)
yy t, In, *Out; int (*Handler) ();
{
yy A, B;
A = In;
if (! Handler(yybroadcast_EXPORTABLE, t, In, Out)) {
switch(t[0]) {
case 1: 
*Out = A;
break;
case 2: 
*Out = A;
break;
}
}
}
int yyparse_rc = 0;
ROOT()
{
extern char *THIS_RUNTIME_SYSTEM;
char *GENTLE = "Gentle 3.0 01100401 (C) 1992, 1997";
if (strcmp(THIS_RUNTIME_SYSTEM, GENTLE))
{ printf("INCONSISTENT GENTLE RUNTIME SYSTEM\n"); exit(1); }
yyExtend();
yyparse_rc = yyparse();
}
PrepareImport(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_T;
yy yy_0_1_2;
yy yy_1_1;
yy yyv_S;
yy yy_1_2;
yy yy_2_1;
yy yy_3_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_157_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_H = yy_0_1_1;
yyv_T = yy_0_1_2;
yy_1_1 = yyv_H;
id_to_string(yy_1_1, &yy_1_2);
yyv_S = yy_1_2;
yy_2_1 = yyv_S;
define_file(yy_2_1);
yy_3_1 = yyv_T;
PrepareImport(yy_3_1);
return;
yyfl_157_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_157_2;
return;
yyfl_157_2 : ;
}
yyErr(2,1101);
}
New_REF_IDENTLIST(yyout_1)
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_P;
yyb = yyh;
yyh += 2; if (yyh > yyhx) yyExtend();
yyv_P = yyb + 0;
yyb[1] = yyu;
yy_0_1 = yyv_P;
*yyout_1 = yy_0_1;
return;
}
}
EnterLiteral(yyin_1, yyin_2, yyout_1)
yy yyin_1;
yy yyin_2;
yy *yyout_1;
{
{
yy yyb;
yy yyv_Str;
yy yy_0_1;
yy yyv_Pos;
yy yy_0_2;
yy yy_0_3;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yy_1_1_2_1;
yy yy_1_1_2_2;
yy yyv_L;
yy yy_2;
yy yy_3_1;
yy yy_3_2;
yy yy_3_3;
yy yyv_Id;
yy yy_3_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Str = yy_0_1;
yyv_Pos = yy_0_2;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Str;
yy_1_1_1_2 = ((yy)"");
if (strcmp((char *) yy_1_1_1_1, (char *) yy_1_1_1_2)  !=  0) goto yyfl_166_1_1_1;
yy_1_1_2_1 = ((yy)"empty string not allowed as token");
yy_1_1_2_2 = yyv_Pos;
MESSAGE(yy_1_1_2_1, yy_1_1_2_2);
goto yysl_166_1_1;
yyfl_166_1_1_1 : ;
goto yysl_166_1_1;
yysl_166_1_1 : ;
yyb = yysb;
}
yy_2 = yyglov_tokenList;
if (yy_2 == (yy) yyu) yyErr(1,1142);
yyv_L = yy_2;
yy_3_1 = yyv_L;
yy_3_2 = yyv_Str;
yy_3_3 = yyv_Pos;
LookupAnonToken(yy_3_1, yy_3_2, yy_3_3, &yy_3_4);
yyv_Id = yy_3_4;
yy_0_3 = yyv_Id;
*yyout_1 = yy_0_3;
return;
}
}
LookupAnonToken(yyin_1, yyin_2, yyin_3, yyout_1)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Index;
yy yy_0_1_1;
yy yyv_Tail;
yy yy_0_1_2;
yy yyv_Str;
yy yy_0_2;
yy yyv_Pos;
yy yy_0_3;
yy yy_0_4;
yy yyv_Repr;
yy yy_1;
yy yy_2_1_1_1;
yy yy_2_1_1_2;
yy yyv_Ident;
yy yy_2_1_2;
yy yy_2_1_3_1_1;
yy yy_2_1_3_1_2_1;
yy yy_2_1_3_1_2_2;
yy yy_2_1_3_1_2_3;
yy yy_2_1_3_1_2_4;
yy yy_2_1_4_1;
yy yyv_Id;
yy yy_2_1_4_2;
yy yy_2_2_1_1;
yy yy_2_2_1_2;
yy yy_2_2_1_3;
yy yy_2_2_1_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 1) goto yyfl_167_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_Index = yy_0_1_1;
yyv_Tail = yy_0_1_2;
yyv_Str = yy_0_2;
yyv_Pos = yy_0_3;
yy_1 = (yy) yyv_Index[3];
if (yy_1 == (yy) yyu) yyErr(1,1148);
yyv_Repr = yy_1;
{
yy yysb = yyb;
yy_2_1_1_1 = yyv_Str;
yy_2_1_1_2 = yyv_Repr;
if (strcmp((char *) yy_2_1_1_1, (char *) yy_2_1_1_2)  !=  0) goto yyfl_167_1_2_1;
yy_2_1_2 = (yy) yyv_Index[2];
if (yy_2_1_2 == (yy) yyu) yyErr(1,1151);
yyv_Ident = yy_2_1_2;
{
yy yysb = yyb;
yy_2_1_3_1_1 = (yy) yyv_Index[1];
if (yy_2_1_3_1_1 == (yy) yyu) yyErr(1,1153);
if (yy_2_1_3_1_1[0] != 1) goto yyfl_167_1_2_1_3_1;
yy_2_1_3_1_2_1 = ((yy)"use symbolic name '");
yy_2_1_3_1_2_2 = yyv_Ident;
yy_2_1_3_1_2_3 = ((yy)"'");
yy_2_1_3_1_2_4 = yyv_Pos;
MESSAGE1(yy_2_1_3_1_2_1, yy_2_1_3_1_2_2, yy_2_1_3_1_2_3, yy_2_1_3_1_2_4);
goto yysl_167_1_2_1_3;
yyfl_167_1_2_1_3_1 : ;
goto yysl_167_1_2_1_3;
yysl_167_1_2_1_3 : ;
yyb = yysb;
}
yy_2_1_4_1 = yyv_Ident;
yy_2_1_4_2 = yy_2_1_4_1;
yyv_Id = yy_2_1_4_2;
goto yysl_167_1_2;
yyfl_167_1_2_1 : ;
yy_2_2_1_1 = yyv_Tail;
yy_2_2_1_2 = yyv_Str;
yy_2_2_1_3 = yyv_Pos;
LookupAnonToken(yy_2_2_1_1, yy_2_2_1_2, yy_2_2_1_3, &yy_2_2_1_4);
yyv_Id = yy_2_2_1_4;
goto yysl_167_1_2;
yysl_167_1_2 : ;
yyb = yysb;
}
yy_0_4 = yyv_Id;
*yyout_1 = yy_0_4;
return;
yyfl_167_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Str;
yy yy_0_2;
yy yyv_Pos;
yy yy_0_3;
yy yy_0_4;
yy yyv_I;
yy yy_2_1;
yy yyv_Id;
yy yy_2_2;
yy yy_3;
yy yy_4;
yy yy_5;
yy yy_6;
yy yyv_L;
yy yy_7;
yy yy_8;
yy yy_8_1;
yy yy_8_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 2) goto yyfl_167_2;
yyv_Str = yy_0_2;
yyv_Pos = yy_0_3;
yyb = yyh;
yyh += 9; if (yyh > yyhx) yyExtend();
yyv_I = yyb + 0;
yyb[4] = yyu;
yyb[3] = yyu;
yyb[2] = yyu;
yyb[1] = yyu;
yy_2_1 = yyv_Str;
CreateNameForToken(yy_2_1, &yy_2_2);
yyv_Id = yy_2_2;
yy_3 = yyb + 5;
yy_3[0] = 2;
yyv_I[1] = (long) yy_3;
yy_4 = yyv_Id;
yyv_I[2] = (long) yy_4;
yy_5 = ((yy)"");
yyv_I[4] = (long) yy_5;
yy_6 = yyv_Str;
yyv_I[3] = (long) yy_6;
yy_7 = yyglov_tokenList;
if (yy_7 == (yy) yyu) yyErr(1,1170);
yyv_L = yy_7;
yy_8_1 = yyv_I;
yy_8_2 = yyv_L;
yy_8 = yyb + 6;
yy_8[0] = 1;
yy_8[1] = ((long)yy_8_1);
yy_8[2] = ((long)yy_8_2);
yyglov_tokenList = yy_8;
yy_0_4 = yyv_Id;
*yyout_1 = yy_0_4;
return;
yyfl_167_2 : ;
}
yyErr(2,1145);
}
WriteTokenDescrFile()
{
{
yy yyb;
yy yy_1_1;
yy yyv_L;
yy yy_2;
yy yy_3_1;
yy_1_1 = ((yy)"gen.lit");
TellFile(yy_1_1);
yy_2 = yyglov_tokenList;
if (yy_2 == (yy) yyu) yyErr(1,1177);
yyv_L = yy_2;
yy_3_1 = yyv_L;
WriteTokenDescr(yy_3_1);
return;
}
}
WriteTokenDescr(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Index;
yy yy_0_1_1;
yy yyv_Tail;
yy yy_0_1_2;
yy yyv_Id;
yy yy_1;
yy yyv_Repr;
yy yy_2;
yy yyv_Handler;
yy yy_3;
yy yy_4_1;
yy yy_5_1;
yy yy_6_1;
yy yy_7_1;
yy yy_9_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_169_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_Index = yy_0_1_1;
yyv_Tail = yy_0_1_2;
yy_1 = (yy) yyv_Index[2];
if (yy_1 == (yy) yyu) yyErr(1,1182);
yyv_Id = yy_1;
yy_2 = (yy) yyv_Index[3];
if (yy_2 == (yy) yyu) yyErr(1,1183);
yyv_Repr = yy_2;
yy_3 = (yy) yyv_Index[4];
if (yy_3 == (yy) yyu) yyErr(1,1184);
yyv_Handler = yy_3;
yy_4_1 = yyv_Repr;
qu_s(yy_4_1);
yy_5_1 = ((yy)" { yysetpos(); return ");
s(yy_5_1);
yy_6_1 = yyv_Id;
id(yy_6_1);
yy_7_1 = ((yy)"; }");
s(yy_7_1);
nl();
yy_9_1 = yyv_Tail;
WriteTokenDescr(yy_9_1);
return;
yyfl_169_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_169_2;
return;
yyfl_169_2 : ;
}
yyErr(2,1180);
}
Trafo(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yyv_Unit;
yy yy_0_1;
yy yyv_ImportedUnits;
yy yy_0_2;
yy yy_1;
yy yy_2;
yy yy_3;
yy yy_4_1;
yy yyv_T_ERROR_ID;
yy yy_4_2;
yy yy_5;
yy yy_6;
yy yy_7;
yy yy_8;
yy yy_9;
yy yy_11_1;
yy yy_12_1;
yy yy_14_1;
yy yy_14_2;
yy yyv_Name;
yy yy_14_2_1;
yy yy_14_2_2;
yy yyv_OwnExport;
yy yy_14_2_2_1;
yy yyv_ExportPos;
yy yy_14_2_2_2;
yy yy_14_2_3;
yy yyv_UnitPos;
yy yy_14_2_4;
yy yyv_SN;
yy yy_15_1;
yy yy_16_1;
yy yyv_UN;
yy yy_16_2;
yy yy_17_1_1_1;
yy yy_17_1_1_2;
yy yy_17_2_1_1;
yy yy_17_2_1_2;
yy yy_17_2_1_3;
yy yy_17_2_1_4;
yy yy_18_1;
yy yy_18_2;
yy yy_19_1;
yy yy_19_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Unit = yy_0_1;
yyv_ImportedUnits = yy_0_2;
yyb = yyh;
yyh += 6; if (yyh > yyhx) yyExtend();
yy_1 = yyb + 0;
yy_1[0] = 2;
yyglov_containsGrammar = yy_1;
yy_2 = yyb + 1;
yy_2[0] = 2;
yyglov_containsRoot = yy_2;
yy_3 = ((yy)1);
yyglov_maxAttr = yy_3;
yy_4_1 = ((yy)"<ERROR>");
string_to_id(yy_4_1, &yy_4_2);
yyv_T_ERROR_ID = yy_4_2;
yy_5 = yyv_T_ERROR_ID;
yyglov_errorId = yy_5;
yy_6 = yyb + 2;
yy_6[0] = 2;
yyglov_choice_Declarations = yy_6;
yy_7 = yyb + 3;
yy_7[0] = 2;
yyglov_choice_Types = yy_7;
yy_8 = yyb + 4;
yy_8[0] = 2;
yyglov_insideChoiceRule = yy_8;
yy_9 = yyb + 5;
yy_9[0] = 2;
yyglov_importedDeclarations = yy_9;
MakeStdItemsVisible();
yy_11_1 = yyv_ImportedUnits;
MakeImportedItemsVisible_UNITLIST(yy_11_1);
yy_12_1 = yyv_Unit;
MakeGlobalItemsVisible(yy_12_1);
{
yy yysb = yyb;
if (! SymbolFileOption()) goto yyfl_172_1_13_1;
TellSymbolFile();
goto yysl_172_1_13;
yyfl_172_1_13_1 : ;
goto yysl_172_1_13;
yysl_172_1_13 : ;
yyb = yysb;
}
yy_14_1 = yyv_Unit;
yy_14_2 = yy_14_1;
if (yy_14_2[0] != 1) goto yyfl_172_1;
yy_14_2_1 = ((yy)yy_14_2[1]);
yy_14_2_2 = ((yy)yy_14_2[2]);
yy_14_2_3 = ((yy)yy_14_2[3]);
yy_14_2_4 = ((yy)yy_14_2[4]);
yyv_Name = yy_14_2_1;
if (yy_14_2_2[0] != 1) goto yyfl_172_1;
yy_14_2_2_1 = ((yy)yy_14_2_2[1]);
yy_14_2_2_2 = ((yy)yy_14_2_2[2]);
yyv_OwnExport = yy_14_2_2_1;
yyv_ExportPos = yy_14_2_2_2;
yyv_UnitPos = yy_14_2_4;
GetSourceName(&yy_15_1);
yyv_SN = yy_15_1;
yy_16_1 = yyv_Name;
id_to_string(yy_16_1, &yy_16_2);
yyv_UN = yy_16_2;
{
yy yysb = yyb;
yy_17_1_1_1 = yyv_SN;
yy_17_1_1_2 = yyv_UN;
if (strcmp((char *) yy_17_1_1_1, (char *) yy_17_1_1_2)  !=  0) goto yyfl_172_1_17_1;
goto yysl_172_1_17;
yyfl_172_1_17_1 : ;
yy_17_2_1_1 = ((yy)"'");
yy_17_2_1_2 = yyv_Name;
yy_17_2_1_3 = ((yy)"' does not correspond to file name");
yy_17_2_1_4 = yyv_UnitPos;
MESSAGE1(yy_17_2_1_1, yy_17_2_1_2, yy_17_2_1_3, yy_17_2_1_4);
goto yysl_172_1_17;
yysl_172_1_17 : ;
yyb = yysb;
}
yy_18_1 = yyv_OwnExport;
yy_18_2 = yyv_ExportPos;
CheckExport(yy_18_1, yy_18_2);
yy_19_1 = yyv_Unit;
yy_19_2 = yyv_ImportedUnits;
Code(yy_19_1, yy_19_2);
Told();
return;
yyfl_172_1 : ;
}
yyErr(2,1195);
}
CheckExport(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_T;
yy yy_0_1_2;
yy yyv_Pos;
yy yy_0_2;
yy yy_1_1_1_1;
yy yyv_Assoc;
yy yy_1_1_1_2;
yy yy_1_1_2_1_1_1;
yy yy_1_1_2_1_1_2;
yy yy_1_1_2_2_1_1;
yy yy_1_1_2_2_1_2;
yy yy_1_1_2_2_1_3;
yy yy_1_1_2_2_1_4;
yy yy_1_1_3_1_1_1;
yy yy_1_1_3_1_1_2;
yy yy_1_1_3_1_1_2_1;
yy yy_1_1_3_1_1_2_2;
yy yy_1_1_3_1_1_2_3;
yy yy_1_1_3_1_1_2_4;
yy yy_1_1_3_1_2_1;
yy yy_1_1_3_1_2_2;
yy yy_1_1_3_1_2_3;
yy yy_1_1_3_1_2_4;
yy yy_1_1_3_2_1_1;
yy yy_1_1_3_2_1_2;
yy yy_1_1_3_2_1_2_1;
yy yyv_Class;
yy yy_1_1_3_2_1_2_2;
yy yy_1_1_3_2_1_2_3;
yy yy_1_1_3_2_1_2_4;
yy yy_1_1_3_2_2_1_1_1;
yy yy_1_1_3_2_2_1_1_2;
yy yy_1_1_3_2_2_2_1_1;
yy yy_1_1_3_2_2_2_1_2;
yy yy_1_1_3_2_3_1;
yy yy_1_1_3_2_3_2;
yy yy_1_1_3_2_3_3;
yy yy_1_1_3_2_3_4;
yy yy_1_1_4_1_2_1;
yy yy_1_1_4_1_2_2;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_1_2_1_3;
yy yy_1_2_1_4;
yy yy_2_1;
yy yy_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_177_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_H = yy_0_1_1;
yyv_T = yy_0_1_2;
yyv_Pos = yy_0_2;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_H;
if (! GetGlobalMeaning(yy_1_1_1_1, &yy_1_1_1_2)) goto yyfl_177_1_1_1;
yyv_Assoc = yy_1_1_1_2;
{
yy yysb = yyb;
yy_1_1_2_1_1_1 = yyv_H;
if (! GetExportFlag(yy_1_1_2_1_1_1, &yy_1_1_2_1_1_2)) goto yyfl_177_1_1_1_2_1;
if (yy_1_1_2_1_1_2[0] != 1) goto yyfl_177_1_1_1_2_1;
goto yysl_177_1_1_1_2;
yyfl_177_1_1_1_2_1 : ;
yy_1_1_2_2_1_1 = ((yy)"'");
yy_1_1_2_2_1_2 = yyv_H;
yy_1_1_2_2_1_3 = ((yy)"' cannot be exported");
yy_1_1_2_2_1_4 = yyv_Pos;
MESSAGE1(yy_1_1_2_2_1_1, yy_1_1_2_2_1_2, yy_1_1_2_2_1_3, yy_1_1_2_2_1_4);
goto yysl_177_1_1_1_2;
yysl_177_1_1_1_2 : ;
yyb = yysb;
}
{
yy yysb = yyb;
yy_1_1_3_1_1_1 = yyv_Assoc;
yy_1_1_3_1_1_2 = yy_1_1_3_1_1_1;
if (yy_1_1_3_1_1_2[0] != 4) goto yyfl_177_1_1_1_3_1;
yy_1_1_3_1_1_2_1 = ((yy)yy_1_1_3_1_1_2[1]);
yy_1_1_3_1_1_2_2 = ((yy)yy_1_1_3_1_1_2[2]);
yy_1_1_3_1_1_2_3 = ((yy)yy_1_1_3_1_1_2[3]);
yy_1_1_3_1_1_2_4 = ((yy)yy_1_1_3_1_1_2[4]);
if (yy_1_1_3_1_1_2_2[0] != 5) goto yyfl_177_1_1_1_3_1;
yy_1_1_3_1_2_1 = ((yy)"CHOICE predicate '");
yy_1_1_3_1_2_2 = yyv_H;
yy_1_1_3_1_2_3 = ((yy)"' cannot be exported");
yy_1_1_3_1_2_4 = yyv_Pos;
MESSAGE1(yy_1_1_3_1_2_1, yy_1_1_3_1_2_2, yy_1_1_3_1_2_3, yy_1_1_3_1_2_4);
goto yysl_177_1_1_1_3;
yyfl_177_1_1_1_3_1 : ;
yy_1_1_3_2_1_1 = yyv_Assoc;
yy_1_1_3_2_1_2 = yy_1_1_3_2_1_1;
if (yy_1_1_3_2_1_2[0] != 4) goto yyfl_177_1_1_1_3_2;
yy_1_1_3_2_1_2_1 = ((yy)yy_1_1_3_2_1_2[1]);
yy_1_1_3_2_1_2_2 = ((yy)yy_1_1_3_2_1_2[2]);
yy_1_1_3_2_1_2_3 = ((yy)yy_1_1_3_2_1_2[3]);
yy_1_1_3_2_1_2_4 = ((yy)yy_1_1_3_2_1_2[4]);
yyv_Class = yy_1_1_3_2_1_2_2;
{
yy yysb = yyb;
yy_1_1_3_2_2_1_1_1 = yyv_Class;
yy_1_1_3_2_2_1_1_2 = yy_1_1_3_2_2_1_1_1;
if (yy_1_1_3_2_2_1_1_2[0] != 3) goto yyfl_177_1_1_1_3_2_2_1;
goto yysl_177_1_1_1_3_2_2;
yyfl_177_1_1_1_3_2_2_1 : ;
yy_1_1_3_2_2_2_1_1 = yyv_Class;
yy_1_1_3_2_2_2_1_2 = yy_1_1_3_2_2_2_1_1;
if (yy_1_1_3_2_2_2_1_2[0] != 4) goto yyfl_177_1_1_1_3_2_2_2;
goto yysl_177_1_1_1_3_2_2;
yyfl_177_1_1_1_3_2_2_2 : ;
goto yyfl_177_1_1_1_3_2;
yysl_177_1_1_1_3_2_2 : ;
yyb = yysb;
}
yy_1_1_3_2_3_1 = ((yy)"grammar symbol '");
yy_1_1_3_2_3_2 = yyv_H;
yy_1_1_3_2_3_3 = ((yy)"' cannot be exported");
yy_1_1_3_2_3_4 = yyv_Pos;
MESSAGE1(yy_1_1_3_2_3_1, yy_1_1_3_2_3_2, yy_1_1_3_2_3_3, yy_1_1_3_2_3_4);
goto yysl_177_1_1_1_3;
yyfl_177_1_1_1_3_2 : ;
goto yysl_177_1_1_1_3;
yysl_177_1_1_1_3 : ;
yyb = yysb;
}
{
yy yysb = yyb;
if (! SymbolFileOption()) goto yyfl_177_1_1_1_4_1;
yy_1_1_4_1_2_1 = yyv_H;
yy_1_1_4_1_2_2 = yyv_Assoc;
SF_Assoc(yy_1_1_4_1_2_1, yy_1_1_4_1_2_2);
goto yysl_177_1_1_1_4;
yyfl_177_1_1_1_4_1 : ;
goto yysl_177_1_1_1_4;
yysl_177_1_1_1_4 : ;
yyb = yysb;
}
goto yysl_177_1_1;
yyfl_177_1_1_1 : ;
yy_1_2_1_1 = ((yy)"exported '");
yy_1_2_1_2 = yyv_H;
yy_1_2_1_3 = ((yy)"' is not declared");
yy_1_2_1_4 = yyv_Pos;
MESSAGE1(yy_1_2_1_1, yy_1_2_1_2, yy_1_2_1_3, yy_1_2_1_4);
goto yysl_177_1_1;
yysl_177_1_1 : ;
yyb = yysb;
}
yy_2_1 = yyv_T;
yy_2_2 = yyv_Pos;
CheckExport(yy_2_1, yy_2_2);
return;
yyfl_177_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Pos;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_177_2;
yyv_Pos = yy_0_2;
return;
yyfl_177_2 : ;
}
yyErr(2,1238);
}
MakeStdItemsVisible()
{
{
yy yyb;
yy yy_1_1;
yy yyv_T_ID_INT;
yy yy_1_2;
yy yy_2;
yy yy_3_1;
yy yy_3_2;
yy yy_3_2_1;
yy yy_3_2_2;
yy yy_3_3;
yy yy_4_1;
yy yyv_T_ID_STRING;
yy yy_4_2;
yy yy_5;
yy yy_6_1;
yy yy_6_2;
yy yy_6_2_1;
yy yy_6_2_2;
yy yy_6_3;
yy yy_7_1;
yy yyv_T_ID_POS;
yy yy_7_2;
yy yy_8;
yy yy_9_1;
yy yy_9_2;
yy yy_9_2_1;
yy yy_9_2_2;
yy yy_9_3;
yy yy_10_1;
yy yyv_WHERENEXT;
yy yy_10_2;
yy yy_11_1;
yy yy_11_2;
yy yy_11_3;
yy yy_12_1;
yy yyv_WHERE;
yy yy_12_2;
yy yy_13_1;
yy yy_13_2;
yy yy_13_3;
yy yy_14_1;
yy yyv_LET;
yy yy_14_2;
yy yy_15_1;
yy yy_15_2;
yy yy_15_3;
yy yy_16_1;
yy yyv_IS;
yy yy_16_2;
yy yy_17_1;
yy yy_17_2;
yy yy_17_3;
yy yy_18_1;
yy yyv_VAL;
yy yy_18_2;
yy yy_19_1;
yy yy_19_2;
yy yy_19_3;
yy yy_20_1;
yy yyv_EQ;
yy yy_20_2;
yy yy_21_1;
yy yy_21_2;
yy yy_21_3;
yy yy_22_1;
yy yyv_NE;
yy yy_22_2;
yy yy_23_1;
yy yy_23_2;
yy yy_23_3;
yy yy_24_1;
yy yyv_LT;
yy yy_24_2;
yy yy_25_1;
yy yy_25_2;
yy yy_25_3;
yy yy_26_1;
yy yyv_LE;
yy yy_26_2;
yy yy_27_1;
yy yy_27_2;
yy yy_27_3;
yy yy_28_1;
yy yyv_GT;
yy yy_28_2;
yy yy_29_1;
yy yy_29_2;
yy yy_29_3;
yy yy_30_1;
yy yyv_GE;
yy yy_30_2;
yy yy_31_1;
yy yy_31_2;
yy yy_31_3;
yy yy_32_1;
yy yyv_PRINT;
yy yy_32_2;
yy yy_33_1;
yy yy_33_2;
yy yy_33_3;
yy yy_34_1;
yy yyv_SKIP;
yy yy_34_2;
yy yy_35_1;
yy yy_35_2;
yy yy_35_3;
yyb = yyh;
yyh += 44; if (yyh > yyhx) yyExtend();
yy_1_1 = ((yy)"INT");
string_to_id(yy_1_1, &yy_1_2);
yyv_T_ID_INT = yy_1_2;
yy_2 = yyv_T_ID_INT;
yyglov_id_INT = yy_2;
yy_3_1 = yyv_T_ID_INT;
yy_3_2_1 = yyb + 3;
yy_3_2_1[0] = 1;
yy_3_2_2 = yyb + 4;
yy_3_2_2[0] = 2;
yy_3_2 = yyb + 0;
yy_3_2[0] = 1;
yy_3_2[1] = ((long)yy_3_2_1);
yy_3_2[2] = ((long)yy_3_2_2);
yy_3_3 = yyb + 5;
yy_3_3[0] = 2;
DefGlobalMeaning(yy_3_1, yy_3_2, yy_3_3);
yy_4_1 = ((yy)"STRING");
string_to_id(yy_4_1, &yy_4_2);
yyv_T_ID_STRING = yy_4_2;
yy_5 = yyv_T_ID_STRING;
yyglov_id_STRING = yy_5;
yy_6_1 = yyv_T_ID_STRING;
yy_6_2_1 = yyb + 9;
yy_6_2_1[0] = 1;
yy_6_2_2 = yyb + 10;
yy_6_2_2[0] = 2;
yy_6_2 = yyb + 6;
yy_6_2[0] = 1;
yy_6_2[1] = ((long)yy_6_2_1);
yy_6_2[2] = ((long)yy_6_2_2);
yy_6_3 = yyb + 11;
yy_6_3[0] = 2;
DefGlobalMeaning(yy_6_1, yy_6_2, yy_6_3);
yy_7_1 = ((yy)"POS");
string_to_id(yy_7_1, &yy_7_2);
yyv_T_ID_POS = yy_7_2;
yy_8 = yyv_T_ID_POS;
yyglov_id_POS = yy_8;
yy_9_1 = yyv_T_ID_POS;
yy_9_2_1 = yyb + 15;
yy_9_2_1[0] = 1;
yy_9_2_2 = yyb + 16;
yy_9_2_2[0] = 2;
yy_9_2 = yyb + 12;
yy_9_2[0] = 1;
yy_9_2[1] = ((long)yy_9_2_1);
yy_9_2[2] = ((long)yy_9_2_2);
yy_9_3 = yyb + 17;
yy_9_3[0] = 2;
DefGlobalMeaning(yy_9_1, yy_9_2, yy_9_3);
yy_10_1 = ((yy)"wherenext");
string_to_id(yy_10_1, &yy_10_2);
yyv_WHERENEXT = yy_10_2;
yy_11_1 = yyv_WHERENEXT;
yy_11_2 = yyb + 18;
yy_11_2[0] = 5;
yy_11_3 = yyb + 19;
yy_11_3[0] = 2;
DefGlobalMeaning(yy_11_1, yy_11_2, yy_11_3);
yy_12_1 = ((yy)"where");
string_to_id(yy_12_1, &yy_12_2);
yyv_WHERE = yy_12_2;
yy_13_1 = yyv_WHERE;
yy_13_2 = yyb + 20;
yy_13_2[0] = 5;
yy_13_3 = yyb + 21;
yy_13_3[0] = 2;
DefGlobalMeaning(yy_13_1, yy_13_2, yy_13_3);
yy_14_1 = ((yy)"let");
string_to_id(yy_14_1, &yy_14_2);
yyv_LET = yy_14_2;
yy_15_1 = yyv_LET;
yy_15_2 = yyb + 22;
yy_15_2[0] = 5;
yy_15_3 = yyb + 23;
yy_15_3[0] = 2;
DefGlobalMeaning(yy_15_1, yy_15_2, yy_15_3);
yy_16_1 = ((yy)"is");
string_to_id(yy_16_1, &yy_16_2);
yyv_IS = yy_16_2;
yy_17_1 = yyv_IS;
yy_17_2 = yyb + 24;
yy_17_2[0] = 5;
yy_17_3 = yyb + 25;
yy_17_3[0] = 2;
DefGlobalMeaning(yy_17_1, yy_17_2, yy_17_3);
yy_18_1 = ((yy)"val");
string_to_id(yy_18_1, &yy_18_2);
yyv_VAL = yy_18_2;
yy_19_1 = yyv_VAL;
yy_19_2 = yyb + 26;
yy_19_2[0] = 5;
yy_19_3 = yyb + 27;
yy_19_3[0] = 2;
DefGlobalMeaning(yy_19_1, yy_19_2, yy_19_3);
yy_20_1 = ((yy)"eq");
string_to_id(yy_20_1, &yy_20_2);
yyv_EQ = yy_20_2;
yy_21_1 = yyv_EQ;
yy_21_2 = yyb + 28;
yy_21_2[0] = 5;
yy_21_3 = yyb + 29;
yy_21_3[0] = 2;
DefGlobalMeaning(yy_21_1, yy_21_2, yy_21_3);
yy_22_1 = ((yy)"ne");
string_to_id(yy_22_1, &yy_22_2);
yyv_NE = yy_22_2;
yy_23_1 = yyv_NE;
yy_23_2 = yyb + 30;
yy_23_2[0] = 5;
yy_23_3 = yyb + 31;
yy_23_3[0] = 2;
DefGlobalMeaning(yy_23_1, yy_23_2, yy_23_3);
yy_24_1 = ((yy)"lt");
string_to_id(yy_24_1, &yy_24_2);
yyv_LT = yy_24_2;
yy_25_1 = yyv_LT;
yy_25_2 = yyb + 32;
yy_25_2[0] = 5;
yy_25_3 = yyb + 33;
yy_25_3[0] = 2;
DefGlobalMeaning(yy_25_1, yy_25_2, yy_25_3);
yy_26_1 = ((yy)"le");
string_to_id(yy_26_1, &yy_26_2);
yyv_LE = yy_26_2;
yy_27_1 = yyv_LE;
yy_27_2 = yyb + 34;
yy_27_2[0] = 5;
yy_27_3 = yyb + 35;
yy_27_3[0] = 2;
DefGlobalMeaning(yy_27_1, yy_27_2, yy_27_3);
yy_28_1 = ((yy)"gt");
string_to_id(yy_28_1, &yy_28_2);
yyv_GT = yy_28_2;
yy_29_1 = yyv_GT;
yy_29_2 = yyb + 36;
yy_29_2[0] = 5;
yy_29_3 = yyb + 37;
yy_29_3[0] = 2;
DefGlobalMeaning(yy_29_1, yy_29_2, yy_29_3);
yy_30_1 = ((yy)"ge");
string_to_id(yy_30_1, &yy_30_2);
yyv_GE = yy_30_2;
yy_31_1 = yyv_GE;
yy_31_2 = yyb + 38;
yy_31_2[0] = 5;
yy_31_3 = yyb + 39;
yy_31_3[0] = 2;
DefGlobalMeaning(yy_31_1, yy_31_2, yy_31_3);
yy_32_1 = ((yy)"print");
string_to_id(yy_32_1, &yy_32_2);
yyv_PRINT = yy_32_2;
yy_33_1 = yyv_PRINT;
yy_33_2 = yyb + 40;
yy_33_2[0] = 5;
yy_33_3 = yyb + 41;
yy_33_3[0] = 2;
DefGlobalMeaning(yy_33_1, yy_33_2, yy_33_3);
yy_34_1 = ((yy)"skip");
string_to_id(yy_34_1, &yy_34_2);
yyv_SKIP = yy_34_2;
yy_35_1 = yyv_SKIP;
yy_35_2 = yyb + 42;
yy_35_2[0] = 5;
yy_35_3 = yyb + 43;
yy_35_3[0] = 2;
DefGlobalMeaning(yy_35_1, yy_35_2, yy_35_3);
return;
}
}
MakeImportedItemsVisible_UNITLIST(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_T;
yy yy_0_1_2;
yy yy_1_1;
yy yy_2_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_179_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_H = yy_0_1_1;
yyv_T = yy_0_1_2;
yy_1_1 = yyv_H;
MakeImportedItemsVisible_UNIT(yy_1_1);
yy_2_1 = yyv_T;
MakeImportedItemsVisible_UNITLIST(yy_2_1);
return;
yyfl_179_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_179_2;
return;
yyfl_179_2 : ;
}
yyErr(2,1314);
}
MakeImportedItemsVisible_UNIT(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Name;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_Export;
yy yy_0_1_2_1;
yy yy_0_1_2_2;
yy yyv_Declarations;
yy yy_0_1_3;
yy yyv_Pos;
yy yy_0_1_4;
yy yy_1_1;
yy yy_1_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_180_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yyv_Name = yy_0_1_1;
if (yy_0_1_2[0] != 1) goto yyfl_180_1;
yy_0_1_2_1 = ((yy)yy_0_1_2[1]);
yy_0_1_2_2 = ((yy)yy_0_1_2[2]);
yyv_Export = yy_0_1_2_1;
yyv_Declarations = yy_0_1_3;
yyv_Pos = yy_0_1_4;
yy_1_1 = yyv_Declarations;
yy_1_2 = yyv_Export;
MakeImportedItemsVisible_DECLARATIONLIST(yy_1_1, yy_1_2);
return;
yyfl_180_1 : ;
}
yyErr(2,1322);
}
MakeImportedItemsVisible_DECLARATIONLIST(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_T;
yy yy_0_1_2;
yy yyv_ExportList;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy yy_2_1;
yy yy_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_181_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_H = yy_0_1_1;
yyv_T = yy_0_1_2;
yyv_ExportList = yy_0_2;
yy_1_1 = yyv_H;
yy_1_2 = yyv_ExportList;
MakeImportedItemsVisible_DECLARATION(yy_1_1, yy_1_2);
yy_2_1 = yyv_T;
yy_2_2 = yyv_ExportList;
MakeImportedItemsVisible_DECLARATIONLIST(yy_2_1, yy_2_2);
return;
yyfl_181_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_ExportList;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_181_2;
yyv_ExportList = yy_0_2;
return;
yyfl_181_2 : ;
}
yyErr(2,1328);
}
DefineItem(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yyv_Decl;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_Id;
yy yy_0_1_1_1;
yy yyv_Pos;
yy yy_0_1_1_2;
yy yyv_Assoc;
yy yy_0_1_1_3;
yy yyv_Exportable;
yy yy_0_2;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yy_1_1_1_2_1;
yy yyv_Class;
yy yy_1_1_1_2_2;
yy yyv_Formals;
yy yy_1_1_1_2_3;
yy yyv_Rules;
yy yy_1_1_1_2_4;
yy yy_1_1_2_1_1_1;
yy yyv_Old;
yy yy_1_1_2_1_1_2;
yy yy_1_1_2_1_2_1_1_1;
yy yy_1_1_2_1_2_1_1_2;
yy yy_1_1_2_1_2_1_2_1;
yy yy_1_1_2_1_2_1_2_2;
yy yy_1_1_2_1_2_1_2_3;
yy yy_1_1_2_1_2_1_2_4;
yy yy_1_1_2_1_3_1;
yy yy_1_1_2_1_3_2;
yy yy_1_1_2_1_3_3;
yy yy_1_1_2_1_3_4;
yy yy_1_1_3_1;
yy yy_1_1_3_2;
yy yy_1_1_3_3;
yy yy_1_2_1_1_1_1;
yy yy_1_2_1_1_1_2;
yy yy_1_2_1_1_1_2_1;
yy yyv_Functors;
yy yy_1_2_1_1_1_2_2;
yy yy_1_2_1_1_2_1;
yy yy_1_2_1_1_2_2;
yy yy_1_2_2_1_1_1;
yy yy_1_2_2_1_1_2;
yy yy_1_2_2_1_2_1;
yy yy_1_2_2_1_2_2;
yy yy_1_2_2_1_2_3;
yy yy_1_2_2_1_2_4;
yy yy_1_2_2_2_1_1;
yy yy_1_2_2_2_1_2;
yy yy_1_2_2_2_1_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_1_1 = yy_0_1;
yyv_Decl = yy_0_1;
if (yy_0_1_1[0] != 2) goto yyfl_183_1;
yy_0_1_1_1 = ((yy)yy_0_1_1[1]);
yy_0_1_1_2 = ((yy)yy_0_1_1[2]);
yy_0_1_1_3 = ((yy)yy_0_1_1[3]);
yyv_Id = yy_0_1_1_1;
yyv_Pos = yy_0_1_1_2;
yyv_Assoc = yy_0_1_1_3;
yyv_Exportable = yy_0_2;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Assoc;
yy_1_1_1_2 = yy_1_1_1_1;
if (yy_1_1_1_2[0] != 4) goto yyfl_183_1_1_1;
yy_1_1_1_2_1 = ((yy)yy_1_1_1_2[1]);
yy_1_1_1_2_2 = ((yy)yy_1_1_1_2[2]);
yy_1_1_1_2_3 = ((yy)yy_1_1_1_2[3]);
yy_1_1_1_2_4 = ((yy)yy_1_1_1_2[4]);
yyv_Class = yy_1_1_1_2_2;
yyv_Formals = yy_1_1_1_2_3;
yyv_Rules = yy_1_1_1_2_4;
{
yy yysb = yyb;
yy_1_1_2_1_1_1 = yyv_Id;
if (! GetGlobalMeaning(yy_1_1_2_1_1_1, &yy_1_1_2_1_1_2)) goto yyfl_183_1_1_1_2_1;
yyv_Old = yy_1_1_2_1_1_2;
{
yy yysb = yyb;
yy_1_1_2_1_2_1_1_1 = yyv_Old;
yy_1_1_2_1_2_1_1_2 = yy_1_1_2_1_2_1_1_1;
if (yy_1_1_2_1_2_1_1_2[0] != 5) goto yyfl_183_1_1_1_2_1_2_1;
yy_1_1_2_1_2_1_2_1 = ((yy)"'");
yy_1_1_2_1_2_1_2_2 = yyv_Id;
yy_1_1_2_1_2_1_2_3 = ((yy)"' is a predefined predicate");
yy_1_1_2_1_2_1_2_4 = yyv_Pos;
MESSAGE1(yy_1_1_2_1_2_1_2_1, yy_1_1_2_1_2_1_2_2, yy_1_1_2_1_2_1_2_3, yy_1_1_2_1_2_1_2_4);
goto yysl_183_1_1_1_2_1_2;
yyfl_183_1_1_1_2_1_2_1 : ;
goto yysl_183_1_1_1_2_1_2;
yysl_183_1_1_1_2_1_2 : ;
yyb = yysb;
}
yy_1_1_2_1_3_1 = ((yy)"'");
yy_1_1_2_1_3_2 = yyv_Id;
yy_1_1_2_1_3_3 = ((yy)"' already declared");
yy_1_1_2_1_3_4 = yyv_Pos;
MESSAGE1(yy_1_1_2_1_3_1, yy_1_1_2_1_3_2, yy_1_1_2_1_3_3, yy_1_1_2_1_3_4);
goto yysl_183_1_1_1_2;
yyfl_183_1_1_1_2_1 : ;
goto yysl_183_1_1_1_2;
yysl_183_1_1_1_2 : ;
yyb = yysb;
}
yy_1_1_3_1 = yyv_Id;
yy_1_1_3_2 = yyv_Assoc;
yy_1_1_3_3 = yyv_Exportable;
DefGlobalMeaning(yy_1_1_3_1, yy_1_1_3_2, yy_1_1_3_3);
goto yysl_183_1_1;
yyfl_183_1_1_1 : ;
{
yy yysb = yyb;
yy_1_2_1_1_1_1 = yyv_Assoc;
yy_1_2_1_1_1_2 = yy_1_2_1_1_1_1;
if (yy_1_2_1_1_1_2[0] != 1) goto yyfl_183_1_1_2_1_1;
yy_1_2_1_1_1_2_1 = ((yy)yy_1_2_1_1_1_2[1]);
yy_1_2_1_1_1_2_2 = ((yy)yy_1_2_1_1_1_2[2]);
yyv_Functors = yy_1_2_1_1_1_2_2;
yy_1_2_1_1_2_1 = yyv_Id;
yy_1_2_1_1_2_2 = yyv_Functors;
EnterFunctors(yy_1_2_1_1_2_1, yy_1_2_1_1_2_2);
goto yysl_183_1_1_2_1;
yyfl_183_1_1_2_1_1 : ;
goto yysl_183_1_1_2_1;
yysl_183_1_1_2_1 : ;
yyb = yysb;
}
{
yy yysb = yyb;
yy_1_2_2_1_1_1 = yyv_Id;
if (! GetGlobalMeaning(yy_1_2_2_1_1_1, &yy_1_2_2_1_1_2)) goto yyfl_183_1_1_2_2_1;
yy_1_2_2_1_2_1 = ((yy)"'");
yy_1_2_2_1_2_2 = yyv_Id;
yy_1_2_2_1_2_3 = ((yy)"' already declared");
yy_1_2_2_1_2_4 = yyv_Pos;
MESSAGE1(yy_1_2_2_1_2_1, yy_1_2_2_1_2_2, yy_1_2_2_1_2_3, yy_1_2_2_1_2_4);
goto yysl_183_1_1_2_2;
yyfl_183_1_1_2_2_1 : ;
yy_1_2_2_2_1_1 = yyv_Id;
yy_1_2_2_2_1_2 = yyv_Assoc;
yy_1_2_2_2_1_3 = yyv_Exportable;
DefGlobalMeaning(yy_1_2_2_2_1_1, yy_1_2_2_2_1_2, yy_1_2_2_2_1_3);
goto yysl_183_1_1_2_2;
yysl_183_1_1_2_2 : ;
yyb = yysb;
}
goto yysl_183_1_1;
yysl_183_1_1 : ;
yyb = yysb;
}
return;
yyfl_183_1 : ;
}
yyErr(2,1340);
}
EnterFunctors(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yyv_Type;
yy yy_0_1;
yy yy_0_2;
yy yyv_Head;
yy yy_0_2_1;
yy yyv_Tail;
yy yy_0_2_2;
yy yy_1_1;
yy yy_1_2;
yy yyv_Functor;
yy yy_1_2_1;
yy yy_1_2_2;
yy yy_1_2_3;
yy yy_2_1_1_1;
yy yyv_Old;
yy yy_2_1_1_2;
yy yy_2_2_1_1;
yy yy_2_2_1_2;
yy yy_3_1;
yy yy_3_2;
yy yy_3_2_1;
yy yy_3_2_2;
yy yy_4_1;
yy yy_4_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Type = yy_0_1;
if (yy_0_2[0] != 1) goto yyfl_184_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yyv_Head = yy_0_2_1;
yyv_Tail = yy_0_2_2;
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_Head;
yy_1_2 = yy_1_1;
if (yy_1_2[0] != 1) goto yyfl_184_1;
yy_1_2_1 = ((yy)yy_1_2[1]);
yy_1_2_2 = ((yy)yy_1_2[2]);
yy_1_2_3 = ((yy)yy_1_2[3]);
yyv_Functor = yy_1_2_1;
{
yy yysb = yyb;
yy_2_1_1_1 = yyv_Functor;
if (! GetFunctorMeaning(yy_2_1_1_1, &yy_2_1_1_2)) goto yyfl_184_1_2_1;
yyv_Old = yy_2_1_1_2;
goto yysl_184_1_2;
yyfl_184_1_2_1 : ;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_2_2_1_1 = yyb + 0;
yy_2_2_1_1[0] = 2;
yy_2_2_1_2 = yy_2_2_1_1;
yyv_Old = yy_2_2_1_2;
goto yysl_184_1_2;
yysl_184_1_2 : ;
yyb = yysb;
}
yy_3_1 = yyv_Functor;
yy_3_2_1 = yyv_Type;
yy_3_2_2 = yyv_Old;
yy_3_2 = yyb + 0;
yy_3_2[0] = 1;
yy_3_2[1] = ((long)yy_3_2_1);
yy_3_2[2] = ((long)yy_3_2_2);
DefFunctorMeaning(yy_3_1, yy_3_2);
yy_4_1 = yyv_Type;
yy_4_2 = yyv_Tail;
EnterFunctors(yy_4_1, yy_4_2);
return;
yyfl_184_1 : ;
}
{
yy yyb;
yy yyv_Type;
yy yy_0_1;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Type = yy_0_1;
if (yy_0_2[0] != 2) goto yyfl_184_2;
return;
yyfl_184_2 : ;
}
yyErr(2,1368);
}
MakeImportedItemsVisible_DECLARATION(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yyv_Decl;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_Id;
yy yy_0_1_1_1;
yy yyv_P;
yy yy_0_1_1_2;
yy yyv_Assoc;
yy yy_0_1_1_3;
yy yyv_ExportList;
yy yy_0_2;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yy_1_1_2_1;
yy yy_1_1_2_2;
yy yyv_L;
yy yy_1_1_3;
yy yy_1_1_4;
yy yy_1_1_4_1;
yy yy_1_1_4_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_1_1 = yy_0_1;
yyv_Decl = yy_0_1;
if (yy_0_1_1[0] != 2) goto yyfl_185_1;
yy_0_1_1_1 = ((yy)yy_0_1_1[1]);
yy_0_1_1_2 = ((yy)yy_0_1_1[2]);
yy_0_1_1_3 = ((yy)yy_0_1_1[3]);
yyv_Id = yy_0_1_1_1;
yyv_P = yy_0_1_1_2;
yyv_Assoc = yy_0_1_1_3;
yyv_ExportList = yy_0_2;
{
yy yysb = yyb;
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yy_1_1_1_1 = yyv_ExportList;
yy_1_1_1_2 = yyv_Id;
if (! IsInList(yy_1_1_1_1, yy_1_1_1_2)) goto yyfl_185_1_1_1;
yy_1_1_2_1 = yyv_Decl;
yy_1_1_2_2 = yyb + 0;
yy_1_1_2_2[0] = 2;
DefineItem(yy_1_1_2_1, yy_1_1_2_2);
yy_1_1_3 = yyglov_importedDeclarations;
if (yy_1_1_3 == (yy) yyu) yyErr(1,1387);
yyv_L = yy_1_1_3;
yy_1_1_4_1 = yyv_Decl;
yy_1_1_4_2 = yyv_L;
yy_1_1_4 = yyb + 1;
yy_1_1_4[0] = 1;
yy_1_1_4[1] = ((long)yy_1_1_4_1);
yy_1_1_4[2] = ((long)yy_1_1_4_2);
yyglov_importedDeclarations = yy_1_1_4;
goto yysl_185_1_1;
yyfl_185_1_1_1 : ;
goto yysl_185_1_1;
yysl_185_1_1 : ;
yyb = yysb;
}
return;
yyfl_185_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_185_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
return;
yyfl_185_2 : ;
}
yyErr(2,1380);
}
MakeGlobalItemsVisible(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Name;
yy yy_0_1_1;
yy yyv_Import;
yy yy_0_1_2;
yy yyv_Declarations;
yy yy_0_1_3;
yy yyv_P;
yy yy_0_1_4;
yy yy_1_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_186_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yyv_Name = yy_0_1_1;
yyv_Import = yy_0_1_2;
yyv_Declarations = yy_0_1_3;
yyv_P = yy_0_1_4;
yy_1_1 = yyv_Declarations;
MakeGlobalItemsVisible_DECLARATIONLIST(yy_1_1);
return;
yyfl_186_1 : ;
}
yyErr(2,1397);
}
MakeGlobalItemsVisible_DECLARATIONLIST(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_T;
yy yy_0_1_2;
yy yy_1_1;
yy yy_2_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_187_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_H = yy_0_1_1;
yyv_T = yy_0_1_2;
yy_1_1 = yyv_H;
MakeGlobalItemsVisible_DECLARATION(yy_1_1);
yy_2_1 = yyv_T;
MakeGlobalItemsVisible_DECLARATIONLIST(yy_2_1);
return;
yyfl_187_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_187_2;
return;
yyfl_187_2 : ;
}
yyErr(2,1403);
}
MakeGlobalItemsVisible_DECLARATION(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yyv_Decl;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_Id;
yy yy_0_1_1_1;
yy yyv_P;
yy yy_0_1_1_2;
yy yyv_Assoc;
yy yy_0_1_1_3;
yy yy_1_1;
yy yy_2_1;
yy yy_2_2;
yy_0_1 = yyin_1;
yy_0_1_1 = yy_0_1;
yyv_Decl = yy_0_1;
if (yy_0_1_1[0] != 2) goto yyfl_188_1;
yy_0_1_1_1 = ((yy)yy_0_1_1[1]);
yy_0_1_1_2 = ((yy)yy_0_1_1[2]);
yy_0_1_1_3 = ((yy)yy_0_1_1[3]);
yyv_Id = yy_0_1_1_1;
yyv_P = yy_0_1_1_2;
yyv_Assoc = yy_0_1_1_3;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_Assoc;
LookForGrammarSymbol(yy_1_1);
yy_2_1 = yyv_Decl;
yy_2_2 = yyb + 0;
yy_2_2[0] = 1;
DefineItem(yy_2_1, yy_2_2);
return;
yyfl_188_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_P;
yy yy_0_1_1;
yy yyv_M;
yy yy_0_1_2;
yy yy_1_1_1;
yy yy_1_1_2_1;
yy yy_1_1_2_2;
yy yy_1_2_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_188_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_P = yy_0_1_1;
yyv_M = yy_0_1_2;
{
yy yysb = yyb;
yy_1_1_1 = yyglov_containsRoot;
if (yy_1_1_1 == (yy) yyu) yyErr(1,1419);
if (yy_1_1_1[0] != 1) goto yyfl_188_2_1_1;
yy_1_1_2_1 = ((yy)"more than one ROOT specification");
yy_1_1_2_2 = yyv_P;
MESSAGE(yy_1_1_2_1, yy_1_1_2_2);
goto yysl_188_2_1;
yyfl_188_2_1_1 : ;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_1_2_1 = yyb + 0;
yy_1_2_1[0] = 1;
yyglov_containsRoot = yy_1_2_1;
goto yysl_188_2_1;
yysl_188_2_1 : ;
yyb = yysb;
}
return;
yyfl_188_2 : ;
}
yyErr(2,1412);
}
LookForGrammarSymbol(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_Class;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_In;
yy yy_0_1_3_1;
yy yyv_Out;
yy yy_0_1_3_2;
yy yyv_RL;
yy yy_0_1_4;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_2_1;
yy yy_2_2;
yy yy_3;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 4) goto yyfl_189_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yyv_Class = yy_0_1_2;
if (yy_0_1_3[0] != 1) goto yyfl_189_1;
yy_0_1_3_1 = ((yy)yy_0_1_3[1]);
yy_0_1_3_2 = ((yy)yy_0_1_3[2]);
yyv_In = yy_0_1_3_1;
yyv_Out = yy_0_1_3_2;
yyv_RL = yy_0_1_4;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Class;
yy_1_1_1_2 = yy_1_1_1_1;
if (yy_1_1_1_2[0] != 3) goto yyfl_189_1_1_1;
goto yysl_189_1_1;
yyfl_189_1_1_1 : ;
yy_1_2_1_1 = yyv_Class;
yy_1_2_1_2 = yy_1_2_1_1;
if (yy_1_2_1_2[0] != 4) goto yyfl_189_1_1_2;
goto yysl_189_1_1;
yyfl_189_1_1_2 : ;
goto yyfl_189_1;
yysl_189_1_1 : ;
yyb = yysb;
}
yy_2_1 = yyv_Out;
yy_2_2 = ((yy)0);
CountAttributes(yy_2_1, yy_2_2);
yy_3 = yyb + 0;
yy_3[0] = 1;
yyglov_containsGrammar = yy_3;
return;
yyfl_189_1 : ;
}
{
yy yyb;
yy yyv_Other;
yy yy_0_1;
yy_0_1 = yyin_1;
yyv_Other = yy_0_1;
return;
yyfl_189_2 : ;
}
yyErr(2,1426);
}
Length_ARGSPECLIST(yyin_1, yyout_1)
yy yyin_1;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_T;
yy yy_0_1_3;
yy yy_0_2;
yy yy_0_2_1;
yy yy_0_2_2;
yy yy_1_1;
yy yyv_N;
yy yy_1_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_190_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_H = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_T = yy_0_1_3;
yy_1_1 = yyv_T;
Length_ARGSPECLIST(yy_1_1, &yy_1_2);
yyv_N = yy_1_2;
yy_0_2_1 = yyv_N;
yy_0_2_2 = ((yy)1);
yy_0_2 = (yy)(((long)yy_0_2_1)+((long)yy_0_2_2));
*yyout_1 = yy_0_2;
return;
yyfl_190_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_190_2;
yy_0_2 = ((yy)0);
*yyout_1 = yy_0_2;
return;
yyfl_190_2 : ;
}
yyErr(2,1436);
}
CountAttributes(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_T;
yy yy_0_1_3;
yy yyv_N;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy yy_1_2_1;
yy yy_1_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_191_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_H = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_T = yy_0_1_3;
yyv_N = yy_0_2;
yy_1_1 = yyv_T;
yy_1_2_1 = yyv_N;
yy_1_2_2 = ((yy)1);
yy_1_2 = (yy)(((long)yy_1_2_1)+((long)yy_1_2_2));
CountAttributes(yy_1_1, yy_1_2);
return;
yyfl_191_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_N;
yy yy_0_2;
yy yyv_Max;
yy yy_1;
yy yy_2_1_1_1;
yy yy_2_1_1_2;
yy yy_2_1_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_191_2;
yyv_N = yy_0_2;
yy_1 = yyglov_maxAttr;
if (yy_1 == (yy) yyu) yyErr(1,1447);
yyv_Max = yy_1;
{
yy yysb = yyb;
yy_2_1_1_1 = yyv_N;
yy_2_1_1_2 = yyv_Max;
if ((long)yy_2_1_1_1 <= (long)yy_2_1_1_2) goto yyfl_191_2_2_1;
yy_2_1_2 = yyv_N;
yyglov_maxAttr = yy_2_1_2;
goto yysl_191_2_2;
yyfl_191_2_2_1 : ;
goto yysl_191_2_2;
yysl_191_2_2 : ;
yyb = yysb;
}
return;
yyfl_191_2 : ;
}
yyErr(2,1442);
}
ImportDeclarations_DECLARATIONLIST(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_T;
yy yy_0_1_2;
yy yy_1_1;
yy yy_2_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_192_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_H = yy_0_1_1;
yyv_T = yy_0_1_2;
yy_1_1 = yyv_H;
ImportDeclarations_DECLARATION(yy_1_1);
yy_2_1 = yyv_T;
ImportDeclarations_DECLARATIONLIST(yy_2_1);
return;
yyfl_192_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_192_2;
return;
yyfl_192_2 : ;
}
yyErr(2,1454);
}
ImportDeclarations_DECLARATION(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Id;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_Assoc;
yy yy_0_1_3;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yy_1_1_1_2_1;
yy yyv_Functors;
yy yy_1_1_1_2_2;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yyv_Type;
yy yy_1_2_1_2_1;
yy yyv_Pos;
yy yy_1_2_1_2_2;
yy yy_1_2_2_1;
yy yy_1_2_3_1;
yy yy_1_2_4_1;
yy yy_1_3_1_1;
yy yy_1_3_1_2;
yy yyv_Fields;
yy yy_1_3_1_2_1;
yy yy_1_4_1_1;
yy yy_1_4_1_2;
yy yy_1_4_1_2_1;
yy yyv_Class;
yy yy_1_4_1_2_2;
yy yyv_Formals;
yy yy_1_4_1_2_3;
yy yyv_Rules;
yy yy_1_4_1_2_4;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_193_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Id = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_Assoc = yy_0_1_3;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Assoc;
yy_1_1_1_2 = yy_1_1_1_1;
if (yy_1_1_1_2[0] != 1) goto yyfl_193_1_1_1;
yy_1_1_1_2_1 = ((yy)yy_1_1_1_2[1]);
yy_1_1_1_2_2 = ((yy)yy_1_1_1_2[2]);
yyv_Functors = yy_1_1_1_2_2;
goto yysl_193_1_1;
yyfl_193_1_1_1 : ;
yy_1_2_1_1 = yyv_Assoc;
yy_1_2_1_2 = yy_1_2_1_1;
if (yy_1_2_1_2[0] != 2) goto yyfl_193_1_1_2;
yy_1_2_1_2_1 = ((yy)yy_1_2_1_2[1]);
yy_1_2_1_2_2 = ((yy)yy_1_2_1_2[2]);
yyv_Type = yy_1_2_1_2_1;
yyv_Pos = yy_1_2_1_2_2;
yy_1_2_2_1 = ((yy)"extern yy ");
s(yy_1_2_2_1);
yy_1_2_3_1 = yyv_Id;
glovarid(yy_1_2_3_1);
yy_1_2_4_1 = ((yy)";");
s(yy_1_2_4_1);
nl();
goto yysl_193_1_1;
yyfl_193_1_1_2 : ;
yy_1_3_1_1 = yyv_Assoc;
yy_1_3_1_2 = yy_1_3_1_1;
if (yy_1_3_1_2[0] != 3) goto yyfl_193_1_1_3;
yy_1_3_1_2_1 = ((yy)yy_1_3_1_2[1]);
yyv_Fields = yy_1_3_1_2_1;
goto yysl_193_1_1;
yyfl_193_1_1_3 : ;
yy_1_4_1_1 = yyv_Assoc;
yy_1_4_1_2 = yy_1_4_1_1;
if (yy_1_4_1_2[0] != 4) goto yyfl_193_1_1_4;
yy_1_4_1_2_1 = ((yy)yy_1_4_1_2[1]);
yy_1_4_1_2_2 = ((yy)yy_1_4_1_2[2]);
yy_1_4_1_2_3 = ((yy)yy_1_4_1_2[3]);
yy_1_4_1_2_4 = ((yy)yy_1_4_1_2[4]);
yyv_Class = yy_1_4_1_2_2;
yyv_Formals = yy_1_4_1_2_3;
yyv_Rules = yy_1_4_1_2_4;
goto yysl_193_1_1;
yyfl_193_1_1_4 : ;
goto yyfl_193_1;
yysl_193_1_1 : ;
yyb = yysb;
}
return;
yyfl_193_1 : ;
}
yyErr(2,1462);
}
DefineTempos_RULE(yyin_1, yyout_1)
yy yyin_1;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Lhs;
yy yy_0_1_1;
yy yyv_Rhs;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_2;
yy yy_1;
yy yy_3_1;
yy yy_3_2;
yy yy_3_2_1;
yy yy_3_2_2;
yy yyv_LhsSpace;
yy yy_4_1;
yy yy_5_1;
yy yy_5_2;
yy yy_5_2_1;
yy yy_5_2_2;
yy yy_5_3;
yy yyv_V;
yy yy_5_4;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_195_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Lhs = yy_0_1_1;
yyv_Rhs = yy_0_1_2;
yyb = yyh;
yyh += 10; if (yyh > yyhx) yyExtend();
yy_1 = yyb + 0;
yy_1[0] = 2;
yyglov_declaredVars = yy_1;
InitSpace();
yy_3_1 = yyv_Lhs;
yy_3_2_1 = ((yy)0);
yy_3_2_2 = yyb + 4;
yy_3_2_2[0] = 2;
yy_3_2 = yyb + 1;
yy_3_2[0] = 1;
yy_3_2[1] = ((long)yy_3_2_1);
yy_3_2[2] = ((long)yy_3_2_2);
DefineTempos_LHS(yy_3_1, yy_3_2);
GetSpace(&yy_4_1);
yyv_LhsSpace = yy_4_1;
yy_5_1 = yyv_Rhs;
yy_5_2_1 = ((yy)1);
yy_5_2_2 = yyb + 8;
yy_5_2_2[0] = 2;
yy_5_2 = yyb + 5;
yy_5_2[0] = 1;
yy_5_2[1] = ((long)yy_5_2_1);
yy_5_2[2] = ((long)yy_5_2_2);
yy_5_3 = yyb + 9;
yy_5_3[0] = 2;
DefineTempos_RHS(yy_5_1, yy_5_2, yy_5_3, &yy_5_4);
yyv_V = yy_5_4;
yy_0_2 = yyv_LhsSpace;
*yyout_1 = yy_0_2;
return;
yyfl_195_1 : ;
}
yyErr(2,1478);
}
DefineTempos_LHS(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Predicate;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_InArgs;
yy yy_0_1_3;
yy yyv_OutArgs;
yy yy_0_1_4;
yy yyv_Prefix;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy yy_1_2_1;
yy yy_1_2_2;
yy yy_1_3;
yy yyv_Next;
yy yy_1_4;
yy yyv_V;
yy yy_1_5;
yy yy_2_1;
yy yy_2_2;
yy yyv_Dummy;
yy yy_2_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_196_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yyv_Predicate = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_InArgs = yy_0_1_3;
yyv_OutArgs = yy_0_1_4;
yyv_Prefix = yy_0_2;
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_InArgs;
yy_1_2_1 = ((yy)1);
yy_1_2_2 = yyv_Prefix;
yy_1_2 = yyb + 0;
yy_1_2[0] = 1;
yy_1_2[1] = ((long)yy_1_2_1);
yy_1_2[2] = ((long)yy_1_2_2);
yy_1_3 = yyb + 3;
yy_1_3[0] = 2;
DefineTempos_DEFARGS(yy_1_1, yy_1_2, yy_1_3, &yy_1_4, &yy_1_5);
yyv_Next = yy_1_4;
yyv_V = yy_1_5;
yy_2_1 = yyv_OutArgs;
yy_2_2 = yyv_Next;
DefineTempos_USEARGS(yy_2_1, yy_2_2, &yy_2_3);
yyv_Dummy = yy_2_3;
return;
yyfl_196_1 : ;
}
yyErr(2,1488);
}
DefineTempos_RHS(yyin_1, yyin_2, yyin_3, yyout_1)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_List;
yy yy_0_1_1;
yy yyv_RefSpace;
yy yy_0_1_2;
yy yyv_TEMPO;
yy yy_0_2;
yy yyv_IDENTLIST1;
yy yy_0_3;
yy yy_0_4;
yy yyv_LhsSpace;
yy yy_1_1;
yy yy_2_1;
yy yy_2_2;
yy yy_2_3;
yy yyv_IDENTLIST2;
yy yy_2_4;
yy yyv_S;
yy yy_3_1;
yy yy_4_1;
yy yy_4_2;
yy yy_4_2_1;
yy yy_4_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 1) goto yyfl_197_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_List = yy_0_1_1;
yyv_RefSpace = yy_0_1_2;
yyv_TEMPO = yy_0_2;
yyv_IDENTLIST1 = yy_0_3;
GetSpace(&yy_1_1);
yyv_LhsSpace = yy_1_1;
yy_2_1 = yyv_List;
yy_2_2 = yyv_TEMPO;
yy_2_3 = yyv_IDENTLIST1;
DefineTempos_MEMBERLIST(yy_2_1, yy_2_2, yy_2_3, &yy_2_4);
yyv_IDENTLIST2 = yy_2_4;
GetSpace(&yy_3_1);
yyv_S = yy_3_1;
yy_4_1 = yyv_RefSpace;
yy_4_2_1 = yyv_S;
yy_4_2_2 = yyv_LhsSpace;
yy_4_2 = (yy)(((long)yy_4_2_1)-((long)yy_4_2_2));
SetINT(yy_4_1, yy_4_2);
yy_0_4 = yyv_IDENTLIST2;
*yyout_1 = yy_0_4;
return;
yyfl_197_1 : ;
}
yyErr(2,1494);
}
DefineTempos_MEMBERS(yyin_1, yyin_2, yyin_3, yyout_1)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_List;
yy yy_0_1_1;
yy yyv_RefSpace;
yy yy_0_1_2;
yy yyv_TEMPO;
yy yy_0_2;
yy yyv_IDENTLIST1;
yy yy_0_3;
yy yy_0_4;
yy yyv_Old;
yy yy_1_1;
yy yy_2_1;
yy yy_3_1;
yy yy_3_2;
yy yy_3_3;
yy yyv_IDENTLIST2;
yy yy_3_4;
yy yyv_S;
yy yy_4_1;
yy yy_5_1;
yy yy_5_2;
yy yy_6_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 1) goto yyfl_198_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_List = yy_0_1_1;
yyv_RefSpace = yy_0_1_2;
yyv_TEMPO = yy_0_2;
yyv_IDENTLIST1 = yy_0_3;
GetSpace(&yy_1_1);
yyv_Old = yy_1_1;
yy_2_1 = ((yy)0);
PutSpace(yy_2_1);
yy_3_1 = yyv_List;
yy_3_2 = yyv_TEMPO;
yy_3_3 = yyv_IDENTLIST1;
DefineTempos_MEMBERLIST(yy_3_1, yy_3_2, yy_3_3, &yy_3_4);
yyv_IDENTLIST2 = yy_3_4;
GetSpace(&yy_4_1);
yyv_S = yy_4_1;
yy_5_1 = yyv_RefSpace;
yy_5_2 = yyv_S;
SetINT(yy_5_1, yy_5_2);
yy_6_1 = yyv_Old;
PutSpace(yy_6_1);
yy_0_4 = yyv_IDENTLIST2;
*yyout_1 = yy_0_4;
return;
yyfl_198_1 : ;
}
yyErr(2,1503);
}
DefineTempos_MEMBERLIST(yyin_1, yyin_2, yyin_3, yyout_1)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_T;
yy yy_0_1_2;
yy yy_0_2;
yy yyv_N;
yy yy_0_2_1;
yy yyv_Tl;
yy yy_0_2_2;
yy yyv_V0;
yy yy_0_3;
yy yy_0_4;
yy yy_1_1;
yy yy_1_2;
yy yy_1_2_1;
yy yy_1_2_2;
yy yy_1_3;
yy yyv_V1;
yy yy_1_4;
yy yy_2_1;
yy yy_2_2;
yy yy_2_2_1;
yy yy_2_2_1_1;
yy yy_2_2_1_2;
yy yy_2_2_2;
yy yy_2_3;
yy yyv_V2;
yy yy_2_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 1) goto yyfl_199_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_H = yy_0_1_1;
yyv_T = yy_0_1_2;
if (yy_0_2[0] != 1) goto yyfl_199_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yyv_N = yy_0_2_1;
yyv_Tl = yy_0_2_2;
yyv_V0 = yy_0_3;
yyb = yyh;
yyh += 6; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_H;
yy_1_2_1 = yyv_N;
yy_1_2_2 = yyv_Tl;
yy_1_2 = yyb + 0;
yy_1_2[0] = 1;
yy_1_2[1] = ((long)yy_1_2_1);
yy_1_2[2] = ((long)yy_1_2_2);
yy_1_3 = yyv_V0;
DefineTempos_MEMBER(yy_1_1, yy_1_2, yy_1_3, &yy_1_4);
yyv_V1 = yy_1_4;
yy_2_1 = yyv_T;
yy_2_2_1_1 = yyv_N;
yy_2_2_1_2 = ((yy)1);
yy_2_2_1 = (yy)(((long)yy_2_2_1_1)+((long)yy_2_2_1_2));
yy_2_2_2 = yyv_Tl;
yy_2_2 = yyb + 3;
yy_2_2[0] = 1;
yy_2_2[1] = ((long)yy_2_2_1);
yy_2_2[2] = ((long)yy_2_2_2);
yy_2_3 = yyv_V1;
DefineTempos_MEMBERLIST(yy_2_1, yy_2_2, yy_2_3, &yy_2_4);
yyv_V2 = yy_2_4;
yy_0_4 = yyv_V2;
*yyout_1 = yy_0_4;
return;
yyfl_199_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Prefix;
yy yy_0_2;
yy yyv_V;
yy yy_0_3;
yy yy_0_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 2) goto yyfl_199_2;
yyv_Prefix = yy_0_2;
yyv_V = yy_0_3;
yy_0_4 = yyv_V;
*yyout_1 = yy_0_4;
return;
yyfl_199_2 : ;
}
yyErr(2,1514);
}
DefineTempos_MEMBER(yyin_1, yyin_2, yyin_3, yyout_1)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Predicate;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_InArgs;
yy yy_0_1_3;
yy yyv_OutArgs;
yy yy_0_1_4;
yy yyv_Offset;
yy yy_0_1_5;
yy yyv_Prefix;
yy yy_0_2;
yy yyv_V0;
yy yy_0_3;
yy yy_0_4;
yy yy_1_1_1_1;
yy yy_1_1_2_1;
yy yy_1_1_2_2;
yy yy_1_1_2_3;
yy yy_1_1_2_4;
yy yy_1_1_3_1;
yy yy_1_1_3_2;
yy yyv_List;
yy yy_1_1_3_3;
yy yy_1_1_4_1;
yy yyv_N;
yy yy_1_1_4_2;
yy yy_1_1_5_1;
yy yy_1_1_5_1_1;
yy yy_1_1_5_1_2;
yy yyv_OffsetVal;
yy yy_1_1_5_2;
yy yy_1_1_6_1;
yy yy_1_1_6_2;
yy yy_1_1_7_1_1_1;
yy yy_1_1_7_1_1_2;
yy yy_1_1_7_1_1_2_1;
yy yyv_Tempo;
yy yy_1_1_7_1_1_2_1_1;
yy yyv_Key;
yy yy_1_1_7_1_1_2_1_2;
yy yy_1_1_7_1_1_2_1_3;
yy yy_1_1_7_1_1_2_2;
yy yy_1_1_7_1_1_2_3;
yy yy_1_1_7_1_2_1;
yy yy_2_1;
yy yy_2_2;
yy yy_2_2_1;
yy yy_2_2_2;
yy yyv_Next;
yy yy_2_3;
yy yy_3_1;
yy yy_3_2;
yy yy_3_3;
yy yyv_Dummy;
yy yy_3_4;
yy yyv_V1;
yy yy_3_5;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 7) goto yyfl_200_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yy_0_1_5 = ((yy)yy_0_1[5]);
yyv_Predicate = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_InArgs = yy_0_1_3;
yyv_OutArgs = yy_0_1_4;
yyv_Offset = yy_0_1_5;
yyv_Prefix = yy_0_2;
yyv_V0 = yy_0_3;
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Predicate;
if (! IsTable(yy_1_1_1_1)) goto yyfl_200_1_1_1;
yy_1_1_2_1 = ((yy)"'");
yy_1_1_2_2 = yyv_Predicate;
yy_1_1_2_3 = ((yy)"' cannot be used as predicate");
yy_1_1_2_4 = yyv_P;
MESSAGE1(yy_1_1_2_1, yy_1_1_2_2, yy_1_1_2_3, yy_1_1_2_4);
yy_1_1_3_1 = yyv_Predicate;
yy_1_1_3_2 = yyv_P;
GetTableFields(yy_1_1_3_1, yy_1_1_3_2, &yy_1_1_3_3);
yyv_List = yy_1_1_3_3;
yy_1_1_4_1 = yyv_List;
Length(yy_1_1_4_1, &yy_1_1_4_2);
yyv_N = yy_1_1_4_2;
yy_1_1_5_1_1 = yyv_N;
yy_1_1_5_1_2 = ((yy)1);
yy_1_1_5_1 = (yy)(((long)yy_1_1_5_1_1)+((long)yy_1_1_5_1_2));
ReserveSpace(yy_1_1_5_1, &yy_1_1_5_2);
yyv_OffsetVal = yy_1_1_5_2;
yy_1_1_6_1 = yyv_Offset;
yy_1_1_6_2 = yyv_OffsetVal;
SetINT(yy_1_1_6_1, yy_1_1_6_2);
{
yy yysb = yyb;
yy_1_1_7_1_1_1 = yyv_OutArgs;
yy_1_1_7_1_1_2 = yy_1_1_7_1_1_1;
if (yy_1_1_7_1_1_2[0] != 1) goto yyfl_200_1_1_1_7_1;
yy_1_1_7_1_1_2_1 = ((yy)yy_1_1_7_1_1_2[1]);
yy_1_1_7_1_1_2_2 = ((yy)yy_1_1_7_1_1_2[2]);
yy_1_1_7_1_1_2_3 = ((yy)yy_1_1_7_1_1_2[3]);
if (yy_1_1_7_1_1_2_1[0] != 3) goto yyfl_200_1_1_1_7_1;
yy_1_1_7_1_1_2_1_1 = ((yy)yy_1_1_7_1_1_2_1[1]);
yy_1_1_7_1_1_2_1_2 = ((yy)yy_1_1_7_1_1_2_1[2]);
yy_1_1_7_1_1_2_1_3 = ((yy)yy_1_1_7_1_1_2_1[3]);
yyv_Tempo = yy_1_1_7_1_1_2_1_1;
yyv_Key = yy_1_1_7_1_1_2_1_2;
yy_1_1_7_1_2_1 = yyv_Key;
MakeLocalVar(yy_1_1_7_1_2_1);
goto yysl_200_1_1_1_7;
yyfl_200_1_1_1_7_1 : ;
goto yysl_200_1_1_1_7;
yysl_200_1_1_1_7 : ;
yyb = yysb;
}
goto yysl_200_1_1;
yyfl_200_1_1_1 : ;
goto yysl_200_1_1;
yysl_200_1_1 : ;
yyb = yysb;
}
yy_2_1 = yyv_InArgs;
yy_2_2_1 = ((yy)1);
yy_2_2_2 = yyv_Prefix;
yy_2_2 = yyb + 0;
yy_2_2[0] = 1;
yy_2_2[1] = ((long)yy_2_2_1);
yy_2_2[2] = ((long)yy_2_2_2);
DefineTempos_USEARGS(yy_2_1, yy_2_2, &yy_2_3);
yyv_Next = yy_2_3;
yy_3_1 = yyv_OutArgs;
yy_3_2 = yyv_Next;
yy_3_3 = yyv_V0;
DefineTempos_DEFARGS(yy_3_1, yy_3_2, yy_3_3, &yy_3_4, &yy_3_5);
yyv_Dummy = yy_3_4;
yyv_V1 = yy_3_5;
yy_0_4 = yyv_V1;
*yyout_1 = yy_0_4;
return;
yyfl_200_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_DefArg;
yy yy_0_1_1;
yy yyv_UseArg;
yy yy_0_1_2;
yy yyv_Pos;
yy yy_0_1_3;
yy yyv_Prefix;
yy yy_0_2;
yy yyv_V;
yy yy_0_3;
yy yy_0_4;
yy yy_1_1;
yy yy_1_2;
yy yy_1_2_1;
yy yy_1_2_2;
yy yy_1_3;
yy yyv_V2;
yy yy_1_4;
yy yy_2_1;
yy yy_2_2;
yy yy_2_2_1;
yy yy_2_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 1) goto yyfl_200_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_DefArg = yy_0_1_1;
yyv_UseArg = yy_0_1_2;
yyv_Pos = yy_0_1_3;
yyv_Prefix = yy_0_2;
yyv_V = yy_0_3;
yyb = yyh;
yyh += 6; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_DefArg;
yy_1_2_1 = ((yy)1);
yy_1_2_2 = yyv_Prefix;
yy_1_2 = yyb + 0;
yy_1_2[0] = 1;
yy_1_2[1] = ((long)yy_1_2_1);
yy_1_2[2] = ((long)yy_1_2_2);
yy_1_3 = yyv_V;
DefineTempos_DEFARG(yy_1_1, yy_1_2, yy_1_3, &yy_1_4);
yyv_V2 = yy_1_4;
yy_2_1 = yyv_UseArg;
yy_2_2_1 = ((yy)2);
yy_2_2_2 = yyv_Prefix;
yy_2_2 = yyb + 3;
yy_2_2[0] = 1;
yy_2_2[1] = ((long)yy_2_2_1);
yy_2_2[2] = ((long)yy_2_2_2);
DefineTempos_USEARG(yy_2_1, yy_2_2);
yy_0_4 = yyv_V2;
*yyout_1 = yy_0_4;
return;
yyfl_200_2 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Var;
yy yy_0_1_1;
yy yyv_Pos;
yy yy_0_1_2;
yy yyv_Source;
yy yy_0_1_3;
yy yyv_Prefix;
yy yy_0_2;
yy yyv_V;
yy yy_0_3;
yy yy_0_4;
yy yy_1_1;
yy yy_1_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 2) goto yyfl_200_3;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Var = yy_0_1_1;
yyv_Pos = yy_0_1_2;
yyv_Source = yy_0_1_3;
yyv_Prefix = yy_0_2;
yyv_V = yy_0_3;
yy_1_1 = yyv_Source;
yy_1_2 = yyv_Prefix;
DefineTempos_USEARG(yy_1_1, yy_1_2);
yy_0_4 = yyv_V;
*yyout_1 = yy_0_4;
return;
yyfl_200_3 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Var;
yy yy_0_1_1;
yy yyv_Pos;
yy yy_0_1_2;
yy yyv_Pattern;
yy yy_0_1_3;
yy yyv_Prefix;
yy yy_0_2;
yy yyv_V;
yy yy_0_3;
yy yy_0_4;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yyv_V2;
yy yy_1_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 3) goto yyfl_200_4;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Var = yy_0_1_1;
yyv_Pos = yy_0_1_2;
yyv_Pattern = yy_0_1_3;
yyv_Prefix = yy_0_2;
yyv_V = yy_0_3;
yy_1_1 = yyv_Pattern;
yy_1_2 = yyv_Prefix;
yy_1_3 = yyv_V;
DefineTempos_DEFARG(yy_1_1, yy_1_2, yy_1_3, &yy_1_4);
yyv_V2 = yy_1_4;
yy_0_4 = yyv_V2;
*yyout_1 = yy_0_4;
return;
yyfl_200_4 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Pattern;
yy yy_0_1_1;
yy yyv_Pos;
yy yy_0_1_2;
yy yyv_Prefix;
yy yy_0_2;
yy yyv_V;
yy yy_0_3;
yy yy_0_4;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yyv_V2;
yy yy_1_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 9) goto yyfl_200_5;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_Pattern = yy_0_1_1;
yyv_Pos = yy_0_1_2;
yyv_Prefix = yy_0_2;
yyv_V = yy_0_3;
yy_1_1 = yyv_Pattern;
yy_1_2 = yyv_Prefix;
yy_1_3 = yyv_V;
DefineTempos_DEFARG(yy_1_1, yy_1_2, yy_1_3, &yy_1_4);
yyv_V2 = yy_1_4;
yy_0_4 = yyv_V2;
*yyout_1 = yy_0_4;
return;
yyfl_200_5 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Key;
yy yy_0_1_1;
yy yyv_Field;
yy yy_0_1_2;
yy yyv_Val;
yy yy_0_1_3;
yy yyv_Pos;
yy yy_0_1_4;
yy yyv_Prefix;
yy yy_0_2;
yy yyv_V;
yy yy_0_3;
yy yy_0_4;
yy yy_1_1;
yy yy_1_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 5) goto yyfl_200_6;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yyv_Key = yy_0_1_1;
yyv_Field = yy_0_1_2;
yyv_Val = yy_0_1_3;
yyv_Pos = yy_0_1_4;
yyv_Prefix = yy_0_2;
yyv_V = yy_0_3;
yy_1_1 = yyv_Val;
yy_1_2 = yyv_Prefix;
DefineTempos_USEARG(yy_1_1, yy_1_2);
yy_0_4 = yyv_V;
*yyout_1 = yy_0_4;
return;
yyfl_200_6 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Key;
yy yy_0_1_1;
yy yyv_Field;
yy yy_0_1_2;
yy yyv_Val;
yy yy_0_1_3;
yy yyv_Pos;
yy yy_0_1_4;
yy yyv_Prefix;
yy yy_0_2;
yy yyv_V;
yy yy_0_3;
yy yy_0_4;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yyv_V2;
yy yy_1_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 6) goto yyfl_200_7;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yyv_Key = yy_0_1_1;
yyv_Field = yy_0_1_2;
yyv_Val = yy_0_1_3;
yyv_Pos = yy_0_1_4;
yyv_Prefix = yy_0_2;
yyv_V = yy_0_3;
yy_1_1 = yyv_Val;
yy_1_2 = yyv_Prefix;
yy_1_3 = yyv_V;
DefineTempos_DEFARG(yy_1_1, yy_1_2, yy_1_3, &yy_1_4);
yyv_V2 = yy_1_4;
yy_0_4 = yyv_V2;
*yyout_1 = yy_0_4;
return;
yyfl_200_7 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Type;
yy yy_0_1_1;
yy yyv_Key;
yy yy_0_1_2;
yy yyv_Pos;
yy yy_0_1_3;
yy yyv_Offset;
yy yy_0_1_4;
yy yyv_Prefix;
yy yy_0_2;
yy yyv_V;
yy yy_0_3;
yy yy_0_4;
yy yy_1_1;
yy yy_1_2;
yy yyv_List;
yy yy_1_3;
yy yy_2_1;
yy yyv_N;
yy yy_2_2;
yy yy_3_1;
yy yy_3_1_1;
yy yy_3_1_2;
yy yyv_OffsetVal;
yy yy_3_2;
yy yy_4_1;
yy yy_4_2;
yy yy_5_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 4) goto yyfl_200_8;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yyv_Type = yy_0_1_1;
yyv_Key = yy_0_1_2;
yyv_Pos = yy_0_1_3;
yyv_Offset = yy_0_1_4;
yyv_Prefix = yy_0_2;
yyv_V = yy_0_3;
yy_1_1 = yyv_Type;
yy_1_2 = yyv_Pos;
GetTableFields(yy_1_1, yy_1_2, &yy_1_3);
yyv_List = yy_1_3;
yy_2_1 = yyv_List;
Length(yy_2_1, &yy_2_2);
yyv_N = yy_2_2;
yy_3_1_1 = yyv_N;
yy_3_1_2 = ((yy)1);
yy_3_1 = (yy)(((long)yy_3_1_1)+((long)yy_3_1_2));
ReserveSpace(yy_3_1, &yy_3_2);
yyv_OffsetVal = yy_3_2;
yy_4_1 = yyv_Offset;
yy_4_2 = yyv_OffsetVal;
SetINT(yy_4_1, yy_4_2);
yy_5_1 = yyv_Key;
MakeLocalVar(yy_5_1);
yy_0_4 = yyv_V;
*yyout_1 = yy_0_4;
return;
yyfl_200_8 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Cases;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_Pos;
yy yy_0_1_3;
yy yyv_Prefix;
yy yy_0_2;
yy yyv_V0;
yy yy_0_3;
yy yy_0_4;
yy yy_1_1;
yy yy_1_2;
yy yy_1_2_1;
yy yy_1_2_2;
yy yyv_V1;
yy yy_1_3;
yy yy_2;
yy yy_3_1;
yy yy_3_2;
yy yyv_V2;
yy yy_3_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 10) goto yyfl_200_9;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Cases = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_Pos = yy_0_1_3;
yyv_Prefix = yy_0_2;
yyv_V0 = yy_0_3;
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_Cases;
yy_1_2_1 = ((yy)1);
yy_1_2_2 = yyv_Prefix;
yy_1_2 = yyb + 0;
yy_1_2[0] = 1;
yy_1_2[1] = ((long)yy_1_2_1);
yy_1_2[2] = ((long)yy_1_2_2);
DefineTempos_ALTERNATIVELIST(yy_1_1, yy_1_2, &yy_1_3);
yyv_V1 = yy_1_3;
yy_2 = yyv_V1;
yyv_P[1] = (long) yy_2;
yy_3_1 = yyv_V0;
yy_3_2 = yyv_V1;
Append(yy_3_1, yy_3_2, &yy_3_3);
yyv_V2 = yy_3_3;
yy_0_4 = yyv_V2;
*yyout_1 = yy_0_4;
return;
yyfl_200_9 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_M;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_Prefix;
yy yy_0_2;
yy yyv_V0;
yy yy_0_3;
yy yy_0_4;
yy yy_1_1;
yy yy_1_2;
yy yy_1_2_1;
yy yy_1_2_2;
yy yy_1_3;
yy yy_1_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 11) goto yyfl_200_10;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_M = yy_0_1_1;
yyv_Prefix = yy_0_2;
yyv_V0 = yy_0_3;
yyb = yyh;
yyh += 5; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_M;
yy_1_2_1 = ((yy)1);
yy_1_2_2 = yyv_Prefix;
yy_1_2 = yyb + 1;
yy_1_2[0] = 1;
yy_1_2[1] = ((long)yy_1_2_1);
yy_1_2[2] = ((long)yy_1_2_2);
yy_1_3 = yyb + 4;
yy_1_3[0] = 2;
DefineTempos_MEMBERS(yy_1_1, yy_1_2, yy_1_3, &yy_1_4);
yy_0_4 = yyb + 0;
yy_0_4[0] = 2;
*yyout_1 = yy_0_4;
return;
yyfl_200_10 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_S;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_Prefix;
yy yy_0_2;
yy yyv_V;
yy yy_0_3;
yy yy_0_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 8) goto yyfl_200_11;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_S = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_Prefix = yy_0_2;
yyv_V = yy_0_3;
yy_0_4 = yyv_V;
*yyout_1 = yy_0_4;
return;
yyfl_200_11 : ;
}
yyErr(2,1522);
}
DefineTempos_ALTERNATIVELIST(yyin_1, yyin_2, yyout_1)
yy yyin_1;
yy yyin_2;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_2;
yy yyv_N;
yy yy_0_2_1;
yy yyv_Prefix;
yy yy_0_2_2;
yy yy_0_3;
yy yy_1_1;
yy yy_1_2;
yy yy_1_2_1;
yy yy_1_2_2;
yy yy_1_2_2_1;
yy yy_1_2_2_2;
yy yy_1_3;
yy yyv_V;
yy yy_1_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_201_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_H = yy_0_1_1;
yyv_P = yy_0_1_2;
if (yy_0_1_3[0] != 2) goto yyfl_201_1;
if (yy_0_2[0] != 1) goto yyfl_201_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yyv_N = yy_0_2_1;
yyv_Prefix = yy_0_2_2;
yyb = yyh;
yyh += 7; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_H;
yy_1_2_1 = ((yy)1);
yy_1_2_2_1 = yyv_N;
yy_1_2_2_2 = yyv_Prefix;
yy_1_2_2 = yyb + 3;
yy_1_2_2[0] = 1;
yy_1_2_2[1] = ((long)yy_1_2_2_1);
yy_1_2_2[2] = ((long)yy_1_2_2_2);
yy_1_2 = yyb + 0;
yy_1_2[0] = 1;
yy_1_2[1] = ((long)yy_1_2_1);
yy_1_2[2] = ((long)yy_1_2_2);
yy_1_3 = yyb + 6;
yy_1_3[0] = 2;
DefineTempos_MEMBERS(yy_1_1, yy_1_2, yy_1_3, &yy_1_4);
yyv_V = yy_1_4;
yy_0_3 = yyv_V;
*yyout_1 = yy_0_3;
return;
yyfl_201_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_T;
yy yy_0_1_3;
yy yy_0_2;
yy yyv_N;
yy yy_0_2_1;
yy yyv_Prefix;
yy yy_0_2_2;
yy yy_0_3;
yy yy_1_1;
yy yy_1_2;
yy yy_1_2_1;
yy yy_1_2_2;
yy yy_1_2_2_1;
yy yy_1_2_2_2;
yy yy_1_3;
yy yyv_V1;
yy yy_1_4;
yy yy_2_1;
yy yy_2_2;
yy yy_2_2_1;
yy yy_2_2_1_1;
yy yy_2_2_1_2;
yy yy_2_2_2;
yy yyv_V2;
yy yy_2_3;
yy yy_3_1;
yy yy_3_2;
yy yyv_V;
yy yy_3_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_201_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_H = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_T = yy_0_1_3;
if (yy_0_2[0] != 1) goto yyfl_201_2;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yyv_N = yy_0_2_1;
yyv_Prefix = yy_0_2_2;
yyb = yyh;
yyh += 10; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_H;
yy_1_2_1 = ((yy)1);
yy_1_2_2_1 = yyv_N;
yy_1_2_2_2 = yyv_Prefix;
yy_1_2_2 = yyb + 3;
yy_1_2_2[0] = 1;
yy_1_2_2[1] = ((long)yy_1_2_2_1);
yy_1_2_2[2] = ((long)yy_1_2_2_2);
yy_1_2 = yyb + 0;
yy_1_2[0] = 1;
yy_1_2[1] = ((long)yy_1_2_1);
yy_1_2[2] = ((long)yy_1_2_2);
yy_1_3 = yyb + 6;
yy_1_3[0] = 2;
DefineTempos_MEMBERS(yy_1_1, yy_1_2, yy_1_3, &yy_1_4);
yyv_V1 = yy_1_4;
yy_2_1 = yyv_T;
yy_2_2_1_1 = yyv_N;
yy_2_2_1_2 = ((yy)1);
yy_2_2_1 = (yy)(((long)yy_2_2_1_1)+((long)yy_2_2_1_2));
yy_2_2_2 = yyv_Prefix;
yy_2_2 = yyb + 7;
yy_2_2[0] = 1;
yy_2_2[1] = ((long)yy_2_2_1);
yy_2_2[2] = ((long)yy_2_2_2);
DefineTempos_ALTERNATIVELIST(yy_2_1, yy_2_2, &yy_2_3);
yyv_V2 = yy_2_3;
yy_3_1 = yyv_V1;
yy_3_2 = yyv_V2;
Intersection(yy_3_1, yy_3_2, &yy_3_3);
yyv_V = yy_3_3;
yy_0_3 = yyv_V;
*yyout_1 = yy_0_3;
return;
yyfl_201_2 : ;
}
yyErr(2,1577);
}
DefineTempos_DEFARGS(yyin_1, yyin_2, yyin_3, yyout_1, yyout_2)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy *yyout_1;
yy *yyout_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_T;
yy yy_0_1_3;
yy yy_0_2;
yy yyv_N;
yy yy_0_2_1;
yy yyv_Tl;
yy yy_0_2_2;
yy yyv_V0;
yy yy_0_3;
yy yy_0_4;
yy yy_0_5;
yy yy_1_1;
yy yy_1_2;
yy yy_1_2_1;
yy yy_1_2_2;
yy yy_1_3;
yy yyv_V1;
yy yy_1_4;
yy yy_2_1;
yy yy_2_2;
yy yy_2_2_1;
yy yy_2_2_1_1;
yy yy_2_2_1_2;
yy yy_2_2_2;
yy yy_2_3;
yy yyv_Next;
yy yy_2_4;
yy yyv_V2;
yy yy_2_5;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 1) goto yyfl_202_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_H = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_T = yy_0_1_3;
if (yy_0_2[0] != 1) goto yyfl_202_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yyv_N = yy_0_2_1;
yyv_Tl = yy_0_2_2;
yyv_V0 = yy_0_3;
yyb = yyh;
yyh += 6; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_H;
yy_1_2_1 = yyv_N;
yy_1_2_2 = yyv_Tl;
yy_1_2 = yyb + 0;
yy_1_2[0] = 1;
yy_1_2[1] = ((long)yy_1_2_1);
yy_1_2[2] = ((long)yy_1_2_2);
yy_1_3 = yyv_V0;
DefineTempos_DEFARG(yy_1_1, yy_1_2, yy_1_3, &yy_1_4);
yyv_V1 = yy_1_4;
yy_2_1 = yyv_T;
yy_2_2_1_1 = yyv_N;
yy_2_2_1_2 = ((yy)1);
yy_2_2_1 = (yy)(((long)yy_2_2_1_1)+((long)yy_2_2_1_2));
yy_2_2_2 = yyv_Tl;
yy_2_2 = yyb + 3;
yy_2_2[0] = 1;
yy_2_2[1] = ((long)yy_2_2_1);
yy_2_2[2] = ((long)yy_2_2_2);
yy_2_3 = yyv_V1;
DefineTempos_DEFARGS(yy_2_1, yy_2_2, yy_2_3, &yy_2_4, &yy_2_5);
yyv_Next = yy_2_4;
yyv_V2 = yy_2_5;
yy_0_4 = yyv_Next;
yy_0_5 = yyv_V2;
*yyout_1 = yy_0_4;
*yyout_2 = yy_0_5;
return;
yyfl_202_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_P;
yy yy_0_1_1;
yy yyv_Prefix;
yy yy_0_2;
yy yyv_V;
yy yy_0_3;
yy yy_0_4;
yy yy_0_5;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 2) goto yyfl_202_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yyv_P = yy_0_1_1;
yyv_Prefix = yy_0_2;
yyv_V = yy_0_3;
yy_0_4 = yyv_Prefix;
yy_0_5 = yyv_V;
*yyout_1 = yy_0_4;
*yyout_2 = yy_0_5;
return;
yyfl_202_2 : ;
}
yyErr(2,1586);
}
DefineTempos_USEARGS(yyin_1, yyin_2, yyout_1)
yy yyin_1;
yy yyin_2;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_T;
yy yy_0_1_3;
yy yy_0_2;
yy yyv_N;
yy yy_0_2_1;
yy yyv_Tl;
yy yy_0_2_2;
yy yy_0_3;
yy yy_1_1;
yy yy_1_2;
yy yy_1_2_1;
yy yy_1_2_2;
yy yy_2_1;
yy yy_2_2;
yy yy_2_2_1;
yy yy_2_2_1_1;
yy yy_2_2_1_2;
yy yy_2_2_2;
yy yyv_Next;
yy yy_2_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_203_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_H = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_T = yy_0_1_3;
if (yy_0_2[0] != 1) goto yyfl_203_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yyv_N = yy_0_2_1;
yyv_Tl = yy_0_2_2;
yyb = yyh;
yyh += 6; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_H;
yy_1_2_1 = yyv_N;
yy_1_2_2 = yyv_Tl;
yy_1_2 = yyb + 0;
yy_1_2[0] = 1;
yy_1_2[1] = ((long)yy_1_2_1);
yy_1_2[2] = ((long)yy_1_2_2);
DefineTempos_USEARG(yy_1_1, yy_1_2);
yy_2_1 = yyv_T;
yy_2_2_1_1 = yyv_N;
yy_2_2_1_2 = ((yy)1);
yy_2_2_1 = (yy)(((long)yy_2_2_1_1)+((long)yy_2_2_1_2));
yy_2_2_2 = yyv_Tl;
yy_2_2 = yyb + 3;
yy_2_2[0] = 1;
yy_2_2[1] = ((long)yy_2_2_1);
yy_2_2[2] = ((long)yy_2_2_2);
DefineTempos_USEARGS(yy_2_1, yy_2_2, &yy_2_3);
yyv_Next = yy_2_3;
yy_0_3 = yyv_Next;
*yyout_1 = yy_0_3;
return;
yyfl_203_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_P;
yy yy_0_1_1;
yy yyv_Prefix;
yy yy_0_2;
yy yy_0_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_203_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yyv_P = yy_0_1_1;
yyv_Prefix = yy_0_2;
yy_0_3 = yyv_Prefix;
*yyout_1 = yy_0_3;
return;
yyfl_203_2 : ;
}
yyErr(2,1595);
}
DefineTempos_DEFARG(yyin_1, yyin_2, yyin_3, yyout_1)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_Functor;
yy yy_0_1_2;
yy yyv_P;
yy yy_0_1_3;
yy yyv_Args;
yy yy_0_1_4;
yy yyv_Prefix;
yy yy_0_2;
yy yyv_V0;
yy yy_0_3;
yy yy_0_4;
yy yy_1_1;
yy yy_1_2;
yy yy_2_1;
yy yy_2_2;
yy yy_2_2_1;
yy yy_2_2_2;
yy yy_2_3;
yy yyv_Dummy;
yy yy_2_4;
yy yyv_V1;
yy yy_2_5;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 1) goto yyfl_204_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yyv_Tmp = yy_0_1_1;
yyv_Functor = yy_0_1_2;
yyv_P = yy_0_1_3;
yyv_Args = yy_0_1_4;
yyv_Prefix = yy_0_2;
yyv_V0 = yy_0_3;
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_Tmp;
yy_1_2 = yyv_Prefix;
MakeTempo(yy_1_1, yy_1_2);
yy_2_1 = yyv_Args;
yy_2_2_1 = ((yy)1);
yy_2_2_2 = yyv_Prefix;
yy_2_2 = yyb + 0;
yy_2_2[0] = 1;
yy_2_2[1] = ((long)yy_2_2_1);
yy_2_2[2] = ((long)yy_2_2_2);
yy_2_3 = yyv_V0;
DefineTempos_DEFARGS(yy_2_1, yy_2_2, yy_2_3, &yy_2_4, &yy_2_5);
yyv_Dummy = yy_2_4;
yyv_V1 = yy_2_5;
yy_0_4 = yyv_V1;
*yyout_1 = yy_0_4;
return;
yyfl_204_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tp;
yy yy_0_1_1;
yy yyv_Tmp;
yy yy_0_1_2;
yy yyv_Functor;
yy yy_0_1_3;
yy yyv_P;
yy yy_0_1_4;
yy yyv_Args;
yy yy_0_1_5;
yy yyv_Prefix;
yy yy_0_2;
yy yyv_V0;
yy yy_0_3;
yy yy_0_4;
yy yy_1_1;
yy yy_1_2;
yy yy_2_1;
yy yy_2_2;
yy yy_2_2_1;
yy yy_2_2_2;
yy yy_2_3;
yy yyv_Dummy;
yy yy_2_4;
yy yyv_V1;
yy yy_2_5;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 2) goto yyfl_204_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yy_0_1_5 = ((yy)yy_0_1[5]);
yyv_Tp = yy_0_1_1;
yyv_Tmp = yy_0_1_2;
yyv_Functor = yy_0_1_3;
yyv_P = yy_0_1_4;
yyv_Args = yy_0_1_5;
yyv_Prefix = yy_0_2;
yyv_V0 = yy_0_3;
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_Tmp;
yy_1_2 = yyv_Prefix;
MakeTempo(yy_1_1, yy_1_2);
yy_2_1 = yyv_Args;
yy_2_2_1 = ((yy)1);
yy_2_2_2 = yyv_Prefix;
yy_2_2 = yyb + 0;
yy_2_2[0] = 1;
yy_2_2[1] = ((long)yy_2_2_1);
yy_2_2[2] = ((long)yy_2_2_2);
yy_2_3 = yyv_V0;
DefineTempos_DEFARGS(yy_2_1, yy_2_2, yy_2_3, &yy_2_4, &yy_2_5);
yyv_Dummy = yy_2_4;
yyv_V1 = yy_2_5;
yy_0_4 = yyv_V1;
*yyout_1 = yy_0_4;
return;
yyfl_204_2 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_Name;
yy yy_0_1_2;
yy yyv_P;
yy yy_0_1_3;
yy yyv_Prefix;
yy yy_0_2;
yy yyv_V;
yy yy_0_3;
yy yy_0_4;
yy yy_0_4_1;
yy yy_0_4_2;
yy yy_1_1;
yy yy_2_1;
yy yy_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 3) goto yyfl_204_3;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Tmp = yy_0_1_1;
yyv_Name = yy_0_1_2;
yyv_P = yy_0_1_3;
yyv_Prefix = yy_0_2;
yyv_V = yy_0_3;
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_Name;
MakeLocalVar(yy_1_1);
yy_2_1 = yyv_Tmp;
yy_2_2 = yyv_Prefix;
MakeTempo(yy_2_1, yy_2_2);
yy_0_4_1 = yyv_Name;
yy_0_4_2 = yyv_V;
yy_0_4 = yyb + 0;
yy_0_4[0] = 1;
yy_0_4[1] = ((long)yy_0_4_1);
yy_0_4[2] = ((long)yy_0_4_2);
*yyout_1 = yy_0_4;
return;
yyfl_204_3 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_Name;
yy yy_0_1_2;
yy yyv_P;
yy yy_0_1_3;
yy yyv_Arg;
yy yy_0_1_4;
yy yyv_Prefix;
yy yy_0_2;
yy yyv_V0;
yy yy_0_3;
yy yy_0_4;
yy yy_1_1;
yy yy_2_1;
yy yy_2_2;
yy yy_3_1;
yy yy_3_2;
yy yy_3_2_1;
yy yy_3_2_2;
yy yy_3_3;
yy yyv_V1;
yy yy_3_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 4) goto yyfl_204_4;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yyv_Tmp = yy_0_1_1;
yyv_Name = yy_0_1_2;
yyv_P = yy_0_1_3;
yyv_Arg = yy_0_1_4;
yyv_Prefix = yy_0_2;
yyv_V0 = yy_0_3;
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_Name;
MakeLocalVar(yy_1_1);
yy_2_1 = yyv_Tmp;
yy_2_2 = yyv_Prefix;
MakeTempo(yy_2_1, yy_2_2);
yy_3_1 = yyv_Arg;
yy_3_2_1 = ((yy)1);
yy_3_2_2 = yyv_Prefix;
yy_3_2 = yyb + 0;
yy_3_2[0] = 1;
yy_3_2[1] = ((long)yy_3_2_1);
yy_3_2[2] = ((long)yy_3_2_2);
yy_3_3 = yyv_V0;
DefineTempos_DEFARG(yy_3_1, yy_3_2, yy_3_3, &yy_3_4);
yyv_V1 = yy_3_4;
yy_0_4 = yyv_V1;
*yyout_1 = yy_0_4;
return;
yyfl_204_4 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_Prefix;
yy yy_0_2;
yy yyv_V;
yy yy_0_3;
yy yy_0_4;
yy yy_1_1;
yy yy_1_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 5) goto yyfl_204_5;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_Tmp = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_Prefix = yy_0_2;
yyv_V = yy_0_3;
yy_1_1 = yyv_Tmp;
yy_1_2 = yyv_Prefix;
MakeTempo(yy_1_1, yy_1_2);
yy_0_4 = yyv_V;
*yyout_1 = yy_0_4;
return;
yyfl_204_5 : ;
}
yyErr(2,1604);
}
DefineTempos_USEARG(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_Op;
yy yy_0_1_2;
yy yyv_P;
yy yy_0_1_3;
yy yyv_Left;
yy yy_0_1_4;
yy yyv_Right;
yy yy_0_1_5;
yy yyv_Prefix;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy yy_2_1;
yy yy_2_2;
yy yy_2_2_1;
yy yy_2_2_2;
yy yy_3_1;
yy yy_3_2;
yy yy_3_2_1;
yy yy_3_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_205_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yy_0_1_5 = ((yy)yy_0_1[5]);
yyv_Tmp = yy_0_1_1;
yyv_Op = yy_0_1_2;
yyv_P = yy_0_1_3;
yyv_Left = yy_0_1_4;
yyv_Right = yy_0_1_5;
yyv_Prefix = yy_0_2;
yyb = yyh;
yyh += 6; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_Tmp;
yy_1_2 = yyv_Prefix;
MakeTempo(yy_1_1, yy_1_2);
yy_2_1 = yyv_Left;
yy_2_2_1 = ((yy)1);
yy_2_2_2 = yyv_Prefix;
yy_2_2 = yyb + 0;
yy_2_2[0] = 1;
yy_2_2[1] = ((long)yy_2_2_1);
yy_2_2[2] = ((long)yy_2_2_2);
DefineTempos_USEARG(yy_2_1, yy_2_2);
yy_3_1 = yyv_Right;
yy_3_2_1 = ((yy)2);
yy_3_2_2 = yyv_Prefix;
yy_3_2 = yyb + 3;
yy_3_2[0] = 1;
yy_3_2[1] = ((long)yy_3_2_1);
yy_3_2[2] = ((long)yy_3_2_2);
DefineTempos_USEARG(yy_3_1, yy_3_2);
return;
yyfl_205_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_Op;
yy yy_0_1_2;
yy yyv_P;
yy yy_0_1_3;
yy yyv_Arg;
yy yy_0_1_4;
yy yyv_Prefix;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy yy_2_1;
yy yy_2_2;
yy yy_2_2_1;
yy yy_2_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_205_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yyv_Tmp = yy_0_1_1;
yyv_Op = yy_0_1_2;
yyv_P = yy_0_1_3;
yyv_Arg = yy_0_1_4;
yyv_Prefix = yy_0_2;
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_Tmp;
yy_1_2 = yyv_Prefix;
MakeTempo(yy_1_1, yy_1_2);
yy_2_1 = yyv_Arg;
yy_2_2_1 = ((yy)1);
yy_2_2_2 = yyv_Prefix;
yy_2_2 = yyb + 0;
yy_2_2[0] = 1;
yy_2_2[1] = ((long)yy_2_2_1);
yy_2_2[2] = ((long)yy_2_2_2);
DefineTempos_USEARG(yy_2_1, yy_2_2);
return;
yyfl_205_2 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_Term;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_Prefix;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 4) goto yyfl_205_3;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Term = yy_0_1_2;
yyv_Prefix = yy_0_2;
yy_1_1 = yyv_Term;
yy_1_2 = yyv_Prefix;
DefineTempos_USEARG(yy_1_1, yy_1_2);
return;
yyfl_205_3 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_Offset;
yy yy_0_1_2;
yy yyv_Functor;
yy yy_0_1_3;
yy yyv_P;
yy yy_0_1_4;
yy yyv_Args;
yy yy_0_1_5;
yy yyv_Prefix;
yy yy_0_2;
yy yy_1_1;
yy yyv_N;
yy yy_1_2;
yy yy_2_1;
yy yy_2_1_1;
yy yy_2_1_2;
yy yyv_OffsetVal;
yy yy_2_2;
yy yy_3_1;
yy yy_3_2;
yy yy_4_1;
yy yy_4_2;
yy yy_5_1;
yy yy_5_2;
yy yy_5_2_1;
yy yy_5_2_2;
yy yyv_Dummy;
yy yy_5_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 3) goto yyfl_205_4;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yy_0_1_5 = ((yy)yy_0_1[5]);
yyv_Tmp = yy_0_1_1;
yyv_Offset = yy_0_1_2;
yyv_Functor = yy_0_1_3;
yyv_P = yy_0_1_4;
yyv_Args = yy_0_1_5;
yyv_Prefix = yy_0_2;
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_Args;
NumberOfArgs(yy_1_1, &yy_1_2);
yyv_N = yy_1_2;
yy_2_1_1 = yyv_N;
yy_2_1_2 = ((yy)1);
yy_2_1 = (yy)(((long)yy_2_1_1)+((long)yy_2_1_2));
ReserveSpace(yy_2_1, &yy_2_2);
yyv_OffsetVal = yy_2_2;
yy_3_1 = yyv_Offset;
yy_3_2 = yyv_OffsetVal;
SetINT(yy_3_1, yy_3_2);
yy_4_1 = yyv_Tmp;
yy_4_2 = yyv_Prefix;
MakeTempo(yy_4_1, yy_4_2);
yy_5_1 = yyv_Args;
yy_5_2_1 = ((yy)1);
yy_5_2_2 = yyv_Prefix;
yy_5_2 = yyb + 0;
yy_5_2[0] = 1;
yy_5_2[1] = ((long)yy_5_2_1);
yy_5_2[2] = ((long)yy_5_2_2);
DefineTempos_USEARGS(yy_5_1, yy_5_2, &yy_5_3);
yyv_Dummy = yy_5_3;
return;
yyfl_205_4 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_Name;
yy yy_0_1_2;
yy yyv_P;
yy yy_0_1_3;
yy yyv_Prefix;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 5) goto yyfl_205_5;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Tmp = yy_0_1_1;
yyv_Name = yy_0_1_2;
yyv_P = yy_0_1_3;
yyv_Prefix = yy_0_2;
yy_1_1 = yyv_Tmp;
yy_1_2 = yyv_Prefix;
MakeTempo(yy_1_1, yy_1_2);
return;
yyfl_205_5 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_Number;
yy yy_0_1_2;
yy yyv_P;
yy yy_0_1_3;
yy yyv_Prefix;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 6) goto yyfl_205_6;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Tmp = yy_0_1_1;
yyv_Number = yy_0_1_2;
yyv_P = yy_0_1_3;
yyv_Prefix = yy_0_2;
yy_1_1 = yyv_Tmp;
yy_1_2 = yyv_Prefix;
MakeTempo(yy_1_1, yy_1_2);
return;
yyfl_205_6 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_String;
yy yy_0_1_2;
yy yyv_P;
yy yy_0_1_3;
yy yyv_Prefix;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 7) goto yyfl_205_7;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Tmp = yy_0_1_1;
yyv_String = yy_0_1_2;
yyv_P = yy_0_1_3;
yyv_Prefix = yy_0_2;
yy_1_1 = yyv_Tmp;
yy_1_2 = yyv_Prefix;
MakeTempo(yy_1_1, yy_1_2);
return;
yyfl_205_7 : ;
}
yyErr(2,1628);
}
MakeTempo(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yyv_RefTempo;
yy yy_0_1;
yy yyv_Prefix;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy yy_2_1;
yy yy_3_1;
yy yy_4_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_RefTempo = yy_0_1;
yyv_Prefix = yy_0_2;
yy_1_1 = yyv_RefTempo;
yy_1_2 = yyv_Prefix;
SetTempo(yy_1_1, yy_1_2);
yy_2_1 = ((yy)"yy ");
s(yy_2_1);
yy_3_1 = yyv_RefTempo;
tmp(yy_3_1);
yy_4_1 = ((yy)";");
s(yy_4_1);
nl();
return;
}
}
MakeLocalVar(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yyv_Name;
yy yy_0_1;
yy yy_1_1_1_1;
yy yyv_L;
yy yy_1_2_1;
yy yy_1_2_2;
yy yy_1_2_2_1;
yy yy_1_2_2_2;
yy yy_1_2_3_1;
yy yy_1_2_4_1;
yy yy_1_2_5_1;
yy_0_1 = yyin_1;
yyv_Name = yy_0_1;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Name;
if (! IsDeclared(yy_1_1_1_1)) goto yyfl_207_1_1_1;
goto yysl_207_1_1;
yyfl_207_1_1_1 : ;
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yy_1_2_1 = yyglov_declaredVars;
if (yy_1_2_1 == (yy) yyu) yyErr(1,1668);
yyv_L = yy_1_2_1;
yy_1_2_2_1 = yyv_Name;
yy_1_2_2_2 = yyv_L;
yy_1_2_2 = yyb + 0;
yy_1_2_2[0] = 1;
yy_1_2_2[1] = ((long)yy_1_2_2_1);
yy_1_2_2[2] = ((long)yy_1_2_2_2);
yyglov_declaredVars = yy_1_2_2;
yy_1_2_3_1 = ((yy)"yy ");
s(yy_1_2_3_1);
yy_1_2_4_1 = yyv_Name;
varid(yy_1_2_4_1);
yy_1_2_5_1 = ((yy)";");
s(yy_1_2_5_1);
nl();
goto yysl_207_1_1;
yysl_207_1_1 : ;
yyb = yysb;
}
return;
}
}
int IsDeclared(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yyv_L;
yy yy_1;
yy yy_2_1;
yy yy_2_2;
yy_0_1 = yyin_1;
yyv_Id = yy_0_1;
yy_1 = yyglov_declaredVars;
if (yy_1 == (yy) yyu) yyErr(1,1677);
yyv_L = yy_1;
yy_2_1 = yyv_L;
yy_2_2 = yyv_Id;
if (! IsInList(yy_2_1, yy_2_2)) goto yyfl_208_1;
return 1;
yyfl_208_1 : ;
}
return 0;
}
int IsInList(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_T;
yy yy_0_1_2;
yy yyv_Id;
yy yy_0_2;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_209_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_H = yy_0_1_1;
yyv_T = yy_0_1_2;
yyv_Id = yy_0_2;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_H;
yy_1_1_1_2 = yyv_Id;
if (! yyeq_IDENT(yy_1_1_1_1, yy_1_1_1_2)) goto yyfl_209_1_1_1;
goto yysl_209_1_1;
yyfl_209_1_1_1 : ;
yy_1_2_1_1 = yyv_T;
yy_1_2_1_2 = yyv_Id;
if (! IsInList(yy_1_2_1_1, yy_1_2_1_2)) goto yyfl_209_1_1_2;
goto yysl_209_1_1;
yyfl_209_1_1_2 : ;
goto yyfl_209_1;
yysl_209_1_1 : ;
yyb = yysb;
}
return 1;
yyfl_209_1 : ;
}
return 0;
}
Length(yyin_1, yyout_1)
yy yyin_1;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_T;
yy yy_0_1_2;
yy yy_0_2;
yy yy_0_2_1;
yy yy_0_2_2;
yy yy_1_1;
yy yyv_N;
yy yy_1_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_210_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_H = yy_0_1_1;
yyv_T = yy_0_1_2;
yy_1_1 = yyv_T;
Length(yy_1_1, &yy_1_2);
yyv_N = yy_1_2;
yy_0_2_1 = yyv_N;
yy_0_2_2 = ((yy)1);
yy_0_2 = (yy)(((long)yy_0_2_1)+((long)yy_0_2_2));
*yyout_1 = yy_0_2;
return;
yyfl_210_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_210_2;
yy_0_2 = ((yy)0);
*yyout_1 = yy_0_2;
return;
yyfl_210_2 : ;
}
yyErr(2,1686);
}
NumberOfArgs(yyin_1, yyout_1)
yy yyin_1;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_T;
yy yy_0_1_3;
yy yy_0_2;
yy yy_0_2_1;
yy yy_0_2_2;
yy yy_1_1;
yy yyv_N;
yy yy_1_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_211_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_H = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_T = yy_0_1_3;
yy_1_1 = yyv_T;
NumberOfArgs(yy_1_1, &yy_1_2);
yyv_N = yy_1_2;
yy_0_2_1 = yyv_N;
yy_0_2_2 = ((yy)1);
yy_0_2 = (yy)(((long)yy_0_2_1)+((long)yy_0_2_2));
*yyout_1 = yy_0_2;
return;
yyfl_211_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_P;
yy yy_0_1_1;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_211_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yyv_P = yy_0_1_1;
yy_0_2 = ((yy)0);
*yyout_1 = yy_0_2;
return;
yyfl_211_2 : ;
}
yyErr(2,1692);
}
Intersection(yyin_1, yyin_2, yyout_1)
yy yyin_1;
yy yyin_2;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_T;
yy yy_0_1_2;
yy yyv_L;
yy yy_0_2;
yy yy_0_3;
yy yy_0_3_1;
yy yy_0_3_2;
yy yy_1_1;
yy yy_1_2;
yy yy_2_1;
yy yy_2_2;
yy yyv_TL;
yy yy_2_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_214_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_H = yy_0_1_1;
yyv_T = yy_0_1_2;
yyv_L = yy_0_2;
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_L;
yy_1_2 = yyv_H;
if (! IsInList(yy_1_1, yy_1_2)) goto yyfl_214_1;
yy_2_1 = yyv_T;
yy_2_2 = yyv_L;
Intersection(yy_2_1, yy_2_2, &yy_2_3);
yyv_TL = yy_2_3;
yy_0_3_1 = yyv_H;
yy_0_3_2 = yyv_TL;
yy_0_3 = yyb + 0;
yy_0_3[0] = 1;
yy_0_3[1] = ((long)yy_0_3_1);
yy_0_3[2] = ((long)yy_0_3_2);
*yyout_1 = yy_0_3;
return;
yyfl_214_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_T;
yy yy_0_1_2;
yy yyv_L;
yy yy_0_2;
yy yy_0_3;
yy yy_1_1;
yy yy_1_2;
yy yyv_TL;
yy yy_1_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_214_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_H = yy_0_1_1;
yyv_T = yy_0_1_2;
yyv_L = yy_0_2;
yy_1_1 = yyv_T;
yy_1_2 = yyv_L;
Intersection(yy_1_1, yy_1_2, &yy_1_3);
yyv_TL = yy_1_3;
yy_0_3 = yyv_TL;
*yyout_1 = yy_0_3;
return;
yyfl_214_2 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_L;
yy yy_0_2;
yy yy_0_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_214_3;
yyv_L = yy_0_2;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_0_3 = yyb + 0;
yy_0_3[0] = 2;
*yyout_1 = yy_0_3;
return;
yyfl_214_3 : ;
}
yyErr(2,1704);
}
Append(yyin_1, yyin_2, yyout_1)
yy yyin_1;
yy yyin_2;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_T;
yy yy_0_1_2;
yy yyv_L;
yy yy_0_2;
yy yy_0_3;
yy yy_0_3_1;
yy yy_0_3_2;
yy yy_1_1;
yy yy_1_2;
yy yyv_TL;
yy yy_1_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_215_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_H = yy_0_1_1;
yyv_T = yy_0_1_2;
yyv_L = yy_0_2;
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_T;
yy_1_2 = yyv_L;
Append(yy_1_1, yy_1_2, &yy_1_3);
yyv_TL = yy_1_3;
yy_0_3_1 = yyv_H;
yy_0_3_2 = yyv_TL;
yy_0_3 = yyb + 0;
yy_0_3[0] = 1;
yy_0_3[1] = ((long)yy_0_3_1);
yy_0_3[2] = ((long)yy_0_3_2);
*yyout_1 = yy_0_3;
return;
yyfl_215_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_L;
yy yy_0_2;
yy yy_0_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_215_2;
yyv_L = yy_0_2;
yy_0_3 = yyv_L;
*yyout_1 = yy_0_3;
return;
yyfl_215_2 : ;
}
yyErr(2,1714);
}
BuildTypedVars(yyin_1, yyout_1)
yy yyin_1;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_T;
yy yy_0_1_2;
yy yy_0_2;
yy yy_0_2_1;
yy yy_0_2_1_1;
yy yy_0_2_1_2;
yy yy_0_2_2;
yy yy_1_1;
yy yyv_Type;
yy yy_1_2;
yy yy_2_1;
yy yyv_T2;
yy yy_2_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_216_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_H = yy_0_1_1;
yyv_T = yy_0_1_2;
yyb = yyh;
yyh += 6; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_H;
if (! GetLocalMeaning(yy_1_1, &yy_1_2)) goto yyfl_216_1;
yyv_Type = yy_1_2;
yy_2_1 = yyv_T;
BuildTypedVars(yy_2_1, &yy_2_2);
yyv_T2 = yy_2_2;
yy_0_2_1_1 = yyv_H;
yy_0_2_1_2 = yyv_Type;
yy_0_2_1 = yyb + 3;
yy_0_2_1[0] = 1;
yy_0_2_1[1] = ((long)yy_0_2_1_1);
yy_0_2_1[2] = ((long)yy_0_2_1_2);
yy_0_2_2 = yyv_T2;
yy_0_2 = yyb + 0;
yy_0_2[0] = 1;
yy_0_2[1] = ((long)yy_0_2_1);
yy_0_2[2] = ((long)yy_0_2_2);
*yyout_1 = yy_0_2;
return;
yyfl_216_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_216_2;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_0_2 = yyb + 0;
yy_0_2[0] = 2;
*yyout_1 = yy_0_2;
return;
yyfl_216_2 : ;
}
yyErr(2,1721);
}
DeclareTypedVars(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_Id;
yy yy_0_1_1_1;
yy yyv_Type;
yy yy_0_1_1_2;
yy yyv_Tl;
yy yy_0_1_2;
yy yyv_P;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yy_1_3_1;
yy yy_1_4;
yy yy_2_1;
yy yy_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_217_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
if (yy_0_1_1[0] != 1) goto yyfl_217_1;
yy_0_1_1_1 = ((yy)yy_0_1_1[1]);
yy_0_1_1_2 = ((yy)yy_0_1_1[2]);
yyv_Id = yy_0_1_1_1;
yyv_Type = yy_0_1_1_2;
yyv_Tl = yy_0_1_2;
yyv_P = yy_0_2;
yy_1_1 = yyv_Id;
yy_1_2 = yyv_Type;
yy_1_3_1 = ((yy)1);
yy_1_3 = (yy)(-((long)yy_1_3_1));
yy_1_4 = yyv_P;
DefineLocalVar(yy_1_1, yy_1_2, yy_1_3, yy_1_4);
yy_2_1 = yyv_Tl;
yy_2_2 = yyv_P;
DeclareTypedVars(yy_2_1, yy_2_2);
return;
yyfl_217_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_P;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_217_2;
yyv_P = yy_0_2;
return;
yyfl_217_2 : ;
}
yyErr(2,1729);
}
CheckConsistent(yyin_1, yyin_2, yyin_3)
yy yyin_1;
yy yyin_2;
yy yyin_3;
{
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_Id1;
yy yy_0_1_1_1;
yy yyv_Type1;
yy yy_0_1_1_2;
yy yyv_Tl1;
yy yy_0_1_2;
yy yy_0_2;
yy yy_0_2_1;
yy yyv_Id2;
yy yy_0_2_1_1;
yy yyv_Type2;
yy yy_0_2_1_2;
yy yyv_Tl2;
yy yy_0_2_2;
yy yyv_P;
yy yy_0_3;
yy yy_1_1;
yy yy_1_2;
yy yy_2_1;
yy yy_2_2;
yy yy_2_3;
yy yy_2_4;
yy yy_3_1;
yy yy_3_2;
yy yy_3_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 1) goto yyfl_218_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
if (yy_0_1_1[0] != 1) goto yyfl_218_1;
yy_0_1_1_1 = ((yy)yy_0_1_1[1]);
yy_0_1_1_2 = ((yy)yy_0_1_1[2]);
yyv_Id1 = yy_0_1_1_1;
yyv_Type1 = yy_0_1_1_2;
yyv_Tl1 = yy_0_1_2;
if (yy_0_2[0] != 1) goto yyfl_218_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
if (yy_0_2_1[0] != 1) goto yyfl_218_1;
yy_0_2_1_1 = ((yy)yy_0_2_1[1]);
yy_0_2_1_2 = ((yy)yy_0_2_1[2]);
yyv_Id2 = yy_0_2_1_1;
yyv_Type2 = yy_0_2_1_2;
yyv_Tl2 = yy_0_2_2;
yyv_P = yy_0_3;
yy_1_1 = yyv_Id1;
yy_1_2 = yyv_Id2;
if (! yyeq_IDENT(yy_1_1, yy_1_2)) goto yyfl_218_1;
yy_2_1 = yyv_Id1;
yy_2_2 = yyv_Type1;
yy_2_3 = yyv_Type2;
yy_2_4 = yyv_P;
CheckEqType(yy_2_1, yy_2_2, yy_2_3, yy_2_4);
yy_3_1 = yyv_Tl1;
yy_3_2 = yyv_Tl2;
yy_3_3 = yyv_P;
CheckConsistent(yy_3_1, yy_3_2, yy_3_3);
return;
yyfl_218_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_0_2;
yy yyv_P;
yy yy_0_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 2) goto yyfl_218_2;
if (yy_0_2[0] != 2) goto yyfl_218_2;
yyv_P = yy_0_3;
return;
yyfl_218_2 : ;
}
yyErr(2,1737);
}
InitSpace()
{
{
yy yyb;
yy yy_1;
yy_1 = ((yy)0);
yyglov_spaceRequired = yy_1;
return;
}
}
ReserveSpace(yyin_1, yyout_1)
yy yyin_1;
yy *yyout_1;
{
{
yy yyb;
yy yyv_N;
yy yy_0_1;
yy yy_0_2;
yy yyv_S;
yy yy_1;
yy yy_2;
yy yy_2_1;
yy yy_2_2;
yy_0_1 = yyin_1;
yyv_N = yy_0_1;
yy_1 = yyglov_spaceRequired;
if (yy_1 == (yy) yyu) yyErr(1,1761);
yyv_S = yy_1;
yy_2_1 = yyv_S;
yy_2_2 = yyv_N;
yy_2 = (yy)(((long)yy_2_1)+((long)yy_2_2));
yyglov_spaceRequired = yy_2;
yy_0_2 = yyv_S;
*yyout_1 = yy_0_2;
return;
}
}
PutSpace(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yyv_N;
yy yy_0_1;
yy yy_1;
yy_0_1 = yyin_1;
yyv_N = yy_0_1;
yy_1 = yyv_N;
yyglov_spaceRequired = yy_1;
return;
}
}
GetSpace(yyout_1)
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_N;
yy yy_1;
yy_1 = yyglov_spaceRequired;
if (yy_1 == (yy) yyu) yyErr(1,1772);
yyv_N = yy_1;
yy_0_1 = yyv_N;
*yyout_1 = yy_0_1;
return;
}
}
Code(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_N;
yy yy_0_1_1;
yy yyv_I;
yy yy_0_1_2;
yy yyv_D;
yy yy_0_1_3;
yy yyv_P;
yy yy_0_1_4;
yy yyv_Imports;
yy yy_0_2;
yy yy_1;
yy yy_2;
yy yy_3;
yy yyv_ImpDecls;
yy yy_6;
yy yy_7_1;
yy yy_8_1;
yy yy_8_2;
yy yy_10;
yy yy_11_1;
yy yy_12_1_1;
yy yy_12_1_2_1;
yy yy_12_1_3;
yy yy_12_1_5_1;
yy yy_12_1_5_2;
yy yy_12_1_5_3;
yy yyv_L;
yy yy_12_1_6;
yy yy_12_1_7_1;
yy yy_12_1_7_2;
yy yy_12_1_9_1;
yy yy_12_1_10_1;
yy yy_12_1_10_2;
yy yy_12_1_10_3;
yy yy_12_1_11_1;
yy yy_12_1_12_1;
yy yy_12_1_15_1;
yy yyv_Decls;
yy yy_12_1_17;
yy yy_12_1_18_1_1_1;
yy yy_12_1_18_1_1_2;
yy yy_12_1_19_1;
yy yy_12_1_21_1;
yy yy_12_1_22_1;
yy yy_12_1_22_2;
yy yy_12_1_24_1;
yy yy_12_1_26_1;
yy yy_12_1_28;
yy yy_12_1_29_1;
yy yy_12_1_29_2;
yy yy_12_1_29_3;
yy yy_12_1_30_1;
yy yy_12_1_30_2;
yy yy_12_1_31_1;
yy yy_12_1_33;
yy yy_12_1_34_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_227_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yyv_N = yy_0_1_1;
yyv_I = yy_0_1_2;
yyv_D = yy_0_1_3;
yyv_P = yy_0_1_4;
yyv_Imports = yy_0_2;
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yy_1 = yyb + 0;
yy_1[0] = 2;
yyglov_env = yy_1;
yy_2 = yyb + 1;
yy_2[0] = 2;
yyglov_localVars = yy_2;
yy_3 = yyb + 2;
yy_3[0] = 2;
yyglov_currentSuccessLabel = yy_3;
TellClauseFile();
Prelude();
yy_6 = yyglov_importedDeclarations;
if (yy_6 == (yy) yyu) yyErr(1,1796);
yyv_ImpDecls = yy_6;
yy_7_1 = yyv_ImpDecls;
ImportDeclarations_DECLARATIONLIST(yy_7_1);
yy_8_1 = yyv_D;
yy_8_2 = yyb + 3;
yy_8_2[0] = 2;
ForwardCode_DECLARATIONLIST(yy_8_1, yy_8_2);
CodeChoiceProcedures();
yy_10 = ((yy)0);
yyglov_currentProcNumber = yy_10;
yy_11_1 = yyv_D;
Code_DECLARATIONLIST(yy_11_1);
{
yy yysb = yyb;
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yy_12_1_1 = yyglov_containsGrammar;
if (yy_12_1_1 == (yy) yyu) yyErr(1,1806);
if (yy_12_1_1[0] != 1) goto yyfl_227_1_12_1;
yy_12_1_2_1 = ((yy)"gen.h");
TellFile(yy_12_1_2_1);
yy_12_1_3 = ((yy)257);
yyglov_currentTokenCode = yy_12_1_3;
DefYYSTYPE();
yy_12_1_5_1 = yyv_D;
yy_12_1_5_2 = ((yy)"#define ");
yy_12_1_5_3 = yyb + 0;
yy_12_1_5_3[0] = 1;
TokenCode_DECLARATIONLIST(yy_12_1_5_1, yy_12_1_5_2, yy_12_1_5_3);
yy_12_1_6 = yyglov_tokenList;
if (yy_12_1_6 == (yy) yyu) yyErr(1,1814);
yyv_L = yy_12_1_6;
yy_12_1_7_1 = yyv_L;
yy_12_1_7_2 = ((yy)"#define ");
CodeAnonTokenList(yy_12_1_7_1, yy_12_1_7_2);
WriteTokenDescrFile();
yy_12_1_9_1 = ((yy)"gen.tkn");
TellFile(yy_12_1_9_1);
yy_12_1_10_1 = yyv_D;
yy_12_1_10_2 = ((yy)"");
yy_12_1_10_3 = yyb + 1;
yy_12_1_10_3[0] = 2;
TokenCode_DECLARATIONLIST(yy_12_1_10_1, yy_12_1_10_2, yy_12_1_10_3);
yy_12_1_11_1 = ((yy)"gen.y");
TellFile(yy_12_1_11_1);
yy_12_1_12_1 = ((yy)"%{");
s(yy_12_1_12_1);
nl();
Prelude();
yy_12_1_15_1 = ((yy)"/* start */");
s(yy_12_1_15_1);
nl();
yy_12_1_17 = yyglov_choice_Declarations;
if (yy_12_1_17 == (yy) yyu) yyErr(1,1832);
yyv_Decls = yy_12_1_17;
{
yy yysb = yyb;
yy_12_1_18_1_1_1 = yyv_Decls;
yy_12_1_18_1_1_2 = yy_12_1_18_1_1_1;
if (yy_12_1_18_1_1_2[0] != 2) goto yyfl_227_1_12_1_18_1;
goto yysl_227_1_12_1_18;
yyfl_227_1_12_1_18_1 : ;
ChoicePrelude();
goto yysl_227_1_12_1_18;
yysl_227_1_12_1_18 : ;
yyb = yysb;
}
yy_12_1_19_1 = ((yy)"/* end */");
s(yy_12_1_19_1);
nl();
yy_12_1_21_1 = yyv_ImpDecls;
ImportDeclarations_DECLARATIONLIST(yy_12_1_21_1);
yy_12_1_22_1 = yyv_D;
yy_12_1_22_2 = yyb + 2;
yy_12_1_22_2[0] = 1;
ForwardCode_DECLARATIONLIST(yy_12_1_22_1, yy_12_1_22_2);
DefYYSTYPE();
yy_12_1_24_1 = ((yy)"%}");
s(yy_12_1_24_1);
nl();
yy_12_1_26_1 = ((yy)"%start ROOT_");
s(yy_12_1_26_1);
nl();
yy_12_1_28 = ((yy)257);
yyglov_currentTokenCode = yy_12_1_28;
yy_12_1_29_1 = yyv_D;
yy_12_1_29_2 = ((yy)"%token ");
yy_12_1_29_3 = yyb + 3;
yy_12_1_29_3[0] = 1;
TokenCode_DECLARATIONLIST(yy_12_1_29_1, yy_12_1_29_2, yy_12_1_29_3);
yy_12_1_30_1 = yyv_L;
yy_12_1_30_2 = ((yy)"%token ");
CodeAnonTokenList(yy_12_1_30_1, yy_12_1_30_2);
/* --PATCH-- */ yy_12_1_31_1 = ((yy)"%glr-parser\n%%");
s(yy_12_1_31_1);
nl();
yy_12_1_33 = ((yy)0);
yyglov_currentProcNumber = yy_12_1_33;
yy_12_1_34_1 = yyv_D;
GrammarCode_DECLARATIONLIST(yy_12_1_34_1);
goto yysl_227_1_12;
yyfl_227_1_12_1 : ;
goto yysl_227_1_12;
yysl_227_1_12 : ;
yyb = yysb;
}
Told();
return;
yyfl_227_1 : ;
}
yyErr(2,1782);
}
TokenCode_DECLARATIONLIST(yyin_1, yyin_2, yyin_3)
yy yyin_1;
yy yyin_2;
yy yyin_3;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_T;
yy yy_0_1_2;
yy yyv_Prefix;
yy yy_0_2;
yy yyv_Flag;
yy yy_0_3;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yy_2_1;
yy yy_2_2;
yy yy_2_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 1) goto yyfl_228_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_H = yy_0_1_1;
yyv_T = yy_0_1_2;
yyv_Prefix = yy_0_2;
yyv_Flag = yy_0_3;
yy_1_1 = yyv_H;
yy_1_2 = yyv_Prefix;
yy_1_3 = yyv_Flag;
TokenCode_DECLARATION(yy_1_1, yy_1_2, yy_1_3);
yy_2_1 = yyv_T;
yy_2_2 = yyv_Prefix;
yy_2_3 = yyv_Flag;
TokenCode_DECLARATIONLIST(yy_2_1, yy_2_2, yy_2_3);
return;
yyfl_228_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Prefix;
yy yy_0_2;
yy yyv_Flag;
yy yy_0_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 2) goto yyfl_228_2;
yyv_Prefix = yy_0_2;
yyv_Flag = yy_0_3;
return;
yyfl_228_2 : ;
}
yyErr(2,1855);
}
GrammarCode_DECLARATIONLIST(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_T;
yy yy_0_1_2;
yy yyv_P;
yy yy_1;
yy yy_2;
yy yy_2_1;
yy yy_2_2;
yy yy_3_1;
yy yy_4_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_229_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_H = yy_0_1_1;
yyv_T = yy_0_1_2;
yy_1 = yyglov_currentProcNumber;
if (yy_1 == (yy) yyu) yyErr(1,1866);
yyv_P = yy_1;
yy_2_1 = yyv_P;
yy_2_2 = ((yy)1);
yy_2 = (yy)(((long)yy_2_1)+((long)yy_2_2));
yyglov_currentProcNumber = yy_2;
yy_3_1 = yyv_H;
GrammarCode_DECLARATION(yy_3_1);
yy_4_1 = yyv_T;
GrammarCode_DECLARATIONLIST(yy_4_1);
return;
yyfl_229_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_229_2;
return;
yyfl_229_2 : ;
}
yyErr(2,1863);
}
ForwardCode_DECLARATIONLIST(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_T;
yy yy_0_1_2;
yy yyv_Kind;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy yy_2_1;
yy yy_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_231_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_H = yy_0_1_1;
yyv_T = yy_0_1_2;
yyv_Kind = yy_0_2;
yy_1_1 = yyv_H;
yy_1_2 = yyv_Kind;
ForwardCode_DECLARATION(yy_1_1, yy_1_2);
yy_2_1 = yyv_T;
yy_2_2 = yyv_Kind;
ForwardCode_DECLARATIONLIST(yy_2_1, yy_2_2);
return;
yyfl_231_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Kind;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_231_2;
yyv_Kind = yy_0_2;
return;
yyfl_231_2 : ;
}
yyErr(2,1876);
}
Code_DECLARATIONLIST(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_T;
yy yy_0_1_2;
yy yyv_P;
yy yy_1;
yy yy_2;
yy yy_2_1;
yy yy_2_2;
yy yy_3_1;
yy yy_4_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_232_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_H = yy_0_1_1;
yyv_T = yy_0_1_2;
yy_1 = yyglov_currentProcNumber;
if (yy_1 == (yy) yyu) yyErr(1,1888);
yyv_P = yy_1;
yy_2_1 = yyv_P;
yy_2_2 = ((yy)1);
yy_2 = (yy)(((long)yy_2_1)+((long)yy_2_2));
yyglov_currentProcNumber = yy_2;
yy_3_1 = yyv_H;
Code_DECLARATION(yy_3_1);
yy_4_1 = yyv_T;
Code_DECLARATIONLIST(yy_4_1);
return;
yyfl_232_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_232_2;
return;
yyfl_232_2 : ;
}
yyErr(2,1885);
}
TokenCode_DECLARATION(yyin_1, yyin_2, yyin_3)
yy yyin_1;
yy yyin_2;
yy yyin_3;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Id;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_3_1;
yy yy_0_1_3_2;
yy yy_0_1_3_3;
yy yyv_InArgs;
yy yy_0_1_3_3_1;
yy yyv_OutArgs;
yy yy_0_1_3_3_2;
yy yyv_Rules;
yy yy_0_1_3_4;
yy yyv_Prefix;
yy yy_0_2;
yy yyv_Flag;
yy yy_0_3;
yy yy_1_1_1;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_2_1_1_1;
yy yy_2_1_1_2;
yy yy_2_2_1_1;
yy yy_2_2_1_2;
yy yy_3_1_1_1;
yy yy_3_1_1_2;
yy yy_3_2_1_1;
yy yy_3_2_1_2;
yy yy_4_1;
yy yy_4_2;
yy yy_5_1_1_1;
yy yy_5_1_1_2;
yy yyv_N;
yy yy_5_1_2;
yy yy_5_1_3_1;
yy yy_5_1_4_1;
yy yy_5_1_5_1;
yy yy_5_1_6_1;
yy yy_5_1_8;
yy yy_5_1_8_1;
yy yy_5_1_8_2;
yy yy_5_2_1_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 2) goto yyfl_233_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Id = yy_0_1_1;
yyv_P = yy_0_1_2;
if (yy_0_1_3[0] != 4) goto yyfl_233_1;
yy_0_1_3_1 = ((yy)yy_0_1_3[1]);
yy_0_1_3_2 = ((yy)yy_0_1_3[2]);
yy_0_1_3_3 = ((yy)yy_0_1_3[3]);
yy_0_1_3_4 = ((yy)yy_0_1_3[4]);
if (yy_0_1_3_2[0] != 4) goto yyfl_233_1;
if (yy_0_1_3_3[0] != 1) goto yyfl_233_1;
yy_0_1_3_3_1 = ((yy)yy_0_1_3_3[1]);
yy_0_1_3_3_2 = ((yy)yy_0_1_3_3[2]);
yyv_InArgs = yy_0_1_3_3_1;
yyv_OutArgs = yy_0_1_3_3_2;
yyv_Rules = yy_0_1_3_4;
yyv_Prefix = yy_0_2;
yyv_Flag = yy_0_3;
{
yy yysb = yyb;
yy_1_1_1 = yyglov_containsRoot;
if (yy_1_1_1 == (yy) yyu) yyErr(1,1899);
if (yy_1_1_1[0] != 1) goto yyfl_233_1_1_1;
goto yysl_233_1_1;
yyfl_233_1_1_1 : ;
yy_1_2_1_1 = ((yy)"missing ROOT specification");
yy_1_2_1_2 = yyv_P;
MESSAGE(yy_1_2_1_1, yy_1_2_1_2);
goto yysl_233_1_1;
yysl_233_1_1 : ;
yyb = yysb;
}
{
yy yysb = yyb;
yy_2_1_1_1 = yyv_InArgs;
yy_2_1_1_2 = yy_2_1_1_1;
if (yy_2_1_1_2[0] != 2) goto yyfl_233_1_2_1;
goto yysl_233_1_2;
yyfl_233_1_2_1 : ;
yy_2_2_1_1 = ((yy)"input arguments not allowed");
yy_2_2_1_2 = yyv_P;
MESSAGE(yy_2_2_1_1, yy_2_2_1_2);
goto yysl_233_1_2;
yysl_233_1_2 : ;
yyb = yysb;
}
{
yy yysb = yyb;
yy_3_1_1_1 = yyv_Rules;
yy_3_1_1_2 = yy_3_1_1_1;
if (yy_3_1_1_2[0] != 2) goto yyfl_233_1_3_1;
goto yysl_233_1_3;
yyfl_233_1_3_1 : ;
yy_3_2_1_1 = ((yy)"rules not allowed for tokens");
yy_3_2_1_2 = yyv_P;
MESSAGE(yy_3_2_1_1, yy_3_2_1_2);
goto yysl_233_1_3;
yysl_233_1_3 : ;
yyb = yysb;
}
yy_4_1 = yyv_InArgs;
yy_4_2 = yyv_OutArgs;
CheckFormalParams(yy_4_1, yy_4_2);
{
yy yysb = yyb;
yy_5_1_1_1 = yyv_Flag;
yy_5_1_1_2 = yy_5_1_1_1;
if (yy_5_1_1_2[0] != 1) goto yyfl_233_1_5_1;
yy_5_1_2 = yyglov_currentTokenCode;
if (yy_5_1_2 == (yy) yyu) yyErr(1,1907);
yyv_N = yy_5_1_2;
yy_5_1_3_1 = yyv_Prefix;
s(yy_5_1_3_1);
yy_5_1_4_1 = yyv_Id;
id(yy_5_1_4_1);
yy_5_1_5_1 = ((yy)" ");
s(yy_5_1_5_1);
yy_5_1_6_1 = yyv_N;
i(yy_5_1_6_1);
nl();
yy_5_1_8_1 = yyv_N;
yy_5_1_8_2 = ((yy)1);
yy_5_1_8 = (yy)(((long)yy_5_1_8_1)+((long)yy_5_1_8_2));
yyglov_currentTokenCode = yy_5_1_8;
goto yysl_233_1_5;
yyfl_233_1_5_1 : ;
yy_5_2_1_1 = yyv_Id;
id(yy_5_2_1_1);
nl();
goto yysl_233_1_5;
yysl_233_1_5 : ;
yyb = yysb;
}
return;
yyfl_233_1 : ;
}
{
yy yyb;
yy yyv_Other;
yy yy_0_1;
yy yyv_Prefix;
yy yy_0_2;
yy yyv_Flag;
yy yy_0_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Other = yy_0_1;
yyv_Prefix = yy_0_2;
yyv_Flag = yy_0_3;
return;
yyfl_233_2 : ;
}
yyErr(2,1894);
}
CodeAnonTokenList(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Index;
yy yy_0_1_1;
yy yyv_Tail;
yy yy_0_1_2;
yy yyv_Prefix;
yy yy_0_2;
yy yyv_Id;
yy yy_1;
yy yyv_N;
yy yy_2;
yy yy_3_1;
yy yy_4_1;
yy yy_5_1;
yy yy_6_1;
yy yy_8;
yy yy_8_1;
yy yy_8_2;
yy yy_9_1;
yy yy_9_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_234_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_Index = yy_0_1_1;
yyv_Tail = yy_0_1_2;
yyv_Prefix = yy_0_2;
yy_1 = (yy) yyv_Index[2];
if (yy_1 == (yy) yyu) yyErr(1,1918);
yyv_Id = yy_1;
yy_2 = yyglov_currentTokenCode;
if (yy_2 == (yy) yyu) yyErr(1,1919);
yyv_N = yy_2;
yy_3_1 = yyv_Prefix;
s(yy_3_1);
yy_4_1 = yyv_Id;
id(yy_4_1);
yy_5_1 = ((yy)" ");
s(yy_5_1);
yy_6_1 = yyv_N;
i(yy_6_1);
nl();
yy_8_1 = yyv_N;
yy_8_2 = ((yy)1);
yy_8 = (yy)(((long)yy_8_1)+((long)yy_8_2));
yyglov_currentTokenCode = yy_8;
yy_9_1 = yyv_Tail;
yy_9_2 = yyv_Prefix;
CodeAnonTokenList(yy_9_1, yy_9_2);
return;
yyfl_234_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Prefix;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_234_2;
yyv_Prefix = yy_0_2;
return;
yyfl_234_2 : ;
}
yyErr(2,1916);
}
GrammarCode_DECLARATION(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Id;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_3_1;
yy yy_0_1_3_2;
yy yy_0_1_3_3;
yy yyv_InArgs;
yy yy_0_1_3_3_1;
yy yyv_OutArgs;
yy yy_0_1_3_3_2;
yy yyv_Rules;
yy yy_0_1_3_4;
yy yy_1_1_1;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_2_1_1_1;
yy yy_2_1_1_2;
yy yy_2_2_1_1;
yy yy_2_2_1_2;
yy yy_3_1_1_1;
yy yy_3_1_1_2;
yy yy_3_1_2_1;
yy yy_3_1_2_2;
yy yy_4_1;
yy yy_4_2;
yy yy_5;
yy yy_6;
yy yy_7_1;
yy yy_7_2;
yy yy_7_3;
yy yy_7_4;
yy yy_7_5;
yy yy_7_6;
yy yyv_MayFail;
yy yy_7_7;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_235_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Id = yy_0_1_1;
yyv_P = yy_0_1_2;
if (yy_0_1_3[0] != 4) goto yyfl_235_1;
yy_0_1_3_1 = ((yy)yy_0_1_3[1]);
yy_0_1_3_2 = ((yy)yy_0_1_3[2]);
yy_0_1_3_3 = ((yy)yy_0_1_3[3]);
yy_0_1_3_4 = ((yy)yy_0_1_3[4]);
if (yy_0_1_3_2[0] != 3) goto yyfl_235_1;
if (yy_0_1_3_3[0] != 1) goto yyfl_235_1;
yy_0_1_3_3_1 = ((yy)yy_0_1_3_3[1]);
yy_0_1_3_3_2 = ((yy)yy_0_1_3_3[2]);
yyv_InArgs = yy_0_1_3_3_1;
yyv_OutArgs = yy_0_1_3_3_2;
yyv_Rules = yy_0_1_3_4;
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
{
yy yysb = yyb;
yy_1_1_1 = yyglov_containsRoot;
if (yy_1_1_1 == (yy) yyu) yyErr(1,1933);
if (yy_1_1_1[0] != 1) goto yyfl_235_1_1_1;
goto yysl_235_1_1;
yyfl_235_1_1_1 : ;
yy_1_2_1_1 = ((yy)"missing ROOT specification");
yy_1_2_1_2 = yyv_P;
MESSAGE(yy_1_2_1_1, yy_1_2_1_2);
goto yysl_235_1_1;
yysl_235_1_1 : ;
yyb = yysb;
}
{
yy yysb = yyb;
yy_2_1_1_1 = yyv_InArgs;
yy_2_1_1_2 = yy_2_1_1_1;
if (yy_2_1_1_2[0] != 2) goto yyfl_235_1_2_1;
goto yysl_235_1_2;
yyfl_235_1_2_1 : ;
yy_2_2_1_1 = ((yy)"input arguments not allowed");
yy_2_2_1_2 = yyv_P;
MESSAGE(yy_2_2_1_1, yy_2_2_1_2);
goto yysl_235_1_2;
yysl_235_1_2 : ;
yyb = yysb;
}
{
yy yysb = yyb;
yy_3_1_1_1 = yyv_Rules;
yy_3_1_1_2 = yy_3_1_1_1;
if (yy_3_1_1_2[0] != 2) goto yyfl_235_1_3_1;
yy_3_1_2_1 = ((yy)"missing rules for nonterminal");
yy_3_1_2_2 = yyv_P;
MESSAGE(yy_3_1_2_1, yy_3_1_2_2);
goto yysl_235_1_3;
yyfl_235_1_3_1 : ;
goto yysl_235_1_3;
yysl_235_1_3 : ;
yyb = yysb;
}
yy_4_1 = yyv_InArgs;
yy_4_2 = yyv_OutArgs;
CheckFormalParams(yy_4_1, yy_4_2);
yy_5 = yyb + 0;
yy_5[0] = 2;
yyglov_currentFailLabel = yy_5;
yy_6 = yyb + 1;
yy_6[0] = 2;
yyglov_currentFailLabelUsed = yy_6;
yy_7_1 = yyb + 2;
yy_7_1[0] = 3;
yy_7_2 = yyv_Id;
yy_7_3 = yyv_InArgs;
yy_7_4 = yyv_OutArgs;
yy_7_5 = yyv_Rules;
yy_7_6 = ((yy)1);
Code_Rules(yy_7_1, yy_7_2, yy_7_3, yy_7_4, yy_7_5, yy_7_6, &yy_7_7);
yyv_MayFail = yy_7_7;
return;
yyfl_235_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_P;
yy yy_0_1_1;
yy yyv_M;
yy yy_0_1_2;
yy yy_1_1;
yy yyv_RootId;
yy yy_1_2;
yy yy_2_1;
yy yy_2_1_1;
yy yy_2_1_2;
yy yy_2_1_3;
yy yy_2_1_3_1;
yy yy_2_1_3_2;
yy yy_2_1_3_3;
yy yy_2_1_3_3_1;
yy yy_2_1_3_3_2;
yy yy_2_1_3_4;
yy yy_2_1_3_4_1;
yy yy_2_1_3_4_1_1;
yy yy_2_1_3_4_1_1_1;
yy yy_2_1_3_4_1_1_2;
yy yy_2_1_3_4_1_1_3;
yy yy_2_1_3_4_1_1_3_1;
yy yy_2_1_3_4_1_1_4;
yy yy_2_1_3_4_1_1_4_1;
yy yy_2_1_3_4_1_2;
yy yy_2_1_3_4_1_3;
yy yy_2_1_3_4_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_235_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_P = yy_0_1_1;
yyv_M = yy_0_1_2;
yyb = yyh;
yyh += 33; if (yyh > yyhx) yyExtend();
yy_1_1 = ((yy)"ROOT_");
string_to_id(yy_1_1, &yy_1_2);
yyv_RootId = yy_1_2;
yy_2_1_1 = yyv_RootId;
yy_2_1_2 = yyv_P;
yy_2_1_3_1 = yyb + 9;
yy_2_1_3_1[0] = 3;
yy_2_1_3_2 = yyb + 10;
yy_2_1_3_2[0] = 3;
yy_2_1_3_3_1 = yyb + 14;
yy_2_1_3_3_1[0] = 2;
yy_2_1_3_3_2 = yyb + 15;
yy_2_1_3_3_2[0] = 2;
yy_2_1_3_3 = yyb + 11;
yy_2_1_3_3[0] = 1;
yy_2_1_3_3[1] = ((long)yy_2_1_3_3_1);
yy_2_1_3_3[2] = ((long)yy_2_1_3_3_2);
yy_2_1_3_4_1_1_1 = yyv_RootId;
yy_2_1_3_4_1_1_2 = yyv_P;
yy_2_1_3_4_1_1_3_1 = yyv_P;
yy_2_1_3_4_1_1_3 = yyb + 28;
yy_2_1_3_4_1_1_3[0] = 2;
yy_2_1_3_4_1_1_3[1] = ((long)yy_2_1_3_4_1_1_3_1);
yy_2_1_3_4_1_1_4_1 = yyv_P;
yy_2_1_3_4_1_1_4 = yyb + 30;
yy_2_1_3_4_1_1_4[0] = 2;
yy_2_1_3_4_1_1_4[1] = ((long)yy_2_1_3_4_1_1_4_1);
yy_2_1_3_4_1_1 = yyb + 23;
yy_2_1_3_4_1_1[0] = 1;
yy_2_1_3_4_1_1[1] = ((long)yy_2_1_3_4_1_1_1);
yy_2_1_3_4_1_1[2] = ((long)yy_2_1_3_4_1_1_2);
yy_2_1_3_4_1_1[3] = ((long)yy_2_1_3_4_1_1_3);
yy_2_1_3_4_1_1[4] = ((long)yy_2_1_3_4_1_1_4);
yy_2_1_3_4_1_2 = yyv_M;
yy_2_1_3_4_1_3 = ((yy)0);
yy_2_1_3_4_1 = yyb + 19;
yy_2_1_3_4_1[0] = 1;
yy_2_1_3_4_1[1] = ((long)yy_2_1_3_4_1_1);
yy_2_1_3_4_1[2] = ((long)yy_2_1_3_4_1_2);
yy_2_1_3_4_1[3] = ((long)yy_2_1_3_4_1_3);
yy_2_1_3_4_2 = yyb + 32;
yy_2_1_3_4_2[0] = 2;
yy_2_1_3_4 = yyb + 16;
yy_2_1_3_4[0] = 1;
yy_2_1_3_4[1] = ((long)yy_2_1_3_4_1);
yy_2_1_3_4[2] = ((long)yy_2_1_3_4_2);
yy_2_1_3 = yyb + 4;
yy_2_1_3[0] = 4;
yy_2_1_3[1] = ((long)yy_2_1_3_1);
yy_2_1_3[2] = ((long)yy_2_1_3_2);
yy_2_1_3[3] = ((long)yy_2_1_3_3);
yy_2_1_3[4] = ((long)yy_2_1_3_4);
yy_2_1 = yyb + 0;
yy_2_1[0] = 2;
yy_2_1[1] = ((long)yy_2_1_1);
yy_2_1[2] = ((long)yy_2_1_2);
yy_2_1[3] = ((long)yy_2_1_3);
GrammarCode_DECLARATION(yy_2_1);
return;
yyfl_235_2 : ;
}
{
yy yyb;
yy yyv_Other;
yy yy_0_1;
yy_0_1 = yyin_1;
yyv_Other = yy_0_1;
return;
yyfl_235_3 : ;
}
yyErr(2,1927);
}
ForwardCode_DECLARATION(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Id;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_Type;
yy yy_0_1_3_1;
yy yyv_P2;
yy yy_0_1_3_2;
yy yy_0_2;
yy yy_1_1;
yy yy_2_1;
yy yy_3_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_236_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Id = yy_0_1_1;
yyv_P = yy_0_1_2;
if (yy_0_1_3[0] != 2) goto yyfl_236_1;
yy_0_1_3_1 = ((yy)yy_0_1_3[1]);
yy_0_1_3_2 = ((yy)yy_0_1_3[2]);
yyv_Type = yy_0_1_3_1;
yyv_P2 = yy_0_1_3_2;
if (yy_0_2[0] != 2) goto yyfl_236_1;
yy_1_1 = ((yy)"yy ");
s(yy_1_1);
yy_2_1 = yyv_Id;
glovarid(yy_2_1);
yy_3_1 = ((yy)" = (yy) yyu;");
s(yy_3_1);
nl();
return;
yyfl_236_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Id;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_Type;
yy yy_0_1_3_1;
yy yyv_P2;
yy yy_0_1_3_2;
yy yy_0_2;
yy yy_1_1;
yy yy_2_1;
yy yy_3_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_236_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Id = yy_0_1_1;
yyv_P = yy_0_1_2;
if (yy_0_1_3[0] != 2) goto yyfl_236_2;
yy_0_1_3_1 = ((yy)yy_0_1_3[1]);
yy_0_1_3_2 = ((yy)yy_0_1_3[2]);
yyv_Type = yy_0_1_3_1;
yyv_P2 = yy_0_1_3_2;
if (yy_0_2[0] != 1) goto yyfl_236_2;
yy_1_1 = ((yy)"extern yy ");
s(yy_1_1);
yy_2_1 = yyv_Id;
glovarid(yy_2_1);
yy_3_1 = ((yy)";");
s(yy_3_1);
nl();
return;
yyfl_236_2 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Id;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_Attr;
yy yy_0_1_3_1;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_236_3;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Id = yy_0_1_1;
yyv_P = yy_0_1_2;
if (yy_0_1_3[0] != 3) goto yyfl_236_3;
yy_0_1_3_1 = ((yy)yy_0_1_3[1]);
yyv_Attr = yy_0_1_3_1;
if (yy_0_2[0] != 1) goto yyfl_236_3;
return;
yyfl_236_3 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Id;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_Attr;
yy yy_0_1_3_1;
yy yy_0_2;
yy yy_1_1;
yy yy_2_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_236_4;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Id = yy_0_1_1;
yyv_P = yy_0_1_2;
if (yy_0_1_3[0] != 3) goto yyfl_236_4;
yy_0_1_3_1 = ((yy)yy_0_1_3[1]);
yyv_Attr = yy_0_1_3_1;
if (yy_0_2[0] != 2) goto yyfl_236_4;
yy_1_1 = yyv_Id;
Code_TableEQRoutines(yy_1_1);
yy_2_1 = yyv_Id;
Code_TablePrintRoutine(yy_2_1);
return;
yyfl_236_4 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Id;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_3_1;
yy yyv_F;
yy yy_0_1_3_2;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_236_5;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Id = yy_0_1_1;
yyv_P = yy_0_1_2;
if (yy_0_1_3[0] != 1) goto yyfl_236_5;
yy_0_1_3_1 = ((yy)yy_0_1_3[1]);
yy_0_1_3_2 = ((yy)yy_0_1_3[2]);
yyv_F = yy_0_1_3_2;
if (yy_0_2[0] != 1) goto yyfl_236_5;
return;
yyfl_236_5 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Id;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_Kind;
yy yy_0_1_3_1;
yy yyv_F;
yy yy_0_1_3_2;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yy_2_1;
yy yy_2_2;
yy yy_2_3;
yy yy_3_1;
yy yy_3_2;
yy yy_3_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_236_6;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Id = yy_0_1_1;
yyv_P = yy_0_1_2;
if (yy_0_1_3[0] != 1) goto yyfl_236_6;
yy_0_1_3_1 = ((yy)yy_0_1_3[1]);
yy_0_1_3_2 = ((yy)yy_0_1_3[2]);
yyv_Kind = yy_0_1_3_1;
yyv_F = yy_0_1_3_2;
if (yy_0_2[0] != 2) goto yyfl_236_6;
yy_1_1 = yyv_Id;
yy_1_2 = yyv_Kind;
yy_1_3 = yyv_F;
Code_EQRoutines(yy_1_1, yy_1_2, yy_1_3);
yy_2_1 = yyv_Id;
yy_2_2 = yyv_Kind;
yy_2_3 = yyv_F;
Code_PrintRoutine(yy_2_1, yy_2_2, yy_2_3);
yy_3_1 = yyv_Id;
yy_3_2 = yyv_Kind;
yy_3_3 = yyv_F;
Code_BroadcastRoutine(yy_3_1, yy_3_2, yy_3_3);
return;
yyfl_236_6 : ;
}
{
yy yyb;
yy yyv_Decl;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_Id;
yy yy_0_1_1_1;
yy yyv_P;
yy yy_0_1_1_2;
yy yy_0_1_1_3;
yy yyv_Name;
yy yy_0_1_1_3_1;
yy yy_0_1_1_3_2;
yy yy_0_1_1_3_3;
yy yyv_InArgs;
yy yy_0_1_1_3_3_1;
yy yyv_OutArgs;
yy yy_0_1_1_3_3_2;
yy yyv_Rules;
yy yy_0_1_1_3_4;
yy yy_0_2;
yy yyv_Old;
yy yy_1;
yy yy_2;
yy yy_2_1;
yy yy_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_1_1 = yy_0_1;
yyv_Decl = yy_0_1;
if (yy_0_1_1[0] != 2) goto yyfl_236_7;
yy_0_1_1_1 = ((yy)yy_0_1_1[1]);
yy_0_1_1_2 = ((yy)yy_0_1_1[2]);
yy_0_1_1_3 = ((yy)yy_0_1_1[3]);
yyv_Id = yy_0_1_1_1;
yyv_P = yy_0_1_1_2;
if (yy_0_1_1_3[0] != 4) goto yyfl_236_7;
yy_0_1_1_3_1 = ((yy)yy_0_1_1_3[1]);
yy_0_1_1_3_2 = ((yy)yy_0_1_1_3[2]);
yy_0_1_1_3_3 = ((yy)yy_0_1_1_3[3]);
yy_0_1_1_3_4 = ((yy)yy_0_1_1_3[4]);
yyv_Name = yy_0_1_1_3_1;
if (yy_0_1_1_3_2[0] != 5) goto yyfl_236_7;
if (yy_0_1_1_3_3[0] != 1) goto yyfl_236_7;
yy_0_1_1_3_3_1 = ((yy)yy_0_1_1_3_3[1]);
yy_0_1_1_3_3_2 = ((yy)yy_0_1_1_3_3[2]);
yyv_InArgs = yy_0_1_1_3_3_1;
yyv_OutArgs = yy_0_1_1_3_3_2;
yyv_Rules = yy_0_1_1_3_4;
if (yy_0_2[0] != 2) goto yyfl_236_7;
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yy_1 = yyglov_choice_Declarations;
if (yy_1 == (yy) yyu) yyErr(1,1980);
yyv_Old = yy_1;
yy_2_1 = yyv_Decl;
yy_2_2 = yyv_Old;
yy_2 = yyb + 0;
yy_2[0] = 1;
yy_2[1] = ((long)yy_2_1);
yy_2[2] = ((long)yy_2_2);
yyglov_choice_Declarations = yy_2;
return;
yyfl_236_7 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Id;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_Assoc;
yy yy_0_1_3;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_236_8;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Id = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_Assoc = yy_0_1_3;
return;
yyfl_236_8 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_P;
yy yy_0_1_1;
yy yyv_M;
yy yy_0_1_2;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_236_9;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_P = yy_0_1_1;
yyv_M = yy_0_1_2;
return;
yyfl_236_9 : ;
}
yyErr(2,1953);
}
Code_TableEQRoutines(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yy_1_1;
yy yy_2_1;
yy yy_3_1;
yy yy_5_1;
yy yy_7_1;
yy yy_9_1;
yy_0_1 = yyin_1;
yyv_Id = yy_0_1;
yy_1_1 = ((yy)"yyeq_");
s(yy_1_1);
yy_2_1 = yyv_Id;
id(yy_2_1);
yy_3_1 = ((yy)"(t1, t2) yy t1, t2;");
s(yy_3_1);
nl();
yy_5_1 = ((yy)"{");
s(yy_5_1);
nl();
yy_7_1 = ((yy)"return t1 == t2;");
s(yy_7_1);
nl();
yy_9_1 = ((yy)"}");
s(yy_9_1);
nl();
return;
}
}
Code_TablePrintRoutine(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yy_1_1;
yy yy_2_1;
yy yy_3_1;
yy yy_5_1;
yy yy_7_1;
yy yy_9_1;
yy_0_1 = yyin_1;
yyv_Id = yy_0_1;
yy_1_1 = ((yy)"yyPrint_");
s(yy_1_1);
yy_2_1 = yyv_Id;
id(yy_2_1);
yy_3_1 = ((yy)"(t) yy t;");
s(yy_3_1);
nl();
yy_5_1 = ((yy)"{");
s(yy_5_1);
nl();
yy_7_1 = ((yy)"yyPrintIndex(t);");
s(yy_7_1);
nl();
/* --PATCH-- */ yy_9_1 = ((yy)"return 0;\n}");
s(yy_9_1);
nl();
return;
}
}
Code_EQRoutines(yyin_1, yyin_2, yyin_3)
yy yyin_1;
yy yyin_2;
yy yyin_3;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yyv_Kind;
yy yy_0_2;
yy yyv_SpecList;
yy yy_0_3;
yy yy_1_1;
yy yy_2_1;
yy yy_3_1;
yy yy_5_1;
yy yy_7_1_1_1_1_1;
yy yy_7_1_1_1_1_2;
yy yy_7_1_1_2_1_1;
yy yy_7_1_1_2_1_2;
yy yy_7_1_2_1;
yy yy_7_2_1_1;
yy yy_7_2_3_1;
yy yy_7_2_3_2;
yy yy_7_2_4_1;
yy yy_8_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Id = yy_0_1;
yyv_Kind = yy_0_2;
yyv_SpecList = yy_0_3;
yy_1_1 = ((yy)"yyeq_");
s(yy_1_1);
yy_2_1 = yyv_Id;
id(yy_2_1);
yy_3_1 = ((yy)"(t1, t2) yy t1, t2;");
s(yy_3_1);
nl();
yy_5_1 = ((yy)"{");
s(yy_5_1);
nl();
{
yy yysb = yyb;
{
yy yysb = yyb;
yy_7_1_1_1_1_1 = yyv_SpecList;
yy_7_1_1_1_1_2 = yy_7_1_1_1_1_1;
if (yy_7_1_1_1_1_2[0] != 2) goto yyfl_239_1_7_1_1_1;
goto yysl_239_1_7_1_1;
yyfl_239_1_7_1_1_1 : ;
yy_7_1_1_2_1_1 = yyv_Kind;
yy_7_1_1_2_1_2 = yy_7_1_1_2_1_1;
if (yy_7_1_1_2_1_2[0] != 2) goto yyfl_239_1_7_1_1_2;
goto yysl_239_1_7_1_1;
yyfl_239_1_7_1_1_2 : ;
goto yyfl_239_1_7_1;
yysl_239_1_7_1_1 : ;
yyb = yysb;
}
yy_7_1_2_1 = ((yy)"return t1 == t2;");
s(yy_7_1_2_1);
nl();
goto yysl_239_1_7;
yyfl_239_1_7_1 : ;
yy_7_2_1_1 = ((yy)"switch(t1[0]) {");
s(yy_7_2_1_1);
nl();
yy_7_2_3_1 = yyv_SpecList;
yy_7_2_3_2 = ((yy)1);
Code_EQCases(yy_7_2_3_1, yy_7_2_3_2);
yy_7_2_4_1 = ((yy)"}");
s(yy_7_2_4_1);
nl();
goto yysl_239_1_7;
yysl_239_1_7 : ;
yyb = yysb;
}
/* --PATCH-- */ yy_8_1 = ((yy)"return 0;\n}");
s(yy_8_1);
nl();
return;
}
}
Code_PrintRoutine(yyin_1, yyin_2, yyin_3)
yy yyin_1;
yy yyin_2;
yy yyin_3;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yyv_Kind;
yy yy_0_2;
yy yyv_SpecList;
yy yy_0_3;
yy yy_1_1;
yy yy_2_1;
yy yy_3_1;
yy yy_5_1;
yy yy_7_1_1_1_1_1;
yy yy_7_1_1_1_1_2;
yy yy_7_1_1_2_1_1;
yy yy_7_1_1_2_1_2;
yy yy_7_1_2_1;
yy yy_7_2_1_1;
yy yy_7_2_3_1;
yy yy_7_2_3_2;
yy yy_7_2_4_1;
yy yy_8_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Id = yy_0_1;
yyv_Kind = yy_0_2;
yyv_SpecList = yy_0_3;
yy_1_1 = ((yy)"yyPrint_");
s(yy_1_1);
yy_2_1 = yyv_Id;
id(yy_2_1);
yy_3_1 = ((yy)"(t) yy t;");
s(yy_3_1);
nl();
yy_5_1 = ((yy)"{");
s(yy_5_1);
nl();
{
yy yysb = yyb;
{
yy yysb = yyb;
yy_7_1_1_1_1_1 = yyv_SpecList;
yy_7_1_1_1_1_2 = yy_7_1_1_1_1_1;
if (yy_7_1_1_1_1_2[0] != 2) goto yyfl_240_1_7_1_1_1;
goto yysl_240_1_7_1_1;
yyfl_240_1_7_1_1_1 : ;
yy_7_1_1_2_1_1 = yyv_Kind;
yy_7_1_1_2_1_2 = yy_7_1_1_2_1_1;
if (yy_7_1_1_2_1_2[0] != 2) goto yyfl_240_1_7_1_1_2;
goto yysl_240_1_7_1_1;
yyfl_240_1_7_1_1_2 : ;
goto yyfl_240_1_7_1;
yysl_240_1_7_1_1 : ;
yyb = yysb;
}
yy_7_1_2_1 = ((yy)"yyPrintOpaque(t);");
s(yy_7_1_2_1);
nl();
goto yysl_240_1_7;
yyfl_240_1_7_1 : ;
yy_7_2_1_1 = ((yy)"switch(t[0]) {");
s(yy_7_2_1_1);
nl();
yy_7_2_3_1 = yyv_SpecList;
yy_7_2_3_2 = ((yy)1);
Code_PrintCases(yy_7_2_3_1, yy_7_2_3_2);
yy_7_2_4_1 = ((yy)"}");
s(yy_7_2_4_1);
nl();
goto yysl_240_1_7;
yysl_240_1_7 : ;
yyb = yysb;
}
/* --PATCH-- */ yy_8_1 = ((yy)"return 0;\n}");
s(yy_8_1);
nl();
return;
}
}
Code_BroadcastRoutine(yyin_1, yyin_2, yyin_3)
yy yyin_1;
yy yyin_2;
yy yyin_3;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yyv_Kind;
yy yy_0_2;
yy yyv_SpecList;
yy yy_0_3;
yy yy_1_1;
yy yy_2_1;
yy yy_3_1;
yy yy_5_1;
yy yy_7_1;
yy yy_9_1;
yy yy_11_1;
yy yy_13_1;
yy yy_14_1;
yy yy_15_1;
yy yy_17_1_1_1_1_1;
yy yy_17_1_1_1_1_2;
yy yy_17_1_1_2_1_1;
yy yy_17_1_1_2_1_2;
yy yy_17_1_2_1;
yy yy_17_2_1_1;
yy yy_17_2_3_1;
yy yy_17_2_3_2;
yy yy_17_2_4_1;
yy yy_18_1;
yy yy_20_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Id = yy_0_1;
yyv_Kind = yy_0_2;
yyv_SpecList = yy_0_3;
yy_1_1 = ((yy)"yybroadcast_");
s(yy_1_1);
yy_2_1 = yyv_Id;
id(yy_2_1);
yy_3_1 = ((yy)"(t,In,Out,Handler)");
s(yy_3_1);
nl();
yy_5_1 = ((yy)"yy t, In, *Out; int (*Handler) ();");
s(yy_5_1);
nl();
yy_7_1 = ((yy)"{");
s(yy_7_1);
nl();
yy_9_1 = ((yy)"yy A, B;");
s(yy_9_1);
nl();
yy_11_1 = ((yy)"A = In;");
s(yy_11_1);
nl();
yy_13_1 = ((yy)"if (! Handler(yybroadcast_");
s(yy_13_1);
yy_14_1 = yyv_Id;
id(yy_14_1);
yy_15_1 = ((yy)", t, In, Out)) {");
s(yy_15_1);
nl();
{
yy yysb = yyb;
{
yy yysb = yyb;
yy_17_1_1_1_1_1 = yyv_SpecList;
yy_17_1_1_1_1_2 = yy_17_1_1_1_1_1;
if (yy_17_1_1_1_1_2[0] != 2) goto yyfl_241_1_17_1_1_1;
goto yysl_241_1_17_1_1;
yyfl_241_1_17_1_1_1 : ;
yy_17_1_1_2_1_1 = yyv_Kind;
yy_17_1_1_2_1_2 = yy_17_1_1_2_1_1;
if (yy_17_1_1_2_1_2[0] != 2) goto yyfl_241_1_17_1_1_2;
goto yysl_241_1_17_1_1;
yyfl_241_1_17_1_1_2 : ;
goto yyfl_241_1_17_1;
yysl_241_1_17_1_1 : ;
yyb = yysb;
}
yy_17_1_2_1 = ((yy)"*Out = In;");
s(yy_17_1_2_1);
goto yysl_241_1_17;
yyfl_241_1_17_1 : ;
yy_17_2_1_1 = ((yy)"switch(t[0]) {");
s(yy_17_2_1_1);
nl();
yy_17_2_3_1 = yyv_SpecList;
yy_17_2_3_2 = ((yy)1);
Code_BroadcastCases(yy_17_2_3_1, yy_17_2_3_2);
yy_17_2_4_1 = ((yy)"}");
s(yy_17_2_4_1);
nl();
goto yysl_241_1_17;
yysl_241_1_17 : ;
yyb = yysb;
}
yy_18_1 = ((yy)"}");
s(yy_18_1);
nl();
/* --PATCH-- */yy_20_1 = ((yy)"return 0;\n}");
s(yy_20_1);
nl();
return;
}
}
Code_EQCases(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Head;
yy yy_0_1_1;
yy yyv_Tail;
yy yy_0_1_2;
yy yyv_N;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy yyv_Name;
yy yy_1_2_1;
yy yyv_Pos;
yy yy_1_2_2;
yy yyv_Fields;
yy yy_1_2_3;
yy yy_2_1;
yy yy_3_1;
yy yy_4_1;
yy yy_5_1;
yy yy_6_1;
yy yy_8_1;
yy yy_8_2;
yy yy_9_1;
yy yy_11_1;
yy yy_11_2;
yy yy_11_2_1;
yy yy_11_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_242_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_Head = yy_0_1_1;
yyv_Tail = yy_0_1_2;
yyv_N = yy_0_2;
yy_1_1 = yyv_Head;
yy_1_2 = yy_1_1;
if (yy_1_2[0] != 1) goto yyfl_242_1;
yy_1_2_1 = ((yy)yy_1_2[1]);
yy_1_2_2 = ((yy)yy_1_2[2]);
yy_1_2_3 = ((yy)yy_1_2[3]);
yyv_Name = yy_1_2_1;
yyv_Pos = yy_1_2_2;
yyv_Fields = yy_1_2_3;
yy_2_1 = ((yy)"case ");
s(yy_2_1);
yy_3_1 = yyv_N;
i(yy_3_1);
yy_4_1 = ((yy)": return (t2[0] == ");
s(yy_4_1);
yy_5_1 = yyv_N;
i(yy_5_1);
yy_6_1 = ((yy)")");
s(yy_6_1);
nl();
yy_8_1 = yyv_Fields;
yy_8_2 = ((yy)1);
Code_EQFields(yy_8_1, yy_8_2);
yy_9_1 = ((yy)";");
s(yy_9_1);
nl();
yy_11_1 = yyv_Tail;
yy_11_2_1 = yyv_N;
yy_11_2_2 = ((yy)1);
yy_11_2 = (yy)(((long)yy_11_2_1)+((long)yy_11_2_2));
Code_EQCases(yy_11_1, yy_11_2);
return;
yyfl_242_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_N;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_242_2;
yyv_N = yy_0_2;
return;
yyfl_242_2 : ;
}
yyErr(2,2060);
}
Code_PrintCases(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Head;
yy yy_0_1_1;
yy yyv_Tail;
yy yy_0_1_2;
yy yyv_N;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy yyv_Name;
yy yy_1_2_1;
yy yyv_Pos;
yy yy_1_2_2;
yy yyv_Fields;
yy yy_1_2_3;
yy yy_2_1;
yy yy_3_1;
yy yy_4_1;
yy yy_6_1;
yy yy_8_1;
yy yy_10_1;
yy yy_12_1_1_1;
yy yy_12_1_1_2;
yy yy_12_1_2_1;
yy yy_12_2_1_1;
yy yy_12_2_1_2;
yy yy_13_1;
yy yy_15_1;
yy yy_15_2;
yy yy_15_2_1;
yy yy_15_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_243_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_Head = yy_0_1_1;
yyv_Tail = yy_0_1_2;
yyv_N = yy_0_2;
yy_1_1 = yyv_Head;
yy_1_2 = yy_1_1;
if (yy_1_2[0] != 1) goto yyfl_243_1;
yy_1_2_1 = ((yy)yy_1_2[1]);
yy_1_2_2 = ((yy)yy_1_2[2]);
yy_1_2_3 = ((yy)yy_1_2[3]);
yyv_Name = yy_1_2_1;
yyv_Pos = yy_1_2_2;
yyv_Fields = yy_1_2_3;
yy_2_1 = ((yy)"case ");
s(yy_2_1);
yy_3_1 = yyv_N;
i(yy_3_1);
yy_4_1 = ((yy)": ");
s(yy_4_1);
nl();
yy_6_1 = ((yy)"yyTerm(");
s(yy_6_1);
doublequote();
yy_8_1 = yyv_Name;
id(yy_8_1);
doublequote();
yy_10_1 = ((yy)");");
s(yy_10_1);
nl();
{
yy yysb = yyb;
yy_12_1_1_1 = yyv_Fields;
yy_12_1_1_2 = yy_12_1_1_1;
if (yy_12_1_1_2[0] != 2) goto yyfl_243_1_12_1;
yy_12_1_2_1 = ((yy)"yyNoArgs();");
s(yy_12_1_2_1);
nl();
goto yysl_243_1_12;
yyfl_243_1_12_1 : ;
yy_12_2_1_1 = yyv_Fields;
yy_12_2_1_2 = ((yy)1);
Code_PrintFields(yy_12_2_1_1, yy_12_2_1_2);
goto yysl_243_1_12;
yysl_243_1_12 : ;
yyb = yysb;
}
yy_13_1 = ((yy)"break;");
s(yy_13_1);
nl();
yy_15_1 = yyv_Tail;
yy_15_2_1 = yyv_N;
yy_15_2_2 = ((yy)1);
yy_15_2 = (yy)(((long)yy_15_2_1)+((long)yy_15_2_2));
Code_PrintCases(yy_15_1, yy_15_2);
return;
yyfl_243_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_N;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_243_2;
yyv_N = yy_0_2;
return;
yyfl_243_2 : ;
}
yyErr(2,2074);
}
Code_BroadcastCases(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Head;
yy yy_0_1_1;
yy yyv_Tail;
yy yy_0_1_2;
yy yyv_N;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy yyv_Name;
yy yy_1_2_1;
yy yyv_Pos;
yy yy_1_2_2;
yy yyv_Fields;
yy yy_1_2_3;
yy yy_2_1;
yy yy_3_1;
yy yy_4_1;
yy yy_6_1;
yy yy_6_2;
yy yy_6_3;
yy yy_6_4;
yy yy_7_1;
yy yy_9_1;
yy yy_9_2;
yy yy_9_2_1;
yy yy_9_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_244_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_Head = yy_0_1_1;
yyv_Tail = yy_0_1_2;
yyv_N = yy_0_2;
yy_1_1 = yyv_Head;
yy_1_2 = yy_1_1;
if (yy_1_2[0] != 1) goto yyfl_244_1;
yy_1_2_1 = ((yy)yy_1_2[1]);
yy_1_2_2 = ((yy)yy_1_2[2]);
yy_1_2_3 = ((yy)yy_1_2[3]);
yyv_Name = yy_1_2_1;
yyv_Pos = yy_1_2_2;
yyv_Fields = yy_1_2_3;
yy_2_1 = ((yy)"case ");
s(yy_2_1);
yy_3_1 = yyv_N;
i(yy_3_1);
yy_4_1 = ((yy)": ");
s(yy_4_1);
nl();
yy_6_1 = yyv_Fields;
yy_6_2 = ((yy)1);
yy_6_3 = ((yy)"A");
yy_6_4 = ((yy)"B");
Code_BroadcastFields(yy_6_1, yy_6_2, yy_6_3, yy_6_4);
yy_7_1 = ((yy)"break;");
s(yy_7_1);
nl();
yy_9_1 = yyv_Tail;
yy_9_2_1 = yyv_N;
yy_9_2_2 = ((yy)1);
yy_9_2 = (yy)(((long)yy_9_2_1)+((long)yy_9_2_2));
Code_BroadcastCases(yy_9_1, yy_9_2);
return;
yyfl_244_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_N;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_244_2;
yyv_N = yy_0_2;
return;
yyfl_244_2 : ;
}
yyErr(2,2093);
}
Code_EQFields(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Head;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_Tail;
yy yy_0_1_3;
yy yyv_N;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy yy_1_2_1;
yy yyv_Type;
yy yy_1_2_2;
yy yy_1_2_3;
yy yy_2_1_1_1_1_1;
yy yy_2_1_1_1_1_2;
yy yy_2_1_1_2_1_1;
yy yy_2_1_1_2_1_2;
yy yy_2_1_2_1;
yy yy_2_1_3_1;
yy yy_2_1_4_1;
yy yy_2_1_5_1;
yy yy_2_1_6_1;
yy yy_2_2_1_1;
yy yy_2_2_1_2;
yy yy_2_2_2_1;
yy yy_2_2_3_1;
yy yy_2_2_4_1;
yy yy_2_2_5_1;
yy yy_2_2_6_1;
yy yy_2_3_1_1;
yy yy_2_3_2_1;
yy yy_2_3_3_1;
yy yy_2_3_4_1;
yy yy_2_3_5_1;
yy yy_2_3_6_1;
yy yy_2_3_7_1;
yy yy_3_1;
yy yy_3_2;
yy yy_3_2_1;
yy yy_3_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_245_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Head = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_Tail = yy_0_1_3;
yyv_N = yy_0_2;
yy_1_1 = yyv_Head;
yy_1_2 = yy_1_1;
if (yy_1_2[0] != 1) goto yyfl_245_1;
yy_1_2_1 = ((yy)yy_1_2[1]);
yy_1_2_2 = ((yy)yy_1_2[2]);
yy_1_2_3 = ((yy)yy_1_2[3]);
yyv_Type = yy_1_2_2;
{
yy yysb = yyb;
{
yy yysb = yyb;
yy_2_1_1_1_1_1 = yyv_Type;
yy_2_1_1_1_1_2 = ((yy)"INT");
if (! IsIdent(yy_2_1_1_1_1_1, yy_2_1_1_1_1_2)) goto yyfl_245_1_2_1_1_1;
goto yysl_245_1_2_1_1;
yyfl_245_1_2_1_1_1 : ;
yy_2_1_1_2_1_1 = yyv_Type;
yy_2_1_1_2_1_2 = ((yy)"POS");
if (! IsIdent(yy_2_1_1_2_1_1, yy_2_1_1_2_1_2)) goto yyfl_245_1_2_1_1_2;
goto yysl_245_1_2_1_1;
yyfl_245_1_2_1_1_2 : ;
goto yyfl_245_1_2_1;
yysl_245_1_2_1_1 : ;
yyb = yysb;
}
yy_2_1_2_1 = ((yy)"&& (t1[");
s(yy_2_1_2_1);
yy_2_1_3_1 = yyv_N;
i(yy_2_1_3_1);
yy_2_1_4_1 = ((yy)"] == t2[");
s(yy_2_1_4_1);
yy_2_1_5_1 = yyv_N;
i(yy_2_1_5_1);
yy_2_1_6_1 = ((yy)"])");
s(yy_2_1_6_1);
nl();
goto yysl_245_1_2;
yyfl_245_1_2_1 : ;
yy_2_2_1_1 = yyv_Type;
yy_2_2_1_2 = ((yy)"STRING");
if (! IsIdent(yy_2_2_1_1, yy_2_2_1_2)) goto yyfl_245_1_2_2;
yy_2_2_2_1 = ((yy)"&&(strcmp((char *) t1[");
s(yy_2_2_2_1);
yy_2_2_3_1 = yyv_N;
i(yy_2_2_3_1);
yy_2_2_4_1 = ((yy)"], (char *) t2[");
s(yy_2_2_4_1);
yy_2_2_5_1 = yyv_N;
i(yy_2_2_5_1);
yy_2_2_6_1 = ((yy)"]) == 0)");
s(yy_2_2_6_1);
nl();
goto yysl_245_1_2;
yyfl_245_1_2_2 : ;
yy_2_3_1_1 = ((yy)"&& yyeq_");
s(yy_2_3_1_1);
yy_2_3_2_1 = yyv_Type;
id(yy_2_3_2_1);
yy_2_3_3_1 = ((yy)"((yy)t1[");
s(yy_2_3_3_1);
yy_2_3_4_1 = yyv_N;
i(yy_2_3_4_1);
yy_2_3_5_1 = ((yy)"], (yy)t2[");
s(yy_2_3_5_1);
yy_2_3_6_1 = yyv_N;
i(yy_2_3_6_1);
yy_2_3_7_1 = ((yy)"])");
s(yy_2_3_7_1);
nl();
goto yysl_245_1_2;
yysl_245_1_2 : ;
yyb = yysb;
}
yy_3_1 = yyv_Tail;
yy_3_2_1 = yyv_N;
yy_3_2_2 = ((yy)1);
yy_3_2 = (yy)(((long)yy_3_2_1)+((long)yy_3_2_2));
Code_EQFields(yy_3_1, yy_3_2);
return;
yyfl_245_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_N;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_245_2;
yyv_N = yy_0_2;
return;
yyfl_245_2 : ;
}
yyErr(2,2107);
}
Code_PrintFields(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Head;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_Tail;
yy yy_0_1_3;
yy yyv_N;
yy yy_0_2;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yy_1_1_2_1;
yy yy_1_2_1_1;
yy yy_2_1;
yy yy_2_2;
yy yy_2_2_1;
yy yyv_Type;
yy yy_2_2_2;
yy yy_2_2_3;
yy yy_3_1;
yy yy_4_1;
yy yy_5_1;
yy yy_6_1;
yy yy_7_1;
yy yy_9_1;
yy yy_9_2;
yy yy_9_2_1;
yy yy_9_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_246_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Head = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_Tail = yy_0_1_3;
yyv_N = yy_0_2;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_N;
yy_1_1_1_2 = ((yy)1);
if ((long)yy_1_1_1_1 != (long)yy_1_1_1_2) goto yyfl_246_1_1_1;
yy_1_1_2_1 = ((yy)"yyFirstArg();");
s(yy_1_1_2_1);
nl();
goto yysl_246_1_1;
yyfl_246_1_1_1 : ;
yy_1_2_1_1 = ((yy)"yyNextArg();");
s(yy_1_2_1_1);
nl();
goto yysl_246_1_1;
yysl_246_1_1 : ;
yyb = yysb;
}
yy_2_1 = yyv_Head;
yy_2_2 = yy_2_1;
if (yy_2_2[0] != 1) goto yyfl_246_1;
yy_2_2_1 = ((yy)yy_2_2[1]);
yy_2_2_2 = ((yy)yy_2_2[2]);
yy_2_2_3 = ((yy)yy_2_2[3]);
yyv_Type = yy_2_2_2;
yy_3_1 = ((yy)"yyPrint_");
s(yy_3_1);
yy_4_1 = yyv_Type;
id(yy_4_1);
yy_5_1 = ((yy)"((yy)t[");
s(yy_5_1);
yy_6_1 = yyv_N;
i(yy_6_1);
yy_7_1 = ((yy)"]);");
s(yy_7_1);
nl();
yy_9_1 = yyv_Tail;
yy_9_2_1 = yyv_N;
yy_9_2_2 = ((yy)1);
yy_9_2 = (yy)(((long)yy_9_2_1)+((long)yy_9_2_2));
Code_PrintFields(yy_9_1, yy_9_2);
return;
yyfl_246_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_N;
yy yy_0_2;
yy yy_1_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_246_2;
yyv_N = yy_0_2;
yy_1_1 = ((yy)"yyEndArgs();");
s(yy_1_1);
nl();
return;
yyfl_246_2 : ;
}
yyErr(2,2125);
}
Code_BroadcastFields(yyin_1, yyin_2, yyin_3, yyin_4)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy yyin_4;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Head;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_Tail;
yy yy_0_1_3;
yy yyv_N;
yy yy_0_2;
yy yyv_InVar;
yy yy_0_3;
yy yyv_OutVar;
yy yy_0_4;
yy yy_1_1;
yy yy_1_2;
yy yy_1_2_1;
yy yyv_Type;
yy yy_1_2_2;
yy yy_1_2_3;
yy yyv_Int;
yy yy_2_1_1;
yy yy_2_1_2_1;
yy yy_2_1_2_2;
yy yy_2_1_3_1;
yy yy_2_1_3_2;
yy yy_2_1_3_2_1;
yy yy_2_1_3_2_2;
yy yy_2_1_3_3;
yy yy_2_1_3_4;
yy yyv_Pos;
yy yy_2_2_1;
yy yy_2_2_2_1;
yy yy_2_2_2_2;
yy yy_2_2_3_1;
yy yy_2_2_3_2;
yy yy_2_2_3_2_1;
yy yy_2_2_3_2_2;
yy yy_2_2_3_3;
yy yy_2_2_3_4;
yy yyv_String;
yy yy_2_3_1;
yy yy_2_3_2_1;
yy yy_2_3_2_2;
yy yy_2_3_3_1;
yy yy_2_3_3_2;
yy yy_2_3_3_2_1;
yy yy_2_3_3_2_2;
yy yy_2_3_3_3;
yy yy_2_3_3_4;
yy yy_2_4_1_1;
yy yy_2_4_1_2;
yy yy_2_4_1_2_1;
yy yy_2_4_2_1;
yy yy_2_4_2_2;
yy yy_2_4_2_2_1;
yy yy_2_4_2_2_2;
yy yy_2_4_2_3;
yy yy_2_4_2_4;
yy yy_2_5_1_1;
yy yy_2_5_2_1;
yy yy_2_5_3_1;
yy yy_2_5_4_1;
yy yy_2_5_5_1;
yy yy_2_5_6_1;
yy yy_2_5_7_1;
yy yy_2_5_8_1;
yy yy_2_5_9_1;
yy yy_2_5_11_1;
yy yy_2_5_11_2;
yy yy_2_5_11_2_1;
yy yy_2_5_11_2_2;
yy yy_2_5_11_3;
yy yy_2_5_11_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
if (yy_0_1[0] != 1) goto yyfl_247_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Head = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_Tail = yy_0_1_3;
yyv_N = yy_0_2;
yyv_InVar = yy_0_3;
yyv_OutVar = yy_0_4;
yy_1_1 = yyv_Head;
yy_1_2 = yy_1_1;
if (yy_1_2[0] != 1) goto yyfl_247_1;
yy_1_2_1 = ((yy)yy_1_2[1]);
yy_1_2_2 = ((yy)yy_1_2[2]);
yy_1_2_3 = ((yy)yy_1_2[3]);
yyv_Type = yy_1_2_2;
{
yy yysb = yyb;
yy_2_1_1 = yyglov_id_INT;
if (yy_2_1_1 == (yy) yyu) yyErr(1,2147);
yyv_Int = yy_2_1_1;
yy_2_1_2_1 = yyv_Type;
yy_2_1_2_2 = yyv_Int;
if (! yyeq_IDENT(yy_2_1_2_1, yy_2_1_2_2)) goto yyfl_247_1_2_1;
yy_2_1_3_1 = yyv_Tail;
yy_2_1_3_2_1 = yyv_N;
yy_2_1_3_2_2 = ((yy)1);
yy_2_1_3_2 = (yy)(((long)yy_2_1_3_2_1)+((long)yy_2_1_3_2_2));
yy_2_1_3_3 = yyv_InVar;
yy_2_1_3_4 = yyv_OutVar;
Code_BroadcastFields(yy_2_1_3_1, yy_2_1_3_2, yy_2_1_3_3, yy_2_1_3_4);
goto yysl_247_1_2;
yyfl_247_1_2_1 : ;
yy_2_2_1 = yyglov_id_POS;
if (yy_2_2_1 == (yy) yyu) yyErr(1,2151);
yyv_Pos = yy_2_2_1;
yy_2_2_2_1 = yyv_Type;
yy_2_2_2_2 = yyv_Pos;
if (! yyeq_IDENT(yy_2_2_2_1, yy_2_2_2_2)) goto yyfl_247_1_2_2;
yy_2_2_3_1 = yyv_Tail;
yy_2_2_3_2_1 = yyv_N;
yy_2_2_3_2_2 = ((yy)1);
yy_2_2_3_2 = (yy)(((long)yy_2_2_3_2_1)+((long)yy_2_2_3_2_2));
yy_2_2_3_3 = yyv_InVar;
yy_2_2_3_4 = yyv_OutVar;
Code_BroadcastFields(yy_2_2_3_1, yy_2_2_3_2, yy_2_2_3_3, yy_2_2_3_4);
goto yysl_247_1_2;
yyfl_247_1_2_2 : ;
yy_2_3_1 = yyglov_id_STRING;
if (yy_2_3_1 == (yy) yyu) yyErr(1,2155);
yyv_String = yy_2_3_1;
yy_2_3_2_1 = yyv_Type;
yy_2_3_2_2 = yyv_String;
if (! yyeq_IDENT(yy_2_3_2_1, yy_2_3_2_2)) goto yyfl_247_1_2_3;
yy_2_3_3_1 = yyv_Tail;
yy_2_3_3_2_1 = yyv_N;
yy_2_3_3_2_2 = ((yy)1);
yy_2_3_3_2 = (yy)(((long)yy_2_3_3_2_1)+((long)yy_2_3_3_2_2));
yy_2_3_3_3 = yyv_InVar;
yy_2_3_3_4 = yyv_OutVar;
Code_BroadcastFields(yy_2_3_3_1, yy_2_3_3_2, yy_2_3_3_3, yy_2_3_3_4);
goto yysl_247_1_2;
yyfl_247_1_2_3 : ;
yy_2_4_1_1 = yyv_Type;
if (! GetGlobalMeaning(yy_2_4_1_1, &yy_2_4_1_2)) goto yyfl_247_1_2_4;
if (yy_2_4_1_2[0] != 3) goto yyfl_247_1_2_4;
yy_2_4_1_2_1 = ((yy)yy_2_4_1_2[1]);
yy_2_4_2_1 = yyv_Tail;
yy_2_4_2_2_1 = yyv_N;
yy_2_4_2_2_2 = ((yy)1);
yy_2_4_2_2 = (yy)(((long)yy_2_4_2_2_1)+((long)yy_2_4_2_2_2));
yy_2_4_2_3 = yyv_InVar;
yy_2_4_2_4 = yyv_OutVar;
Code_BroadcastFields(yy_2_4_2_1, yy_2_4_2_2, yy_2_4_2_3, yy_2_4_2_4);
goto yysl_247_1_2;
yyfl_247_1_2_4 : ;
yy_2_5_1_1 = ((yy)"yybroadcast_");
s(yy_2_5_1_1);
yy_2_5_2_1 = yyv_Type;
id(yy_2_5_2_1);
yy_2_5_3_1 = ((yy)"((yy)t[");
s(yy_2_5_3_1);
yy_2_5_4_1 = yyv_N;
i(yy_2_5_4_1);
yy_2_5_5_1 = ((yy)"], ");
s(yy_2_5_5_1);
yy_2_5_6_1 = yyv_InVar;
s(yy_2_5_6_1);
yy_2_5_7_1 = ((yy)", &");
s(yy_2_5_7_1);
yy_2_5_8_1 = yyv_OutVar;
s(yy_2_5_8_1);
yy_2_5_9_1 = ((yy)", Handler);");
s(yy_2_5_9_1);
nl();
yy_2_5_11_1 = yyv_Tail;
yy_2_5_11_2_1 = yyv_N;
yy_2_5_11_2_2 = ((yy)1);
yy_2_5_11_2 = (yy)(((long)yy_2_5_11_2_1)+((long)yy_2_5_11_2_2));
yy_2_5_11_3 = yyv_OutVar;
yy_2_5_11_4 = yyv_InVar;
Code_BroadcastFields(yy_2_5_11_1, yy_2_5_11_2, yy_2_5_11_3, yy_2_5_11_4);
goto yysl_247_1_2;
yysl_247_1_2 : ;
yyb = yysb;
}
return;
yyfl_247_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_N;
yy yy_0_2;
yy yyv_InVar;
yy yy_0_3;
yy yyv_OutVar;
yy yy_0_4;
yy yy_1_1;
yy yy_2_1;
yy yy_3_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
if (yy_0_1[0] != 2) goto yyfl_247_2;
yyv_N = yy_0_2;
yyv_InVar = yy_0_3;
yyv_OutVar = yy_0_4;
yy_1_1 = ((yy)"*Out = ");
s(yy_1_1);
yy_2_1 = yyv_InVar;
s(yy_2_1);
yy_3_1 = ((yy)";");
s(yy_3_1);
nl();
return;
yyfl_247_2 : ;
}
yyErr(2,2143);
}
Code_DECLARATION(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Id;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_3_1;
yy yyv_Functors;
yy yy_0_1_3_2;
yy yy_1_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_248_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Id = yy_0_1_1;
yyv_P = yy_0_1_2;
if (yy_0_1_3[0] != 1) goto yyfl_248_1;
yy_0_1_3_1 = ((yy)yy_0_1_3[1]);
yy_0_1_3_2 = ((yy)yy_0_1_3[2]);
yyv_Functors = yy_0_1_3_2;
yy_1_1 = yyv_Functors;
CheckFunctors(yy_1_1);
return;
yyfl_248_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Id;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_Type;
yy yy_0_1_3_1;
yy yyv_P2;
yy yy_0_1_3_2;
yy yy_1_1;
yy yy_1_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_248_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Id = yy_0_1_1;
yyv_P = yy_0_1_2;
if (yy_0_1_3[0] != 2) goto yyfl_248_2;
yy_0_1_3_1 = ((yy)yy_0_1_3[1]);
yy_0_1_3_2 = ((yy)yy_0_1_3[2]);
yyv_Type = yy_0_1_3_1;
yyv_P2 = yy_0_1_3_2;
yy_1_1 = yyv_Type;
yy_1_2 = yyv_P2;
CheckType(yy_1_1, yy_1_2);
return;
yyfl_248_2 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Id;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_Attr;
yy yy_0_1_3_1;
yy yy_1_1;
yy yy_1_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_248_3;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Id = yy_0_1_1;
yyv_P = yy_0_1_2;
if (yy_0_1_3[0] != 3) goto yyfl_248_3;
yy_0_1_3_1 = ((yy)yy_0_1_3[1]);
yyv_Attr = yy_0_1_3_1;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_Attr;
yy_1_2 = yyb + 0;
yy_1_2[0] = 2;
Check_UniqueIds(yy_1_1, yy_1_2);
return;
yyfl_248_3 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Id;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_1_3_1;
yy yyv_Class;
yy yy_0_1_3_2;
yy yy_0_1_3_3;
yy yyv_InArgs;
yy yy_0_1_3_3_1;
yy yyv_OutArgs;
yy yy_0_1_3_3_2;
yy yy_0_1_3_4;
yy yy_1_1_1_1;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_2_1_1_1;
yy yy_2_1_1_2;
yy yy_2_1_2_1;
yy yy_2_1_2_2;
yy yy_2_2_1_1;
yy yy_2_2_1_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_248_4;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Id = yy_0_1_1;
yyv_P = yy_0_1_2;
if (yy_0_1_3[0] != 4) goto yyfl_248_4;
yy_0_1_3_1 = ((yy)yy_0_1_3[1]);
yy_0_1_3_2 = ((yy)yy_0_1_3[2]);
yy_0_1_3_3 = ((yy)yy_0_1_3[3]);
yy_0_1_3_4 = ((yy)yy_0_1_3[4]);
yyv_Class = yy_0_1_3_2;
if (yy_0_1_3_3[0] != 1) goto yyfl_248_4;
yy_0_1_3_3_1 = ((yy)yy_0_1_3_3[1]);
yy_0_1_3_3_2 = ((yy)yy_0_1_3_3[2]);
yyv_InArgs = yy_0_1_3_3_1;
yyv_OutArgs = yy_0_1_3_3_2;
if (yy_0_1_3_4[0] != 2) goto yyfl_248_4;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Class;
if (! ProcOrCond(yy_1_1_1_1)) goto yyfl_248_4_1_1;
goto yysl_248_4_1;
yyfl_248_4_1_1 : ;
yy_1_2_1_1 = yyv_Class;
yy_1_2_1_2 = yy_1_2_1_1;
if (yy_1_2_1_2[0] != 6) goto yyfl_248_4_1_2;
goto yysl_248_4_1;
yyfl_248_4_1_2 : ;
goto yyfl_248_4;
yysl_248_4_1 : ;
yyb = yysb;
}
{
yy yysb = yyb;
yy_2_1_1_1 = yyv_Class;
yy_2_1_1_2 = yy_2_1_1_1;
if (yy_2_1_1_2[0] != 6) goto yyfl_248_4_2_1;
yy_2_1_2_1 = ((yy)"no rules for 'sweep' predicate");
yy_2_1_2_2 = yyv_P;
MESSAGE(yy_2_1_2_1, yy_2_1_2_2);
goto yysl_248_4_2;
yyfl_248_4_2_1 : ;
yy_2_2_1_1 = yyv_InArgs;
yy_2_2_1_2 = yyv_OutArgs;
CheckFormalParams(yy_2_2_1_1, yy_2_2_1_2);
goto yysl_248_4_2;
yysl_248_4_2 : ;
yyb = yysb;
}
return;
yyfl_248_4 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Id;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_Name;
yy yy_0_1_3_1;
yy yyv_Class;
yy yy_0_1_3_2;
yy yy_0_1_3_3;
yy yyv_InArgs;
yy yy_0_1_3_3_1;
yy yyv_OutArgs;
yy yy_0_1_3_3_2;
yy yyv_Rules;
yy yy_0_1_3_4;
yy yy_1_1_1_1;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_2_1_1_1;
yy yy_2_1_1_2;
yy yy_2_1_2_1_1_1;
yy yy_2_1_2_1_1_2;
yy yy_2_1_2_1_2_1;
yy yy_2_1_2_1_2_2;
yy yy_2_1_2_2_1_1;
yy yy_2_1_2_2_1_2;
yy yyv_Head;
yy yy_2_1_2_2_1_2_1;
yy yyv_Pos;
yy yy_2_1_2_2_1_2_2;
yy yyv_Tail;
yy yy_2_1_2_2_1_2_3;
yy yy_2_1_2_2_2_1_1_1;
yy yy_2_1_2_2_2_1_1_2;
yy yy_2_1_2_2_2_1_1_2_1;
yy yyv_TypeId;
yy yy_2_1_2_2_2_1_1_2_2;
yy yy_2_1_2_2_2_1_1_2_3;
yy yy_2_1_2_2_2_1_2_1;
yy yyv_I;
yy yy_2_1_2_2_2_1_2_2;
yy yy_2_1_2_2_2_1_3_1;
yy yy_2_1_2_2_2_1_3_2;
yy yy_2_1_2_2_2_2_1_1;
yy yy_2_1_2_2_2_2_1_2;
yy yy_2_1_2_2_3_1_1_1;
yy yy_2_1_2_2_3_1_1_2;
yy yy_2_1_2_2_3_1_2_1_1_1;
yy yy_2_1_2_2_3_1_2_1_1_2;
yy yy_2_1_2_2_3_1_2_2_1_1;
yy yy_2_1_2_2_3_1_2_2_1_2;
yy yy_2_1_2_2_3_2_1_1;
yy yy_2_1_2_2_3_2_1_2;
yy yy_2_1_2_2_3_2_1_2_1;
yy yy_2_1_2_2_3_2_1_2_1_1;
yy yyv_T1;
yy yy_2_1_2_2_3_2_1_2_1_2;
yy yy_2_1_2_2_3_2_1_2_1_3;
yy yy_2_1_2_2_3_2_1_2_2;
yy yy_2_1_2_2_3_2_1_2_3;
yy yy_2_1_2_2_3_2_2_1_1_1;
yy yy_2_1_2_2_3_2_2_1_1_2;
yy yy_2_1_2_2_3_2_2_2_1_1;
yy yy_2_1_2_2_3_2_2_2_1_2;
yy yy_2_1_2_2_3_2_2_2_1_2_1;
yy yy_2_1_2_2_3_2_2_2_1_2_1_1;
yy yyv_T2;
yy yy_2_1_2_2_3_2_2_2_1_2_1_2;
yy yy_2_1_2_2_3_2_2_2_1_2_1_3;
yy yyv_P2;
yy yy_2_1_2_2_3_2_2_2_1_2_2;
yy yy_2_1_2_2_3_2_2_2_1_2_3;
yy yy_2_1_2_2_3_2_2_2_2_1_1_1;
yy yy_2_1_2_2_3_2_2_2_2_1_1_2;
yy yy_2_1_2_2_3_2_2_2_2_2_1_1;
yy yy_2_1_2_2_3_2_2_2_2_2_1_2;
yy yy_2_1_2_2_3_2_2_2_2_2_1_3;
yy yy_2_1_2_2_3_2_2_2_2_2_1_4;
yy yy_2_1_2_2_3_2_2_2_2_2_1_5;
yy yy_2_1_2_2_3_2_2_2_2_2_1_6;
yy yy_2_1_2_2_3_2_2_3_1_1;
yy yy_2_1_2_2_3_2_2_3_1_2;
yy yy_2_1_2_2_3_3_1_1;
yy yy_2_1_2_2_3_3_1_2;
yy yy_2_1_2_2_4_1;
yy yy_2_1_2_2_4_2;
yy yy_2_2_1_1;
yy yy_2_2_1_2;
yy yy_3_1;
yy yy_4_1;
yy yy_5_1;
yy yy_6_1_1_1;
yy yy_6_1_1_2;
yy yy_6_1_2_1;
yy yy_6_2_1_1;
yy yy_6_2_1_2;
yy yy_6_2_1_3;
yy yy_6_2_1_4;
yy yyv_Sep;
yy yy_6_2_1_5;
yy yy_6_2_2_1;
yy yy_6_2_2_2;
yy yy_6_2_2_3;
yy yy_6_2_2_4;
yy yyv_Dummy;
yy yy_6_2_2_5;
yy yy_7_1;
yy yy_9_1_1_1;
yy yy_9_1_1_2;
yy yy_9_1_2_1;
yy yy_9_1_4_1;
yy yy_9_2_1_1;
yy yy_9_2_1_2;
yy yy_9_2_1_3;
yy yy_9_2_2_1;
yy yy_9_2_2_2;
yy yy_9_2_2_3;
yy yy_10_1;
yy yy_12;
yy yy_13;
yy yy_14_1_1_1;
yy yy_14_1_1_2;
yy yy_14_1_2_1;
yy yyv_RuleGroups;
yy yy_14_1_2_2;
yy yy_14_1_3_1;
yy yy_14_1_5_1;
yy yy_14_1_6_1;
yy yy_14_1_6_2;
yy yy_14_1_6_3;
yy yy_14_1_6_4;
yy yy_14_1_6_5;
yy yy_14_1_7_1;
yy yy_14_1_9_1;
yy yy_14_2_1_1_2_1;
yy yy_14_2_1_1_4_1;
yy yy_14_2_2_1;
yy yy_14_2_2_2;
yy yy_14_2_2_3;
yy yy_14_2_2_4;
yy yy_14_2_2_5;
yy yy_14_2_2_6;
yy yyv_MayFail;
yy yy_14_2_2_7;
yy yy_14_2_3_1_2_1;
yy yy_14_2_4_1_1_1;
yy yy_14_2_4_1_1_2;
yy yy_14_2_4_1_2_1;
yy yy_14_2_4_1_2_2;
yy yy_15_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_248_5;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Id = yy_0_1_1;
yyv_P = yy_0_1_2;
if (yy_0_1_3[0] != 4) goto yyfl_248_5;
yy_0_1_3_1 = ((yy)yy_0_1_3[1]);
yy_0_1_3_2 = ((yy)yy_0_1_3[2]);
yy_0_1_3_3 = ((yy)yy_0_1_3[3]);
yy_0_1_3_4 = ((yy)yy_0_1_3[4]);
yyv_Name = yy_0_1_3_1;
yyv_Class = yy_0_1_3_2;
if (yy_0_1_3_3[0] != 1) goto yyfl_248_5;
yy_0_1_3_3_1 = ((yy)yy_0_1_3_3[1]);
yy_0_1_3_3_2 = ((yy)yy_0_1_3_3[2]);
yyv_InArgs = yy_0_1_3_3_1;
yyv_OutArgs = yy_0_1_3_3_2;
yyv_Rules = yy_0_1_3_4;
yyb = yyh;
yyh += 2; if (yyh > yyhx) yyExtend();
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Class;
if (! ProcOrCond(yy_1_1_1_1)) goto yyfl_248_5_1_1;
goto yysl_248_5_1;
yyfl_248_5_1_1 : ;
yy_1_2_1_1 = yyv_Class;
yy_1_2_1_2 = yy_1_2_1_1;
if (yy_1_2_1_2[0] != 6) goto yyfl_248_5_1_2;
goto yysl_248_5_1;
yyfl_248_5_1_2 : ;
goto yyfl_248_5;
yysl_248_5_1 : ;
yyb = yysb;
}
{
yy yysb = yyb;
yy_2_1_1_1 = yyv_Class;
yy_2_1_1_2 = yy_2_1_1_1;
if (yy_2_1_1_2[0] != 6) goto yyfl_248_5_2_1;
{
yy yysb = yyb;
yy_2_1_2_1_1_1 = yyv_InArgs;
yy_2_1_2_1_1_2 = yy_2_1_2_1_1_1;
if (yy_2_1_2_1_1_2[0] != 2) goto yyfl_248_5_2_1_2_1;
yy_2_1_2_1_2_1 = ((yy)"At least on input argument required");
yy_2_1_2_1_2_2 = yyv_P;
MESSAGE(yy_2_1_2_1_2_1, yy_2_1_2_1_2_2);
goto yysl_248_5_2_1_2;
yyfl_248_5_2_1_2_1 : ;
yy_2_1_2_2_1_1 = yyv_InArgs;
yy_2_1_2_2_1_2 = yy_2_1_2_2_1_1;
if (yy_2_1_2_2_1_2[0] != 1) goto yyfl_248_5_2_1_2_2;
yy_2_1_2_2_1_2_1 = ((yy)yy_2_1_2_2_1_2[1]);
yy_2_1_2_2_1_2_2 = ((yy)yy_2_1_2_2_1_2[2]);
yy_2_1_2_2_1_2_3 = ((yy)yy_2_1_2_2_1_2[3]);
yyv_Head = yy_2_1_2_2_1_2_1;
yyv_Pos = yy_2_1_2_2_1_2_2;
yyv_Tail = yy_2_1_2_2_1_2_3;
{
yy yysb = yyb;
yy_2_1_2_2_2_1_1_1 = yyv_Head;
yy_2_1_2_2_2_1_1_2 = yy_2_1_2_2_2_1_1_1;
if (yy_2_1_2_2_2_1_1_2[0] != 1) goto yyfl_248_5_2_1_2_2_2_1;
yy_2_1_2_2_2_1_1_2_1 = ((yy)yy_2_1_2_2_2_1_1_2[1]);
yy_2_1_2_2_2_1_1_2_2 = ((yy)yy_2_1_2_2_2_1_1_2[2]);
yy_2_1_2_2_2_1_1_2_3 = ((yy)yy_2_1_2_2_2_1_1_2[3]);
yyv_TypeId = yy_2_1_2_2_2_1_1_2_2;
yy_2_1_2_2_2_1_2_1 = ((yy)"ANY");
string_to_id(yy_2_1_2_2_2_1_2_1, &yy_2_1_2_2_2_1_2_2);
yyv_I = yy_2_1_2_2_2_1_2_2;
yy_2_1_2_2_2_1_3_1 = yyv_TypeId;
yy_2_1_2_2_2_1_3_2 = yyv_I;
if (! yyeq_IDENT(yy_2_1_2_2_2_1_3_1, yy_2_1_2_2_2_1_3_2)) goto yyfl_248_5_2_1_2_2_2_1;
goto yysl_248_5_2_1_2_2_2;
yyfl_248_5_2_1_2_2_2_1 : ;
yy_2_1_2_2_2_2_1_1 = ((yy)"type of primary argument must be specified as 'ANY'");
yy_2_1_2_2_2_2_1_2 = yyv_Pos;
MESSAGE(yy_2_1_2_2_2_2_1_1, yy_2_1_2_2_2_2_1_2);
goto yysl_248_5_2_1_2_2_2;
yysl_248_5_2_1_2_2_2 : ;
yyb = yysb;
}
{
yy yysb = yyb;
yy_2_1_2_2_3_1_1_1 = yyv_Tail;
yy_2_1_2_2_3_1_1_2 = yy_2_1_2_2_3_1_1_1;
if (yy_2_1_2_2_3_1_1_2[0] != 2) goto yyfl_248_5_2_1_2_2_3_1;
{
yy yysb = yyb;
yy_2_1_2_2_3_1_2_1_1_1 = yyv_OutArgs;
yy_2_1_2_2_3_1_2_1_1_2 = yy_2_1_2_2_3_1_2_1_1_1;
if (yy_2_1_2_2_3_1_2_1_1_2[0] != 2) goto yyfl_248_5_2_1_2_2_3_1_2_1;
goto yysl_248_5_2_1_2_2_3_1_2;
yyfl_248_5_2_1_2_2_3_1_2_1 : ;
yy_2_1_2_2_3_1_2_2_1_1 = ((yy)"output argument requires input argument of same type");
yy_2_1_2_2_3_1_2_2_1_2 = yyv_P;
MESSAGE(yy_2_1_2_2_3_1_2_2_1_1, yy_2_1_2_2_3_1_2_2_1_2);
goto yysl_248_5_2_1_2_2_3_1_2;
yysl_248_5_2_1_2_2_3_1_2 : ;
yyb = yysb;
}
goto yysl_248_5_2_1_2_2_3;
yyfl_248_5_2_1_2_2_3_1 : ;
yy_2_1_2_2_3_2_1_1 = yyv_Tail;
yy_2_1_2_2_3_2_1_2 = yy_2_1_2_2_3_2_1_1;
if (yy_2_1_2_2_3_2_1_2[0] != 1) goto yyfl_248_5_2_1_2_2_3_2;
yy_2_1_2_2_3_2_1_2_1 = ((yy)yy_2_1_2_2_3_2_1_2[1]);
yy_2_1_2_2_3_2_1_2_2 = ((yy)yy_2_1_2_2_3_2_1_2[2]);
yy_2_1_2_2_3_2_1_2_3 = ((yy)yy_2_1_2_2_3_2_1_2[3]);
if (yy_2_1_2_2_3_2_1_2_1[0] != 1) goto yyfl_248_5_2_1_2_2_3_2;
yy_2_1_2_2_3_2_1_2_1_1 = ((yy)yy_2_1_2_2_3_2_1_2_1[1]);
yy_2_1_2_2_3_2_1_2_1_2 = ((yy)yy_2_1_2_2_3_2_1_2_1[2]);
yy_2_1_2_2_3_2_1_2_1_3 = ((yy)yy_2_1_2_2_3_2_1_2_1[3]);
yyv_T1 = yy_2_1_2_2_3_2_1_2_1_2;
if (yy_2_1_2_2_3_2_1_2_3[0] != 2) goto yyfl_248_5_2_1_2_2_3_2;
{
yy yysb = yyb;
yy_2_1_2_2_3_2_2_1_1_1 = yyv_OutArgs;
yy_2_1_2_2_3_2_2_1_1_2 = yy_2_1_2_2_3_2_2_1_1_1;
if (yy_2_1_2_2_3_2_2_1_1_2[0] != 2) goto yyfl_248_5_2_1_2_2_3_2_2_1;
goto yysl_248_5_2_1_2_2_3_2_2;
yyfl_248_5_2_1_2_2_3_2_2_1 : ;
yy_2_1_2_2_3_2_2_2_1_1 = yyv_OutArgs;
yy_2_1_2_2_3_2_2_2_1_2 = yy_2_1_2_2_3_2_2_2_1_1;
if (yy_2_1_2_2_3_2_2_2_1_2[0] != 1) goto yyfl_248_5_2_1_2_2_3_2_2_2;
yy_2_1_2_2_3_2_2_2_1_2_1 = ((yy)yy_2_1_2_2_3_2_2_2_1_2[1]);
yy_2_1_2_2_3_2_2_2_1_2_2 = ((yy)yy_2_1_2_2_3_2_2_2_1_2[2]);
yy_2_1_2_2_3_2_2_2_1_2_3 = ((yy)yy_2_1_2_2_3_2_2_2_1_2[3]);
if (yy_2_1_2_2_3_2_2_2_1_2_1[0] != 1) goto yyfl_248_5_2_1_2_2_3_2_2_2;
yy_2_1_2_2_3_2_2_2_1_2_1_1 = ((yy)yy_2_1_2_2_3_2_2_2_1_2_1[1]);
yy_2_1_2_2_3_2_2_2_1_2_1_2 = ((yy)yy_2_1_2_2_3_2_2_2_1_2_1[2]);
yy_2_1_2_2_3_2_2_2_1_2_1_3 = ((yy)yy_2_1_2_2_3_2_2_2_1_2_1[3]);
yyv_T2 = yy_2_1_2_2_3_2_2_2_1_2_1_2;
yyv_P2 = yy_2_1_2_2_3_2_2_2_1_2_2;
if (yy_2_1_2_2_3_2_2_2_1_2_3[0] != 2) goto yyfl_248_5_2_1_2_2_3_2_2_2;
{
yy yysb = yyb;
yy_2_1_2_2_3_2_2_2_2_1_1_1 = yyv_T1;
yy_2_1_2_2_3_2_2_2_2_1_1_2 = yyv_T2;
if (! yyeq_IDENT(yy_2_1_2_2_3_2_2_2_2_1_1_1, yy_2_1_2_2_3_2_2_2_2_1_1_2)) goto yyfl_248_5_2_1_2_2_3_2_2_2_2_1;
goto yysl_248_5_2_1_2_2_3_2_2_2_2;
yyfl_248_5_2_1_2_2_3_2_2_2_2_1 : ;
yy_2_1_2_2_3_2_2_2_2_2_1_1 = ((yy)"'");
yy_2_1_2_2_3_2_2_2_2_2_1_2 = yyv_T2;
yy_2_1_2_2_3_2_2_2_2_2_1_3 = ((yy)"' does not correspond to '");
yy_2_1_2_2_3_2_2_2_2_2_1_4 = yyv_T1;
yy_2_1_2_2_3_2_2_2_2_2_1_5 = ((yy)"'");
yy_2_1_2_2_3_2_2_2_2_2_1_6 = yyv_P2;
MESSAGE2(yy_2_1_2_2_3_2_2_2_2_2_1_1, yy_2_1_2_2_3_2_2_2_2_2_1_2, yy_2_1_2_2_3_2_2_2_2_2_1_3, yy_2_1_2_2_3_2_2_2_2_2_1_4, yy_2_1_2_2_3_2_2_2_2_2_1_5, yy_2_1_2_2_3_2_2_2_2_2_1_6);
goto yysl_248_5_2_1_2_2_3_2_2_2_2;
yysl_248_5_2_1_2_2_3_2_2_2_2 : ;
yyb = yysb;
}
goto yysl_248_5_2_1_2_2_3_2_2;
yyfl_248_5_2_1_2_2_3_2_2_2 : ;
yy_2_1_2_2_3_2_2_3_1_1 = ((yy)"too many output arguments");
yy_2_1_2_2_3_2_2_3_1_2 = yyv_P;
MESSAGE(yy_2_1_2_2_3_2_2_3_1_1, yy_2_1_2_2_3_2_2_3_1_2);
goto yysl_248_5_2_1_2_2_3_2_2;
yysl_248_5_2_1_2_2_3_2_2 : ;
yyb = yysb;
}
goto yysl_248_5_2_1_2_2_3;
yyfl_248_5_2_1_2_2_3_2 : ;
yy_2_1_2_2_3_3_1_1 = ((yy)"too many input arguments");
yy_2_1_2_2_3_3_1_2 = yyv_P;
MESSAGE(yy_2_1_2_2_3_3_1_1, yy_2_1_2_2_3_3_1_2);
goto yysl_248_5_2_1_2_2_3;
yysl_248_5_2_1_2_2_3 : ;
yyb = yysb;
}
yy_2_1_2_2_4_1 = yyv_Tail;
yy_2_1_2_2_4_2 = yyv_OutArgs;
CheckFormalParams(yy_2_1_2_2_4_1, yy_2_1_2_2_4_2);
goto yysl_248_5_2_1_2;
yyfl_248_5_2_1_2_2 : ;
goto yyfl_248_5_2_1;
yysl_248_5_2_1_2 : ;
yyb = yysb;
}
goto yysl_248_5_2;
yyfl_248_5_2_1 : ;
yy_2_2_1_1 = yyv_InArgs;
yy_2_2_1_2 = yyv_OutArgs;
CheckFormalParams(yy_2_2_1_1, yy_2_2_1_2);
goto yysl_248_5_2;
yysl_248_5_2 : ;
yyb = yysb;
}
yy_3_1 = yyv_Class;
ResultType(yy_3_1);
yy_4_1 = yyv_Name;
name(yy_4_1);
yy_5_1 = ((yy)"(");
s(yy_5_1);
{
yy yysb = yyb;
yy_6_1_1_1 = yyv_Class;
yy_6_1_1_2 = yy_6_1_1_1;
if (yy_6_1_1_2[0] != 6) goto yyfl_248_5_6_1;
yy_6_1_2_1 = ((yy)"yytp, yyin_1, yyin_2, yyout_1 ");
s(yy_6_1_2_1);
goto yysl_248_5_6;
yyfl_248_5_6_1 : ;
yy_6_2_1_1 = yyv_InArgs;
yy_6_2_1_2 = ((yy)1);
yy_6_2_1_3 = ((yy)"yyin_");
yy_6_2_1_4 = ((yy)"");
ListFormalParams(yy_6_2_1_1, yy_6_2_1_2, yy_6_2_1_3, yy_6_2_1_4, &yy_6_2_1_5);
yyv_Sep = yy_6_2_1_5;
yy_6_2_2_1 = yyv_OutArgs;
yy_6_2_2_2 = ((yy)1);
yy_6_2_2_3 = ((yy)"yyout_");
yy_6_2_2_4 = yyv_Sep;
ListFormalParams(yy_6_2_2_1, yy_6_2_2_2, yy_6_2_2_3, yy_6_2_2_4, &yy_6_2_2_5);
yyv_Dummy = yy_6_2_2_5;
goto yysl_248_5_6;
yysl_248_5_6 : ;
yyb = yysb;
}
yy_7_1 = ((yy)")");
s(yy_7_1);
nl();
{
yy yysb = yyb;
yy_9_1_1_1 = yyv_Class;
yy_9_1_1_2 = yy_9_1_1_1;
if (yy_9_1_1_2[0] != 6) goto yyfl_248_5_9_1;
yy_9_1_2_1 = ((yy)"intptr_t yytp;");
s(yy_9_1_2_1);
nl();
yy_9_1_4_1 = ((yy)"yy yyin_1, yyin_2, *yyout_1;");
s(yy_9_1_4_1);
nl();
goto yysl_248_5_9;
yyfl_248_5_9_1 : ;
yy_9_2_1_1 = yyv_InArgs;
yy_9_2_1_2 = ((yy)1);
yy_9_2_1_3 = ((yy)"yyin_");
DeclareFormalParams(yy_9_2_1_1, yy_9_2_1_2, yy_9_2_1_3);
yy_9_2_2_1 = yyv_OutArgs;
yy_9_2_2_2 = ((yy)1);
yy_9_2_2_3 = ((yy)"*yyout_");
DeclareFormalParams(yy_9_2_2_1, yy_9_2_2_2, yy_9_2_2_3);
goto yysl_248_5_9;
yysl_248_5_9 : ;
yyb = yysb;
}
yy_10_1 = ((yy)"{");
s(yy_10_1);
nl();
yy_12 = yyb + 0;
yy_12[0] = 2;
yyglov_currentFailLabel = yy_12;
yy_13 = yyb + 1;
yy_13[0] = 2;
yyglov_currentFailLabelUsed = yy_13;
{
yy yysb = yyb;
yy_14_1_1_1 = yyv_Class;
yy_14_1_1_2 = yy_14_1_1_1;
if (yy_14_1_1_2[0] != 6) goto yyfl_248_5_14_1;
yy_14_1_2_1 = yyv_Rules;
CollectSweepRules(yy_14_1_2_1, &yy_14_1_2_2);
yyv_RuleGroups = yy_14_1_2_2;
yy_14_1_3_1 = ((yy)"{");
s(yy_14_1_3_1);
nl();
yy_14_1_5_1 = yyv_RuleGroups;
Declare_broadcast_Procs(yy_14_1_5_1);
yy_14_1_6_1 = yyv_RuleGroups;
yy_14_1_6_2 = yyv_Id;
yy_14_1_6_3 = yyv_InArgs;
yy_14_1_6_4 = yyv_OutArgs;
yy_14_1_6_5 = ((yy)1);
Code_RuleGroups(yy_14_1_6_1, yy_14_1_6_2, yy_14_1_6_3, yy_14_1_6_4, yy_14_1_6_5);
yy_14_1_7_1 = ((yy)"else return 0;");
s(yy_14_1_7_1);
nl();
yy_14_1_9_1 = ((yy)"}");
s(yy_14_1_9_1);
nl();
goto yysl_248_5_14;
yyfl_248_5_14_1 : ;
{
yy yysb = yyb;
if (! TraceOption()) goto yyfl_248_5_14_2_1_1;
yy_14_2_1_1_2_1 = ((yy)"int LOCALTRACEPTR;");
s(yy_14_2_1_1_2_1);
nl();
yy_14_2_1_1_4_1 = ((yy)"TRACE_BEGIN_RULELIST(&LOCALTRACEPTR);");
s(yy_14_2_1_1_4_1);
nl();
goto yysl_248_5_14_2_1;
yyfl_248_5_14_2_1_1 : ;
goto yysl_248_5_14_2_1;
yysl_248_5_14_2_1 : ;
yyb = yysb;
}
yy_14_2_2_1 = yyv_Class;
yy_14_2_2_2 = yyv_Id;
yy_14_2_2_3 = yyv_InArgs;
yy_14_2_2_4 = yyv_OutArgs;
yy_14_2_2_5 = yyv_Rules;
yy_14_2_2_6 = ((yy)1);
Code_Rules(yy_14_2_2_1, yy_14_2_2_2, yy_14_2_2_3, yy_14_2_2_4, yy_14_2_2_5, yy_14_2_2_6, &yy_14_2_2_7);
yyv_MayFail = yy_14_2_2_7;
{
yy yysb = yyb;
if (! TraceOption()) goto yyfl_248_5_14_2_3_1;
yy_14_2_3_1_2_1 = ((yy)"TRACE_END_RULELIST(LOCALTRACEPTR);");
s(yy_14_2_3_1_2_1);
nl();
goto yysl_248_5_14_2_3;
yyfl_248_5_14_2_3_1 : ;
goto yysl_248_5_14_2_3;
yysl_248_5_14_2_3 : ;
yyb = yysb;
}
{
yy yysb = yyb;
yy_14_2_4_1_1_1 = yyv_MayFail;
yy_14_2_4_1_1_2 = yy_14_2_4_1_1_1;
if (yy_14_2_4_1_1_2[0] != 1) goto yyfl_248_5_14_2_4_1;
yy_14_2_4_1_2_1 = yyv_Class;
yy_14_2_4_1_2_2 = yyv_P;
ReturnFalse(yy_14_2_4_1_2_1, yy_14_2_4_1_2_2);
goto yysl_248_5_14_2_4;
yyfl_248_5_14_2_4_1 : ;
goto yysl_248_5_14_2_4;
yysl_248_5_14_2_4 : ;
yyb = yysb;
}
goto yysl_248_5_14;
yysl_248_5_14 : ;
yyb = yysb;
}
yy_15_1 = ((yy)"}");
s(yy_15_1);
nl();
return;
yyfl_248_5 : ;
}
{
yy yyb;
yy yyv_Decl;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_Id;
yy yy_0_1_1_1;
yy yyv_P;
yy yy_0_1_1_2;
yy yy_0_1_1_3;
yy yyv_Name;
yy yy_0_1_1_3_1;
yy yy_0_1_1_3_2;
yy yy_0_1_1_3_3;
yy yyv_InArgs;
yy yy_0_1_1_3_3_1;
yy yyv_OutArgs;
yy yy_0_1_1_3_3_2;
yy yyv_Rules;
yy yy_0_1_1_3_4;
yy_0_1 = yyin_1;
yy_0_1_1 = yy_0_1;
yyv_Decl = yy_0_1;
if (yy_0_1_1[0] != 2) goto yyfl_248_6;
yy_0_1_1_1 = ((yy)yy_0_1_1[1]);
yy_0_1_1_2 = ((yy)yy_0_1_1[2]);
yy_0_1_1_3 = ((yy)yy_0_1_1[3]);
yyv_Id = yy_0_1_1_1;
yyv_P = yy_0_1_1_2;
if (yy_0_1_1_3[0] != 4) goto yyfl_248_6;
yy_0_1_1_3_1 = ((yy)yy_0_1_1_3[1]);
yy_0_1_1_3_2 = ((yy)yy_0_1_1_3[2]);
yy_0_1_1_3_3 = ((yy)yy_0_1_1_3[3]);
yy_0_1_1_3_4 = ((yy)yy_0_1_1_3[4]);
yyv_Name = yy_0_1_1_3_1;
if (yy_0_1_1_3_2[0] != 5) goto yyfl_248_6;
if (yy_0_1_1_3_3[0] != 1) goto yyfl_248_6;
yy_0_1_1_3_3_1 = ((yy)yy_0_1_1_3_3[1]);
yy_0_1_1_3_3_2 = ((yy)yy_0_1_1_3_3[2]);
yyv_InArgs = yy_0_1_1_3_3_1;
yyv_OutArgs = yy_0_1_1_3_3_2;
yyv_Rules = yy_0_1_1_3_4;
return;
yyfl_248_6 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_P;
yy yy_0_1_1;
yy yyv_M;
yy yy_0_1_2;
yy yy_1_1;
yy yy_3_1;
yy yy_5_1;
yy yy_7_1;
yy yy_9_1;
yy yy_11_1;
yy yy_13_1;
yy yy_15_1;
yy yy_17_1_1;
yy yy_17_1_2_1;
yy yy_17_2_1_1;
yy yy_18_1;
yy yy_20_1_1;
yy yy_20_2_1;
yy yy_20_2_2_1;
yy yyv_RootId;
yy yy_20_2_2_2;
yy yy_20_2_3_1;
yy yy_20_2_3_1_1;
yy yy_20_2_3_1_2;
yy yy_20_2_3_1_3;
yy yy_20_2_3_1_3_1;
yy yy_20_2_3_1_3_2;
yy yy_20_2_3_1_3_3;
yy yy_20_2_3_1_3_3_1;
yy yy_20_2_3_1_3_3_2;
yy yy_20_2_3_1_3_4;
yy yy_20_2_3_1_3_4_1;
yy yy_20_2_3_1_3_4_1_1;
yy yy_20_2_3_1_3_4_1_1_1;
yy yy_20_2_3_1_3_4_1_1_2;
yy yy_20_2_3_1_3_4_1_1_3;
yy yy_20_2_3_1_3_4_1_1_3_1;
yy yy_20_2_3_1_3_4_1_1_4;
yy yy_20_2_3_1_3_4_1_1_4_1;
yy yy_20_2_3_1_3_4_1_2;
yy yy_20_2_3_1_3_4_1_3;
yy yy_20_2_3_1_3_4_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_248_7;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_P = yy_0_1_1;
yyv_M = yy_0_1_2;
yy_1_1 = ((yy)"int yyparse_rc = 0;");
s(yy_1_1);
nl();
/* --PATCH-- */ yy_3_1 = ((yy)"void ROOT()");
s(yy_3_1);
nl();
yy_5_1 = ((yy)"{");
s(yy_5_1);
nl();
yy_7_1 = ((yy)"extern char *THIS_RUNTIME_SYSTEM;");
s(yy_7_1);
nl();
yy_9_1 = ((yy)"char *GENTLE = \"Gentle 3.0 01100401 (C) 1992, 1997\";");
s(yy_9_1);
nl();
yy_11_1 = ((yy)"if (strcmp(THIS_RUNTIME_SYSTEM, GENTLE))");
s(yy_11_1);
nl();
yy_13_1 = ((yy)"{ printf(\"INCONSISTENT GENTLE RUNTIME SYSTEM\\n\"); exit(1); }");
s(yy_13_1);
nl();
yy_15_1 = ((yy)"yyExtend();");
s(yy_15_1);
nl();
{
yy yysb = yyb;
yy_17_1_1 = yyglov_containsGrammar;
if (yy_17_1_1 == (yy) yyu) yyErr(1,2307);
if (yy_17_1_1[0] != 1) goto yyfl_248_7_17_1;
yy_17_1_2_1 = ((yy)"yyparse_rc = yyparse();");
s(yy_17_1_2_1);
nl();
goto yysl_248_7_17;
yyfl_248_7_17_1 : ;
yy_17_2_1_1 = ((yy)"yygoal();");
s(yy_17_2_1_1);
nl();
goto yysl_248_7_17;
yysl_248_7_17 : ;
yyb = yysb;
}
yy_18_1 = ((yy)"}");
s(yy_18_1);
nl();
{
yy yysb = yyb;
yy_20_1_1 = yyglov_containsGrammar;
if (yy_20_1_1 == (yy) yyu) yyErr(1,2314);
if (yy_20_1_1[0] != 1) goto yyfl_248_7_20_1;
goto yysl_248_7_20;
yyfl_248_7_20_1 : ;
yyb = yyh;
yyh += 33; if (yyh > yyhx) yyExtend();
yy_20_2_1 = yyglov_containsGrammar;
if (yy_20_2_1 == (yy) yyu) yyErr(1,2317);
if (yy_20_2_1[0] != 2) goto yyfl_248_7_20_2;
yy_20_2_2_1 = ((yy)"yygoal");
string_to_id(yy_20_2_2_1, &yy_20_2_2_2);
yyv_RootId = yy_20_2_2_2;
yy_20_2_3_1_1 = yyv_RootId;
yy_20_2_3_1_2 = yyv_P;
yy_20_2_3_1_3_1 = yyb + 9;
yy_20_2_3_1_3_1[0] = 2;
yy_20_2_3_1_3_2 = yyb + 10;
yy_20_2_3_1_3_2[0] = 1;
yy_20_2_3_1_3_3_1 = yyb + 14;
yy_20_2_3_1_3_3_1[0] = 2;
yy_20_2_3_1_3_3_2 = yyb + 15;
yy_20_2_3_1_3_3_2[0] = 2;
yy_20_2_3_1_3_3 = yyb + 11;
yy_20_2_3_1_3_3[0] = 1;
yy_20_2_3_1_3_3[1] = ((long)yy_20_2_3_1_3_3_1);
yy_20_2_3_1_3_3[2] = ((long)yy_20_2_3_1_3_3_2);
yy_20_2_3_1_3_4_1_1_1 = yyv_RootId;
yy_20_2_3_1_3_4_1_1_2 = yyv_P;
yy_20_2_3_1_3_4_1_1_3_1 = yyv_P;
yy_20_2_3_1_3_4_1_1_3 = yyb + 28;
yy_20_2_3_1_3_4_1_1_3[0] = 2;
yy_20_2_3_1_3_4_1_1_3[1] = ((long)yy_20_2_3_1_3_4_1_1_3_1);
yy_20_2_3_1_3_4_1_1_4_1 = yyv_P;
yy_20_2_3_1_3_4_1_1_4 = yyb + 30;
yy_20_2_3_1_3_4_1_1_4[0] = 2;
yy_20_2_3_1_3_4_1_1_4[1] = ((long)yy_20_2_3_1_3_4_1_1_4_1);
yy_20_2_3_1_3_4_1_1 = yyb + 23;
yy_20_2_3_1_3_4_1_1[0] = 1;
yy_20_2_3_1_3_4_1_1[1] = ((long)yy_20_2_3_1_3_4_1_1_1);
yy_20_2_3_1_3_4_1_1[2] = ((long)yy_20_2_3_1_3_4_1_1_2);
yy_20_2_3_1_3_4_1_1[3] = ((long)yy_20_2_3_1_3_4_1_1_3);
yy_20_2_3_1_3_4_1_1[4] = ((long)yy_20_2_3_1_3_4_1_1_4);
yy_20_2_3_1_3_4_1_2 = yyv_M;
yy_20_2_3_1_3_4_1_3 = ((yy)0);
yy_20_2_3_1_3_4_1 = yyb + 19;
yy_20_2_3_1_3_4_1[0] = 1;
yy_20_2_3_1_3_4_1[1] = ((long)yy_20_2_3_1_3_4_1_1);
yy_20_2_3_1_3_4_1[2] = ((long)yy_20_2_3_1_3_4_1_2);
yy_20_2_3_1_3_4_1[3] = ((long)yy_20_2_3_1_3_4_1_3);
yy_20_2_3_1_3_4_2 = yyb + 32;
yy_20_2_3_1_3_4_2[0] = 2;
yy_20_2_3_1_3_4 = yyb + 16;
yy_20_2_3_1_3_4[0] = 1;
yy_20_2_3_1_3_4[1] = ((long)yy_20_2_3_1_3_4_1);
yy_20_2_3_1_3_4[2] = ((long)yy_20_2_3_1_3_4_2);
yy_20_2_3_1_3 = yyb + 4;
yy_20_2_3_1_3[0] = 4;
yy_20_2_3_1_3[1] = ((long)yy_20_2_3_1_3_1);
yy_20_2_3_1_3[2] = ((long)yy_20_2_3_1_3_2);
yy_20_2_3_1_3[3] = ((long)yy_20_2_3_1_3_3);
yy_20_2_3_1_3[4] = ((long)yy_20_2_3_1_3_4);
yy_20_2_3_1 = yyb + 0;
yy_20_2_3_1[0] = 2;
yy_20_2_3_1[1] = ((long)yy_20_2_3_1_1);
yy_20_2_3_1[2] = ((long)yy_20_2_3_1_2);
yy_20_2_3_1[3] = ((long)yy_20_2_3_1_3);
Code_DECLARATION(yy_20_2_3_1);
goto yysl_248_7_20;
yyfl_248_7_20_2 : ;
goto yyfl_248_7;
yysl_248_7_20 : ;
yyb = yysb;
}
return;
yyfl_248_7 : ;
}
{
yy yyb;
yy yyv_Other;
yy yy_0_1;
yy_0_1 = yyin_1;
yyv_Other = yy_0_1;
return;
yyfl_248_8 : ;
}
yyErr(2,2174);
}
Declare_broadcast_Procs(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_T;
yy yy_0_1_1_1;
yy yy_0_1_1_2;
yy yyv_Tail;
yy yy_0_1_2;
yy yy_1_1;
yy yy_2_1;
yy yy_3_1;
yy yy_5_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_252_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
if (yy_0_1_1[0] != 1) goto yyfl_252_1;
yy_0_1_1_1 = ((yy)yy_0_1_1[1]);
yy_0_1_1_2 = ((yy)yy_0_1_1[2]);
yyv_T = yy_0_1_1_1;
yyv_Tail = yy_0_1_2;
yy_1_1 = ((yy)"extern yybroadcast_");
s(yy_1_1);
yy_2_1 = yyv_T;
id(yy_2_1);
yy_3_1 = ((yy)"();");
s(yy_3_1);
nl();
yy_5_1 = yyv_Tail;
Declare_broadcast_Procs(yy_5_1);
return;
yyfl_252_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_252_2;
return;
yyfl_252_2 : ;
}
yyErr(2,2339);
}
CollectSweepRules(yyin_1, yyout_1)
yy yyin_1;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Rule;
yy yy_0_1_1;
yy yyv_Tail;
yy yy_0_1_2;
yy yy_0_2;
yy yy_1_1;
yy yyv_NextList;
yy yy_1_2;
yy yy_2_1;
yy yy_2_2;
yy yyv_NewList;
yy yy_2_3;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_253_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_Rule = yy_0_1_1;
yyv_Tail = yy_0_1_2;
yy_1_1 = yyv_Tail;
CollectSweepRules(yy_1_1, &yy_1_2);
yyv_NextList = yy_1_2;
yy_2_1 = yyv_Rule;
yy_2_2 = yyv_NextList;
InspectSweepRule(yy_2_1, yy_2_2, &yy_2_3);
yyv_NewList = yy_2_3;
yy_0_2 = yyv_NewList;
*yyout_1 = yy_0_2;
return;
yyfl_253_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_253_2;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_0_2 = yyb + 0;
yy_0_2[0] = 2;
*yyout_1 = yy_0_2;
return;
yyfl_253_2 : ;
}
yyErr(2,2345);
}
InspectSweepRule(yyin_1, yyin_2, yyout_1)
yy yyin_1;
yy yyin_2;
yy *yyout_1;
{
{
yy yyb;
yy yyv_Rule;
yy yy_0_1;
yy yyv_CurGroups;
yy yy_0_2;
yy yy_0_3;
yy yy_1_1;
yy yyv_Type;
yy yy_1_2;
yy yy_2_1_1_1;
yy yy_2_1_1_2;
yy yy_2_1_1_3;
yy yy_2_1_2_1;
yy yyv_NextGroups;
yy yy_2_1_2_2;
yy yyv_E;
yy yy_2_2_2;
yy yy_2_2_2_1;
yy yy_2_2_2_2;
yy yy_2_2_3_1;
yy yy_2_2_3_1_1;
yy yy_2_2_3_1_1_1;
yy yy_2_2_3_1_1_2;
yy yy_2_2_3_1_2;
yy yy_2_2_3_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Rule = yy_0_1;
yyv_CurGroups = yy_0_2;
yy_1_1 = yyv_Rule;
GetSweepType(yy_1_1, &yy_1_2);
yyv_Type = yy_1_2;
{
yy yysb = yyb;
yy_2_1_1_1 = yyv_Rule;
yy_2_1_1_2 = yyv_Type;
yy_2_1_1_3 = yyv_CurGroups;
if (! AddedSweepRule(yy_2_1_1_1, yy_2_1_1_2, yy_2_1_1_3)) goto yyfl_254_1_2_1;
yy_2_1_2_1 = yyv_CurGroups;
yy_2_1_2_2 = yy_2_1_2_1;
yyv_NextGroups = yy_2_1_2_2;
goto yysl_254_1_2;
yyfl_254_1_2_1 : ;
yyb = yyh;
yyh += 12; if (yyh > yyhx) yyExtend();
yyv_E = yyb + 0;
yyb[1] = yyu;
yy_2_2_2_1 = yyv_Rule;
yy_2_2_2_2 = yyb + 5;
yy_2_2_2_2[0] = 2;
yy_2_2_2 = yyb + 2;
yy_2_2_2[0] = 1;
yy_2_2_2[1] = ((long)yy_2_2_2_1);
yy_2_2_2[2] = ((long)yy_2_2_2_2);
yyv_E[1] = (long) yy_2_2_2;
yy_2_2_3_1_1_1 = yyv_Type;
yy_2_2_3_1_1_2 = yyv_E;
yy_2_2_3_1_1 = yyb + 9;
yy_2_2_3_1_1[0] = 1;
yy_2_2_3_1_1[1] = ((long)yy_2_2_3_1_1_1);
yy_2_2_3_1_1[2] = ((long)yy_2_2_3_1_1_2);
yy_2_2_3_1_2 = yyv_CurGroups;
yy_2_2_3_1 = yyb + 6;
yy_2_2_3_1[0] = 1;
yy_2_2_3_1[1] = ((long)yy_2_2_3_1_1);
yy_2_2_3_1[2] = ((long)yy_2_2_3_1_2);
yy_2_2_3_2 = yy_2_2_3_1;
yyv_NextGroups = yy_2_2_3_2;
goto yysl_254_1_2;
yysl_254_1_2 : ;
yyb = yysb;
}
yy_0_3 = yyv_NextGroups;
*yyout_1 = yy_0_3;
return;
}
}
GetSweepType(yyin_1, yyout_1)
yy yyin_1;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_1_1;
yy yyv_Pos;
yy yy_0_1_1_2;
yy yyv_In;
yy yy_0_1_1_3;
yy yyv_Out;
yy yy_0_1_1_4;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_2;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yyv_Head;
yy yy_1_1_1_2_1;
yy yy_1_1_1_2_2;
yy yy_1_1_1_2_3;
yy yy_1_1_2_1_1_1_1_1;
yy yy_1_1_2_1_1_1_1_2;
yy yy_1_1_2_1_1_1_1_2_1;
yy yy_1_1_2_1_1_1_1_2_2;
yy yy_1_1_2_1_1_1_1_2_3;
yy yy_1_1_2_1_1_1_1_2_4;
yy yy_1_1_2_1_1_1_1_2_4_1;
yy yyv_Functor;
yy yy_1_1_2_1_1_1_1_2_4_2;
yy yyv_PosF;
yy yy_1_1_2_1_1_1_1_2_4_3;
yy yy_1_1_2_1_1_1_1_2_4_4;
yy yy_1_1_2_1_1_2_1_1;
yy yy_1_1_2_1_1_2_1_2;
yy yy_1_1_2_1_1_2_1_2_1;
yy yy_1_1_2_1_1_2_1_2_2;
yy yy_1_1_2_1_1_2_1_2_3;
yy yy_1_1_2_1_1_2_1_2_4;
yy yy_1_1_2_1_2_1_1_1;
yy yy_1_1_2_1_2_1_1_2;
yy yyv_Type;
yy yy_1_1_2_1_2_1_1_2_1;
yy yy_1_1_2_1_2_1_1_2_2;
yy yy_1_1_2_1_2_2_1_1;
yy yy_1_1_2_1_2_2_1_2;
yy yy_1_1_2_1_2_2_1_2_1;
yy yyv_Others;
yy yy_1_1_2_1_2_2_1_2_2;
yy yy_1_1_2_1_2_2_2_1;
yy yy_1_1_2_1_2_2_2_2;
yy yy_1_1_2_1_2_2_2_3;
yy yy_1_1_2_1_2_2_2_4;
yy yy_1_1_2_1_2_3_1_1;
yy yy_1_1_2_1_2_3_1_2;
yy yy_1_1_2_1_2_3_1_3;
yy yy_1_1_2_1_2_3_1_4;
yy yy_1_1_2_1_2_3_2;
yy yy_1_1_2_2_1_1;
yy yy_1_1_2_2_1_2;
yy yy_1_1_2_2_1_2_1;
yy yy_1_1_2_2_1_2_2;
yy yy_1_1_2_2_1_2_3;
yy yy_1_1_2_2_1_2_4;
yy yy_1_1_2_2_1_2_5;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_1_2_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_255_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
if (yy_0_1_1[0] != 1) goto yyfl_255_1;
yy_0_1_1_1 = ((yy)yy_0_1_1[1]);
yy_0_1_1_2 = ((yy)yy_0_1_1[2]);
yy_0_1_1_3 = ((yy)yy_0_1_1[3]);
yy_0_1_1_4 = ((yy)yy_0_1_1[4]);
yyv_Pos = yy_0_1_1_2;
yyv_In = yy_0_1_1_3;
yyv_Out = yy_0_1_1_4;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_In;
yy_1_1_1_2 = yy_1_1_1_1;
if (yy_1_1_1_2[0] != 1) goto yyfl_255_1_1_1;
yy_1_1_1_2_1 = ((yy)yy_1_1_1_2[1]);
yy_1_1_1_2_2 = ((yy)yy_1_1_1_2[2]);
yy_1_1_1_2_3 = ((yy)yy_1_1_1_2[3]);
yyv_Head = yy_1_1_1_2_1;
{
yy yysb = yyb;
{
yy yysb = yyb;
yy_1_1_2_1_1_1_1_1 = yyv_Head;
yy_1_1_2_1_1_1_1_2 = yy_1_1_2_1_1_1_1_1;
if (yy_1_1_2_1_1_1_1_2[0] != 4) goto yyfl_255_1_1_1_2_1_1_1;
yy_1_1_2_1_1_1_1_2_1 = ((yy)yy_1_1_2_1_1_1_1_2[1]);
yy_1_1_2_1_1_1_1_2_2 = ((yy)yy_1_1_2_1_1_1_1_2[2]);
yy_1_1_2_1_1_1_1_2_3 = ((yy)yy_1_1_2_1_1_1_1_2[3]);
yy_1_1_2_1_1_1_1_2_4 = ((yy)yy_1_1_2_1_1_1_1_2[4]);
if (yy_1_1_2_1_1_1_1_2_4[0] != 1) goto yyfl_255_1_1_1_2_1_1_1;
yy_1_1_2_1_1_1_1_2_4_1 = ((yy)yy_1_1_2_1_1_1_1_2_4[1]);
yy_1_1_2_1_1_1_1_2_4_2 = ((yy)yy_1_1_2_1_1_1_1_2_4[2]);
yy_1_1_2_1_1_1_1_2_4_3 = ((yy)yy_1_1_2_1_1_1_1_2_4[3]);
yy_1_1_2_1_1_1_1_2_4_4 = ((yy)yy_1_1_2_1_1_1_1_2_4[4]);
yyv_Functor = yy_1_1_2_1_1_1_1_2_4_2;
yyv_PosF = yy_1_1_2_1_1_1_1_2_4_3;
goto yysl_255_1_1_1_2_1_1;
yyfl_255_1_1_1_2_1_1_1 : ;
yy_1_1_2_1_1_2_1_1 = yyv_Head;
yy_1_1_2_1_1_2_1_2 = yy_1_1_2_1_1_2_1_1;
if (yy_1_1_2_1_1_2_1_2[0] != 1) goto yyfl_255_1_1_1_2_1_1_2;
yy_1_1_2_1_1_2_1_2_1 = ((yy)yy_1_1_2_1_1_2_1_2[1]);
yy_1_1_2_1_1_2_1_2_2 = ((yy)yy_1_1_2_1_1_2_1_2[2]);
yy_1_1_2_1_1_2_1_2_3 = ((yy)yy_1_1_2_1_1_2_1_2[3]);
yy_1_1_2_1_1_2_1_2_4 = ((yy)yy_1_1_2_1_1_2_1_2[4]);
yyv_Functor = yy_1_1_2_1_1_2_1_2_2;
yyv_PosF = yy_1_1_2_1_1_2_1_2_3;
goto yysl_255_1_1_1_2_1_1;
yyfl_255_1_1_1_2_1_1_2 : ;
goto yyfl_255_1_1_1_2_1;
yysl_255_1_1_1_2_1_1 : ;
yyb = yysb;
}
{
yy yysb = yyb;
yy_1_1_2_1_2_1_1_1 = yyv_Functor;
if (! GetFunctorMeaning(yy_1_1_2_1_2_1_1_1, &yy_1_1_2_1_2_1_1_2)) goto yyfl_255_1_1_1_2_1_2_1;
if (yy_1_1_2_1_2_1_1_2[0] != 1) goto yyfl_255_1_1_1_2_1_2_1;
yy_1_1_2_1_2_1_1_2_1 = ((yy)yy_1_1_2_1_2_1_1_2[1]);
yy_1_1_2_1_2_1_1_2_2 = ((yy)yy_1_1_2_1_2_1_1_2[2]);
yyv_Type = yy_1_1_2_1_2_1_1_2_1;
if (yy_1_1_2_1_2_1_1_2_2[0] != 2) goto yyfl_255_1_1_1_2_1_2_1;
goto yysl_255_1_1_1_2_1_2;
yyfl_255_1_1_1_2_1_2_1 : ;
yy_1_1_2_1_2_2_1_1 = yyv_Functor;
if (! GetFunctorMeaning(yy_1_1_2_1_2_2_1_1, &yy_1_1_2_1_2_2_1_2)) goto yyfl_255_1_1_1_2_1_2_2;
if (yy_1_1_2_1_2_2_1_2[0] != 1) goto yyfl_255_1_1_1_2_1_2_2;
yy_1_1_2_1_2_2_1_2_1 = ((yy)yy_1_1_2_1_2_2_1_2[1]);
yy_1_1_2_1_2_2_1_2_2 = ((yy)yy_1_1_2_1_2_2_1_2[2]);
yyv_Type = yy_1_1_2_1_2_2_1_2_1;
yyv_Others = yy_1_1_2_1_2_2_1_2_2;
yy_1_1_2_1_2_2_2_1 = ((yy)"ambigous '");
yy_1_1_2_1_2_2_2_2 = yyv_Functor;
yy_1_1_2_1_2_2_2_3 = ((yy)"' requires type prefix");
yy_1_1_2_1_2_2_2_4 = yyv_PosF;
MESSAGE1(yy_1_1_2_1_2_2_2_1, yy_1_1_2_1_2_2_2_2, yy_1_1_2_1_2_2_2_3, yy_1_1_2_1_2_2_2_4);
goto yysl_255_1_1_1_2_1_2;
yyfl_255_1_1_1_2_1_2_2 : ;
yy_1_1_2_1_2_3_1_1 = ((yy)"unknown functor '");
yy_1_1_2_1_2_3_1_2 = yyv_Functor;
yy_1_1_2_1_2_3_1_3 = ((yy)"'");
yy_1_1_2_1_2_3_1_4 = yyv_PosF;
MESSAGE1(yy_1_1_2_1_2_3_1_1, yy_1_1_2_1_2_3_1_2, yy_1_1_2_1_2_3_1_3, yy_1_1_2_1_2_3_1_4);
yy_1_1_2_1_2_3_2 = yyglov_id_INT;
if (yy_1_1_2_1_2_3_2 == (yy) yyu) yyErr(1,2384);
yyv_Type = yy_1_1_2_1_2_3_2;
goto yysl_255_1_1_1_2_1_2;
yysl_255_1_1_1_2_1_2 : ;
yyb = yysb;
}
goto yysl_255_1_1_1_2;
yyfl_255_1_1_1_2_1 : ;
yy_1_1_2_2_1_1 = yyv_Head;
yy_1_1_2_2_1_2 = yy_1_1_2_2_1_1;
if (yy_1_1_2_2_1_2[0] != 2) goto yyfl_255_1_1_1_2_2;
yy_1_1_2_2_1_2_1 = ((yy)yy_1_1_2_2_1_2[1]);
yy_1_1_2_2_1_2_2 = ((yy)yy_1_1_2_2_1_2[2]);
yy_1_1_2_2_1_2_3 = ((yy)yy_1_1_2_2_1_2[3]);
yy_1_1_2_2_1_2_4 = ((yy)yy_1_1_2_2_1_2[4]);
yy_1_1_2_2_1_2_5 = ((yy)yy_1_1_2_2_1_2[5]);
yyv_Type = yy_1_1_2_2_1_2_1;
yyv_Functor = yy_1_1_2_2_1_2_3;
yyv_PosF = yy_1_1_2_2_1_2_4;
goto yysl_255_1_1_1_2;
yyfl_255_1_1_1_2_2 : ;
goto yyfl_255_1_1_1;
yysl_255_1_1_1_2 : ;
yyb = yysb;
}
goto yysl_255_1_1;
yyfl_255_1_1_1 : ;
yy_1_2_1_1 = ((yy)"invalid primary argument for sweep rule");
yy_1_2_1_2 = yyv_Pos;
MESSAGE(yy_1_2_1_1, yy_1_2_1_2);
yy_1_2_2 = yyglov_id_INT;
if (yy_1_2_2 == (yy) yyu) yyErr(1,2391);
yyv_Type = yy_1_2_2;
goto yysl_255_1_1;
yysl_255_1_1 : ;
yyb = yysb;
}
yy_0_2 = yyv_Type;
*yyout_1 = yy_0_2;
return;
yyfl_255_1 : ;
}
yyErr(2,2367);
}
int AddedSweepRule(yyin_1, yyin_2, yyin_3)
yy yyin_1;
yy yyin_2;
yy yyin_3;
{
{
yy yyb;
yy yyv_Rule;
yy yy_0_1;
yy yyv_Type;
yy yy_0_2;
yy yy_0_3;
yy yyv_Head;
yy yy_0_3_1;
yy yyv_Tail;
yy yy_0_3_2;
yy yy_1_1;
yy yy_1_2;
yy yyv_GroupType;
yy yy_1_2_1;
yy yyv_RuleIndex;
yy yy_1_2_2;
yy yy_2_1_1_1;
yy yy_2_1_1_2;
yy yyv_R;
yy yy_2_1_2;
yy yy_2_1_3;
yy yy_2_1_3_1;
yy yy_2_1_3_2;
yy yy_2_2_1_1;
yy yy_2_2_1_2;
yy yy_2_2_1_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Rule = yy_0_1;
yyv_Type = yy_0_2;
if (yy_0_3[0] != 1) goto yyfl_256_1;
yy_0_3_1 = ((yy)yy_0_3[1]);
yy_0_3_2 = ((yy)yy_0_3[2]);
yyv_Head = yy_0_3_1;
yyv_Tail = yy_0_3_2;
yy_1_1 = yyv_Head;
yy_1_2 = yy_1_1;
if (yy_1_2[0] != 1) goto yyfl_256_1;
yy_1_2_1 = ((yy)yy_1_2[1]);
yy_1_2_2 = ((yy)yy_1_2[2]);
yyv_GroupType = yy_1_2_1;
yyv_RuleIndex = yy_1_2_2;
{
yy yysb = yyb;
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yy_2_1_1_1 = yyv_Type;
yy_2_1_1_2 = yyv_GroupType;
if (! yyeq_IDENT(yy_2_1_1_1, yy_2_1_1_2)) goto yyfl_256_1_2_1;
yy_2_1_2 = (yy) yyv_RuleIndex[1];
if (yy_2_1_2 == (yy) yyu) yyErr(1,2400);
yyv_R = yy_2_1_2;
yy_2_1_3_1 = yyv_Rule;
yy_2_1_3_2 = yyv_R;
yy_2_1_3 = yyb + 0;
yy_2_1_3[0] = 1;
yy_2_1_3[1] = ((long)yy_2_1_3_1);
yy_2_1_3[2] = ((long)yy_2_1_3_2);
yyv_RuleIndex[1] = (long) yy_2_1_3;
goto yysl_256_1_2;
yyfl_256_1_2_1 : ;
yy_2_2_1_1 = yyv_Rule;
yy_2_2_1_2 = yyv_Type;
yy_2_2_1_3 = yyv_Tail;
if (! AddedSweepRule(yy_2_2_1_1, yy_2_2_1_2, yy_2_2_1_3)) goto yyfl_256_1_2_2;
goto yysl_256_1_2;
yyfl_256_1_2_2 : ;
goto yyfl_256_1;
yysl_256_1_2 : ;
yyb = yysb;
}
return 1;
yyfl_256_1 : ;
}
return 0;
}
Code_RuleGroups(yyin_1, yyin_2, yyin_3, yyin_4, yyin_5)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy yyin_4;
yy yyin_5;
{
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_Type;
yy yy_0_1_1_1;
yy yyv_RuleIndex;
yy yy_0_1_1_2;
yy yyv_Tail;
yy yy_0_1_2;
yy yyv_Pred;
yy yy_0_2;
yy yyv_InArgs;
yy yy_0_3;
yy yyv_OutArgs;
yy yy_0_4;
yy yyv_N;
yy yy_0_5;
yy yy_1_1;
yy yy_2_1;
yy yy_3_1;
yy yy_5_1_1_1;
yy yy_5_1_1_2;
yy yy_5_1_2_1;
yy yy_5_2_1_1;
yy yy_6_1;
yy yy_7_1;
yy yy_8_1;
yy yyv_R;
yy yy_10;
yy yy_11_1;
yy yy_11_2;
yy yy_11_3;
yy yy_11_4;
yy yy_11_5;
yy yy_11_6;
yy yy_11_6_1;
yy yy_11_6_1_1;
yy yy_11_6_1_2;
yy yy_11_6_2;
yy yyv_MayFail;
yy yy_11_7;
yy yy_12_1_1_1;
yy yy_12_1_1_2;
yy yy_12_1_2_1;
yy yy_13_1;
yy yy_15_1;
yy yy_17_1;
yy yy_17_2;
yy yy_17_3;
yy yy_17_4;
yy yy_17_5;
yy yy_17_5_1;
yy yy_17_5_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
yy_0_5 = yyin_5;
if (yy_0_1[0] != 1) goto yyfl_257_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
if (yy_0_1_1[0] != 1) goto yyfl_257_1;
yy_0_1_1_1 = ((yy)yy_0_1_1[1]);
yy_0_1_1_2 = ((yy)yy_0_1_1[2]);
yyv_Type = yy_0_1_1_1;
yyv_RuleIndex = yy_0_1_1_2;
yyv_Tail = yy_0_1_2;
yyv_Pred = yy_0_2;
yyv_InArgs = yy_0_3;
yyv_OutArgs = yy_0_4;
yyv_N = yy_0_5;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_1_1 = ((yy)"/*=== Sweep ");
s(yy_1_1);
yy_2_1 = yyv_Type;
id(yy_2_1);
yy_3_1 = ((yy)" ===*/");
s(yy_3_1);
nl();
{
yy yysb = yyb;
yy_5_1_1_1 = yyv_N;
yy_5_1_1_2 = ((yy)1);
if ((long)yy_5_1_1_1 != (long)yy_5_1_1_2) goto yyfl_257_1_5_1;
yy_5_1_2_1 = ((yy)"if ");
s(yy_5_1_2_1);
goto yysl_257_1_5;
yyfl_257_1_5_1 : ;
yy_5_2_1_1 = ((yy)"else if ");
s(yy_5_2_1_1);
goto yysl_257_1_5;
yysl_257_1_5 : ;
yyb = yysb;
}
yy_6_1 = ((yy)"(yytp == (intptr_t) yybroadcast_");
s(yy_6_1);
yy_7_1 = yyv_Type;
id(yy_7_1);
yy_8_1 = ((yy)") {");
s(yy_8_1);
nl();
yy_10 = (yy) yyv_RuleIndex[1];
if (yy_10 == (yy) yyu) yyErr(1,2413);
yyv_R = yy_10;
yy_11_1 = yyb + 0;
yy_11_1[0] = 6;
yy_11_2 = yyv_Pred;
yy_11_3 = yyv_InArgs;
yy_11_4 = yyv_OutArgs;
yy_11_5 = yyv_R;
yy_11_6_1_1 = yyv_N;
yy_11_6_1_2 = ((yy)10000);
yy_11_6_1 = (yy)(((long)yy_11_6_1_1)*((long)yy_11_6_1_2));
yy_11_6_2 = ((yy)1);
yy_11_6 = (yy)(((long)yy_11_6_1)+((long)yy_11_6_2));
Code_Rules(yy_11_1, yy_11_2, yy_11_3, yy_11_4, yy_11_5, yy_11_6, &yy_11_7);
yyv_MayFail = yy_11_7;
{
yy yysb = yyb;
yy_12_1_1_1 = yyv_MayFail;
yy_12_1_1_2 = yy_12_1_1_1;
if (yy_12_1_1_2[0] != 1) goto yyfl_257_1_12_1;
yy_12_1_2_1 = ((yy)"return 0;");
s(yy_12_1_2_1);
nl();
goto yysl_257_1_12;
yyfl_257_1_12_1 : ;
goto yysl_257_1_12;
yysl_257_1_12 : ;
yyb = yysb;
}
yy_13_1 = ((yy)"}");
s(yy_13_1);
nl();
yy_15_1 = ((yy)"/*===*/");
s(yy_15_1);
nl();
yy_17_1 = yyv_Tail;
yy_17_2 = yyv_Pred;
yy_17_3 = yyv_InArgs;
yy_17_4 = yyv_OutArgs;
yy_17_5_1 = yyv_N;
yy_17_5_2 = ((yy)1);
yy_17_5 = (yy)(((long)yy_17_5_1)+((long)yy_17_5_2));
Code_RuleGroups(yy_17_1, yy_17_2, yy_17_3, yy_17_4, yy_17_5);
return;
yyfl_257_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Pred;
yy yy_0_2;
yy yyv_InArgs;
yy yy_0_3;
yy yyv_OutArgs;
yy yy_0_4;
yy yyv_N;
yy yy_0_5;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
yy_0_5 = yyin_5;
if (yy_0_1[0] != 2) goto yyfl_257_2;
yyv_Pred = yy_0_2;
yyv_InArgs = yy_0_3;
yyv_OutArgs = yy_0_4;
yyv_N = yy_0_5;
return;
yyfl_257_2 : ;
}
yyErr(2,2406);
}
Check_UniqueIds(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_Id;
yy yy_0_1_1_1;
yy yy_0_1_1_2;
yy yyv_Pos;
yy yy_0_1_1_3;
yy yyv_Tail;
yy yy_0_1_2;
yy yyv_Inh;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yy_2_1;
yy yy_2_2;
yy yy_2_3;
yy yy_3_1;
yy yy_3_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_258_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
if (yy_0_1_1[0] != 1) goto yyfl_258_1;
yy_0_1_1_1 = ((yy)yy_0_1_1[1]);
yy_0_1_1_2 = ((yy)yy_0_1_1[2]);
yy_0_1_1_3 = ((yy)yy_0_1_1[3]);
yyv_Id = yy_0_1_1_1;
yyv_Pos = yy_0_1_1_3;
yyv_Tail = yy_0_1_2;
yyv_Inh = yy_0_2;
yy_1_1 = yyv_Id;
yy_1_2 = yyv_Tail;
yy_1_3 = yyv_Pos;
Check_UniqueId(yy_1_1, yy_1_2, yy_1_3);
yy_2_1 = yyv_Id;
yy_2_2 = yyv_Inh;
yy_2_3 = yyv_Pos;
Check_UniqueId(yy_2_1, yy_2_2, yy_2_3);
yy_3_1 = yyv_Tail;
yy_3_2 = yyv_Inh;
Check_UniqueIds(yy_3_1, yy_3_2);
return;
yyfl_258_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_258_2;
return;
yyfl_258_2 : ;
}
yyErr(2,2427);
}
Check_UniqueId(yyin_1, yyin_2, yyin_3)
yy yyin_1;
yy yyin_2;
yy yyin_3;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yy_0_2;
yy yy_0_2_1;
yy yyv_Id2;
yy yy_0_2_1_1;
yy yy_0_2_1_2;
yy yy_0_2_1_3;
yy yyv_Tail;
yy yy_0_2_2;
yy yyv_P;
yy yy_0_3;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yy_1_1_2_1;
yy yy_1_1_2_2;
yy yy_1_1_2_3;
yy yy_1_1_2_4;
yy yy_2_1;
yy yy_2_2;
yy yy_2_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Id = yy_0_1;
if (yy_0_2[0] != 1) goto yyfl_259_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
if (yy_0_2_1[0] != 1) goto yyfl_259_1;
yy_0_2_1_1 = ((yy)yy_0_2_1[1]);
yy_0_2_1_2 = ((yy)yy_0_2_1[2]);
yy_0_2_1_3 = ((yy)yy_0_2_1[3]);
yyv_Id2 = yy_0_2_1_1;
yyv_Tail = yy_0_2_2;
yyv_P = yy_0_3;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Id;
yy_1_1_1_2 = yyv_Id2;
if (! yyeq_IDENT(yy_1_1_1_1, yy_1_1_1_2)) goto yyfl_259_1_1_1;
yy_1_1_2_1 = ((yy)"'");
yy_1_1_2_2 = yyv_Id;
yy_1_1_2_3 = ((yy)"' appears more than once");
yy_1_1_2_4 = yyv_P;
MESSAGE1(yy_1_1_2_1, yy_1_1_2_2, yy_1_1_2_3, yy_1_1_2_4);
goto yysl_259_1_1;
yyfl_259_1_1_1 : ;
goto yysl_259_1_1;
yysl_259_1_1 : ;
yyb = yysb;
}
yy_2_1 = yyv_Id;
yy_2_2 = yyv_Tail;
yy_2_3 = yyv_P;
Check_UniqueId(yy_2_1, yy_2_2, yy_2_3);
return;
yyfl_259_1 : ;
}
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yy_0_2;
yy yyv_Pos;
yy yy_0_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Id = yy_0_1;
if (yy_0_2[0] != 2) goto yyfl_259_2;
yyv_Pos = yy_0_3;
return;
yyfl_259_2 : ;
}
yyErr(2,2434);
}
CheckFunctors(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_T;
yy yy_0_1_2;
yy yy_1_1;
yy yy_1_2;
yy yy_2_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_260_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_H = yy_0_1_1;
yyv_T = yy_0_1_2;
yy_1_1 = yyv_H;
yy_1_2 = yyv_T;
CheckFunctor(yy_1_1, yy_1_2);
yy_2_1 = yyv_T;
CheckFunctors(yy_2_1);
return;
yyfl_260_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_260_2;
return;
yyfl_260_2 : ;
}
yyErr(2,2444);
}
CheckFunctor(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Id;
yy yy_0_1_1;
yy yyv_Pos;
yy yy_0_1_2;
yy yyv_Args;
yy yy_0_1_3;
yy yyv_List;
yy yy_0_2;
yy yy_1_1;
yy yy_2_1;
yy yy_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_261_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Id = yy_0_1_1;
yyv_Pos = yy_0_1_2;
yyv_Args = yy_0_1_3;
yyv_List = yy_0_2;
yy_1_1 = yyv_Args;
CheckFunctorArgs(yy_1_1);
yy_2_1 = yyv_Id;
yy_2_2 = yyv_List;
CheckIdNotInFunctorSpecs(yy_2_1, yy_2_2);
return;
yyfl_261_1 : ;
}
yyErr(2,2451);
}
CheckFunctorArgs(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_T;
yy yy_0_1_3;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yy_2_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_262_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_H = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_T = yy_0_1_3;
yy_1_1 = yyv_H;
yy_1_2 = yyv_T;
yy_1_3 = yyv_P;
CheckFunctorArg(yy_1_1, yy_1_2, yy_1_3);
yy_2_1 = yyv_T;
CheckFunctorArgs(yy_2_1);
return;
yyfl_262_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_262_2;
return;
yyfl_262_2 : ;
}
yyErr(2,2457);
}
CheckFunctorArg(yyin_1, yyin_2, yyin_3)
yy yyin_1;
yy yyin_2;
yy yyin_3;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Name;
yy yy_0_1_1;
yy yyv_Type;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_Tail;
yy yy_0_2;
yy yyv_P;
yy yy_0_3;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yy_2_1;
yy yy_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 1) goto yyfl_263_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Name = yy_0_1_1;
yyv_Type = yy_0_1_2;
yyv_Tail = yy_0_2;
yyv_P = yy_0_3;
yy_1_1 = yyv_Name;
yy_1_2 = yyv_Tail;
yy_1_3 = yyv_P;
CheckUniqueArgName(yy_1_1, yy_1_2, yy_1_3);
yy_2_1 = yyv_Type;
yy_2_2 = yyv_P;
CheckType(yy_2_1, yy_2_2);
return;
yyfl_263_1 : ;
}
yyErr(2,2465);
}
CheckUniqueArgName(yyin_1, yyin_2, yyin_3)
yy yyin_1;
yy yyin_2;
yy yyin_3;
{
{
yy yyb;
yy yyv_Name1;
yy yy_0_1;
yy yy_0_2;
yy yy_0_2_1;
yy yyv_Name2;
yy yy_0_2_1_1;
yy yyv_Type;
yy yy_0_2_1_2;
yy yy_0_2_1_3;
yy yyv_P;
yy yy_0_2_2;
yy yyv_Tail;
yy yy_0_2_3;
yy yyv_Pos;
yy yy_0_3;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yyv_Id1;
yy yy_1_1_1_2_1;
yy yy_1_1_2_1;
yy yy_1_1_2_2;
yy yyv_Id2;
yy yy_1_1_2_2_1;
yy yy_1_1_3_1;
yy yy_1_1_3_2;
yy yy_1_1_4_1;
yy yy_1_1_4_2;
yy yy_1_1_4_3;
yy yy_1_1_4_4;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_1_2_1_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Name1 = yy_0_1;
if (yy_0_2[0] != 1) goto yyfl_264_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yy_0_2_3 = ((yy)yy_0_2[3]);
if (yy_0_2_1[0] != 1) goto yyfl_264_1;
yy_0_2_1_1 = ((yy)yy_0_2_1[1]);
yy_0_2_1_2 = ((yy)yy_0_2_1[2]);
yy_0_2_1_3 = ((yy)yy_0_2_1[3]);
yyv_Name2 = yy_0_2_1_1;
yyv_Type = yy_0_2_1_2;
yyv_P = yy_0_2_2;
yyv_Tail = yy_0_2_3;
yyv_Pos = yy_0_3;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Name1;
yy_1_1_1_2 = yy_1_1_1_1;
if (yy_1_1_1_2[0] != 1) goto yyfl_264_1_1_1;
yy_1_1_1_2_1 = ((yy)yy_1_1_1_2[1]);
yyv_Id1 = yy_1_1_1_2_1;
yy_1_1_2_1 = yyv_Name2;
yy_1_1_2_2 = yy_1_1_2_1;
if (yy_1_1_2_2[0] != 1) goto yyfl_264_1_1_1;
yy_1_1_2_2_1 = ((yy)yy_1_1_2_2[1]);
yyv_Id2 = yy_1_1_2_2_1;
yy_1_1_3_1 = yyv_Id1;
yy_1_1_3_2 = yyv_Id2;
if (! yyeq_IDENT(yy_1_1_3_1, yy_1_1_3_2)) goto yyfl_264_1_1_1;
yy_1_1_4_1 = ((yy)"'");
yy_1_1_4_2 = yyv_Id1;
yy_1_1_4_3 = ((yy)"' used more than once");
yy_1_1_4_4 = yyv_Pos;
MESSAGE1(yy_1_1_4_1, yy_1_1_4_2, yy_1_1_4_3, yy_1_1_4_4);
goto yysl_264_1_1;
yyfl_264_1_1_1 : ;
yy_1_2_1_1 = yyv_Name1;
yy_1_2_1_2 = yyv_Tail;
yy_1_2_1_3 = yyv_Pos;
CheckUniqueArgName(yy_1_2_1_1, yy_1_2_1_2, yy_1_2_1_3);
goto yysl_264_1_1;
yysl_264_1_1 : ;
yyb = yysb;
}
return;
yyfl_264_1 : ;
}
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yy_0_2;
yy yyv_Pos;
yy yy_0_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Id = yy_0_1;
if (yy_0_2[0] != 2) goto yyfl_264_2;
yyv_Pos = yy_0_3;
return;
yyfl_264_2 : ;
}
yyErr(2,2471);
}
CheckIdNotInFunctorSpecs(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yy_0_2;
yy yy_0_2_1;
yy yyv_Id2;
yy yy_0_2_1_1;
yy yyv_Pos;
yy yy_0_2_1_2;
yy yyv_Args;
yy yy_0_2_1_3;
yy yyv_Tail;
yy yy_0_2_2;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yy_2_1;
yy yy_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Id = yy_0_1;
if (yy_0_2[0] != 1) goto yyfl_265_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
if (yy_0_2_1[0] != 1) goto yyfl_265_1;
yy_0_2_1_1 = ((yy)yy_0_2_1[1]);
yy_0_2_1_2 = ((yy)yy_0_2_1[2]);
yy_0_2_1_3 = ((yy)yy_0_2_1[3]);
yyv_Id2 = yy_0_2_1_1;
yyv_Pos = yy_0_2_1_2;
yyv_Args = yy_0_2_1_3;
yyv_Tail = yy_0_2_2;
yy_1_1 = yyv_Id;
yy_1_2 = yyv_Id2;
yy_1_3 = yyv_Pos;
CheckUniqueFunctor(yy_1_1, yy_1_2, yy_1_3);
yy_2_1 = yyv_Id;
yy_2_2 = yyv_Tail;
CheckIdNotInFunctorSpecs(yy_2_1, yy_2_2);
return;
yyfl_265_1 : ;
}
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Id = yy_0_1;
if (yy_0_2[0] != 2) goto yyfl_265_2;
return;
yyfl_265_2 : ;
}
yyErr(2,2483);
}
CheckUniqueFunctor(yyin_1, yyin_2, yyin_3)
yy yyin_1;
yy yyin_2;
yy yyin_3;
{
{
yy yyb;
yy yyv_Id1;
yy yy_0_1;
yy yyv_Id2;
yy yy_0_2;
yy yyv_Pos;
yy yy_0_3;
yy yy_1_1;
yy yy_1_2;
yy yy_2_1;
yy yy_2_2;
yy yy_2_3;
yy yy_2_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Id1 = yy_0_1;
yyv_Id2 = yy_0_2;
yyv_Pos = yy_0_3;
yy_1_1 = yyv_Id1;
yy_1_2 = yyv_Id2;
if (! yyeq_IDENT(yy_1_1, yy_1_2)) goto yyfl_266_1;
yy_2_1 = ((yy)"Functor '");
yy_2_2 = yyv_Id1;
yy_2_3 = ((yy)"' defined twice");
yy_2_4 = yyv_Pos;
MESSAGE1(yy_2_1, yy_2_2, yy_2_3, yy_2_4);
return;
yyfl_266_1 : ;
}
{
yy yyb;
yy yyv_Id1;
yy yy_0_1;
yy yyv_Id2;
yy yy_0_2;
yy yyv_Pos;
yy yy_0_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Id1 = yy_0_1;
yyv_Id2 = yy_0_2;
yyv_Pos = yy_0_3;
return;
yyfl_266_2 : ;
}
yyErr(2,2490);
}
int ProcOrCond(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_267_1;
return 1;
yyfl_267_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_267_2;
return 1;
yyfl_267_2 : ;
}
return 0;
}
CheckFormalParams(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yyv_InArgs;
yy yy_0_1;
yy yyv_OutArgs;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy yy_2_1;
yy yy_3_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_InArgs = yy_0_1;
yyv_OutArgs = yy_0_2;
yy_1_1 = yyv_InArgs;
yy_1_2 = yyv_OutArgs;
CheckUniqueParams(yy_1_1, yy_1_2);
yy_2_1 = yyv_InArgs;
CheckFormalParamList(yy_2_1);
yy_3_1 = yyv_OutArgs;
CheckFormalParamList(yy_3_1);
return;
}
}
CheckFormalParamList(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_T;
yy yy_0_1_3;
yy yy_1_1;
yy yy_1_2;
yy yy_2_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_269_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_H = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_T = yy_0_1_3;
yy_1_1 = yyv_H;
yy_1_2 = yyv_P;
CheckFormalParam(yy_1_1, yy_1_2);
yy_2_1 = yyv_T;
CheckFormalParamList(yy_2_1);
return;
yyfl_269_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_269_2;
return;
yyfl_269_2 : ;
}
yyErr(2,2511);
}
CheckFormalParam(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Name;
yy yy_0_1_1;
yy yyv_T;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_P;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_270_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Name = yy_0_1_1;
yyv_T = yy_0_1_2;
yyv_P = yy_0_2;
yy_1_1 = yyv_T;
yy_1_2 = yyv_P;
CheckType(yy_1_1, yy_1_2);
return;
yyfl_270_1 : ;
}
yyErr(2,2519);
}
CheckUniqueParams(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_T;
yy yy_0_1_3;
yy yyv_ArgspecList;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yy_1_4;
yy yy_2_1;
yy yy_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_271_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_H = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_T = yy_0_1_3;
yyv_ArgspecList = yy_0_2;
yy_1_1 = yyv_H;
yy_1_2 = yyv_T;
yy_1_3 = yyv_ArgspecList;
yy_1_4 = yyv_P;
CheckUniqueParam(yy_1_1, yy_1_2, yy_1_3, yy_1_4);
yy_2_1 = yyv_T;
yy_2_2 = yyv_ArgspecList;
CheckUniqueParams(yy_2_1, yy_2_2);
return;
yyfl_271_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_ARGSPECLIST;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_271_2;
yyv_ARGSPECLIST = yy_0_2;
return;
yyfl_271_2 : ;
}
yyErr(2,2524);
}
CheckUniqueParam(yyin_1, yyin_2, yyin_3, yyin_4)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy yyin_4;
{
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_Type;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_List1;
yy yy_0_2;
yy yyv_List2;
yy yy_0_3;
yy yyv_Pos;
yy yy_0_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
if (yy_0_1[0] != 1) goto yyfl_272_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
if (yy_0_1_1[0] != 2) goto yyfl_272_1;
yyv_Type = yy_0_1_2;
yyv_List1 = yy_0_2;
yyv_List2 = yy_0_3;
yyv_Pos = yy_0_4;
return;
yyfl_272_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_Id;
yy yy_0_1_1_1;
yy yyv_Type;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_List1;
yy yy_0_2;
yy yyv_List2;
yy yy_0_3;
yy yyv_Pos;
yy yy_0_4;
yy yy_1_1;
yy yy_1_2;
yy yy_2_1;
yy yy_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
if (yy_0_1[0] != 1) goto yyfl_272_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
if (yy_0_1_1[0] != 1) goto yyfl_272_2;
yy_0_1_1_1 = ((yy)yy_0_1_1[1]);
yyv_Id = yy_0_1_1_1;
yyv_Type = yy_0_1_2;
yyv_List1 = yy_0_2;
yyv_List2 = yy_0_3;
yyv_Pos = yy_0_4;
yy_1_1 = yyv_Id;
yy_1_2 = yyv_List1;
CheckIdNotInParamList(yy_1_1, yy_1_2);
yy_2_1 = yyv_Id;
yy_2_2 = yyv_List2;
CheckIdNotInParamList(yy_2_1, yy_2_2);
return;
yyfl_272_2 : ;
}
yyErr(2,2533);
}
CheckIdNotInParamList(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yy_0_2;
yy yy_0_2_1;
yy yy_0_2_1_1;
yy yyv_Id2;
yy yy_0_2_1_1_1;
yy yyv_Type;
yy yy_0_2_1_2;
yy yy_0_2_1_3;
yy yyv_P;
yy yy_0_2_2;
yy yyv_Tail;
yy yy_0_2_3;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yy_2_1;
yy yy_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Id = yy_0_1;
if (yy_0_2[0] != 1) goto yyfl_273_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yy_0_2_3 = ((yy)yy_0_2[3]);
if (yy_0_2_1[0] != 1) goto yyfl_273_1;
yy_0_2_1_1 = ((yy)yy_0_2_1[1]);
yy_0_2_1_2 = ((yy)yy_0_2_1[2]);
yy_0_2_1_3 = ((yy)yy_0_2_1[3]);
if (yy_0_2_1_1[0] != 1) goto yyfl_273_1;
yy_0_2_1_1_1 = ((yy)yy_0_2_1_1[1]);
yyv_Id2 = yy_0_2_1_1_1;
yyv_Type = yy_0_2_1_2;
yyv_P = yy_0_2_2;
yyv_Tail = yy_0_2_3;
yy_1_1 = yyv_Id;
yy_1_2 = yyv_Id2;
yy_1_3 = yyv_P;
CheckParamNamesDiffer(yy_1_1, yy_1_2, yy_1_3);
yy_2_1 = yyv_Id;
yy_2_2 = yyv_Tail;
CheckIdNotInParamList(yy_2_1, yy_2_2);
return;
yyfl_273_1 : ;
}
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yy_0_2;
yy yy_0_2_1;
yy yy_0_2_1_1;
yy yyv_Type;
yy yy_0_2_1_2;
yy yy_0_2_1_3;
yy yyv_P;
yy yy_0_2_2;
yy yyv_Tail;
yy yy_0_2_3;
yy yy_1_1;
yy yy_1_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Id = yy_0_1;
if (yy_0_2[0] != 1) goto yyfl_273_2;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yy_0_2_3 = ((yy)yy_0_2[3]);
if (yy_0_2_1[0] != 1) goto yyfl_273_2;
yy_0_2_1_1 = ((yy)yy_0_2_1[1]);
yy_0_2_1_2 = ((yy)yy_0_2_1[2]);
yy_0_2_1_3 = ((yy)yy_0_2_1[3]);
if (yy_0_2_1_1[0] != 2) goto yyfl_273_2;
yyv_Type = yy_0_2_1_2;
yyv_P = yy_0_2_2;
yyv_Tail = yy_0_2_3;
yy_1_1 = yyv_Id;
yy_1_2 = yyv_Tail;
CheckIdNotInParamList(yy_1_1, yy_1_2);
return;
yyfl_273_2 : ;
}
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Id = yy_0_1;
if (yy_0_2[0] != 2) goto yyfl_273_3;
return;
yyfl_273_3 : ;
}
yyErr(2,2542);
}
CheckParamNamesDiffer(yyin_1, yyin_2, yyin_3)
yy yyin_1;
yy yyin_2;
yy yyin_3;
{
{
yy yyb;
yy yyv_Id1;
yy yy_0_1;
yy yyv_Id2;
yy yy_0_2;
yy yyv_P;
yy yy_0_3;
yy yy_1_1;
yy yy_1_2;
yy yy_2_1;
yy yy_2_2;
yy yy_2_3;
yy yy_2_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Id1 = yy_0_1;
yyv_Id2 = yy_0_2;
yyv_P = yy_0_3;
yy_1_1 = yyv_Id1;
yy_1_2 = yyv_Id2;
if (! yyeq_IDENT(yy_1_1, yy_1_2)) goto yyfl_274_1;
yy_2_1 = ((yy)"'");
yy_2_2 = yyv_Id1;
yy_2_3 = ((yy)"' multiply defined");
yy_2_4 = yyv_P;
MESSAGE1(yy_2_1, yy_2_2, yy_2_3, yy_2_4);
return;
yyfl_274_1 : ;
}
{
yy yyb;
yy yyv_Id1;
yy yy_0_1;
yy yyv_Id2;
yy yy_0_2;
yy yyv_P;
yy yy_0_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Id1 = yy_0_1;
yyv_Id2 = yy_0_2;
yyv_P = yy_0_3;
return;
yyfl_274_2 : ;
}
yyErr(2,2554);
}
ResultType(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
///* --PATCH-- */ yy yy_1_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_275_1;
///* --PATCH-- */ yy_1_1 = ((yy)"void ");
///* --PATCH-- */ s(yy_1_1);
return;
yyfl_275_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_1_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_275_2;
yy_1_1 = ((yy)"int ");
s(yy_1_1);
return;
yyfl_275_2 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_1_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 6) goto yyfl_275_3;
yy_1_1 = ((yy)"int ");
s(yy_1_1);
return;
yyfl_275_3 : ;
}
yyErr(2,2563);
}
ReturnFalse(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Pos;
yy yy_0_2;
yy yy_1_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_276_1;
yyv_Pos = yy_0_2;
yy_1_1 = ((yy)"return 0;");
s(yy_1_1);
nl();
return;
yyfl_276_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Pos;
yy yy_0_2;
yy yy_1_1_1;
yy yy_1_1_2_1;
yy yyv_Line;
yy yy_1_1_2_2;
yy yy_1_1_3_1;
yy yy_1_1_4_1;
yy yy_1_1_5_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_276_2;
yyv_Pos = yy_0_2;
{
yy yysb = yyb;
yy_1_1_1 = yyglov_currentFailLabelUsed;
if (yy_1_1_1 == (yy) yyu) yyErr(1,2578);
if (yy_1_1_1[0] != 1) goto yyfl_276_2_1_1;
yy_1_1_2_1 = yyv_Pos;
PosToLineNumber(yy_1_1_2_1, &yy_1_1_2_2);
yyv_Line = yy_1_1_2_2;
yy_1_1_3_1 = ((yy)"yyErr(2,");
s(yy_1_1_3_1);
yy_1_1_4_1 = yyv_Line;
i(yy_1_1_4_1);
yy_1_1_5_1 = ((yy)");");
s(yy_1_1_5_1);
nl();
goto yysl_276_2_1;
yyfl_276_2_1_1 : ;
goto yysl_276_2_1;
yysl_276_2_1 : ;
yyb = yysb;
}
return;
yyfl_276_2 : ;
}
yyErr(2,2573);
}
ListFormalParams(yyin_1, yyin_2, yyin_3, yyin_4, yyout_1)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy yyin_4;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_T;
yy yy_0_1_3;
yy yyv_N;
yy yy_0_2;
yy yyv_Letter;
yy yy_0_3;
yy yyv_Sep;
yy yy_0_4;
yy yy_0_5;
yy yy_1_1;
yy yy_2_1;
yy yy_3_1;
yy yy_4_1;
yy yy_4_2;
yy yy_4_2_1;
yy yy_4_2_2;
yy yy_4_3;
yy yy_4_4;
yy yyv_NextSep;
yy yy_4_5;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
if (yy_0_1[0] != 1) goto yyfl_277_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_H = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_T = yy_0_1_3;
yyv_N = yy_0_2;
yyv_Letter = yy_0_3;
yyv_Sep = yy_0_4;
yy_1_1 = yyv_Sep;
s(yy_1_1);
yy_2_1 = yyv_Letter;
s(yy_2_1);
yy_3_1 = yyv_N;
i(yy_3_1);
yy_4_1 = yyv_T;
yy_4_2_1 = yyv_N;
yy_4_2_2 = ((yy)1);
yy_4_2 = (yy)(((long)yy_4_2_1)+((long)yy_4_2_2));
yy_4_3 = yyv_Letter;
yy_4_4 = ((yy)", ");
ListFormalParams(yy_4_1, yy_4_2, yy_4_3, yy_4_4, &yy_4_5);
yyv_NextSep = yy_4_5;
yy_0_5 = yyv_NextSep;
*yyout_1 = yy_0_5;
return;
yyfl_277_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_N;
yy yy_0_2;
yy yyv_Letter;
yy yy_0_3;
yy yyv_Sep;
yy yy_0_4;
yy yy_0_5;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
if (yy_0_1[0] != 2) goto yyfl_277_2;
yyv_N = yy_0_2;
yyv_Letter = yy_0_3;
yyv_Sep = yy_0_4;
yy_0_5 = yyv_Sep;
*yyout_1 = yy_0_5;
return;
yyfl_277_2 : ;
}
yyErr(2,2585);
}
DeclareFormalParams(yyin_1, yyin_2, yyin_3)
yy yyin_1;
yy yyin_2;
yy yyin_3;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_T;
yy yy_0_1_3;
yy yyv_N;
yy yy_0_2;
yy yyv_Letter;
yy yy_0_3;
yy yy_1_1;
yy yy_2_1;
yy yy_3_1;
yy yy_4_1;
yy yy_6_1;
yy yy_6_2;
yy yy_6_2_1;
yy yy_6_2_2;
yy yy_6_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 1) goto yyfl_278_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_H = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_T = yy_0_1_3;
yyv_N = yy_0_2;
yyv_Letter = yy_0_3;
yy_1_1 = ((yy)"yy ");
s(yy_1_1);
yy_2_1 = yyv_Letter;
s(yy_2_1);
yy_3_1 = yyv_N;
i(yy_3_1);
yy_4_1 = ((yy)";");
s(yy_4_1);
nl();
yy_6_1 = yyv_T;
yy_6_2_1 = yyv_N;
yy_6_2_2 = ((yy)1);
yy_6_2 = (yy)(((long)yy_6_2_1)+((long)yy_6_2_2));
yy_6_3 = yyv_Letter;
DeclareFormalParams(yy_6_1, yy_6_2, yy_6_3);
return;
yyfl_278_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_N;
yy yy_0_2;
yy yyv_Letter;
yy yy_0_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 2) goto yyfl_278_2;
yyv_N = yy_0_2;
yyv_Letter = yy_0_3;
return;
yyfl_278_2 : ;
}
yyErr(2,2593);
}
Code_Rules(yyin_1, yyin_2, yyin_3, yyin_4, yyin_5, yyin_6, yyout_1)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy yyin_4;
yy yyin_5;
yy yyin_6;
yy *yyout_1;
{
{
yy yyb;
yy yyv_Class;
yy yy_0_1;
yy yyv_Id;
yy yy_0_2;
yy yyv_InArgs;
yy yy_0_3;
yy yyv_OutArgs;
yy yy_0_4;
yy yy_0_5;
yy yyv_H;
yy yy_0_5_1;
yy yy_0_5_2;
yy yyv_N;
yy yy_0_6;
yy yy_0_7;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yy_1_4;
yy yy_1_5;
yy yy_1_6;
yy yyv_MayFailHead;
yy yy_1_7;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
yy_0_5 = yyin_5;
yy_0_6 = yyin_6;
yyv_Class = yy_0_1;
yyv_Id = yy_0_2;
yyv_InArgs = yy_0_3;
yyv_OutArgs = yy_0_4;
if (yy_0_5[0] != 1) goto yyfl_279_1;
yy_0_5_1 = ((yy)yy_0_5[1]);
yy_0_5_2 = ((yy)yy_0_5[2]);
yyv_H = yy_0_5_1;
if (yy_0_5_2[0] != 2) goto yyfl_279_1;
yyv_N = yy_0_6;
yy_1_1 = yyv_Class;
yy_1_2 = yyv_Id;
yy_1_3 = yyv_InArgs;
yy_1_4 = yyv_OutArgs;
yy_1_5 = yyv_H;
yy_1_6 = yyv_N;
Code_Rule(yy_1_1, yy_1_2, yy_1_3, yy_1_4, yy_1_5, yy_1_6, &yy_1_7);
yyv_MayFailHead = yy_1_7;
yy_0_7 = yyv_MayFailHead;
*yyout_1 = yy_0_7;
return;
yyfl_279_1 : ;
}
{
yy yyb;
yy yyv_Class;
yy yy_0_1;
yy yyv_Id;
yy yy_0_2;
yy yyv_InArgs;
yy yy_0_3;
yy yyv_OutArgs;
yy yy_0_4;
yy yy_0_5;
yy yyv_H;
yy yy_0_5_1;
yy yyv_T;
yy yy_0_5_2;
yy yyv_N;
yy yy_0_6;
yy yy_0_7;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yy_1_4;
yy yy_1_5;
yy yy_1_6;
yy yyv_MayFailHead;
yy yy_1_7;
yy yy_2_1;
yy yy_2_2;
yy yy_2_3;
yy yy_2_4;
yy yy_2_5;
yy yy_2_6;
yy yy_2_6_1;
yy yy_2_6_2;
yy yyv_MayFailTail;
yy yy_2_7;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
yy_0_5 = yyin_5;
yy_0_6 = yyin_6;
yyv_Class = yy_0_1;
yyv_Id = yy_0_2;
yyv_InArgs = yy_0_3;
yyv_OutArgs = yy_0_4;
if (yy_0_5[0] != 1) goto yyfl_279_2;
yy_0_5_1 = ((yy)yy_0_5[1]);
yy_0_5_2 = ((yy)yy_0_5[2]);
yyv_H = yy_0_5_1;
yyv_T = yy_0_5_2;
yyv_N = yy_0_6;
yy_1_1 = yyv_Class;
yy_1_2 = yyv_Id;
yy_1_3 = yyv_InArgs;
yy_1_4 = yyv_OutArgs;
yy_1_5 = yyv_H;
yy_1_6 = yyv_N;
Code_Rule(yy_1_1, yy_1_2, yy_1_3, yy_1_4, yy_1_5, yy_1_6, &yy_1_7);
yyv_MayFailHead = yy_1_7;
yy_2_1 = yyv_Class;
yy_2_2 = yyv_Id;
yy_2_3 = yyv_InArgs;
yy_2_4 = yyv_OutArgs;
yy_2_5 = yyv_T;
yy_2_6_1 = yyv_N;
yy_2_6_2 = ((yy)1);
yy_2_6 = (yy)(((long)yy_2_6_1)+((long)yy_2_6_2));
Code_Rules(yy_2_1, yy_2_2, yy_2_3, yy_2_4, yy_2_5, yy_2_6, &yy_2_7);
yyv_MayFailTail = yy_2_7;
yy_0_7 = yyv_MayFailTail;
*yyout_1 = yy_0_7;
return;
yyfl_279_2 : ;
}
yyErr(2,2601);
}
Code_Rule(yyin_1, yyin_2, yyin_3, yyin_4, yyin_5, yyin_6, yyout_1)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy yyin_4;
yy yyin_5;
yy yyin_6;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Id;
yy yy_0_2;
yy yyv_FormalInArgs;
yy yy_0_3;
yy yyv_FormalOutArgs;
yy yy_0_4;
yy yy_0_5;
yy yy_0_5_1;
yy yyv_P;
yy yy_0_5_1_1;
yy yyv_Pos;
yy yy_0_5_1_2;
yy yyv_InArgs;
yy yy_0_5_1_3;
yy yyv_OutArgs;
yy yy_0_5_1_4;
yy yyv_AllMembers;
yy yy_0_5_2;
yy yyv_Cost;
yy yy_0_5_3;
yy yyv_N;
yy yy_0_6;
yy yy_0_7;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yy_2_1;
yy yyv_GrammarSymbols;
yy yy_2_2;
yy yyv_PredicateMembers;
yy yy_2_3;
yy yy_3_1;
yy yy_4_1;
yy yy_6_1;
yy yy_7_1;
yy yy_8;
yy yy_11_1;
yy yy_13_1;
yy yy_15_1;
yy yy_15_1_1;
yy yy_15_1_1_1;
yy yy_15_1_1_2;
yy yy_15_1_1_3;
yy yy_15_1_1_4;
yy yy_15_1_2;
yy yy_15_1_3;
yy yyv_LhsSpace;
yy yy_15_2;
yy yy_16_1;
yy yy_16_2;
yy yy_16_3;
yy yyv_PosDefMember;
yy yy_16_4;
yy yy_17_1;
yy yy_17_2;
yy yy_17_3;
yy yy_18_1;
yy yy_18_2;
yy yy_18_2_1;
yy yy_18_2_2;
yy yy_18_3;
yy yy_19_1;
yy yy_19_2;
yy yy_20_1;
yy yy_20_2;
yy yy_21_1_1_1;
yy yy_21_1_1_2;
yy yy_21_1_2_1;
yy yy_21_2_1_1;
yy yy_21_2_2_1;
yy yy_21_2_3_1;
yy yyv_Used;
yy yy_22;
yy yy_23_1_1_1;
yy yy_23_1_1_2;
yy yy_23_1_2_1;
yy yy_23_1_4_1;
yy yy_23_1_7_1;
yy yy_23_1_9_1;
yy yyv_Line;
yy yy_23_1_9_2;
yy yy_23_1_10_1;
yy yy_23_1_11_1;
yy yy_23_1_12_1;
yy yy_23_1_15_1;
yy yy_24_1;
yy yy_26_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
yy_0_5 = yyin_5;
yy_0_6 = yyin_6;
if (yy_0_1[0] != 3) goto yyfl_280_1;
yyv_Id = yy_0_2;
yyv_FormalInArgs = yy_0_3;
yyv_FormalOutArgs = yy_0_4;
if (yy_0_5[0] != 1) goto yyfl_280_1;
yy_0_5_1 = ((yy)yy_0_5[1]);
yy_0_5_2 = ((yy)yy_0_5[2]);
yy_0_5_3 = ((yy)yy_0_5[3]);
if (yy_0_5_1[0] != 1) goto yyfl_280_1;
yy_0_5_1_1 = ((yy)yy_0_5_1[1]);
yy_0_5_1_2 = ((yy)yy_0_5_1[2]);
yy_0_5_1_3 = ((yy)yy_0_5_1[3]);
yy_0_5_1_4 = ((yy)yy_0_5_1[4]);
yyv_P = yy_0_5_1_1;
yyv_Pos = yy_0_5_1_2;
yyv_InArgs = yy_0_5_1_3;
yyv_OutArgs = yy_0_5_1_4;
yyv_AllMembers = yy_0_5_2;
yyv_Cost = yy_0_5_3;
yyv_N = yy_0_6;
yyb = yyh;
yyh += 13; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_Id;
yy_1_2 = yyv_P;
yy_1_3 = yyv_Pos;
CheckEqId(yy_1_1, yy_1_2, yy_1_3);
yy_2_1 = yyv_AllMembers;
SplitMembers(yy_2_1, &yy_2_2, &yy_2_3);
yyv_GrammarSymbols = yy_2_2;
yyv_PredicateMembers = yy_2_3;
yy_3_1 = yyv_P;
id(yy_3_1);
yy_4_1 = ((yy)":");
s(yy_4_1);
nl();
yy_6_1 = yyv_GrammarSymbols;
Code_GrammarSymbols(yy_6_1);
yy_7_1 = yyv_PredicateMembers;
CheckNoGrammar_MEMBERLIST(yy_7_1);
yy_8 = yyv_N;
yyglov_currentRuleNumber = yy_8;
BeginLocalScope();
nl();
yy_11_1 = ((yy)"{");
s(yy_11_1);
nl();
yy_13_1 = ((yy)"yy yyb;");
s(yy_13_1);
nl();
yy_15_1_1_1 = yyv_P;
yy_15_1_1_2 = yyv_Pos;
yy_15_1_1_3 = yyv_InArgs;
yy_15_1_1_4 = yyv_OutArgs;
yy_15_1_1 = yyb + 4;
yy_15_1_1[0] = 1;
yy_15_1_1[1] = ((long)yy_15_1_1_1);
yy_15_1_1[2] = ((long)yy_15_1_1_2);
yy_15_1_1[3] = ((long)yy_15_1_1_3);
yy_15_1_1[4] = ((long)yy_15_1_1_4);
yy_15_1_2 = yyv_AllMembers;
yy_15_1_3 = ((yy)0);
yy_15_1 = yyb + 0;
yy_15_1[0] = 1;
yy_15_1[1] = ((long)yy_15_1_1);
yy_15_1[2] = ((long)yy_15_1_2);
yy_15_1[3] = ((long)yy_15_1_3);
DefineTempos_RULE(yy_15_1, &yy_15_2);
yyv_LhsSpace = yy_15_2;
yy_16_1 = yyv_GrammarSymbols;
yy_16_2 = ((yy)1);
yy_16_3 = ((yy)0);
PosDefiningMember(yy_16_1, yy_16_2, yy_16_3, &yy_16_4);
yyv_PosDefMember = yy_16_4;
yy_17_1 = yyv_GrammarSymbols;
yy_17_2 = ((yy)0);
yy_17_3 = yyv_PosDefMember;
AssignAttributes(yy_17_1, yy_17_2, yy_17_3);
yy_18_1 = yyv_AllMembers;
yy_18_2_1 = ((yy)1);
yy_18_2_2 = yyb + 12;
yy_18_2_2[0] = 2;
yy_18_2 = yyb + 9;
yy_18_2[0] = 1;
yy_18_2[1] = ((long)yy_18_2_1);
yy_18_2[2] = ((long)yy_18_2_2);
yy_18_3 = yyv_LhsSpace;
Code_Members(yy_18_1, yy_18_2, yy_18_3);
yy_19_1 = yyv_OutArgs;
yy_19_2 = yyv_FormalOutArgs;
Build(yy_19_1, yy_19_2);
yy_20_1 = yyv_OutArgs;
yy_20_2 = ((yy)1);
AssignOutAttr(yy_20_1, yy_20_2);
{
yy yysb = yyb;
yy_21_1_1_1 = yyv_PosDefMember;
yy_21_1_1_2 = ((yy)0);
if ((long)yy_21_1_1_1 != (long)yy_21_1_1_2) goto yyfl_280_1_21_1;
yy_21_1_2_1 = ((yy)"yyGetPos(&$$.attr[0]);");
s(yy_21_1_2_1);
nl();
goto yysl_280_1_21;
yyfl_280_1_21_1 : ;
yy_21_2_1_1 = ((yy)"$$.attr[0] = $");
s(yy_21_2_1_1);
yy_21_2_2_1 = yyv_PosDefMember;
i(yy_21_2_2_1);
yy_21_2_3_1 = ((yy)".attr[0];");
s(yy_21_2_3_1);
nl();
goto yysl_280_1_21;
yysl_280_1_21 : ;
yyb = yysb;
}
yy_22 = yyglov_currentFailLabelUsed;
if (yy_22 == (yy) yyu) yyErr(1,2657);
yyv_Used = yy_22;
{
yy yysb = yyb;
yy_23_1_1_1 = yyv_Used;
yy_23_1_1_2 = yy_23_1_1_1;
if (yy_23_1_1_2[0] != 1) goto yyfl_280_1_23_1;
yy_23_1_2_1 = ((yy)"goto ");
s(yy_23_1_2_1);
FailLabel();
yy_23_1_4_1 = ((yy)"_A;");
s(yy_23_1_4_1);
nl();
FailLabel();
yy_23_1_7_1 = ((yy)" : ;");
s(yy_23_1_7_1);
nl();
yy_23_1_9_1 = yyv_Pos;
PosToLineNumber(yy_23_1_9_1, &yy_23_1_9_2);
yyv_Line = yy_23_1_9_2;
yy_23_1_10_1 = ((yy)"yyErr(3,");
s(yy_23_1_10_1);
yy_23_1_11_1 = yyv_Line;
i(yy_23_1_11_1);
yy_23_1_12_1 = ((yy)");");
s(yy_23_1_12_1);
nl();
FailLabel();
yy_23_1_15_1 = ((yy)"_A : ;");
s(yy_23_1_15_1);
nl();
goto yysl_280_1_23;
yyfl_280_1_23_1 : ;
goto yysl_280_1_23;
yysl_280_1_23 : ;
yyb = yysb;
}
yy_24_1 = ((yy)"}");
s(yy_24_1);
nl();
yy_26_1 = ((yy)";");
s(yy_26_1);
nl();
EndLocalScope();
yy_0_7 = yyv_Used;
*yyout_1 = yy_0_7;
return;
yyfl_280_1 : ;
}
{
yy yyb;
yy yyv_Class;
yy yy_0_1;
yy yyv_Id;
yy yy_0_2;
yy yyv_FormalInArgs;
yy yy_0_3;
yy yyv_FormalOutArgs;
yy yy_0_4;
yy yyv_Rule;
yy yy_0_5;
yy yy_0_5_1;
yy yy_0_5_1_1;
yy yyv_P;
yy yy_0_5_1_1_1;
yy yyv_Pos;
yy yy_0_5_1_1_2;
yy yyv_InArgs;
yy yy_0_5_1_1_3;
yy yyv_OutArgs;
yy yy_0_5_1_1_4;
yy yyv_Members;
yy yy_0_5_1_2;
yy yyv_Cost;
yy yy_0_5_1_3;
yy yyv_N;
yy yy_0_6;
yy yy_0_7;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yy_2_1;
yy yy_3;
yy yy_5_1_2_1;
yy yy_5_1_3_1;
yy yy_5_1_4_1;
yy yy_6_1;
yy yy_8_1;
yy yy_10_1;
yy yyv_LhsSpace;
yy yy_10_2;
yy yy_11_1;
yy yy_11_2;
yy yy_12_1_1_1;
yy yy_12_1_1_2;
yy yy_12_1_2_1_1_1;
yy yy_12_1_2_1_1_2;
yy yyv_IA_Head;
yy yy_12_1_2_1_1_2_1;
yy yyv_IA_Pos;
yy yy_12_1_2_1_1_2_2;
yy yyv_IA_Tail;
yy yy_12_1_2_1_1_2_3;
yy yy_12_1_2_1_2_1;
yy yy_12_1_2_1_2_2;
yy yyv_FIA_Head;
yy yy_12_1_2_1_2_2_1;
yy yyv_FIA_Pos;
yy yy_12_1_2_1_2_2_2;
yy yyv_FIA_Tail;
yy yy_12_1_2_1_2_2_3;
yy yy_12_1_2_1_3_1_1_1_1_1;
yy yy_12_1_2_1_3_1_1_1_1_2;
yy yy_12_1_2_1_3_1_1_1_1_2_1;
yy yy_12_1_2_1_3_1_1_1_1_2_2;
yy yy_12_1_2_1_3_1_1_1_1_2_3;
yy yy_12_1_2_1_3_1_1_1_1_2_4;
yy yyv_Tmp;
yy yy_12_1_2_1_3_1_1_1_1_2_4_1;
yy yyv_Functor1;
yy yy_12_1_2_1_3_1_1_1_1_2_4_2;
yy yyv_Pos1;
yy yy_12_1_2_1_3_1_1_1_1_2_4_3;
yy yyv_Args;
yy yy_12_1_2_1_3_1_1_1_1_2_4_4;
yy yy_12_1_2_1_3_1_1_2_1_1;
yy yy_12_1_2_1_3_1_1_2_1_2;
yy yy_12_1_2_1_3_1_1_2_1_2_1;
yy yy_12_1_2_1_3_1_1_2_1_2_2;
yy yy_12_1_2_1_3_1_1_2_1_2_3;
yy yy_12_1_2_1_3_1_1_2_1_2_4;
yy yy_12_1_2_1_3_1_2_1_1_1;
yy yy_12_1_2_1_3_1_2_1_1_2;
yy yyv_Type;
yy yy_12_1_2_1_3_1_2_1_1_2_1;
yy yy_12_1_2_1_3_1_2_1_1_2_2;
yy yy_12_1_2_1_3_1_2_2_1_1;
yy yy_12_1_2_1_3_1_2_2_1_2;
yy yy_12_1_2_1_3_1_2_2_1_2_1;
yy yyv_Others;
yy yy_12_1_2_1_3_1_2_2_1_2_2;
yy yy_12_1_2_1_3_1_2_2_2_1;
yy yy_12_1_2_1_3_1_2_2_2_2;
yy yy_12_1_2_1_3_1_2_2_2_3;
yy yy_12_1_2_1_3_1_2_2_2_4;
yy yy_12_1_2_1_3_1_2_3_1_1;
yy yy_12_1_2_1_3_1_2_3_1_2;
yy yy_12_1_2_1_3_1_2_3_1_3;
yy yy_12_1_2_1_3_1_2_3_1_4;
yy yy_12_1_2_1_3_1_2_3_2;
yy yy_12_1_2_1_3_2_1_1;
yy yy_12_1_2_1_3_2_1_2;
yy yy_12_1_2_1_3_2_1_2_1;
yy yy_12_1_2_1_3_2_1_2_2;
yy yy_12_1_2_1_3_2_1_2_3;
yy yy_12_1_2_1_3_2_1_2_4;
yy yy_12_1_2_1_3_2_1_2_5;
yy yy_12_1_2_1_4_1;
yy yy_12_1_2_1_4_2;
yy yy_12_1_2_1_4_2_1;
yy yy_12_1_2_1_4_2_2;
yy yy_12_1_2_1_4_2_3;
yy yy_12_1_2_1_5_1;
yy yy_12_1_2_1_5_2;
yy yy_12_2_1_1;
yy yy_12_2_1_2;
yy yy_13_1;
yy yy_13_2;
yy yy_13_2_1;
yy yy_13_2_2;
yy yy_13_3;
yy yy_14_1;
yy yy_14_2;
yy yy_15_1;
yy yy_15_2;
yy yy_16_1_1_1;
yy yy_16_1_1_2;
yy yy_16_1_2_1;
yy yy_16_1_2_2;
yy yy_16_1_2_2_1;
yy yy_16_1_3_1;
yy yy_17_1_2_1;
yy yy_18_1;
yy yyv_Used;
yy yy_19;
yy yy_20_1_1_1;
yy yy_20_1_1_2;
yy yy_20_1_3_1;
yy yy_21_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
yy_0_5 = yyin_5;
yy_0_6 = yyin_6;
yyv_Class = yy_0_1;
yyv_Id = yy_0_2;
yyv_FormalInArgs = yy_0_3;
yyv_FormalOutArgs = yy_0_4;
yy_0_5_1 = yy_0_5;
yyv_Rule = yy_0_5;
if (yy_0_5_1[0] != 1) goto yyfl_280_2;
yy_0_5_1_1 = ((yy)yy_0_5_1[1]);
yy_0_5_1_2 = ((yy)yy_0_5_1[2]);
yy_0_5_1_3 = ((yy)yy_0_5_1[3]);
if (yy_0_5_1_1[0] != 1) goto yyfl_280_2;
yy_0_5_1_1_1 = ((yy)yy_0_5_1_1[1]);
yy_0_5_1_1_2 = ((yy)yy_0_5_1_1[2]);
yy_0_5_1_1_3 = ((yy)yy_0_5_1_1[3]);
yy_0_5_1_1_4 = ((yy)yy_0_5_1_1[4]);
yyv_P = yy_0_5_1_1_1;
yyv_Pos = yy_0_5_1_1_2;
yyv_InArgs = yy_0_5_1_1_3;
yyv_OutArgs = yy_0_5_1_1_4;
yyv_Members = yy_0_5_1_2;
yyv_Cost = yy_0_5_1_3;
yyv_N = yy_0_6;
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_Id;
yy_1_2 = yyv_P;
yy_1_3 = yyv_Pos;
CheckEqId(yy_1_1, yy_1_2, yy_1_3);
yy_2_1 = yyv_Members;
CheckNoGrammar_MEMBERS(yy_2_1);
yy_3 = yyv_N;
yyglov_currentRuleNumber = yy_3;
BeginLocalScope();
{
yy yysb = yyb;
if (! TraceOption()) goto yyfl_280_2_5_1;
yy_5_1_2_1 = ((yy)"TRACE_BEGIN_RULE(LOCALTRACEPTR, \"");
s(yy_5_1_2_1);
yy_5_1_3_1 = yyv_Id;
id(yy_5_1_3_1);
yy_5_1_4_1 = ((yy)"\");");
s(yy_5_1_4_1);
nl();
goto yysl_280_2_5;
yyfl_280_2_5_1 : ;
goto yysl_280_2_5;
yysl_280_2_5 : ;
yyb = yysb;
}
yy_6_1 = ((yy)"{");
s(yy_6_1);
nl();
yy_8_1 = ((yy)"yy yyb;");
s(yy_8_1);
nl();
yy_10_1 = yyv_Rule;
DefineTempos_RULE(yy_10_1, &yy_10_2);
yyv_LhsSpace = yy_10_2;
yy_11_1 = yyv_InArgs;
yy_11_2 = ((yy)1);
AssignInArgs(yy_11_1, yy_11_2);
{
yy yysb = yyb;
yy_12_1_1_1 = yyv_Class;
yy_12_1_1_2 = yy_12_1_1_1;
if (yy_12_1_1_2[0] != 6) goto yyfl_280_2_12_1;
{
yy yysb = yyb;
yyb = yyh;
yyh += 6; if (yyh > yyhx) yyExtend();
yy_12_1_2_1_1_1 = yyv_InArgs;
yy_12_1_2_1_1_2 = yy_12_1_2_1_1_1;
if (yy_12_1_2_1_1_2[0] != 1) goto yyfl_280_2_12_1_2_1;
yy_12_1_2_1_1_2_1 = ((yy)yy_12_1_2_1_1_2[1]);
yy_12_1_2_1_1_2_2 = ((yy)yy_12_1_2_1_1_2[2]);
yy_12_1_2_1_1_2_3 = ((yy)yy_12_1_2_1_1_2[3]);
yyv_IA_Head = yy_12_1_2_1_1_2_1;
yyv_IA_Pos = yy_12_1_2_1_1_2_2;
yyv_IA_Tail = yy_12_1_2_1_1_2_3;
yy_12_1_2_1_2_1 = yyv_FormalInArgs;
yy_12_1_2_1_2_2 = yy_12_1_2_1_2_1;
if (yy_12_1_2_1_2_2[0] != 1) goto yyfl_280_2_12_1_2_1;
yy_12_1_2_1_2_2_1 = ((yy)yy_12_1_2_1_2_2[1]);
yy_12_1_2_1_2_2_2 = ((yy)yy_12_1_2_1_2_2[2]);
yy_12_1_2_1_2_2_3 = ((yy)yy_12_1_2_1_2_2[3]);
yyv_FIA_Head = yy_12_1_2_1_2_2_1;
yyv_FIA_Pos = yy_12_1_2_1_2_2_2;
yyv_FIA_Tail = yy_12_1_2_1_2_2_3;
{
yy yysb = yyb;
{
yy yysb = yyb;
yy_12_1_2_1_3_1_1_1_1_1 = yyv_IA_Head;
yy_12_1_2_1_3_1_1_1_1_2 = yy_12_1_2_1_3_1_1_1_1_1;
if (yy_12_1_2_1_3_1_1_1_1_2[0] != 4) goto yyfl_280_2_12_1_2_1_3_1_1_1;
yy_12_1_2_1_3_1_1_1_1_2_1 = ((yy)yy_12_1_2_1_3_1_1_1_1_2[1]);
yy_12_1_2_1_3_1_1_1_1_2_2 = ((yy)yy_12_1_2_1_3_1_1_1_1_2[2]);
yy_12_1_2_1_3_1_1_1_1_2_3 = ((yy)yy_12_1_2_1_3_1_1_1_1_2[3]);
yy_12_1_2_1_3_1_1_1_1_2_4 = ((yy)yy_12_1_2_1_3_1_1_1_1_2[4]);
if (yy_12_1_2_1_3_1_1_1_1_2_4[0] != 1) goto yyfl_280_2_12_1_2_1_3_1_1_1;
yy_12_1_2_1_3_1_1_1_1_2_4_1 = ((yy)yy_12_1_2_1_3_1_1_1_1_2_4[1]);
yy_12_1_2_1_3_1_1_1_1_2_4_2 = ((yy)yy_12_1_2_1_3_1_1_1_1_2_4[2]);
yy_12_1_2_1_3_1_1_1_1_2_4_3 = ((yy)yy_12_1_2_1_3_1_1_1_1_2_4[3]);
yy_12_1_2_1_3_1_1_1_1_2_4_4 = ((yy)yy_12_1_2_1_3_1_1_1_1_2_4[4]);
yyv_Tmp = yy_12_1_2_1_3_1_1_1_1_2_4_1;
yyv_Functor1 = yy_12_1_2_1_3_1_1_1_1_2_4_2;
yyv_Pos1 = yy_12_1_2_1_3_1_1_1_1_2_4_3;
yyv_Args = yy_12_1_2_1_3_1_1_1_1_2_4_4;
goto yysl_280_2_12_1_2_1_3_1_1;
yyfl_280_2_12_1_2_1_3_1_1_1 : ;
yy_12_1_2_1_3_1_1_2_1_1 = yyv_IA_Head;
yy_12_1_2_1_3_1_1_2_1_2 = yy_12_1_2_1_3_1_1_2_1_1;
if (yy_12_1_2_1_3_1_1_2_1_2[0] != 1) goto yyfl_280_2_12_1_2_1_3_1_1_2;
yy_12_1_2_1_3_1_1_2_1_2_1 = ((yy)yy_12_1_2_1_3_1_1_2_1_2[1]);
yy_12_1_2_1_3_1_1_2_1_2_2 = ((yy)yy_12_1_2_1_3_1_1_2_1_2[2]);
yy_12_1_2_1_3_1_1_2_1_2_3 = ((yy)yy_12_1_2_1_3_1_1_2_1_2[3]);
yy_12_1_2_1_3_1_1_2_1_2_4 = ((yy)yy_12_1_2_1_3_1_1_2_1_2[4]);
yyv_Tmp = yy_12_1_2_1_3_1_1_2_1_2_1;
yyv_Functor1 = yy_12_1_2_1_3_1_1_2_1_2_2;
yyv_Pos1 = yy_12_1_2_1_3_1_1_2_1_2_3;
yyv_Args = yy_12_1_2_1_3_1_1_2_1_2_4;
goto yysl_280_2_12_1_2_1_3_1_1;
yyfl_280_2_12_1_2_1_3_1_1_2 : ;
goto yyfl_280_2_12_1_2_1_3_1;
yysl_280_2_12_1_2_1_3_1_1 : ;
yyb = yysb;
}
{
yy yysb = yyb;
yy_12_1_2_1_3_1_2_1_1_1 = yyv_Functor1;
if (! GetFunctorMeaning(yy_12_1_2_1_3_1_2_1_1_1, &yy_12_1_2_1_3_1_2_1_1_2)) goto yyfl_280_2_12_1_2_1_3_1_2_1;
if (yy_12_1_2_1_3_1_2_1_1_2[0] != 1) goto yyfl_280_2_12_1_2_1_3_1_2_1;
yy_12_1_2_1_3_1_2_1_1_2_1 = ((yy)yy_12_1_2_1_3_1_2_1_1_2[1]);
yy_12_1_2_1_3_1_2_1_1_2_2 = ((yy)yy_12_1_2_1_3_1_2_1_1_2[2]);
yyv_Type = yy_12_1_2_1_3_1_2_1_1_2_1;
if (yy_12_1_2_1_3_1_2_1_1_2_2[0] != 2) goto yyfl_280_2_12_1_2_1_3_1_2_1;
goto yysl_280_2_12_1_2_1_3_1_2;
yyfl_280_2_12_1_2_1_3_1_2_1 : ;
yy_12_1_2_1_3_1_2_2_1_1 = yyv_Functor1;
if (! GetFunctorMeaning(yy_12_1_2_1_3_1_2_2_1_1, &yy_12_1_2_1_3_1_2_2_1_2)) goto yyfl_280_2_12_1_2_1_3_1_2_2;
if (yy_12_1_2_1_3_1_2_2_1_2[0] != 1) goto yyfl_280_2_12_1_2_1_3_1_2_2;
yy_12_1_2_1_3_1_2_2_1_2_1 = ((yy)yy_12_1_2_1_3_1_2_2_1_2[1]);
yy_12_1_2_1_3_1_2_2_1_2_2 = ((yy)yy_12_1_2_1_3_1_2_2_1_2[2]);
yyv_Type = yy_12_1_2_1_3_1_2_2_1_2_1;
yyv_Others = yy_12_1_2_1_3_1_2_2_1_2_2;
yy_12_1_2_1_3_1_2_2_2_1 = ((yy)"ambigous '");
yy_12_1_2_1_3_1_2_2_2_2 = yyv_Functor1;
yy_12_1_2_1_3_1_2_2_2_3 = ((yy)"' requires type prefix");
yy_12_1_2_1_3_1_2_2_2_4 = yyv_Pos;
MESSAGE1(yy_12_1_2_1_3_1_2_2_2_1, yy_12_1_2_1_3_1_2_2_2_2, yy_12_1_2_1_3_1_2_2_2_3, yy_12_1_2_1_3_1_2_2_2_4);
goto yysl_280_2_12_1_2_1_3_1_2;
yyfl_280_2_12_1_2_1_3_1_2_2 : ;
yy_12_1_2_1_3_1_2_3_1_1 = ((yy)"unknown functor '");
yy_12_1_2_1_3_1_2_3_1_2 = yyv_Functor1;
yy_12_1_2_1_3_1_2_3_1_3 = ((yy)"'");
yy_12_1_2_1_3_1_2_3_1_4 = yyv_Pos;
MESSAGE1(yy_12_1_2_1_3_1_2_3_1_1, yy_12_1_2_1_3_1_2_3_1_2, yy_12_1_2_1_3_1_2_3_1_3, yy_12_1_2_1_3_1_2_3_1_4);
yy_12_1_2_1_3_1_2_3_2 = yyglov_id_INT;
if (yy_12_1_2_1_3_1_2_3_2 == (yy) yyu) yyErr(1,2715);
yyv_Type = yy_12_1_2_1_3_1_2_3_2;
goto yysl_280_2_12_1_2_1_3_1_2;
yysl_280_2_12_1_2_1_3_1_2 : ;
yyb = yysb;
}
goto yysl_280_2_12_1_2_1_3;
yyfl_280_2_12_1_2_1_3_1 : ;
yy_12_1_2_1_3_2_1_1 = yyv_IA_Head;
yy_12_1_2_1_3_2_1_2 = yy_12_1_2_1_3_2_1_1;
if (yy_12_1_2_1_3_2_1_2[0] != 2) goto yyfl_280_2_12_1_2_1_3_2;
yy_12_1_2_1_3_2_1_2_1 = ((yy)yy_12_1_2_1_3_2_1_2[1]);
yy_12_1_2_1_3_2_1_2_2 = ((yy)yy_12_1_2_1_3_2_1_2[2]);
yy_12_1_2_1_3_2_1_2_3 = ((yy)yy_12_1_2_1_3_2_1_2[3]);
yy_12_1_2_1_3_2_1_2_4 = ((yy)yy_12_1_2_1_3_2_1_2[4]);
yy_12_1_2_1_3_2_1_2_5 = ((yy)yy_12_1_2_1_3_2_1_2[5]);
yyv_Type = yy_12_1_2_1_3_2_1_2_1;
yyv_Tmp = yy_12_1_2_1_3_2_1_2_2;
yyv_Functor1 = yy_12_1_2_1_3_2_1_2_3;
yyv_Pos1 = yy_12_1_2_1_3_2_1_2_4;
yyv_Args = yy_12_1_2_1_3_2_1_2_5;
goto yysl_280_2_12_1_2_1_3;
yyfl_280_2_12_1_2_1_3_2 : ;
goto yyfl_280_2_12_1_2_1;
yysl_280_2_12_1_2_1_3 : ;
yyb = yysb;
}
yy_12_1_2_1_4_1 = yyv_IA_Head;
yy_12_1_2_1_4_2_1 = yyb + 4;
yy_12_1_2_1_4_2_1[0] = 2;
yy_12_1_2_1_4_2_2 = yyv_Type;
yy_12_1_2_1_4_2_3 = yyb + 5;
yy_12_1_2_1_4_2_3[0] = 2;
yy_12_1_2_1_4_2 = yyb + 0;
yy_12_1_2_1_4_2[0] = 1;
yy_12_1_2_1_4_2[1] = ((long)yy_12_1_2_1_4_2_1);
yy_12_1_2_1_4_2[2] = ((long)yy_12_1_2_1_4_2_2);
yy_12_1_2_1_4_2[3] = ((long)yy_12_1_2_1_4_2_3);
MatchArg(yy_12_1_2_1_4_1, yy_12_1_2_1_4_2);
yy_12_1_2_1_5_1 = yyv_IA_Tail;
yy_12_1_2_1_5_2 = yyv_FIA_Tail;
MatchArgs(yy_12_1_2_1_5_1, yy_12_1_2_1_5_2);
goto yysl_280_2_12_1_2;
yyfl_280_2_12_1_2_1 : ;
goto yysl_280_2_12_1_2;
yysl_280_2_12_1_2 : ;
yyb = yysb;
}
goto yysl_280_2_12;
yyfl_280_2_12_1 : ;
yy_12_2_1_1 = yyv_InArgs;
yy_12_2_1_2 = yyv_FormalInArgs;
MatchArgs(yy_12_2_1_1, yy_12_2_1_2);
goto yysl_280_2_12;
yysl_280_2_12 : ;
yyb = yysb;
}
yy_13_1 = yyv_Members;
yy_13_2_1 = ((yy)1);
yy_13_2_2 = yyb + 3;
yy_13_2_2[0] = 2;
yy_13_2 = yyb + 0;
yy_13_2[0] = 1;
yy_13_2[1] = ((long)yy_13_2_1);
yy_13_2[2] = ((long)yy_13_2_2);
yy_13_3 = yyv_LhsSpace;
Code_Members(yy_13_1, yy_13_2, yy_13_3);
yy_14_1 = yyv_OutArgs;
yy_14_2 = yyv_FormalOutArgs;
Build(yy_14_1, yy_14_2);
yy_15_1 = yyv_OutArgs;
yy_15_2 = ((yy)1);
AssignOutArgs(yy_15_1, yy_15_2);
{
yy yysb = yyb;
yy_16_1_1_1 = yyv_Class;
yy_16_1_1_2 = yy_16_1_1_1;
if (yy_16_1_1_2[0] != 6) goto yyfl_280_2_16_1;
yy_16_1_2_1 = yyv_OutArgs;
yy_16_1_2_2 = yy_16_1_2_1;
if (yy_16_1_2_2[0] != 2) goto yyfl_280_2_16_1;
yy_16_1_2_2_1 = ((yy)yy_16_1_2_2[1]);
yy_16_1_3_1 = ((yy)"*yyout_1 = yyin_2;");
s(yy_16_1_3_1);
nl();
goto yysl_280_2_16;
yyfl_280_2_16_1 : ;
goto yysl_280_2_16;
yysl_280_2_16 : ;
yyb = yysb;
}
{
yy yysb = yyb;
if (! TraceOption()) goto yyfl_280_2_17_1;
yy_17_1_2_1 = ((yy)"TRACE_SUCCESS();");
s(yy_17_1_2_1);
nl();
goto yysl_280_2_17;
yyfl_280_2_17_1 : ;
goto yysl_280_2_17;
yysl_280_2_17 : ;
yyb = yysb;
}
yy_18_1 = yyv_Class;
ReturnTrue(yy_18_1);
yy_19 = yyglov_currentFailLabelUsed;
if (yy_19 == (yy) yyu) yyErr(1,2745);
yyv_Used = yy_19;
{
yy yysb = yyb;
yy_20_1_1_1 = yyv_Used;
yy_20_1_1_2 = yy_20_1_1_1;
if (yy_20_1_1_2[0] != 1) goto yyfl_280_2_20_1;
FailLabel();
yy_20_1_3_1 = ((yy)" : ;");
s(yy_20_1_3_1);
nl();
goto yysl_280_2_20;
yyfl_280_2_20_1 : ;
goto yysl_280_2_20;
yysl_280_2_20 : ;
yyb = yysb;
}
yy_21_1 = ((yy)"}");
s(yy_21_1);
nl();
EndLocalScope();
yy_0_7 = yyv_Used;
*yyout_1 = yy_0_7;
return;
yyfl_280_2 : ;
}
yyErr(2,2611);
}
PosDefiningMember(yyin_1, yyin_2, yyin_3, yyout_1)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_1_1;
yy yy_0_1_1_2;
yy yy_0_1_1_3;
yy yy_0_1_1_4;
yy yy_0_1_1_5;
yy yyv_Tail;
yy yy_0_1_2;
yy yyv_ThisMem;
yy yy_0_2;
yy yyv_OldDefMem;
yy yy_0_3;
yy yy_0_4;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yy_1_1_2_1;
yy yy_1_1_2_2;
yy yy_1_1_2_2_1;
yy yy_1_1_2_2_2;
yy yy_1_1_2_3;
yy yyv_NewDefMem;
yy yy_1_1_2_4;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_1_2_1_2_1;
yy yy_1_2_1_2_2;
yy yy_1_2_1_3;
yy yy_1_2_1_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 1) goto yyfl_281_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
if (yy_0_1_1[0] != 7) goto yyfl_281_1;
yy_0_1_1_1 = ((yy)yy_0_1_1[1]);
yy_0_1_1_2 = ((yy)yy_0_1_1[2]);
yy_0_1_1_3 = ((yy)yy_0_1_1[3]);
yy_0_1_1_4 = ((yy)yy_0_1_1[4]);
yy_0_1_1_5 = ((yy)yy_0_1_1[5]);
yyv_Tail = yy_0_1_2;
yyv_ThisMem = yy_0_2;
yyv_OldDefMem = yy_0_3;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_OldDefMem;
yy_1_1_1_2 = ((yy)0);
if ((long)yy_1_1_1_1 != (long)yy_1_1_1_2) goto yyfl_281_1_1_1;
yy_1_1_2_1 = yyv_Tail;
yy_1_1_2_2_1 = yyv_ThisMem;
yy_1_1_2_2_2 = ((yy)1);
yy_1_1_2_2 = (yy)(((long)yy_1_1_2_2_1)+((long)yy_1_1_2_2_2));
yy_1_1_2_3 = yyv_ThisMem;
PosDefiningMember(yy_1_1_2_1, yy_1_1_2_2, yy_1_1_2_3, &yy_1_1_2_4);
yyv_NewDefMem = yy_1_1_2_4;
goto yysl_281_1_1;
yyfl_281_1_1_1 : ;
yy_1_2_1_1 = yyv_Tail;
yy_1_2_1_2_1 = yyv_ThisMem;
yy_1_2_1_2_2 = ((yy)1);
yy_1_2_1_2 = (yy)(((long)yy_1_2_1_2_1)+((long)yy_1_2_1_2_2));
yy_1_2_1_3 = yyv_OldDefMem;
PosDefiningMember(yy_1_2_1_1, yy_1_2_1_2, yy_1_2_1_3, &yy_1_2_1_4);
yyv_NewDefMem = yy_1_2_1_4;
goto yysl_281_1_1;
yysl_281_1_1 : ;
yyb = yysb;
}
yy_0_4 = yyv_NewDefMem;
*yyout_1 = yy_0_4;
return;
yyfl_281_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_1_1;
yy yy_0_1_1_2;
yy yyv_Tail;
yy yy_0_1_2;
yy yyv_ThisMem;
yy yy_0_2;
yy yy_0_3;
yy yy_0_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 1) goto yyfl_281_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
if (yy_0_1_1[0] != 8) goto yyfl_281_2;
yy_0_1_1_1 = ((yy)yy_0_1_1[1]);
yy_0_1_1_2 = ((yy)yy_0_1_1[2]);
yyv_Tail = yy_0_1_2;
yyv_ThisMem = yy_0_2;
yy_0_4 = yyv_ThisMem;
*yyout_1 = yy_0_4;
return;
yyfl_281_2 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_1_1;
yy yy_0_1_1_2;
yy yyv_Tail;
yy yy_0_1_2;
yy yyv_ThisMem;
yy yy_0_2;
yy yyv_OldDefMem;
yy yy_0_3;
yy yy_0_4;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yyv_NewDefMem;
yy yy_1_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 1) goto yyfl_281_3;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
if (yy_0_1_1[0] != 9) goto yyfl_281_3;
yy_0_1_1_1 = ((yy)yy_0_1_1[1]);
yy_0_1_1_2 = ((yy)yy_0_1_1[2]);
yyv_Tail = yy_0_1_2;
yyv_ThisMem = yy_0_2;
yyv_OldDefMem = yy_0_3;
yy_1_1 = yyv_Tail;
yy_1_2 = yyv_ThisMem;
yy_1_3 = yyv_OldDefMem;
PosDefiningMember(yy_1_1, yy_1_2, yy_1_3, &yy_1_4);
yyv_NewDefMem = yy_1_4;
yy_0_4 = yyv_NewDefMem;
*yyout_1 = yy_0_4;
return;
yyfl_281_3 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_0_2;
yy yyv_OldDefMem;
yy yy_0_3;
yy yy_0_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 2) goto yyfl_281_4;
yyv_OldDefMem = yy_0_3;
yy_0_4 = yyv_OldDefMem;
*yyout_1 = yy_0_4;
return;
yyfl_281_4 : ;
}
yyErr(2,2761);
}
CheckEqId(yyin_1, yyin_2, yyin_3)
yy yyin_1;
yy yyin_2;
yy yyin_3;
{
{
yy yyb;
yy yyv_I1;
yy yy_0_1;
yy yyv_I2;
yy yy_0_2;
yy yyv_Pos;
yy yy_0_3;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_1_2_1_3;
yy yy_1_2_1_4;
yy yy_1_2_1_5;
yy yy_1_2_1_6;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_I1 = yy_0_1;
yyv_I2 = yy_0_2;
yyv_Pos = yy_0_3;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_I1;
yy_1_1_1_2 = yyv_I2;
if (! yyeq_IDENT(yy_1_1_1_1, yy_1_1_1_2)) goto yyfl_282_1_1_1;
goto yysl_282_1_1;
yyfl_282_1_1_1 : ;
yy_1_2_1_1 = ((yy)"Rule for '");
yy_1_2_1_2 = yyv_I2;
yy_1_2_1_3 = ((yy)"' in rule list for '");
yy_1_2_1_4 = yyv_I1;
yy_1_2_1_5 = ((yy)"'");
yy_1_2_1_6 = yyv_Pos;
MESSAGE2(yy_1_2_1_1, yy_1_2_1_2, yy_1_2_1_3, yy_1_2_1_4, yy_1_2_1_5, yy_1_2_1_6);
goto yysl_282_1_1;
yysl_282_1_1 : ;
yyb = yysb;
}
return;
}
}
ReturnTrue(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yy_1_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_283_1;
yy_1_1 = ((yy)"return;");
s(yy_1_1);
nl();
return;
yyfl_283_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_1_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 5) goto yyfl_283_2;
yy_1_1 = ((yy)"return;");
s(yy_1_1);
nl();
return;
yyfl_283_2 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_1_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_283_3;
yy_1_1 = ((yy)"return 1;");
s(yy_1_1);
nl();
return;
yyfl_283_3 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_1_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 6) goto yyfl_283_4;
yy_1_1 = ((yy)"return 1;");
s(yy_1_1);
nl();
return;
yyfl_283_4 : ;
}
yyErr(2,2790);
}
BeginLocalScope()
{
{
yy yyb;
yy yyv_E;
yy yy_1;
yy yyv_N;
yy yy_2;
yy yy_3;
yy yy_3_1;
yy yy_3_2;
yy yy_4;
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yy_1 = yyglov_env;
if (yy_1 == (yy) yyu) yyErr(1,2810);
yyv_E = yy_1;
yy_2 = yyglov_localVars;
if (yy_2 == (yy) yyu) yyErr(1,2811);
yyv_N = yy_2;
yy_3_1 = yyv_N;
yy_3_2 = yyv_E;
yy_3 = yyb + 0;
yy_3[0] = 1;
yy_3[1] = ((long)yy_3_1);
yy_3[2] = ((long)yy_3_2);
yyglov_env = yy_3;
yy_4 = yyb + 3;
yy_4[0] = 2;
yyglov_localVars = yy_4;
return;
}
}
EndLocalScope()
{
{
yy yyb;
yy yyv_L;
yy yy_1;
yy yy_2_1;
yy yy_3;
yy yyv_N;
yy yy_3_1;
yy yyv_E;
yy yy_3_2;
yy yy_4;
yy yy_5;
yy_1 = yyglov_localVars;
if (yy_1 == (yy) yyu) yyErr(1,2818);
yyv_L = yy_1;
yy_2_1 = yyv_L;
UndeclarelocalVars(yy_2_1);
yy_3 = yyglov_env;
if (yy_3 == (yy) yyu) yyErr(1,2820);
if (yy_3[0] != 1) goto yyfl_288_1;
yy_3_1 = ((yy)yy_3[1]);
yy_3_2 = ((yy)yy_3[2]);
yyv_N = yy_3_1;
yyv_E = yy_3_2;
yy_4 = yyv_E;
yyglov_env = yy_4;
yy_5 = yyv_N;
yyglov_localVars = yy_5;
return;
yyfl_288_1 : ;
}
yyErr(2,2815);
}
UndeclarelocalVars(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_T;
yy yy_0_1_2;
yy yy_1_1;
yy yy_2_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_289_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_H = yy_0_1_1;
yyv_T = yy_0_1_2;
yy_1_1 = yyv_H;
UndeclareLocalVar(yy_1_1);
yy_2_1 = yyv_T;
UndeclarelocalVars(yy_2_1);
return;
yyfl_289_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_289_2;
return;
yyfl_289_2 : ;
}
yyErr(2,2825);
}
UndeclareLocalVar(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yy_1_1;
yy_0_1 = yyin_1;
yyv_Id = yy_0_1;
yy_1_1 = yyv_Id;
ForgetLocalMeaning(yy_1_1);
return;
}
}
AssignInArgs(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_T;
yy yy_0_1_3;
yy yyv_N;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy yy_2_1;
yy yy_2_2;
yy yy_2_2_1;
yy yy_2_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_291_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_H = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_T = yy_0_1_3;
yyv_N = yy_0_2;
yy_1_1 = yyv_H;
yy_1_2 = yyv_N;
AssignInArg(yy_1_1, yy_1_2);
yy_2_1 = yyv_T;
yy_2_2_1 = yyv_N;
yy_2_2_2 = ((yy)1);
yy_2_2 = (yy)(((long)yy_2_2_1)+((long)yy_2_2_2));
AssignInArgs(yy_2_1, yy_2_2);
return;
yyfl_291_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_P;
yy yy_0_1_1;
yy yyv_N;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_291_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yyv_P = yy_0_1_1;
yyv_N = yy_0_2;
return;
yyfl_291_2 : ;
}
yyErr(2,2837);
}
AssignInArg(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yyv_Arg;
yy yy_0_1;
yy yyv_N;
yy yy_0_2;
yy yy_1_1;
yy yyv_Tmp;
yy yy_1_2;
yy yy_2_1;
yy yy_3_1;
yy yy_4_1;
yy yy_5_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Arg = yy_0_1;
yyv_N = yy_0_2;
yy_1_1 = yyv_Arg;
GetDefArgTmp(yy_1_1, &yy_1_2);
yyv_Tmp = yy_1_2;
yy_2_1 = yyv_Tmp;
tmp(yy_2_1);
yy_3_1 = ((yy)" = yyin_");
s(yy_3_1);
yy_4_1 = yyv_N;
i(yy_4_1);
yy_5_1 = ((yy)";");
s(yy_5_1);
nl();
return;
}
}
AssignOutArgs(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_T;
yy yy_0_1_3;
yy yyv_N;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy yy_2_1;
yy yy_2_2;
yy yy_2_2_1;
yy yy_2_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_293_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_H = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_T = yy_0_1_3;
yyv_N = yy_0_2;
yy_1_1 = yyv_H;
yy_1_2 = yyv_N;
AssignOutArg(yy_1_1, yy_1_2);
yy_2_1 = yyv_T;
yy_2_2_1 = yyv_N;
yy_2_2_2 = ((yy)1);
yy_2_2 = (yy)(((long)yy_2_2_1)+((long)yy_2_2_2));
AssignOutArgs(yy_2_1, yy_2_2);
return;
yyfl_293_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_P;
yy yy_0_1_1;
yy yyv_N;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_293_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yyv_P = yy_0_1_1;
yyv_N = yy_0_2;
return;
yyfl_293_2 : ;
}
yyErr(2,2850);
}
AssignOutArg(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yyv_Arg;
yy yy_0_1;
yy yyv_N;
yy yy_0_2;
yy yy_1_1;
yy yyv_Tmp;
yy yy_1_2;
yy yy_2_1;
yy yy_3_1;
yy yy_4_1;
yy yy_5_1;
yy yy_6_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Arg = yy_0_1;
yyv_N = yy_0_2;
yy_1_1 = yyv_Arg;
GetUseArgTmp(yy_1_1, &yy_1_2);
yyv_Tmp = yy_1_2;
yy_2_1 = ((yy)"*yyout_");
s(yy_2_1);
yy_3_1 = yyv_N;
i(yy_3_1);
yy_4_1 = ((yy)" = ");
s(yy_4_1);
yy_5_1 = yyv_Tmp;
tmp(yy_5_1);
yy_6_1 = ((yy)";");
s(yy_6_1);
nl();
return;
}
}
SplitMembers(yyin_1, yyout_1, yyout_2)
yy yyin_1;
yy *yyout_1;
yy *yyout_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_List;
yy yy_0_1_1;
yy yyv_Space;
yy yy_0_1_2;
yy yy_0_2;
yy yy_0_3;
yy yy_1_1;
yy yyv_MEMBERLIST1;
yy yy_1_2;
yy yyv_MEMBERLIST2;
yy yy_1_3;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_295_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_List = yy_0_1_1;
yyv_Space = yy_0_1_2;
yy_1_1 = yyv_List;
SplitMemberlist(yy_1_1, &yy_1_2, &yy_1_3);
yyv_MEMBERLIST1 = yy_1_2;
yyv_MEMBERLIST2 = yy_1_3;
yy_0_2 = yyv_MEMBERLIST1;
yy_0_3 = yyv_MEMBERLIST2;
*yyout_1 = yy_0_2;
*yyout_2 = yy_0_3;
return;
yyfl_295_1 : ;
}
yyErr(2,2864);
}
SplitMemberlist(yyin_1, yyout_1, yyout_2)
yy yyin_1;
yy *yyout_1;
yy *yyout_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_T;
yy yy_0_1_2;
yy yy_0_2;
yy yy_0_2_1;
yy yy_0_2_2;
yy yy_0_3;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yyv_Id;
yy yy_1_1_1_2_1;
yy yyv_P;
yy yy_1_1_1_2_2;
yy yyv_In;
yy yy_1_1_1_2_3;
yy yyv_Out;
yy yy_1_1_1_2_4;
yy yyv_Offset;
yy yy_1_1_1_2_5;
yy yy_1_1_2_1_1_1;
yy yyv_Assoc;
yy yy_1_1_2_1_1_2;
yy yy_1_1_2_2_1_1;
yy yy_1_1_2_2_1_2;
yy yy_1_1_2_2_1_3;
yy yy_1_1_2_2_1_4;
yy yy_1_1_3_1;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_1_2_1_2_1;
yy yy_1_2_1_2_2;
yy yy_1_3_1_1;
yy yy_1_3_1_2;
yy yy_1_3_1_2_1;
yy yy_1_3_1_2_2;
yy yy_2_1;
yy yyv_GrammarSymbols;
yy yy_2_2;
yy yyv_PredicateMembers;
yy yy_2_3;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_296_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_H = yy_0_1_1;
yyv_T = yy_0_1_2;
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_H;
yy_1_1_1_2 = yy_1_1_1_1;
if (yy_1_1_1_2[0] != 7) goto yyfl_296_1_1_1;
yy_1_1_1_2_1 = ((yy)yy_1_1_1_2[1]);
yy_1_1_1_2_2 = ((yy)yy_1_1_1_2[2]);
yy_1_1_1_2_3 = ((yy)yy_1_1_1_2[3]);
yy_1_1_1_2_4 = ((yy)yy_1_1_1_2[4]);
yy_1_1_1_2_5 = ((yy)yy_1_1_1_2[5]);
yyv_Id = yy_1_1_1_2_1;
yyv_P = yy_1_1_1_2_2;
yyv_In = yy_1_1_1_2_3;
yyv_Out = yy_1_1_1_2_4;
yyv_Offset = yy_1_1_1_2_5;
{
yy yysb = yyb;
yy_1_1_2_1_1_1 = yyv_Id;
if (! GetGlobalMeaning(yy_1_1_2_1_1_1, &yy_1_1_2_1_1_2)) goto yyfl_296_1_1_1_2_1;
yyv_Assoc = yy_1_1_2_1_1_2;
goto yysl_296_1_1_1_2;
yyfl_296_1_1_1_2_1 : ;
yy_1_1_2_2_1_1 = ((yy)"'");
yy_1_1_2_2_1_2 = yyv_Id;
yy_1_1_2_2_1_3 = ((yy)"' is not declared");
yy_1_1_2_2_1_4 = yyv_P;
MESSAGE1(yy_1_1_2_2_1_1, yy_1_1_2_2_1_2, yy_1_1_2_2_1_3, yy_1_1_2_2_1_4);
goto yysl_296_1_1_1_2;
yysl_296_1_1_1_2 : ;
yyb = yysb;
}
yy_1_1_3_1 = yyv_Id;
if (! IsSourceSymbol(yy_1_1_3_1)) goto yyfl_296_1_1_1;
goto yysl_296_1_1;
yyfl_296_1_1_1 : ;
yy_1_2_1_1 = yyv_H;
yy_1_2_1_2 = yy_1_2_1_1;
if (yy_1_2_1_2[0] != 8) goto yyfl_296_1_1_2;
yy_1_2_1_2_1 = ((yy)yy_1_2_1_2[1]);
yy_1_2_1_2_2 = ((yy)yy_1_2_1_2[2]);
goto yysl_296_1_1;
yyfl_296_1_1_2 : ;
yy_1_3_1_1 = yyv_H;
yy_1_3_1_2 = yy_1_3_1_1;
if (yy_1_3_1_2[0] != 9) goto yyfl_296_1_1_3;
yy_1_3_1_2_1 = ((yy)yy_1_3_1_2[1]);
yy_1_3_1_2_2 = ((yy)yy_1_3_1_2[2]);
goto yysl_296_1_1;
yyfl_296_1_1_3 : ;
goto yyfl_296_1;
yysl_296_1_1 : ;
yyb = yysb;
}
yy_2_1 = yyv_T;
SplitMemberlist(yy_2_1, &yy_2_2, &yy_2_3);
yyv_GrammarSymbols = yy_2_2;
yyv_PredicateMembers = yy_2_3;
yy_0_2_1 = yyv_H;
yy_0_2_2 = yyv_GrammarSymbols;
yy_0_2 = yyb + 0;
yy_0_2[0] = 1;
yy_0_2[1] = ((long)yy_0_2_1);
yy_0_2[2] = ((long)yy_0_2_2);
yy_0_3 = yyv_PredicateMembers;
*yyout_1 = yy_0_2;
*yyout_2 = yy_0_3;
return;
yyfl_296_1 : ;
}
{
yy yyb;
yy yyv_Members;
yy yy_0_1;
yy yy_0_2;
yy yy_0_3;
yy_0_1 = yyin_1;
yyv_Members = yy_0_1;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_0_2 = yyb + 0;
yy_0_2[0] = 2;
yy_0_3 = yyv_Members;
*yyout_1 = yy_0_2;
*yyout_2 = yy_0_3;
return;
yyfl_296_2 : ;
}
yyErr(2,2869);
}
int IsSourceSymbol(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yy_1_1;
yy yy_1_2;
yy yy_1_2_1;
yy yyv_Class;
yy yy_1_2_2;
yy yy_1_2_3;
yy yyv_I;
yy yy_1_2_3_1;
yy yyv_O;
yy yy_1_2_3_2;
yy yyv_Rules;
yy yy_1_2_4;
yy yy_2_1_1_1;
yy yy_2_1_1_2;
yy yy_2_2_1_1;
yy yy_2_2_1_2;
yy_0_1 = yyin_1;
yyv_Id = yy_0_1;
yy_1_1 = yyv_Id;
if (! GetGlobalMeaning(yy_1_1, &yy_1_2)) goto yyfl_297_1;
if (yy_1_2[0] != 4) goto yyfl_297_1;
yy_1_2_1 = ((yy)yy_1_2[1]);
yy_1_2_2 = ((yy)yy_1_2[2]);
yy_1_2_3 = ((yy)yy_1_2[3]);
yy_1_2_4 = ((yy)yy_1_2[4]);
yyv_Class = yy_1_2_2;
if (yy_1_2_3[0] != 1) goto yyfl_297_1;
yy_1_2_3_1 = ((yy)yy_1_2_3[1]);
yy_1_2_3_2 = ((yy)yy_1_2_3[2]);
yyv_I = yy_1_2_3_1;
yyv_O = yy_1_2_3_2;
yyv_Rules = yy_1_2_4;
{
yy yysb = yyb;
yy_2_1_1_1 = yyv_Class;
yy_2_1_1_2 = yy_2_1_1_1;
if (yy_2_1_1_2[0] != 3) goto yyfl_297_1_2_1;
goto yysl_297_1_2;
yyfl_297_1_2_1 : ;
yy_2_2_1_1 = yyv_Class;
yy_2_2_1_2 = yy_2_2_1_1;
if (yy_2_2_1_2[0] != 4) goto yyfl_297_1_2_2;
goto yysl_297_1_2;
yyfl_297_1_2_2 : ;
goto yyfl_297_1;
yysl_297_1_2 : ;
yyb = yysb;
}
return 1;
yyfl_297_1 : ;
}
return 0;
}
CheckNoGrammar_MEMBERS(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_List;
yy yy_0_1_1;
yy yyv_Space;
yy yy_0_1_2;
yy yy_1_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_298_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_List = yy_0_1_1;
yyv_Space = yy_0_1_2;
yy_1_1 = yyv_List;
CheckNoGrammar_MEMBERLIST(yy_1_1);
return;
yyfl_298_1 : ;
}
yyErr(2,2893);
}
CheckNoGrammar_MEMBERLIST(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_T;
yy yy_0_1_2;
yy yy_1_1;
yy yy_2_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_299_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_H = yy_0_1_1;
yyv_T = yy_0_1_2;
yy_1_1 = yyv_H;
CheckNoGrammar_MEMBER(yy_1_1);
yy_2_1 = yyv_T;
CheckNoGrammar_MEMBERLIST(yy_2_1);
return;
yyfl_299_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_299_2;
return;
yyfl_299_2 : ;
}
yyErr(2,2898);
}
CheckNoGrammar_MEMBER(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Predicate;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_InArgs;
yy yy_0_1_3;
yy yyv_OutArgs;
yy yy_0_1_4;
yy yyv_Offset;
yy yy_0_1_5;
yy yy_1_1_1_1;
yy yy_1_1_2_1;
yy yy_1_1_2_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 7) goto yyfl_300_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yy_0_1_5 = ((yy)yy_0_1[5]);
yyv_Predicate = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_InArgs = yy_0_1_3;
yyv_OutArgs = yy_0_1_4;
yyv_Offset = yy_0_1_5;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Predicate;
if (! IsSourceSymbol(yy_1_1_1_1)) goto yyfl_300_1_1_1;
yy_1_1_2_1 = ((yy)"grammar symbol not allowed here");
yy_1_1_2_2 = yyv_P;
MESSAGE(yy_1_1_2_1, yy_1_1_2_2);
goto yysl_300_1_1;
yyfl_300_1_1_1 : ;
goto yysl_300_1_1;
yysl_300_1_1 : ;
yyb = yysb;
}
return;
yyfl_300_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_S;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yy_1_1;
yy yy_1_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 8) goto yyfl_300_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_S = yy_0_1_1;
yyv_P = yy_0_1_2;
yy_1_1 = ((yy)"grammar symbol not allowed here");
yy_1_2 = yyv_P;
MESSAGE(yy_1_1, yy_1_2);
return;
yyfl_300_2 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yy_1_1;
yy yy_1_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 9) goto yyfl_300_3;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_P = yy_0_1_2;
yy_1_1 = ((yy)"grammar symbol not allowed here");
yy_1_2 = yyv_P;
MESSAGE(yy_1_1, yy_1_2);
return;
yyfl_300_3 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Cases;
yy yy_0_1_1;
yy yyv_CommonVars;
yy yy_0_1_2;
yy yyv_Pos;
yy yy_0_1_3;
yy yy_1_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 10) goto yyfl_300_4;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Cases = yy_0_1_1;
yyv_CommonVars = yy_0_1_2;
yyv_Pos = yy_0_1_3;
yy_1_1 = yyv_Cases;
CheckNoGrammar_ALTERNATIVELIST(yy_1_1);
return;
yyfl_300_4 : ;
}
{
yy yyb;
yy yyv_Other;
yy yy_0_1;
yy_0_1 = yyin_1;
yyv_Other = yy_0_1;
return;
yyfl_300_5 : ;
}
yyErr(2,2905);
}
CheckNoGrammar_ALTERNATIVELIST(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_T;
yy yy_0_1_3;
yy yy_1_1;
yy yy_2_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_301_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_H = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_T = yy_0_1_3;
yy_1_1 = yyv_H;
CheckNoGrammar_MEMBERS(yy_1_1);
yy_2_1 = yyv_T;
CheckNoGrammar_ALTERNATIVELIST(yy_2_1);
return;
yyfl_301_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_301_2;
return;
yyfl_301_2 : ;
}
yyErr(2,2920);
}
Code_GrammarSymbols(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_T;
yy yy_0_1_2;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yyv_Id;
yy yy_1_1_1_2_1;
yy yyv_P;
yy yy_1_1_1_2_2;
yy yyv_In;
yy yy_1_1_1_2_3;
yy yyv_Out;
yy yy_1_1_1_2_4;
yy yyv_Offset;
yy yy_1_1_1_2_5;
yy yy_1_1_2_1;
yy yy_1_1_3_1;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yyv_S;
yy yy_1_2_1_2_1;
yy yy_1_2_1_2_2;
yy yy_1_2_2_1;
yy yy_1_2_2_2;
yy yy_1_2_2_3;
yy yy_1_2_3_1;
yy yy_1_2_4_1;
yy yy_1_3_1_1;
yy yy_1_3_1_2;
yy yyv_Arg;
yy yy_1_3_1_2_1;
yy yy_1_3_1_2_2;
yy yy_2_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_302_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_H = yy_0_1_1;
yyv_T = yy_0_1_2;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_H;
yy_1_1_1_2 = yy_1_1_1_1;
if (yy_1_1_1_2[0] != 7) goto yyfl_302_1_1_1;
yy_1_1_1_2_1 = ((yy)yy_1_1_1_2[1]);
yy_1_1_1_2_2 = ((yy)yy_1_1_1_2[2]);
yy_1_1_1_2_3 = ((yy)yy_1_1_1_2[3]);
yy_1_1_1_2_4 = ((yy)yy_1_1_1_2[4]);
yy_1_1_1_2_5 = ((yy)yy_1_1_1_2[5]);
yyv_Id = yy_1_1_1_2_1;
yyv_P = yy_1_1_1_2_2;
yyv_In = yy_1_1_1_2_3;
yyv_Out = yy_1_1_1_2_4;
yyv_Offset = yy_1_1_1_2_5;
yy_1_1_2_1 = ((yy)"   ");
s(yy_1_1_2_1);
yy_1_1_3_1 = yyv_Id;
id(yy_1_1_3_1);
nl();
goto yysl_302_1_1;
yyfl_302_1_1_1 : ;
yy_1_2_1_1 = yyv_H;
yy_1_2_1_2 = yy_1_2_1_1;
if (yy_1_2_1_2[0] != 8) goto yyfl_302_1_1_2;
yy_1_2_1_2_1 = ((yy)yy_1_2_1_2[1]);
yy_1_2_1_2_2 = ((yy)yy_1_2_1_2[2]);
yyv_S = yy_1_2_1_2_1;
yyv_P = yy_1_2_1_2_2;
yy_1_2_2_1 = yyv_S;
yy_1_2_2_2 = yyv_P;
EnterLiteral(yy_1_2_2_1, yy_1_2_2_2, &yy_1_2_2_3);
yyv_Id = yy_1_2_2_3;
yy_1_2_3_1 = ((yy)"   ");
s(yy_1_2_3_1);
yy_1_2_4_1 = yyv_Id;
id(yy_1_2_4_1);
nl();
goto yysl_302_1_1;
yyfl_302_1_1_2 : ;
yy_1_3_1_1 = yyv_H;
yy_1_3_1_2 = yy_1_3_1_1;
if (yy_1_3_1_2[0] != 9) goto yyfl_302_1_1_3;
yy_1_3_1_2_1 = ((yy)yy_1_3_1_2[1]);
yy_1_3_1_2_2 = ((yy)yy_1_3_1_2[2]);
yyv_Arg = yy_1_3_1_2_1;
yyv_P = yy_1_3_1_2_2;
goto yysl_302_1_1;
yyfl_302_1_1_3 : ;
goto yyfl_302_1;
yysl_302_1_1 : ;
yyb = yysb;
}
yy_2_1 = yyv_T;
Code_GrammarSymbols(yy_2_1);
return;
yyfl_302_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_302_2;
return;
yyfl_302_2 : ;
}
yyErr(2,2927);
}
AssignAttributes(yyin_1, yyin_2, yyin_3)
yy yyin_1;
yy yyin_2;
yy yyin_3;
{
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_1_1;
yy yyv_Pos;
yy yy_0_1_1_2;
yy yyv_In;
yy yy_0_1_1_3;
yy yyv_Out;
yy yy_0_1_1_4;
yy yyv_Offset;
yy yy_0_1_1_5;
yy yyv_T;
yy yy_0_1_2;
yy yyv_N;
yy yy_0_2;
yy yyv_DefMem;
yy yy_0_3;
yy yy_1_1;
yy yy_1_2;
yy yy_1_2_1;
yy yy_1_2_2;
yy yy_2_1;
yy yy_2_2;
yy yy_2_2_1;
yy yy_2_2_2;
yy yy_2_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 1) goto yyfl_303_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
if (yy_0_1_1[0] != 7) goto yyfl_303_1;
yy_0_1_1_1 = ((yy)yy_0_1_1[1]);
yy_0_1_1_2 = ((yy)yy_0_1_1[2]);
yy_0_1_1_3 = ((yy)yy_0_1_1[3]);
yy_0_1_1_4 = ((yy)yy_0_1_1[4]);
yy_0_1_1_5 = ((yy)yy_0_1_1[5]);
yyv_P = yy_0_1_1_1;
yyv_Pos = yy_0_1_1_2;
yyv_In = yy_0_1_1_3;
yyv_Out = yy_0_1_1_4;
yyv_Offset = yy_0_1_1_5;
yyv_T = yy_0_1_2;
yyv_N = yy_0_2;
yyv_DefMem = yy_0_3;
yy_1_1 = yyv_Out;
yy_1_2_1 = yyv_N;
yy_1_2_2 = ((yy)1);
yy_1_2 = (yy)(((long)yy_1_2_1)+((long)yy_1_2_2));
AssignAttributes_DEFARGLIST(yy_1_1, yy_1_2);
yy_2_1 = yyv_T;
yy_2_2_1 = yyv_N;
yy_2_2_2 = ((yy)1);
yy_2_2 = (yy)(((long)yy_2_2_1)+((long)yy_2_2_2));
yy_2_3 = yyv_DefMem;
AssignAttributes(yy_2_1, yy_2_2, yy_2_3);
return;
yyfl_303_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_Arg;
yy yy_0_1_1_1;
yy yyv_P;
yy yy_0_1_1_2;
yy yyv_T;
yy yy_0_1_2;
yy yyv_N;
yy yy_0_2;
yy yyv_DefMem;
yy yy_0_3;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yy_1_1_2_1;
yy yy_1_1_2_2;
yy yyv_Tmp;
yy yy_1_1_2_2_1;
yy yyv_Id;
yy yy_1_1_2_2_2;
yy yyv_P2;
yy yy_1_1_2_2_3;
yy yy_1_1_3_1_1_1;
yy yy_1_1_3_1_1_2;
yy yy_1_1_3_1_2_1;
yy yy_1_1_3_1_3_1;
yy yy_1_1_3_1_4_1;
yy yy_1_1_3_2_1_1;
yy yy_1_1_3_2_2_1;
yy yy_1_1_3_2_3_1;
yy yy_1_1_3_2_4_1;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_1_2_2_1;
yy yy_1_2_3_1;
yy yy_1_2_4_1;
yy yy_1_2_5_1;
yy yy_1_2_6_1;
yy yy_1_2_7_1;
yy yy_2_1;
yy yy_2_2;
yy yy_2_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 1) goto yyfl_303_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
if (yy_0_1_1[0] != 9) goto yyfl_303_2;
yy_0_1_1_1 = ((yy)yy_0_1_1[1]);
yy_0_1_1_2 = ((yy)yy_0_1_1[2]);
yyv_Arg = yy_0_1_1_1;
yyv_P = yy_0_1_1_2;
yyv_T = yy_0_1_2;
yyv_N = yy_0_2;
yyv_DefMem = yy_0_3;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_N;
yy_1_1_1_2 = ((yy)0);
if ((long)yy_1_1_1_1 != (long)yy_1_1_1_2) goto yyfl_303_2_1_1;
yy_1_1_2_1 = yyv_Arg;
yy_1_1_2_2 = yy_1_1_2_1;
if (yy_1_1_2_2[0] != 3) goto yyfl_303_2_1_1;
yy_1_1_2_2_1 = ((yy)yy_1_1_2_2[1]);
yy_1_1_2_2_2 = ((yy)yy_1_1_2_2[2]);
yy_1_1_2_2_3 = ((yy)yy_1_1_2_2[3]);
yyv_Tmp = yy_1_1_2_2_1;
yyv_Id = yy_1_1_2_2_2;
yyv_P2 = yy_1_1_2_2_3;
{
yy yysb = yyb;
yy_1_1_3_1_1_1 = yyv_DefMem;
yy_1_1_3_1_1_2 = ((yy)0);
if ((long)yy_1_1_3_1_1_1 != (long)yy_1_1_3_1_1_2) goto yyfl_303_2_1_1_3_1;
yy_1_1_3_1_2_1 = ((yy)"yyGetPos(&");
s(yy_1_1_3_1_2_1);
yy_1_1_3_1_3_1 = yyv_Tmp;
tmp(yy_1_1_3_1_3_1);
yy_1_1_3_1_4_1 = ((yy)");");
s(yy_1_1_3_1_4_1);
nl();
goto yysl_303_2_1_1_3;
yyfl_303_2_1_1_3_1 : ;
yy_1_1_3_2_1_1 = yyv_Tmp;
tmp(yy_1_1_3_2_1_1);
yy_1_1_3_2_2_1 = ((yy)" = (yy)($");
s(yy_1_1_3_2_2_1);
yy_1_1_3_2_3_1 = yyv_DefMem;
i(yy_1_1_3_2_3_1);
yy_1_1_3_2_4_1 = ((yy)".attr[0]);");
s(yy_1_1_3_2_4_1);
nl();
goto yysl_303_2_1_1_3;
yysl_303_2_1_1_3 : ;
yyb = yysb;
}
goto yysl_303_2_1;
yyfl_303_2_1_1 : ;
yy_1_2_1_1 = yyv_Arg;
GetDefArgTmp(yy_1_2_1_1, &yy_1_2_1_2);
yyv_Tmp = yy_1_2_1_2;
yy_1_2_2_1 = yyv_Tmp;
tmp(yy_1_2_2_1);
yy_1_2_3_1 = ((yy)" = (yy)(");
s(yy_1_2_3_1);
yy_1_2_4_1 = ((yy)"$");
s(yy_1_2_4_1);
yy_1_2_5_1 = yyv_N;
i(yy_1_2_5_1);
yy_1_2_6_1 = ((yy)".attr[0]");
s(yy_1_2_6_1);
yy_1_2_7_1 = ((yy)");");
s(yy_1_2_7_1);
nl();
goto yysl_303_2_1;
yysl_303_2_1 : ;
yyb = yysb;
}
yy_2_1 = yyv_T;
yy_2_2 = yyv_N;
yy_2_3 = yyv_DefMem;
AssignAttributes(yy_2_1, yy_2_2, yy_2_3);
return;
yyfl_303_2 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_1_1;
yy yy_0_1_1_2;
yy yyv_T;
yy yy_0_1_2;
yy yyv_N;
yy yy_0_2;
yy yyv_DefMem;
yy yy_0_3;
yy yy_1_1;
yy yy_1_2;
yy yy_1_2_1;
yy yy_1_2_2;
yy yy_1_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 1) goto yyfl_303_3;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
if (yy_0_1_1[0] != 8) goto yyfl_303_3;
yy_0_1_1_1 = ((yy)yy_0_1_1[1]);
yy_0_1_1_2 = ((yy)yy_0_1_1[2]);
yyv_T = yy_0_1_2;
yyv_N = yy_0_2;
yyv_DefMem = yy_0_3;
yy_1_1 = yyv_T;
yy_1_2_1 = yyv_N;
yy_1_2_2 = ((yy)1);
yy_1_2 = (yy)(((long)yy_1_2_1)+((long)yy_1_2_2));
yy_1_3 = yyv_DefMem;
AssignAttributes(yy_1_1, yy_1_2, yy_1_3);
return;
yyfl_303_3 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_N;
yy yy_0_2;
yy yyv_DefMem;
yy yy_0_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 2) goto yyfl_303_4;
yyv_N = yy_0_2;
yyv_DefMem = yy_0_3;
return;
yyfl_303_4 : ;
}
yyErr(2,2945);
}
AssignAttributes_DEFARGLIST(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Arg;
yy yy_0_1_1;
yy yyv_P3;
yy yy_0_1_2;
yy yyv_Tail;
yy yy_0_1_3;
yy yyv_N;
yy yy_0_2;
yy yy_1_1;
yy yyv_Tmp;
yy yy_1_2;
yy yy_2_1;
yy yy_3_1;
yy yy_4_1;
yy yy_4_2;
yy yy_5_1;
yy yy_7_1;
yy yy_7_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_304_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Arg = yy_0_1_1;
yyv_P3 = yy_0_1_2;
yyv_Tail = yy_0_1_3;
yyv_N = yy_0_2;
yy_1_1 = yyv_Arg;
GetDefArgTmp(yy_1_1, &yy_1_2);
yyv_Tmp = yy_1_2;
yy_2_1 = yyv_Tmp;
tmp(yy_2_1);
yy_3_1 = ((yy)" = (yy)(");
s(yy_3_1);
yy_4_1 = yyv_Tmp;
yy_4_2 = yyv_N;
attr(yy_4_1, yy_4_2);
yy_5_1 = ((yy)");");
s(yy_5_1);
nl();
yy_7_1 = yyv_Tail;
yy_7_2 = yyv_N;
AssignAttributes_DEFARGLIST(yy_7_1, yy_7_2);
return;
yyfl_304_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_P;
yy yy_0_1_1;
yy yyv_N;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_304_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yyv_P = yy_0_1_1;
yyv_N = yy_0_2;
return;
yyfl_304_2 : ;
}
yyErr(2,2986);
}
AssignOutAttr(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Arg;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_Tail;
yy yy_0_1_3;
yy yyv_N;
yy yy_0_2;
yy yy_1_1;
yy yyv_Tmp;
yy yy_1_2;
yy yy_2_1;
yy yy_3_1;
yy yy_4_1;
yy yy_5_1;
yy yy_6_1;
yy yy_8_1;
yy yy_8_2;
yy yy_8_2_1;
yy yy_8_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_305_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Arg = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_Tail = yy_0_1_3;
yyv_N = yy_0_2;
yy_1_1 = yyv_Arg;
GetUseArgTmp(yy_1_1, &yy_1_2);
yyv_Tmp = yy_1_2;
yy_2_1 = ((yy)"$$.attr[");
s(yy_2_1);
yy_3_1 = yyv_N;
i(yy_3_1);
yy_4_1 = ((yy)"] = ");
s(yy_4_1);
yy_5_1 = yyv_Tmp;
tmp_i(yy_5_1);
yy_6_1 = ((yy)";");
s(yy_6_1);
nl();
yy_8_1 = yyv_Tail;
yy_8_2_1 = yyv_N;
yy_8_2_2 = ((yy)1);
yy_8_2 = (yy)(((long)yy_8_2_1)+((long)yy_8_2_2));
AssignOutAttr(yy_8_1, yy_8_2);
return;
yyfl_305_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_P;
yy yy_0_1_1;
yy yyv_N;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_305_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yyv_P = yy_0_1_1;
yyv_N = yy_0_2;
return;
yyfl_305_2 : ;
}
yyErr(2,3007);
}
Code_Members(yyin_1, yyin_2, yyin_3)
yy yyin_1;
yy yyin_2;
yy yyin_3;
{
{
yy yyb;
yy yy_0_1;
yy yyv_List;
yy yy_0_1_1;
yy yyv_RefSpace;
yy yy_0_1_2;
yy yyv_TEMPO;
yy yy_0_2;
yy yyv_AddSpace;
yy yy_0_3;
yy yy_1_1;
yy yyv_Space;
yy yy_1_2;
yy yy_2_1;
yy yy_2_1_1;
yy yy_2_1_2;
yy yy_3_1;
yy yy_3_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 1) goto yyfl_306_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_List = yy_0_1_1;
yyv_RefSpace = yy_0_1_2;
yyv_TEMPO = yy_0_2;
yyv_AddSpace = yy_0_3;
yy_1_1 = yyv_RefSpace;
GetINT(yy_1_1, &yy_1_2);
yyv_Space = yy_1_2;
yy_2_1_1 = yyv_Space;
yy_2_1_2 = yyv_AddSpace;
yy_2_1 = (yy)(((long)yy_2_1_1)+((long)yy_2_1_2));
AllocSpace(yy_2_1);
yy_3_1 = yyv_List;
yy_3_2 = yyv_TEMPO;
Code_Memberlist(yy_3_1, yy_3_2);
return;
yyfl_306_1 : ;
}
yyErr(2,3017);
}
AllocSpace(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yyv_Space;
yy yy_0_1;
yy yy_1_1;
yy yy_1_2;
yy_0_1 = yyin_1;
yyv_Space = yy_0_1;
yy_1_1 = yyv_Space;
yy_1_2 = ((yy)0);
if ((long)yy_1_1 != (long)yy_1_2) goto yyfl_307_1;
return;
yyfl_307_1 : ;
}
{
yy yyb;
yy yyv_Space;
yy yy_0_1;
yy yy_1_1;
yy yy_3_1;
yy yy_4_1;
yy yy_5_1;
yy_0_1 = yyin_1;
yyv_Space = yy_0_1;
yy_1_1 = ((yy)"yyb = yyh;");
s(yy_1_1);
nl();
yy_3_1 = ((yy)"yyh += ");
s(yy_3_1);
yy_4_1 = yyv_Space;
i(yy_4_1);
yy_5_1 = ((yy)"; if (yyh > yyhx) yyExtend();");
s(yy_5_1);
nl();
return;
yyfl_307_2 : ;
}
yyErr(2,3024);
}
Code_Memberlist(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_T;
yy yy_0_1_2;
yy yy_0_2;
yy yyv_N;
yy yy_0_2_1;
yy yyv_Tl;
yy yy_0_2_2;
yy yy_1_1;
yy yy_1_2;
yy yy_1_2_1;
yy yy_1_2_2;
yy yy_2_1;
yy yy_2_2;
yy yy_2_2_1;
yy yy_2_2_1_1;
yy yy_2_2_1_2;
yy yy_2_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_308_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_H = yy_0_1_1;
yyv_T = yy_0_1_2;
if (yy_0_2[0] != 1) goto yyfl_308_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yyv_N = yy_0_2_1;
yyv_Tl = yy_0_2_2;
yyb = yyh;
yyh += 6; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_H;
yy_1_2_1 = yyv_N;
yy_1_2_2 = yyv_Tl;
yy_1_2 = yyb + 0;
yy_1_2[0] = 1;
yy_1_2[1] = ((long)yy_1_2_1);
yy_1_2[2] = ((long)yy_1_2_2);
Code_Member(yy_1_1, yy_1_2);
yy_2_1 = yyv_T;
yy_2_2_1_1 = yyv_N;
yy_2_2_1_2 = ((yy)1);
yy_2_2_1 = (yy)(((long)yy_2_2_1_1)+((long)yy_2_2_1_2));
yy_2_2_2 = yyv_Tl;
yy_2_2 = yyb + 3;
yy_2_2[0] = 1;
yy_2_2[1] = ((long)yy_2_2_1);
yy_2_2[2] = ((long)yy_2_2_2);
Code_Memberlist(yy_2_1, yy_2_2);
return;
yyfl_308_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Prefix;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_308_2;
yyv_Prefix = yy_0_2;
return;
yyfl_308_2 : ;
}
yyErr(2,3033);
}
Code_Member(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_OutArg;
yy yy_0_1_1;
yy yyv_InArg;
yy yy_0_1_2;
yy yyv_Pos;
yy yy_0_1_3;
yy yyv_Tmp;
yy yy_0_2;
yy yy_1_1;
yy yyv_Type;
yy yy_1_2;
yy yy_2_1;
yy yy_2_2;
yy yy_2_2_1;
yy yy_2_2_2;
yy yy_2_2_3;
yy yy_3_1;
yy yyv_InT;
yy yy_3_2;
yy yy_4_1;
yy yyv_OutT;
yy yy_4_2;
yy yy_5_1;
yy yy_6_1;
yy yy_7_1;
yy yy_8_1;
yy yy_10_1;
yy yy_10_2;
yy yy_10_2_1;
yy yy_10_2_2;
yy yy_10_2_3;
yy yy_11_1_1_1;
yy yy_11_1_1_2;
yy yy_11_1_1_2_1;
yy yy_11_1_1_2_2;
yy yyv_Functor;
yy yy_11_1_1_2_3;
yy yyv_FunctorPos;
yy yy_11_1_1_2_4;
yy yy_11_1_1_2_5;
yy yy_11_1_2_1;
yy yy_11_1_2_2;
yy yyv_FunctorCode;
yy yy_11_1_2_3;
yy yy_11_1_2_4;
yy yy_11_1_3_1;
yy yy_11_1_3_1_1;
yy yyv_Name;
yy yy_11_1_3_1_2;
yy yy_11_1_3_1_3;
yy yy_11_1_3_2;
yy yy_11_1_4_1;
yy yy_11_1_4_2;
yy yy_11_1_4_3;
yy yy_11_2_1_1;
yy yy_11_2_1_2;
yy yy_11_2_1_2_1;
yy yy_11_2_1_2_2;
yy yy_11_2_1_2_2_1;
yy yy_11_2_1_2_2_2;
yy yy_11_2_1_2_2_3;
yy yy_11_2_1_2_2_4;
yy yy_11_2_1_2_2_5;
yy yy_11_2_1_2_3;
yy yy_11_2_2_1;
yy yy_11_2_2_2;
yy yy_11_2_2_3;
yy yy_11_2_2_4;
yy yy_11_2_3_1;
yy yy_11_2_3_1_1;
yy yy_11_2_3_1_2;
yy yy_11_2_3_1_3;
yy yy_11_2_3_2;
yy yy_11_2_4_1;
yy yy_11_2_4_2;
yy yy_11_2_4_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_312_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_OutArg = yy_0_1_1;
yyv_InArg = yy_0_1_2;
yyv_Pos = yy_0_1_3;
yyv_Tmp = yy_0_2;
yyb = yyh;
yyh += 12; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_InArg;
DetermineType(yy_1_1, &yy_1_2);
yyv_Type = yy_1_2;
yy_2_1 = yyv_InArg;
yy_2_2_1 = yyb + 4;
yy_2_2_1[0] = 2;
yy_2_2_2 = yyv_Type;
yy_2_2_3 = yyb + 5;
yy_2_2_3[0] = 2;
yy_2_2 = yyb + 0;
yy_2_2[0] = 1;
yy_2_2[1] = ((long)yy_2_2_1);
yy_2_2[2] = ((long)yy_2_2_2);
yy_2_2[3] = ((long)yy_2_2_3);
BuildArg(yy_2_1, yy_2_2);
yy_3_1 = yyv_InArg;
GetUseArgTmp(yy_3_1, &yy_3_2);
yyv_InT = yy_3_2;
yy_4_1 = yyv_OutArg;
GetDefArgTmp(yy_4_1, &yy_4_2);
yyv_OutT = yy_4_2;
yy_5_1 = yyv_OutT;
tmp(yy_5_1);
yy_6_1 = ((yy)" = ");
s(yy_6_1);
yy_7_1 = yyv_InT;
tmp(yy_7_1);
yy_8_1 = ((yy)";");
s(yy_8_1);
nl();
yy_10_1 = yyv_OutArg;
yy_10_2_1 = yyb + 10;
yy_10_2_1[0] = 2;
yy_10_2_2 = yyv_Type;
yy_10_2_3 = yyb + 11;
yy_10_2_3[0] = 2;
yy_10_2 = yyb + 6;
yy_10_2[0] = 1;
yy_10_2[1] = ((long)yy_10_2_1);
yy_10_2[2] = ((long)yy_10_2_2);
yy_10_2[3] = ((long)yy_10_2_3);
MatchArg(yy_10_1, yy_10_2);
{
yy yysb = yyb;
yy_11_1_1_1 = yyv_InArg;
yy_11_1_1_2 = yy_11_1_1_1;
if (yy_11_1_1_2[0] != 3) goto yyfl_312_1_11_1;
yy_11_1_1_2_1 = ((yy)yy_11_1_1_2[1]);
yy_11_1_1_2_2 = ((yy)yy_11_1_1_2[2]);
yy_11_1_1_2_3 = ((yy)yy_11_1_1_2[3]);
yy_11_1_1_2_4 = ((yy)yy_11_1_1_2[4]);
yy_11_1_1_2_5 = ((yy)yy_11_1_1_2[5]);
yyv_Functor = yy_11_1_1_2_3;
yyv_FunctorPos = yy_11_1_1_2_4;
yy_11_1_2_1 = yyv_Functor;
yy_11_1_2_2 = yyv_Type;
GetFunctorCode(yy_11_1_2_1, yy_11_1_2_2, &yy_11_1_2_3, &yy_11_1_2_4);
yyv_FunctorCode = yy_11_1_2_3;
yy_11_1_3_2 = yyv_OutArg;
yy_11_1_3_1 = yy_11_1_3_2;
if (yy_11_1_3_1[0] != 3) goto yyfl_312_1_11_1;
yy_11_1_3_1_1 = ((yy)yy_11_1_3_1[1]);
yy_11_1_3_1_2 = ((yy)yy_11_1_3_1[2]);
yy_11_1_3_1_3 = ((yy)yy_11_1_3_1[3]);
yyv_Name = yy_11_1_3_1_2;
yy_11_1_4_1 = yyv_Name;
yy_11_1_4_2 = yyv_Type;
yy_11_1_4_3 = yyv_FunctorCode;
DefLocalMeaning(yy_11_1_4_1, yy_11_1_4_2, yy_11_1_4_3);
goto yysl_312_1_11;
yyfl_312_1_11_1 : ;
yy_11_2_1_1 = yyv_InArg;
yy_11_2_1_2 = yy_11_2_1_1;
if (yy_11_2_1_2[0] != 4) goto yyfl_312_1_11_2;
yy_11_2_1_2_1 = ((yy)yy_11_2_1_2[1]);
yy_11_2_1_2_2 = ((yy)yy_11_2_1_2[2]);
yy_11_2_1_2_3 = ((yy)yy_11_2_1_2[3]);
if (yy_11_2_1_2_2[0] != 3) goto yyfl_312_1_11_2;
yy_11_2_1_2_2_1 = ((yy)yy_11_2_1_2_2[1]);
yy_11_2_1_2_2_2 = ((yy)yy_11_2_1_2_2[2]);
yy_11_2_1_2_2_3 = ((yy)yy_11_2_1_2_2[3]);
yy_11_2_1_2_2_4 = ((yy)yy_11_2_1_2_2[4]);
yy_11_2_1_2_2_5 = ((yy)yy_11_2_1_2_2[5]);
yyv_Functor = yy_11_2_1_2_2_3;
yyv_FunctorPos = yy_11_2_1_2_2_4;
yy_11_2_2_1 = yyv_Functor;
yy_11_2_2_2 = yyv_Type;
GetFunctorCode(yy_11_2_2_1, yy_11_2_2_2, &yy_11_2_2_3, &yy_11_2_2_4);
yyv_FunctorCode = yy_11_2_2_3;
yy_11_2_3_2 = yyv_OutArg;
yy_11_2_3_1 = yy_11_2_3_2;
if (yy_11_2_3_1[0] != 3) goto yyfl_312_1_11_2;
yy_11_2_3_1_1 = ((yy)yy_11_2_3_1[1]);
yy_11_2_3_1_2 = ((yy)yy_11_2_3_1[2]);
yy_11_2_3_1_3 = ((yy)yy_11_2_3_1[3]);
yyv_Name = yy_11_2_3_1_2;
yy_11_2_4_1 = yyv_Name;
yy_11_2_4_2 = yyv_Type;
yy_11_2_4_3 = yyv_FunctorCode;
DefLocalMeaning(yy_11_2_4_1, yy_11_2_4_2, yy_11_2_4_3);
goto yysl_312_1_11;
yyfl_312_1_11_2 : ;
goto yysl_312_1_11;
yysl_312_1_11 : ;
yyb = yysb;
}
return;
yyfl_312_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Predicate;
yy yy_0_1_1;
yy yyv_Pos;
yy yy_0_1_2;
yy yyv_InArgs;
yy yy_0_1_3;
yy yyv_OutArgs;
yy yy_0_1_4;
yy yyv_Offset;
yy yy_0_1_5;
yy yyv_Tmp;
yy yy_0_2;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yy_1_1_2_1_1_1_1_1;
yy yy_1_1_2_1_1_1_1_2;
yy yy_1_1_2_1_1_2_1_1;
yy yy_1_1_2_1_1_2_1_2;
yy yy_1_1_2_1_1_3_1_1;
yy yy_1_1_2_1_1_3_1_2;
yy yy_1_1_2_1_1_4_1_1;
yy yy_1_1_2_1_1_4_1_2;
yy yy_1_1_2_1_2_1_1_1;
yy yy_1_1_2_1_2_1_1_2;
yy yyv_InArg;
yy yy_1_1_2_1_2_1_1_2_1;
yy yy_1_1_2_1_2_1_1_2_2;
yy yy_1_1_2_1_2_1_1_2_3;
yy yy_1_1_2_1_2_1_1_2_3_1;
yy yy_1_1_2_1_2_1_2_1;
yy yy_1_1_2_1_2_1_2_2;
yy yyv_OutArg;
yy yy_1_1_2_1_2_1_2_2_1;
yy yy_1_1_2_1_2_1_2_2_2;
yy yy_1_1_2_1_2_1_2_2_3;
yy yy_1_1_2_1_2_1_2_2_3_1;
yy yy_1_1_2_1_2_1_3_1;
yy yyv_Type;
yy yy_1_1_2_1_2_1_3_2;
yy yy_1_1_2_1_2_1_4_1;
yy yy_1_1_2_1_2_1_4_2;
yy yy_1_1_2_1_2_1_4_2_1;
yy yy_1_1_2_1_2_1_4_2_2;
yy yy_1_1_2_1_2_1_4_2_3;
yy yy_1_1_2_1_2_1_5_1;
yy yyv_InT;
yy yy_1_1_2_1_2_1_5_2;
yy yy_1_1_2_1_2_1_6_1;
yy yyv_OutT;
yy yy_1_1_2_1_2_1_6_2;
yy yy_1_1_2_1_2_1_7_1;
yy yy_1_1_2_1_2_1_8_1;
yy yy_1_1_2_1_2_1_9_1;
yy yy_1_1_2_1_2_1_10_1;
yy yy_1_1_2_1_2_1_12_1;
yy yy_1_1_2_1_2_1_12_2;
yy yy_1_1_2_1_2_1_12_2_1;
yy yy_1_1_2_1_2_1_12_2_2;
yy yy_1_1_2_1_2_1_12_2_3;
yy yy_1_1_2_1_2_2_1_1;
yy yy_1_1_2_1_2_2_1_2;
yy yy_1_1_2_2_1_1;
yy yy_1_1_2_2_1_2;
yy yy_1_1_2_2_2_1_1_1;
yy yy_1_1_2_2_2_1_1_2;
yy yy_1_1_2_2_2_1_1_2_1;
yy yy_1_1_2_2_2_1_1_2_2;
yy yy_1_1_2_2_2_1_1_2_3;
yy yy_1_1_2_2_2_1_1_2_3_1;
yy yy_1_1_2_2_2_1_2_1;
yy yy_1_1_2_2_2_1_2_2;
yy yy_1_1_2_2_2_1_2_2_1;
yy yy_1_1_2_2_2_1_2_2_2;
yy yy_1_1_2_2_2_1_2_2_3;
yy yy_1_1_2_2_2_1_2_2_3_1;
yy yy_1_1_2_2_2_1_3_1_1_1;
yy yy_1_1_2_2_2_1_3_1_1_2;
yy yy_1_1_2_2_2_1_3_1_1_2_1;
yy yyv_Name;
yy yy_1_1_2_2_2_1_3_1_1_2_2;
yy yyv_POutVar;
yy yy_1_1_2_2_2_1_3_1_1_2_3;
yy yy_1_1_2_2_2_1_3_1_2_1_1_1;
yy yy_1_1_2_2_2_1_3_1_2_1_1_2;
yy yy_1_1_2_2_2_1_3_1_2_2_1_1;
yy yy_1_1_2_2_2_1_3_1_2_2_1_2;
yy yy_1_1_2_2_2_1_3_1_2_2_1_3;
yy yy_1_1_2_2_2_1_3_1_2_2_1_4;
yy yy_1_1_2_2_2_1_3_1_2_2_2;
yy yy_1_1_2_2_2_1_3_2_1_1;
yy yy_1_1_2_2_2_1_3_2_1_2;
yy yy_1_1_2_2_2_1_3_2_2;
yy yy_1_1_2_2_2_1_3_2_3_1;
yy yy_1_1_2_2_2_1_3_2_3_2;
yy yy_1_1_2_2_2_1_4_1;
yy yy_1_1_2_2_2_1_4_2;
yy yy_1_1_2_2_2_1_4_2_1;
yy yy_1_1_2_2_2_1_4_2_2;
yy yy_1_1_2_2_2_1_4_2_3;
yy yy_1_1_2_2_2_1_5_1;
yy yy_1_1_2_2_2_1_5_2;
yy yy_1_1_2_2_2_1_6_1;
yy yy_1_1_2_2_2_1_7_1;
yy yy_1_1_2_2_2_1_8_1;
yy yy_1_1_2_2_2_1_9_1;
yy yy_1_1_2_2_2_2_1_1;
yy yy_1_1_2_2_2_2_1_2;
yy yy_1_1_2_3_1_1;
yy yy_1_1_2_3_1_2;
yy yy_1_1_2_3_2_1_1_1;
yy yy_1_1_2_3_2_1_1_2;
yy yyv_Arg;
yy yy_1_1_2_3_2_1_1_2_1;
yy yy_1_1_2_3_2_1_1_2_2;
yy yy_1_1_2_3_2_1_1_2_3;
yy yy_1_1_2_3_2_1_1_2_3_1;
yy yy_1_1_2_3_2_1_2_1;
yy yy_1_1_2_3_2_1_2_2;
yy yy_1_1_2_3_2_1_3_1;
yy yy_1_1_2_3_2_1_3_2;
yy yy_1_1_2_3_2_1_3_2_1;
yy yy_1_1_2_3_2_1_3_2_2;
yy yy_1_1_2_3_2_1_3_2_3;
yy yy_1_1_2_3_2_1_4_1;
yy yyv_ArgTmp;
yy yy_1_1_2_3_2_1_4_2;
yy yy_1_1_2_3_2_1_5_1;
yy yy_1_1_2_3_2_1_6_1;
yy yy_1_1_2_3_2_1_7_1;
yy yy_1_1_2_3_2_1_8_1;
yy yy_1_1_2_3_2_1_9_1;
yy yy_1_1_2_3_2_1_11_1;
yy yy_1_1_2_3_2_2_1_1;
yy yy_1_1_2_3_2_2_1_2;
yy yy_1_1_2_4_1_1;
yy yy_1_1_2_4_1_2;
yy yy_1_1_2_4_2_1_1_1;
yy yy_1_1_2_4_2_1_1_2;
yy yy_1_1_2_4_2_1_1_2_1;
yy yy_1_1_2_4_2_1_2_1;
yy yy_1_1_2_4_2_1_2_2;
yy yy_1_1_2_4_2_1_2_2_1;
yy yy_1_1_2_4_2_2_1_1;
yy yy_1_1_2_4_2_2_1_2;
yy yy_1_1_2_5_1_1;
yy yy_1_1_2_5_2_1_1_1;
yy yy_1_1_2_5_2_1_1_2;
yy yyv_InArg1;
yy yy_1_1_2_5_2_1_1_2_1;
yy yy_1_1_2_5_2_1_1_2_2;
yy yy_1_1_2_5_2_1_1_2_3;
yy yyv_InArg2;
yy yy_1_1_2_5_2_1_1_2_3_1;
yy yy_1_1_2_5_2_1_1_2_3_2;
yy yy_1_1_2_5_2_1_1_2_3_3;
yy yy_1_1_2_5_2_1_1_2_3_3_1;
yy yy_1_1_2_5_2_1_2_1;
yy yy_1_1_2_5_2_1_2_2;
yy yy_1_1_2_5_2_1_3_1;
yy yy_1_1_2_5_2_1_3_2;
yy yy_1_1_2_5_2_1_3_2_1;
yy yy_1_1_2_5_2_1_3_2_2;
yy yy_1_1_2_5_2_1_3_2_3;
yy yy_1_1_2_5_2_1_4_1;
yy yy_1_1_2_5_2_1_4_2;
yy yy_1_1_2_5_2_1_4_2_1;
yy yy_1_1_2_5_2_1_4_2_2;
yy yy_1_1_2_5_2_1_4_2_3;
yy yy_1_1_2_5_2_1_5_1;
yy yyv_T1;
yy yy_1_1_2_5_2_1_5_2;
yy yy_1_1_2_5_2_1_6_1;
yy yyv_T2;
yy yy_1_1_2_5_2_1_6_2;
yy yy_1_1_2_5_2_1_7_1_1_1_1_1;
yy yy_1_1_2_5_2_1_7_1_1_1_1_2;
yy yy_1_1_2_5_2_1_7_1_1_2_1_1;
yy yy_1_1_2_5_2_1_7_1_1_2_1_2;
yy yy_1_1_2_5_2_1_7_1_2_1;
yy yy_1_1_2_5_2_1_7_1_3_1;
yy yy_1_1_2_5_2_1_7_1_4_1;
yy yy_1_1_2_5_2_1_7_1_5_1;
yy yy_1_1_2_5_2_1_7_1_6_1;
yy yy_1_1_2_5_2_1_7_1_7_1;
yy yy_1_1_2_5_2_1_7_2_1_1;
yy yy_1_1_2_5_2_1_7_2_1_2;
yy yy_1_1_2_5_2_1_7_2_2_1;
yy yy_1_1_2_5_2_1_7_2_3_1;
yy yy_1_1_2_5_2_1_7_2_4_1;
yy yy_1_1_2_5_2_1_7_2_5_1;
yy yy_1_1_2_5_2_1_7_2_6_1;
yy yy_1_1_2_5_2_1_7_2_7_1;
yy yy_1_1_2_5_2_1_7_2_8_1;
yy yy_1_1_2_5_2_1_7_3_1_1_1_1;
yy yy_1_1_2_5_2_1_7_3_1_1_1_2;
yy yy_1_1_2_5_2_1_7_3_1_1_2_1;
yy yy_1_1_2_5_2_1_7_3_1_1_3_1;
yy yy_1_1_2_5_2_1_7_3_1_1_4_1;
yy yy_1_1_2_5_2_1_7_3_1_1_5_1;
yy yy_1_1_2_5_2_1_7_3_1_1_6_1;
yy yy_1_1_2_5_2_1_7_3_1_1_7_1;
yy yy_1_1_2_5_2_1_7_3_1_1_8_1;
yy yy_1_1_2_5_2_1_7_3_1_2_1_1;
yy yy_1_1_2_5_2_1_7_3_1_2_1_2;
yy yy_1_1_2_5_2_1_7_3_1_2_2_1;
yy yy_1_1_2_5_2_1_7_3_1_2_3_1;
yy yy_1_1_2_5_2_1_7_3_1_2_4_1;
yy yy_1_1_2_5_2_1_7_3_1_2_5_1;
yy yy_1_1_2_5_2_1_7_3_1_2_6_1;
yy yy_1_1_2_5_2_1_7_3_1_2_7_1;
yy yy_1_1_2_5_2_1_7_3_1_2_8_1;
yy yy_1_1_2_5_2_1_7_3_1_3_1_1;
yy yy_1_1_2_5_2_1_7_3_1_3_1_2;
yy yy_1_1_2_5_2_1_7_3_1_3_1_3;
yy yy_1_1_2_5_2_1_7_3_1_3_1_4;
yy yy_1_1_2_5_2_2_1_1;
yy yy_1_1_2_5_2_2_1_2;
yy yy_1_2_1_1;
yy yy_1_2_2_1;
yy yy_1_2_2_2;
yy yy_1_2_2_3;
yy yy_1_2_2_4;
yy yy_1_3_1_1;
yy yy_1_3_1_2;
yy yy_1_3_1_3;
yy yyv_CodeName;
yy yy_1_3_1_4;
yy yyv_Class;
yy yy_1_3_1_5;
yy yyv_FormalInArgs;
yy yy_1_3_1_6;
yy yyv_FormalOutArgs;
yy yy_1_3_1_7;
yy yy_1_3_2_1_1_1_1_1;
yy yy_1_3_2_1_1_1_1_2;
yy yy_1_3_2_1_1_2_1_1;
yy yy_1_3_2_1_1_2_1_2;
yy yy_1_3_2_1_2_1;
yy yy_1_3_2_1_2_2;
yy yy_1_3_2_1_3_1;
yy yy_1_3_2_1_3_2;
yy yy_1_3_2_2_1_1;
yy yy_1_3_2_2_2_1;
yy yy_1_3_2_2_2_2;
yy yy_1_3_2_2_3_1;
yy yy_1_3_2_2_3_2;
yy yy_1_3_2_2_3_3;
yy yy_1_3_2_2_3_4;
yy yy_1_3_2_2_4_1;
yy yy_1_3_2_2_4_2;
yy yy_1_3_2_3_1_1;
yy yy_1_3_2_3_1_2;
yy yy_1_3_2_3_2_1_1_1;
yy yy_1_3_2_3_2_1_1_2;
yy yy_1_3_2_3_2_1_1_2_1;
yy yy_1_3_2_3_2_1_1_2_2;
yy yyv_ArgListTail;
yy yy_1_3_2_3_2_1_1_2_3;
yy yy_1_3_2_3_2_1_2_1;
yy yy_1_3_2_3_2_1_2_2;
yy yy_1_3_2_3_2_1_3_1;
yy yy_1_3_2_3_2_1_3_2;
yy yy_1_3_2_3_2_1_3_2_1;
yy yy_1_3_2_3_2_1_3_2_2;
yy yy_1_3_2_3_2_1_3_2_3;
yy yy_1_3_2_3_2_2_1_1;
yy yy_1_3_2_3_2_2_1_2;
yy yy_1_3_2_3_2_2_2_1;
yy yy_1_3_2_3_2_2_2_1_1;
yy yy_1_3_2_3_2_2_2_2;
yy yy_1_3_2_3_2_2_3;
yy yy_1_3_2_3_3_1_1_1;
yy yy_1_3_2_3_3_1_1_2;
yy yy_1_3_2_3_3_1_1_2_1;
yy yy_1_3_2_3_3_1_1_2_2;
yy yyv_FormalInArgsTail;
yy yy_1_3_2_3_3_1_1_2_3;
yy yy_1_3_2_3_3_2_1_1;
yy yy_1_3_2_3_3_2_1_2;
yy yy_1_3_2_3_4_1;
yy yy_1_3_2_3_4_2;
yy yy_1_3_2_3_5_1;
yy yy_1_3_2_3_7_1;
yy yy_1_3_2_3_8_1;
yy yy_1_3_2_3_9_1;
yy yy_1_3_2_3_11_1;
yy yy_1_3_2_3_12_1;
yy yy_1_3_2_3_13_1;
yy yy_1_3_2_3_14_1_1_1;
yy yy_1_3_2_3_14_1_1_2;
yy yy_1_3_2_3_14_1_1_2_1;
yy yy_1_3_2_3_14_1_1_2_2;
yy yy_1_3_2_3_14_1_1_2_3;
yy yy_1_3_2_3_14_1_1_2_3_1;
yy yy_1_3_2_3_14_1_2_1;
yy yy_1_3_2_3_14_1_2_2;
yy yyv_Sep;
yy yy_1_3_2_3_14_1_2_3;
yy yy_1_3_2_3_14_1_3_1;
yy yy_1_3_2_3_14_2_1_1;
yy yy_1_3_2_3_14_2_1_2;
yy yy_1_3_2_3_14_2_1_3;
yy yy_1_3_2_3_15_1_1_1;
yy yy_1_3_2_3_15_1_1_2;
yy yy_1_3_2_3_15_1_1_2_1;
yy yy_1_3_2_3_15_1_2_1;
yy yy_1_3_2_3_15_2_1_1;
yy yy_1_3_2_3_15_2_1_2;
yy yy_1_3_2_3_16_1;
yy yy_1_3_2_3_17_1;
yy yy_1_3_2_3_18_1;
yy yy_1_3_2_3_20_1;
yy yy_1_3_2_3_22_1;
yy yy_1_3_2_3_22_2;
yy yy_1_3_2_4_1_1;
yy yy_1_3_2_4_1_2;
yy yy_1_3_2_4_2_1;
yy yy_1_3_2_4_2_2;
yy yy_1_3_2_4_3_1;
yy yy_1_3_2_4_3_2;
yy yy_1_3_2_4_3_3;
yy yy_1_3_2_4_3_4;
yy yy_1_3_2_4_4_1;
yy yy_1_3_2_4_4_2;
yy yy_1_4_1_1;
yy yy_1_4_1_2;
yy yy_1_4_1_3;
yy yy_1_4_1_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 7) goto yyfl_312_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yy_0_1_5 = ((yy)yy_0_1[5]);
yyv_Predicate = yy_0_1_1;
yyv_Pos = yy_0_1_2;
yyv_InArgs = yy_0_1_3;
yyv_OutArgs = yy_0_1_4;
yyv_Offset = yy_0_1_5;
yyv_Tmp = yy_0_2;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Predicate;
if (! GetGlobalMeaning(yy_1_1_1_1, &yy_1_1_1_2)) goto yyfl_312_2_1_1;
if (yy_1_1_1_2[0] != 5) goto yyfl_312_2_1_1;
{
yy yysb = yyb;
{
yy yysb = yyb;
yy_1_1_2_1_1_1_1_1 = yyv_Predicate;
yy_1_1_2_1_1_1_1_2 = ((yy)"is");
if (! IsIdent(yy_1_1_2_1_1_1_1_1, yy_1_1_2_1_1_1_1_2)) goto yyfl_312_2_1_1_2_1_1_1;
goto yysl_312_2_1_1_2_1_1;
yyfl_312_2_1_1_2_1_1_1 : ;
yy_1_1_2_1_1_2_1_1 = yyv_Predicate;
yy_1_1_2_1_1_2_1_2 = ((yy)"val");
if (! IsIdent(yy_1_1_2_1_1_2_1_1, yy_1_1_2_1_1_2_1_2)) goto yyfl_312_2_1_1_2_1_1_2;
goto yysl_312_2_1_1_2_1_1;
yyfl_312_2_1_1_2_1_1_2 : ;
yy_1_1_2_1_1_3_1_1 = yyv_Predicate;
yy_1_1_2_1_1_3_1_2 = ((yy)"where");
if (! IsIdent(yy_1_1_2_1_1_3_1_1, yy_1_1_2_1_1_3_1_2)) goto yyfl_312_2_1_1_2_1_1_3;
goto yysl_312_2_1_1_2_1_1;
yyfl_312_2_1_1_2_1_1_3 : ;
yy_1_1_2_1_1_4_1_1 = yyv_Predicate;
yy_1_1_2_1_1_4_1_2 = ((yy)"let");
if (! IsIdent(yy_1_1_2_1_1_4_1_1, yy_1_1_2_1_1_4_1_2)) goto yyfl_312_2_1_1_2_1_1_4;
goto yysl_312_2_1_1_2_1_1;
yyfl_312_2_1_1_2_1_1_4 : ;
goto yyfl_312_2_1_1_2_1;
yysl_312_2_1_1_2_1_1 : ;
yyb = yysb;
}
{
yy yysb = yyb;
yyb = yyh;
yyh += 12; if (yyh > yyhx) yyExtend();
yy_1_1_2_1_2_1_1_1 = yyv_InArgs;
yy_1_1_2_1_2_1_1_2 = yy_1_1_2_1_2_1_1_1;
if (yy_1_1_2_1_2_1_1_2[0] != 1) goto yyfl_312_2_1_1_2_1_2_1;
yy_1_1_2_1_2_1_1_2_1 = ((yy)yy_1_1_2_1_2_1_1_2[1]);
yy_1_1_2_1_2_1_1_2_2 = ((yy)yy_1_1_2_1_2_1_1_2[2]);
yy_1_1_2_1_2_1_1_2_3 = ((yy)yy_1_1_2_1_2_1_1_2[3]);
yyv_InArg = yy_1_1_2_1_2_1_1_2_1;
if (yy_1_1_2_1_2_1_1_2_3[0] != 2) goto yyfl_312_2_1_1_2_1_2_1;
yy_1_1_2_1_2_1_1_2_3_1 = ((yy)yy_1_1_2_1_2_1_1_2_3[1]);
yy_1_1_2_1_2_1_2_1 = yyv_OutArgs;
yy_1_1_2_1_2_1_2_2 = yy_1_1_2_1_2_1_2_1;
if (yy_1_1_2_1_2_1_2_2[0] != 1) goto yyfl_312_2_1_1_2_1_2_1;
yy_1_1_2_1_2_1_2_2_1 = ((yy)yy_1_1_2_1_2_1_2_2[1]);
yy_1_1_2_1_2_1_2_2_2 = ((yy)yy_1_1_2_1_2_1_2_2[2]);
yy_1_1_2_1_2_1_2_2_3 = ((yy)yy_1_1_2_1_2_1_2_2[3]);
yyv_OutArg = yy_1_1_2_1_2_1_2_2_1;
if (yy_1_1_2_1_2_1_2_2_3[0] != 2) goto yyfl_312_2_1_1_2_1_2_1;
yy_1_1_2_1_2_1_2_2_3_1 = ((yy)yy_1_1_2_1_2_1_2_2_3[1]);
yy_1_1_2_1_2_1_3_1 = yyv_InArg;
DetermineType(yy_1_1_2_1_2_1_3_1, &yy_1_1_2_1_2_1_3_2);
yyv_Type = yy_1_1_2_1_2_1_3_2;
yy_1_1_2_1_2_1_4_1 = yyv_InArg;
yy_1_1_2_1_2_1_4_2_1 = yyb + 4;
yy_1_1_2_1_2_1_4_2_1[0] = 2;
yy_1_1_2_1_2_1_4_2_2 = yyv_Type;
yy_1_1_2_1_2_1_4_2_3 = yyb + 5;
yy_1_1_2_1_2_1_4_2_3[0] = 2;
yy_1_1_2_1_2_1_4_2 = yyb + 0;
yy_1_1_2_1_2_1_4_2[0] = 1;
yy_1_1_2_1_2_1_4_2[1] = ((long)yy_1_1_2_1_2_1_4_2_1);
yy_1_1_2_1_2_1_4_2[2] = ((long)yy_1_1_2_1_2_1_4_2_2);
yy_1_1_2_1_2_1_4_2[3] = ((long)yy_1_1_2_1_2_1_4_2_3);
BuildArg(yy_1_1_2_1_2_1_4_1, yy_1_1_2_1_2_1_4_2);
yy_1_1_2_1_2_1_5_1 = yyv_InArg;
GetUseArgTmp(yy_1_1_2_1_2_1_5_1, &yy_1_1_2_1_2_1_5_2);
yyv_InT = yy_1_1_2_1_2_1_5_2;
yy_1_1_2_1_2_1_6_1 = yyv_OutArg;
GetDefArgTmp(yy_1_1_2_1_2_1_6_1, &yy_1_1_2_1_2_1_6_2);
yyv_OutT = yy_1_1_2_1_2_1_6_2;
yy_1_1_2_1_2_1_7_1 = yyv_OutT;
tmp(yy_1_1_2_1_2_1_7_1);
yy_1_1_2_1_2_1_8_1 = ((yy)" = ");
s(yy_1_1_2_1_2_1_8_1);
yy_1_1_2_1_2_1_9_1 = yyv_InT;
tmp(yy_1_1_2_1_2_1_9_1);
yy_1_1_2_1_2_1_10_1 = ((yy)";");
s(yy_1_1_2_1_2_1_10_1);
nl();
yy_1_1_2_1_2_1_12_1 = yyv_OutArg;
yy_1_1_2_1_2_1_12_2_1 = yyb + 10;
yy_1_1_2_1_2_1_12_2_1[0] = 2;
yy_1_1_2_1_2_1_12_2_2 = yyv_Type;
yy_1_1_2_1_2_1_12_2_3 = yyb + 11;
yy_1_1_2_1_2_1_12_2_3[0] = 2;
yy_1_1_2_1_2_1_12_2 = yyb + 6;
yy_1_1_2_1_2_1_12_2[0] = 1;
yy_1_1_2_1_2_1_12_2[1] = ((long)yy_1_1_2_1_2_1_12_2_1);
yy_1_1_2_1_2_1_12_2[2] = ((long)yy_1_1_2_1_2_1_12_2_2);
yy_1_1_2_1_2_1_12_2[3] = ((long)yy_1_1_2_1_2_1_12_2_3);
MatchArg(yy_1_1_2_1_2_1_12_1, yy_1_1_2_1_2_1_12_2);
goto yysl_312_2_1_1_2_1_2;
yyfl_312_2_1_1_2_1_2_1 : ;
yy_1_1_2_1_2_2_1_1 = ((yy)"invalid number of arguments");
yy_1_1_2_1_2_2_1_2 = yyv_Pos;
MESSAGE(yy_1_1_2_1_2_2_1_1, yy_1_1_2_1_2_2_1_2);
goto yysl_312_2_1_1_2_1_2;
yysl_312_2_1_1_2_1_2 : ;
yyb = yysb;
}
goto yysl_312_2_1_1_2;
yyfl_312_2_1_1_2_1 : ;
yy_1_1_2_2_1_1 = yyv_Predicate;
yy_1_1_2_2_1_2 = ((yy)"wherenext");
if (! IsIdent(yy_1_1_2_2_1_1, yy_1_1_2_2_1_2)) goto yyfl_312_2_1_1_2_2;
{
yy yysb = yyb;
yyb = yyh;
yyh += 6; if (yyh > yyhx) yyExtend();
yy_1_1_2_2_2_1_1_1 = yyv_InArgs;
yy_1_1_2_2_2_1_1_2 = yy_1_1_2_2_2_1_1_1;
if (yy_1_1_2_2_2_1_1_2[0] != 1) goto yyfl_312_2_1_1_2_2_2_1;
yy_1_1_2_2_2_1_1_2_1 = ((yy)yy_1_1_2_2_2_1_1_2[1]);
yy_1_1_2_2_2_1_1_2_2 = ((yy)yy_1_1_2_2_2_1_1_2[2]);
yy_1_1_2_2_2_1_1_2_3 = ((yy)yy_1_1_2_2_2_1_1_2[3]);
yyv_InArg = yy_1_1_2_2_2_1_1_2_1;
if (yy_1_1_2_2_2_1_1_2_3[0] != 2) goto yyfl_312_2_1_1_2_2_2_1;
yy_1_1_2_2_2_1_1_2_3_1 = ((yy)yy_1_1_2_2_2_1_1_2_3[1]);
yy_1_1_2_2_2_1_2_1 = yyv_OutArgs;
yy_1_1_2_2_2_1_2_2 = yy_1_1_2_2_2_1_2_1;
if (yy_1_1_2_2_2_1_2_2[0] != 1) goto yyfl_312_2_1_1_2_2_2_1;
yy_1_1_2_2_2_1_2_2_1 = ((yy)yy_1_1_2_2_2_1_2_2[1]);
yy_1_1_2_2_2_1_2_2_2 = ((yy)yy_1_1_2_2_2_1_2_2[2]);
yy_1_1_2_2_2_1_2_2_3 = ((yy)yy_1_1_2_2_2_1_2_2[3]);
yyv_OutArg = yy_1_1_2_2_2_1_2_2_1;
if (yy_1_1_2_2_2_1_2_2_3[0] != 2) goto yyfl_312_2_1_1_2_2_2_1;
yy_1_1_2_2_2_1_2_2_3_1 = ((yy)yy_1_1_2_2_2_1_2_2_3[1]);
{
yy yysb = yyb;
yy_1_1_2_2_2_1_3_1_1_1 = yyv_OutArg;
yy_1_1_2_2_2_1_3_1_1_2 = yy_1_1_2_2_2_1_3_1_1_1;
if (yy_1_1_2_2_2_1_3_1_1_2[0] != 3) goto yyfl_312_2_1_1_2_2_2_1_3_1;
yy_1_1_2_2_2_1_3_1_1_2_1 = ((yy)yy_1_1_2_2_2_1_3_1_1_2[1]);
yy_1_1_2_2_2_1_3_1_1_2_2 = ((yy)yy_1_1_2_2_2_1_3_1_1_2[2]);
yy_1_1_2_2_2_1_3_1_1_2_3 = ((yy)yy_1_1_2_2_2_1_3_1_1_2[3]);
yyv_Name = yy_1_1_2_2_2_1_3_1_1_2_2;
yyv_POutVar = yy_1_1_2_2_2_1_3_1_1_2_3;
{
yy yysb = yyb;
yy_1_1_2_2_2_1_3_1_2_1_1_1 = yyv_Name;
if (! GetLocalMeaning(yy_1_1_2_2_2_1_3_1_2_1_1_1, &yy_1_1_2_2_2_1_3_1_2_1_1_2)) goto yyfl_312_2_1_1_2_2_2_1_3_1_2_1;
yyv_Type = yy_1_1_2_2_2_1_3_1_2_1_1_2;
goto yysl_312_2_1_1_2_2_2_1_3_1_2;
yyfl_312_2_1_1_2_2_2_1_3_1_2_1 : ;
yy_1_1_2_2_2_1_3_1_2_2_1_1 = ((yy)"'");
yy_1_1_2_2_2_1_3_1_2_2_1_2 = yyv_Name;
yy_1_1_2_2_2_1_3_1_2_2_1_3 = ((yy)"' is not defined");
yy_1_1_2_2_2_1_3_1_2_2_1_4 = yyv_POutVar;
MESSAGE1(yy_1_1_2_2_2_1_3_1_2_2_1_1, yy_1_1_2_2_2_1_3_1_2_2_1_2, yy_1_1_2_2_2_1_3_1_2_2_1_3, yy_1_1_2_2_2_1_3_1_2_2_1_4);
yy_1_1_2_2_2_1_3_1_2_2_2 = yyglov_id_INT;
if (yy_1_1_2_2_2_1_3_1_2_2_2 == (yy) yyu) yyErr(1,3100);
yyv_Type = yy_1_1_2_2_2_1_3_1_2_2_2;
goto yysl_312_2_1_1_2_2_2_1_3_1_2;
yysl_312_2_1_1_2_2_2_1_3_1_2 : ;
yyb = yysb;
}
goto yysl_312_2_1_1_2_2_2_1_3;
yyfl_312_2_1_1_2_2_2_1_3_1 : ;
yy_1_1_2_2_2_1_3_2_1_1 = ((yy)"output parameter must be a variable");
yy_1_1_2_2_2_1_3_2_1_2 = yyv_Pos;
MESSAGE(yy_1_1_2_2_2_1_3_2_1_1, yy_1_1_2_2_2_1_3_2_1_2);
yy_1_1_2_2_2_1_3_2_2 = yyglov_id_INT;
if (yy_1_1_2_2_2_1_3_2_2 == (yy) yyu) yyErr(1,3104);
yyv_Type = yy_1_1_2_2_2_1_3_2_2;
yy_1_1_2_2_2_1_3_2_3_1 = ((yy)"<noname>");
string_to_id(yy_1_1_2_2_2_1_3_2_3_1, &yy_1_1_2_2_2_1_3_2_3_2);
yyv_Name = yy_1_1_2_2_2_1_3_2_3_2;
goto yysl_312_2_1_1_2_2_2_1_3;
yysl_312_2_1_1_2_2_2_1_3 : ;
yyb = yysb;
}
yy_1_1_2_2_2_1_4_1 = yyv_InArg;
yy_1_1_2_2_2_1_4_2_1 = yyb + 4;
yy_1_1_2_2_2_1_4_2_1[0] = 2;
yy_1_1_2_2_2_1_4_2_2 = yyv_Type;
yy_1_1_2_2_2_1_4_2_3 = yyb + 5;
yy_1_1_2_2_2_1_4_2_3[0] = 2;
yy_1_1_2_2_2_1_4_2 = yyb + 0;
yy_1_1_2_2_2_1_4_2[0] = 1;
yy_1_1_2_2_2_1_4_2[1] = ((long)yy_1_1_2_2_2_1_4_2_1);
yy_1_1_2_2_2_1_4_2[2] = ((long)yy_1_1_2_2_2_1_4_2_2);
yy_1_1_2_2_2_1_4_2[3] = ((long)yy_1_1_2_2_2_1_4_2_3);
BuildArg(yy_1_1_2_2_2_1_4_1, yy_1_1_2_2_2_1_4_2);
yy_1_1_2_2_2_1_5_1 = yyv_InArg;
GetUseArgTmp(yy_1_1_2_2_2_1_5_1, &yy_1_1_2_2_2_1_5_2);
yyv_InT = yy_1_1_2_2_2_1_5_2;
yy_1_1_2_2_2_1_6_1 = yyv_Name;
varid(yy_1_1_2_2_2_1_6_1);
yy_1_1_2_2_2_1_7_1 = ((yy)" = ");
s(yy_1_1_2_2_2_1_7_1);
yy_1_1_2_2_2_1_8_1 = yyv_InT;
tmp(yy_1_1_2_2_2_1_8_1);
yy_1_1_2_2_2_1_9_1 = ((yy)";");
s(yy_1_1_2_2_2_1_9_1);
nl();
goto yysl_312_2_1_1_2_2_2;
yyfl_312_2_1_1_2_2_2_1 : ;
yy_1_1_2_2_2_2_1_1 = ((yy)"invalid number of arguments");
yy_1_1_2_2_2_2_1_2 = yyv_Pos;
MESSAGE(yy_1_1_2_2_2_2_1_1, yy_1_1_2_2_2_2_1_2);
goto yysl_312_2_1_1_2_2_2;
yysl_312_2_1_1_2_2_2 : ;
yyb = yysb;
}
goto yysl_312_2_1_1_2;
yyfl_312_2_1_1_2_2 : ;
yy_1_1_2_3_1_1 = yyv_Predicate;
yy_1_1_2_3_1_2 = ((yy)"print");
if (! IsIdent(yy_1_1_2_3_1_1, yy_1_1_2_3_1_2)) goto yyfl_312_2_1_1_2_3;
{
yy yysb = yyb;
yyb = yyh;
yyh += 6; if (yyh > yyhx) yyExtend();
yy_1_1_2_3_2_1_1_1 = yyv_InArgs;
yy_1_1_2_3_2_1_1_2 = yy_1_1_2_3_2_1_1_1;
if (yy_1_1_2_3_2_1_1_2[0] != 1) goto yyfl_312_2_1_1_2_3_2_1;
yy_1_1_2_3_2_1_1_2_1 = ((yy)yy_1_1_2_3_2_1_1_2[1]);
yy_1_1_2_3_2_1_1_2_2 = ((yy)yy_1_1_2_3_2_1_1_2[2]);
yy_1_1_2_3_2_1_1_2_3 = ((yy)yy_1_1_2_3_2_1_1_2[3]);
yyv_Arg = yy_1_1_2_3_2_1_1_2_1;
if (yy_1_1_2_3_2_1_1_2_3[0] != 2) goto yyfl_312_2_1_1_2_3_2_1;
yy_1_1_2_3_2_1_1_2_3_1 = ((yy)yy_1_1_2_3_2_1_1_2_3[1]);
yy_1_1_2_3_2_1_2_1 = yyv_Arg;
DetermineType(yy_1_1_2_3_2_1_2_1, &yy_1_1_2_3_2_1_2_2);
yyv_Type = yy_1_1_2_3_2_1_2_2;
yy_1_1_2_3_2_1_3_1 = yyv_Arg;
yy_1_1_2_3_2_1_3_2_1 = yyb + 4;
yy_1_1_2_3_2_1_3_2_1[0] = 2;
yy_1_1_2_3_2_1_3_2_2 = yyv_Type;
yy_1_1_2_3_2_1_3_2_3 = yyb + 5;
yy_1_1_2_3_2_1_3_2_3[0] = 2;
yy_1_1_2_3_2_1_3_2 = yyb + 0;
yy_1_1_2_3_2_1_3_2[0] = 1;
yy_1_1_2_3_2_1_3_2[1] = ((long)yy_1_1_2_3_2_1_3_2_1);
yy_1_1_2_3_2_1_3_2[2] = ((long)yy_1_1_2_3_2_1_3_2_2);
yy_1_1_2_3_2_1_3_2[3] = ((long)yy_1_1_2_3_2_1_3_2_3);
BuildArg(yy_1_1_2_3_2_1_3_1, yy_1_1_2_3_2_1_3_2);
yy_1_1_2_3_2_1_4_1 = yyv_Arg;
GetUseArgTmp(yy_1_1_2_3_2_1_4_1, &yy_1_1_2_3_2_1_4_2);
yyv_ArgTmp = yy_1_1_2_3_2_1_4_2;
yy_1_1_2_3_2_1_5_1 = ((yy)"yyPrint_");
s(yy_1_1_2_3_2_1_5_1);
yy_1_1_2_3_2_1_6_1 = yyv_Type;
id(yy_1_1_2_3_2_1_6_1);
yy_1_1_2_3_2_1_7_1 = ((yy)"(");
s(yy_1_1_2_3_2_1_7_1);
yy_1_1_2_3_2_1_8_1 = yyv_ArgTmp;
tmp(yy_1_1_2_3_2_1_8_1);
yy_1_1_2_3_2_1_9_1 = ((yy)");");
s(yy_1_1_2_3_2_1_9_1);
nl();
yy_1_1_2_3_2_1_11_1 = ((yy)"yyEndPrint();");
s(yy_1_1_2_3_2_1_11_1);
nl();
goto yysl_312_2_1_1_2_3_2;
yyfl_312_2_1_1_2_3_2_1 : ;
yy_1_1_2_3_2_2_1_1 = ((yy)"invalid number of arguments");
yy_1_1_2_3_2_2_1_2 = yyv_Pos;
MESSAGE(yy_1_1_2_3_2_2_1_1, yy_1_1_2_3_2_2_1_2);
goto yysl_312_2_1_1_2_3_2;
yysl_312_2_1_1_2_3_2 : ;
yyb = yysb;
}
goto yysl_312_2_1_1_2;
yyfl_312_2_1_1_2_3 : ;
yy_1_1_2_4_1_1 = yyv_Predicate;
yy_1_1_2_4_1_2 = ((yy)"skip");
if (! IsIdent(yy_1_1_2_4_1_1, yy_1_1_2_4_1_2)) goto yyfl_312_2_1_1_2_4;
{
yy yysb = yyb;
yy_1_1_2_4_2_1_1_1 = yyv_InArgs;
yy_1_1_2_4_2_1_1_2 = yy_1_1_2_4_2_1_1_1;
if (yy_1_1_2_4_2_1_1_2[0] != 2) goto yyfl_312_2_1_1_2_4_2_1;
yy_1_1_2_4_2_1_1_2_1 = ((yy)yy_1_1_2_4_2_1_1_2[1]);
yy_1_1_2_4_2_1_2_1 = yyv_OutArgs;
yy_1_1_2_4_2_1_2_2 = yy_1_1_2_4_2_1_2_1;
if (yy_1_1_2_4_2_1_2_2[0] != 2) goto yyfl_312_2_1_1_2_4_2_1;
yy_1_1_2_4_2_1_2_2_1 = ((yy)yy_1_1_2_4_2_1_2_2[1]);
goto yysl_312_2_1_1_2_4_2;
yyfl_312_2_1_1_2_4_2_1 : ;
yy_1_1_2_4_2_2_1_1 = ((yy)"invalid number of arguments");
yy_1_1_2_4_2_2_1_2 = yyv_Pos;
MESSAGE(yy_1_1_2_4_2_2_1_1, yy_1_1_2_4_2_2_1_2);
goto yysl_312_2_1_1_2_4_2;
yysl_312_2_1_1_2_4_2 : ;
yyb = yysb;
}
goto yysl_312_2_1_1_2;
yyfl_312_2_1_1_2_4 : ;
yy_1_1_2_5_1_1 = yyv_Predicate;
if (! IsRelation(yy_1_1_2_5_1_1)) goto yyfl_312_2_1_1_2_5;
{
yy yysb = yyb;
yyb = yyh;
yyh += 12; if (yyh > yyhx) yyExtend();
yy_1_1_2_5_2_1_1_1 = yyv_InArgs;
yy_1_1_2_5_2_1_1_2 = yy_1_1_2_5_2_1_1_1;
if (yy_1_1_2_5_2_1_1_2[0] != 1) goto yyfl_312_2_1_1_2_5_2_1;
yy_1_1_2_5_2_1_1_2_1 = ((yy)yy_1_1_2_5_2_1_1_2[1]);
yy_1_1_2_5_2_1_1_2_2 = ((yy)yy_1_1_2_5_2_1_1_2[2]);
yy_1_1_2_5_2_1_1_2_3 = ((yy)yy_1_1_2_5_2_1_1_2[3]);
yyv_InArg1 = yy_1_1_2_5_2_1_1_2_1;
if (yy_1_1_2_5_2_1_1_2_3[0] != 1) goto yyfl_312_2_1_1_2_5_2_1;
yy_1_1_2_5_2_1_1_2_3_1 = ((yy)yy_1_1_2_5_2_1_1_2_3[1]);
yy_1_1_2_5_2_1_1_2_3_2 = ((yy)yy_1_1_2_5_2_1_1_2_3[2]);
yy_1_1_2_5_2_1_1_2_3_3 = ((yy)yy_1_1_2_5_2_1_1_2_3[3]);
yyv_InArg2 = yy_1_1_2_5_2_1_1_2_3_1;
if (yy_1_1_2_5_2_1_1_2_3_3[0] != 2) goto yyfl_312_2_1_1_2_5_2_1;
yy_1_1_2_5_2_1_1_2_3_3_1 = ((yy)yy_1_1_2_5_2_1_1_2_3_3[1]);
yy_1_1_2_5_2_1_2_1 = yyv_InArg1;
DetermineType(yy_1_1_2_5_2_1_2_1, &yy_1_1_2_5_2_1_2_2);
yyv_Type = yy_1_1_2_5_2_1_2_2;
yy_1_1_2_5_2_1_3_1 = yyv_InArg1;
yy_1_1_2_5_2_1_3_2_1 = yyb + 4;
yy_1_1_2_5_2_1_3_2_1[0] = 2;
yy_1_1_2_5_2_1_3_2_2 = yyv_Type;
yy_1_1_2_5_2_1_3_2_3 = yyb + 5;
yy_1_1_2_5_2_1_3_2_3[0] = 2;
yy_1_1_2_5_2_1_3_2 = yyb + 0;
yy_1_1_2_5_2_1_3_2[0] = 1;
yy_1_1_2_5_2_1_3_2[1] = ((long)yy_1_1_2_5_2_1_3_2_1);
yy_1_1_2_5_2_1_3_2[2] = ((long)yy_1_1_2_5_2_1_3_2_2);
yy_1_1_2_5_2_1_3_2[3] = ((long)yy_1_1_2_5_2_1_3_2_3);
BuildArg(yy_1_1_2_5_2_1_3_1, yy_1_1_2_5_2_1_3_2);
yy_1_1_2_5_2_1_4_1 = yyv_InArg2;
yy_1_1_2_5_2_1_4_2_1 = yyb + 10;
yy_1_1_2_5_2_1_4_2_1[0] = 2;
yy_1_1_2_5_2_1_4_2_2 = yyv_Type;
yy_1_1_2_5_2_1_4_2_3 = yyb + 11;
yy_1_1_2_5_2_1_4_2_3[0] = 2;
yy_1_1_2_5_2_1_4_2 = yyb + 6;
yy_1_1_2_5_2_1_4_2[0] = 1;
yy_1_1_2_5_2_1_4_2[1] = ((long)yy_1_1_2_5_2_1_4_2_1);
yy_1_1_2_5_2_1_4_2[2] = ((long)yy_1_1_2_5_2_1_4_2_2);
yy_1_1_2_5_2_1_4_2[3] = ((long)yy_1_1_2_5_2_1_4_2_3);
BuildArg(yy_1_1_2_5_2_1_4_1, yy_1_1_2_5_2_1_4_2);
yy_1_1_2_5_2_1_5_1 = yyv_InArg1;
GetUseArgTmp(yy_1_1_2_5_2_1_5_1, &yy_1_1_2_5_2_1_5_2);
yyv_T1 = yy_1_1_2_5_2_1_5_2;
yy_1_1_2_5_2_1_6_1 = yyv_InArg2;
GetUseArgTmp(yy_1_1_2_5_2_1_6_1, &yy_1_1_2_5_2_1_6_2);
yyv_T2 = yy_1_1_2_5_2_1_6_2;
{
yy yysb = yyb;
{
yy yysb = yyb;
yy_1_1_2_5_2_1_7_1_1_1_1_1 = yyv_Type;
yy_1_1_2_5_2_1_7_1_1_1_1_2 = ((yy)"INT");
if (! IsIdent(yy_1_1_2_5_2_1_7_1_1_1_1_1, yy_1_1_2_5_2_1_7_1_1_1_1_2)) goto yyfl_312_2_1_1_2_5_2_1_7_1_1_1;
goto yysl_312_2_1_1_2_5_2_1_7_1_1;
yyfl_312_2_1_1_2_5_2_1_7_1_1_1 : ;
yy_1_1_2_5_2_1_7_1_1_2_1_1 = yyv_Type;
yy_1_1_2_5_2_1_7_1_1_2_1_2 = ((yy)"POS");
if (! IsIdent(yy_1_1_2_5_2_1_7_1_1_2_1_1, yy_1_1_2_5_2_1_7_1_1_2_1_2)) goto yyfl_312_2_1_1_2_5_2_1_7_1_1_2;
goto yysl_312_2_1_1_2_5_2_1_7_1_1;
yyfl_312_2_1_1_2_5_2_1_7_1_1_2 : ;
goto yyfl_312_2_1_1_2_5_2_1_7_1;
yysl_312_2_1_1_2_5_2_1_7_1_1 : ;
yyb = yysb;
}
yy_1_1_2_5_2_1_7_1_2_1 = ((yy)"if ((intptr_t)");
s(yy_1_1_2_5_2_1_7_1_2_1);
yy_1_1_2_5_2_1_7_1_3_1 = yyv_T1;
tmp(yy_1_1_2_5_2_1_7_1_3_1);
yy_1_1_2_5_2_1_7_1_4_1 = yyv_Predicate;
NegIntRel(yy_1_1_2_5_2_1_7_1_4_1);
yy_1_1_2_5_2_1_7_1_5_1 = ((yy)"(intptr_t)");
s(yy_1_1_2_5_2_1_7_1_5_1);
yy_1_1_2_5_2_1_7_1_6_1 = yyv_T2;
tmp(yy_1_1_2_5_2_1_7_1_6_1);
yy_1_1_2_5_2_1_7_1_7_1 = ((yy)") ");
s(yy_1_1_2_5_2_1_7_1_7_1);
Fail();
nl();
goto yysl_312_2_1_1_2_5_2_1_7;
yyfl_312_2_1_1_2_5_2_1_7_1 : ;
yy_1_1_2_5_2_1_7_2_1_1 = yyv_Type;
yy_1_1_2_5_2_1_7_2_1_2 = ((yy)"STRING");
if (! IsIdent(yy_1_1_2_5_2_1_7_2_1_1, yy_1_1_2_5_2_1_7_2_1_2)) goto yyfl_312_2_1_1_2_5_2_1_7_2;
yy_1_1_2_5_2_1_7_2_2_1 = ((yy)"if (strcmp((char *) ");
s(yy_1_1_2_5_2_1_7_2_2_1);
yy_1_1_2_5_2_1_7_2_3_1 = yyv_T1;
tmp(yy_1_1_2_5_2_1_7_2_3_1);
yy_1_1_2_5_2_1_7_2_4_1 = ((yy)", (char *) ");
s(yy_1_1_2_5_2_1_7_2_4_1);
yy_1_1_2_5_2_1_7_2_5_1 = yyv_T2;
tmp(yy_1_1_2_5_2_1_7_2_5_1);
yy_1_1_2_5_2_1_7_2_6_1 = ((yy)") ");
s(yy_1_1_2_5_2_1_7_2_6_1);
yy_1_1_2_5_2_1_7_2_7_1 = yyv_Predicate;
NegIntRel(yy_1_1_2_5_2_1_7_2_7_1);
yy_1_1_2_5_2_1_7_2_8_1 = ((yy)" 0) ");
s(yy_1_1_2_5_2_1_7_2_8_1);
Fail();
nl();
goto yysl_312_2_1_1_2_5_2_1_7;
yyfl_312_2_1_1_2_5_2_1_7_2 : ;
{
yy yysb = yyb;
yy_1_1_2_5_2_1_7_3_1_1_1_1 = yyv_Predicate;
yy_1_1_2_5_2_1_7_3_1_1_1_2 = ((yy)"eq");
if (! IsIdent(yy_1_1_2_5_2_1_7_3_1_1_1_1, yy_1_1_2_5_2_1_7_3_1_1_1_2)) goto yyfl_312_2_1_1_2_5_2_1_7_3_1_1;
yy_1_1_2_5_2_1_7_3_1_1_2_1 = ((yy)"if (! yyeq_");
s(yy_1_1_2_5_2_1_7_3_1_1_2_1);
yy_1_1_2_5_2_1_7_3_1_1_3_1 = yyv_Type;
id(yy_1_1_2_5_2_1_7_3_1_1_3_1);
yy_1_1_2_5_2_1_7_3_1_1_4_1 = ((yy)"(");
s(yy_1_1_2_5_2_1_7_3_1_1_4_1);
yy_1_1_2_5_2_1_7_3_1_1_5_1 = yyv_T1;
tmp(yy_1_1_2_5_2_1_7_3_1_1_5_1);
yy_1_1_2_5_2_1_7_3_1_1_6_1 = ((yy)", ");
s(yy_1_1_2_5_2_1_7_3_1_1_6_1);
yy_1_1_2_5_2_1_7_3_1_1_7_1 = yyv_T2;
tmp(yy_1_1_2_5_2_1_7_3_1_1_7_1);
yy_1_1_2_5_2_1_7_3_1_1_8_1 = ((yy)")) ");
s(yy_1_1_2_5_2_1_7_3_1_1_8_1);
Fail();
nl();
goto yysl_312_2_1_1_2_5_2_1_7_3_1;
yyfl_312_2_1_1_2_5_2_1_7_3_1_1 : ;
yy_1_1_2_5_2_1_7_3_1_2_1_1 = yyv_Predicate;
yy_1_1_2_5_2_1_7_3_1_2_1_2 = ((yy)"ne");
if (! IsIdent(yy_1_1_2_5_2_1_7_3_1_2_1_1, yy_1_1_2_5_2_1_7_3_1_2_1_2)) goto yyfl_312_2_1_1_2_5_2_1_7_3_1_2;
yy_1_1_2_5_2_1_7_3_1_2_2_1 = ((yy)"if (yyeq_");
s(yy_1_1_2_5_2_1_7_3_1_2_2_1);
yy_1_1_2_5_2_1_7_3_1_2_3_1 = yyv_Type;
id(yy_1_1_2_5_2_1_7_3_1_2_3_1);
yy_1_1_2_5_2_1_7_3_1_2_4_1 = ((yy)"(");
s(yy_1_1_2_5_2_1_7_3_1_2_4_1);
yy_1_1_2_5_2_1_7_3_1_2_5_1 = yyv_T1;
tmp(yy_1_1_2_5_2_1_7_3_1_2_5_1);
yy_1_1_2_5_2_1_7_3_1_2_6_1 = ((yy)", ");
s(yy_1_1_2_5_2_1_7_3_1_2_6_1);
yy_1_1_2_5_2_1_7_3_1_2_7_1 = yyv_T2;
tmp(yy_1_1_2_5_2_1_7_3_1_2_7_1);
yy_1_1_2_5_2_1_7_3_1_2_8_1 = ((yy)")) ");
s(yy_1_1_2_5_2_1_7_3_1_2_8_1);
Fail();
nl();
goto yysl_312_2_1_1_2_5_2_1_7_3_1;
yyfl_312_2_1_1_2_5_2_1_7_3_1_2 : ;
yy_1_1_2_5_2_1_7_3_1_3_1_1 = ((yy)"relation not defined for type '");
yy_1_1_2_5_2_1_7_3_1_3_1_2 = yyv_Type;
yy_1_1_2_5_2_1_7_3_1_3_1_3 = ((yy)"'");
yy_1_1_2_5_2_1_7_3_1_3_1_4 = yyv_Pos;
MESSAGE1(yy_1_1_2_5_2_1_7_3_1_3_1_1, yy_1_1_2_5_2_1_7_3_1_3_1_2, yy_1_1_2_5_2_1_7_3_1_3_1_3, yy_1_1_2_5_2_1_7_3_1_3_1_4);
goto yysl_312_2_1_1_2_5_2_1_7_3_1;
yysl_312_2_1_1_2_5_2_1_7_3_1 : ;
yyb = yysb;
}
goto yysl_312_2_1_1_2_5_2_1_7;
yysl_312_2_1_1_2_5_2_1_7 : ;
yyb = yysb;
}
goto yysl_312_2_1_1_2_5_2;
yyfl_312_2_1_1_2_5_2_1 : ;
yy_1_1_2_5_2_2_1_1 = ((yy)"invalid number of arguments");
yy_1_1_2_5_2_2_1_2 = yyv_Pos;
MESSAGE(yy_1_1_2_5_2_2_1_1, yy_1_1_2_5_2_2_1_2);
goto yysl_312_2_1_1_2_5_2;
yysl_312_2_1_1_2_5_2 : ;
yyb = yysb;
}
goto yysl_312_2_1_1_2;
yyfl_312_2_1_1_2_5 : ;
goto yyfl_312_2_1_1;
yysl_312_2_1_1_2 : ;
yyb = yysb;
}
goto yysl_312_2_1;
yyfl_312_2_1_1 : ;
yy_1_2_1_1 = yyv_Predicate;
if (! IsTable(yy_1_2_1_1)) goto yyfl_312_2_1_2;
yy_1_2_2_1 = ((yy)"'");
yy_1_2_2_2 = yyv_Predicate;
yy_1_2_2_3 = ((yy)"' cannot be used as predicate");
yy_1_2_2_4 = yyv_Pos;
MESSAGE1(yy_1_2_2_1, yy_1_2_2_2, yy_1_2_2_3, yy_1_2_2_4);
goto yysl_312_2_1;
yyfl_312_2_1_2 : ;
yy_1_3_1_1 = yyv_Predicate;
yy_1_3_1_2 = yyv_InArgs;
yy_1_3_1_3 = yyv_Pos;
if (! IsPredicate(yy_1_3_1_1, yy_1_3_1_2, yy_1_3_1_3, &yy_1_3_1_4, &yy_1_3_1_5, &yy_1_3_1_6, &yy_1_3_1_7)) goto yyfl_312_2_1_3;
yyv_CodeName = yy_1_3_1_4;
yyv_Class = yy_1_3_1_5;
yyv_FormalInArgs = yy_1_3_1_6;
yyv_FormalOutArgs = yy_1_3_1_7;
{
yy yysb = yyb;
{
yy yysb = yyb;
yy_1_3_2_1_1_1_1_1 = yyv_Class;
yy_1_3_2_1_1_1_1_2 = yy_1_3_2_1_1_1_1_1;
if (yy_1_3_2_1_1_1_1_2[0] != 3) goto yyfl_312_2_1_3_2_1_1_1;
goto yysl_312_2_1_3_2_1_1;
yyfl_312_2_1_3_2_1_1_1 : ;
yy_1_3_2_1_1_2_1_1 = yyv_Class;
yy_1_3_2_1_1_2_1_2 = yy_1_3_2_1_1_2_1_1;
if (yy_1_3_2_1_1_2_1_2[0] != 4) goto yyfl_312_2_1_3_2_1_1_2;
goto yysl_312_2_1_3_2_1_1;
yyfl_312_2_1_3_2_1_1_2 : ;
goto yyfl_312_2_1_3_2_1;
yysl_312_2_1_3_2_1_1 : ;
yyb = yysb;
}
yy_1_3_2_1_2_1 = yyv_InArgs;
yy_1_3_2_1_2_2 = yyv_FormalInArgs;
Build(yy_1_3_2_1_2_1, yy_1_3_2_1_2_2);
yy_1_3_2_1_3_1 = yyv_OutArgs;
yy_1_3_2_1_3_2 = yyv_FormalOutArgs;
MatchArgs(yy_1_3_2_1_3_1, yy_1_3_2_1_3_2);
goto yysl_312_2_1_3_2;
yyfl_312_2_1_3_2_1 : ;
yy_1_3_2_2_1_1 = yyv_Class;
if (! ProcOrCond(yy_1_3_2_2_1_1)) goto yyfl_312_2_1_3_2_2;
yy_1_3_2_2_2_1 = yyv_InArgs;
yy_1_3_2_2_2_2 = yyv_FormalInArgs;
Build(yy_1_3_2_2_2_1, yy_1_3_2_2_2_2);
yy_1_3_2_2_3_1 = yyv_Class;
yy_1_3_2_2_3_2 = yyv_CodeName;
yy_1_3_2_2_3_3 = yyv_InArgs;
yy_1_3_2_2_3_4 = yyv_OutArgs;
Call(yy_1_3_2_2_3_1, yy_1_3_2_2_3_2, yy_1_3_2_2_3_3, yy_1_3_2_2_3_4);
yy_1_3_2_2_4_1 = yyv_OutArgs;
yy_1_3_2_2_4_2 = yyv_FormalOutArgs;
MatchArgs(yy_1_3_2_2_4_1, yy_1_3_2_2_4_2);
goto yysl_312_2_1_3_2;
yyfl_312_2_1_3_2_2 : ;
yy_1_3_2_3_1_1 = yyv_Class;
yy_1_3_2_3_1_2 = yy_1_3_2_3_1_1;
if (yy_1_3_2_3_1_2[0] != 6) goto yyfl_312_2_1_3_2_3;
{
yy yysb = yyb;
yyb = yyh;
yyh += 6; if (yyh > yyhx) yyExtend();
yy_1_3_2_3_2_1_1_1 = yyv_InArgs;
yy_1_3_2_3_2_1_1_2 = yy_1_3_2_3_2_1_1_1;
if (yy_1_3_2_3_2_1_1_2[0] != 1) goto yyfl_312_2_1_3_2_3_2_1;
yy_1_3_2_3_2_1_1_2_1 = ((yy)yy_1_3_2_3_2_1_1_2[1]);
yy_1_3_2_3_2_1_1_2_2 = ((yy)yy_1_3_2_3_2_1_1_2[2]);
yy_1_3_2_3_2_1_1_2_3 = ((yy)yy_1_3_2_3_2_1_1_2[3]);
yyv_Arg = yy_1_3_2_3_2_1_1_2_1;
yyv_ArgListTail = yy_1_3_2_3_2_1_1_2_3;
yy_1_3_2_3_2_1_2_1 = yyv_Arg;
DetermineType(yy_1_3_2_3_2_1_2_1, &yy_1_3_2_3_2_1_2_2);
yyv_Type = yy_1_3_2_3_2_1_2_2;
yy_1_3_2_3_2_1_3_1 = yyv_Arg;
yy_1_3_2_3_2_1_3_2_1 = yyb + 4;
yy_1_3_2_3_2_1_3_2_1[0] = 2;
yy_1_3_2_3_2_1_3_2_2 = yyv_Type;
yy_1_3_2_3_2_1_3_2_3 = yyb + 5;
yy_1_3_2_3_2_1_3_2_3[0] = 2;
yy_1_3_2_3_2_1_3_2 = yyb + 0;
yy_1_3_2_3_2_1_3_2[0] = 1;
yy_1_3_2_3_2_1_3_2[1] = ((long)yy_1_3_2_3_2_1_3_2_1);
yy_1_3_2_3_2_1_3_2[2] = ((long)yy_1_3_2_3_2_1_3_2_2);
yy_1_3_2_3_2_1_3_2[3] = ((long)yy_1_3_2_3_2_1_3_2_3);
BuildArg(yy_1_3_2_3_2_1_3_1, yy_1_3_2_3_2_1_3_2);
goto yysl_312_2_1_3_2_3_2;
yyfl_312_2_1_3_2_3_2_1 : ;
yyb = yyh;
yyh += 2; if (yyh > yyhx) yyExtend();
yy_1_3_2_3_2_2_1_1 = ((yy)"invalid number of arguments");
yy_1_3_2_3_2_2_1_2 = yyv_Pos;
MESSAGE(yy_1_3_2_3_2_2_1_1, yy_1_3_2_3_2_2_1_2);
yy_1_3_2_3_2_2_2_1_1 = yyv_Pos;
yy_1_3_2_3_2_2_2_1 = yyb + 0;
yy_1_3_2_3_2_2_2_1[0] = 2;
yy_1_3_2_3_2_2_2_1[1] = ((long)yy_1_3_2_3_2_2_2_1_1);
yy_1_3_2_3_2_2_2_2 = yy_1_3_2_3_2_2_2_1;
yyv_ArgListTail = yy_1_3_2_3_2_2_2_2;
yy_1_3_2_3_2_2_3 = yyglov_id_INT;
if (yy_1_3_2_3_2_2_3 == (yy) yyu) yyErr(1,3213);
yyv_Type = yy_1_3_2_3_2_2_3;
goto yysl_312_2_1_3_2_3_2;
yysl_312_2_1_3_2_3_2 : ;
yyb = yysb;
}
{
yy yysb = yyb;
yy_1_3_2_3_3_1_1_1 = yyv_FormalInArgs;
yy_1_3_2_3_3_1_1_2 = yy_1_3_2_3_3_1_1_1;
if (yy_1_3_2_3_3_1_1_2[0] != 1) goto yyfl_312_2_1_3_2_3_3_1;
yy_1_3_2_3_3_1_1_2_1 = ((yy)yy_1_3_2_3_3_1_1_2[1]);
yy_1_3_2_3_3_1_1_2_2 = ((yy)yy_1_3_2_3_3_1_1_2[2]);
yy_1_3_2_3_3_1_1_2_3 = ((yy)yy_1_3_2_3_3_1_1_2[3]);
yyv_FormalInArgsTail = yy_1_3_2_3_3_1_1_2_3;
goto yysl_312_2_1_3_2_3_3;
yyfl_312_2_1_3_2_3_3_1 : ;
yy_1_3_2_3_3_2_1_1 = yyv_FormalInArgs;
yy_1_3_2_3_3_2_1_2 = yy_1_3_2_3_3_2_1_1;
yyv_FormalInArgsTail = yy_1_3_2_3_3_2_1_2;
goto yysl_312_2_1_3_2_3_3;
yysl_312_2_1_3_2_3_3 : ;
yyb = yysb;
}
yy_1_3_2_3_4_1 = yyv_ArgListTail;
yy_1_3_2_3_4_2 = yyv_FormalInArgsTail;
Build(yy_1_3_2_3_4_1, yy_1_3_2_3_4_2);
yy_1_3_2_3_5_1 = ((yy)"{");
s(yy_1_3_2_3_5_1);
nl();
yy_1_3_2_3_7_1 = ((yy)"extern ");
s(yy_1_3_2_3_7_1);
yy_1_3_2_3_8_1 = yyv_CodeName;
name(yy_1_3_2_3_8_1);
yy_1_3_2_3_9_1 = ((yy)"();");
s(yy_1_3_2_3_9_1);
nl();
yy_1_3_2_3_11_1 = ((yy)"yybroadcast_");
s(yy_1_3_2_3_11_1);
yy_1_3_2_3_12_1 = yyv_Type;
id(yy_1_3_2_3_12_1);
yy_1_3_2_3_13_1 = ((yy)"(");
s(yy_1_3_2_3_13_1);
{
yy yysb = yyb;
yy_1_3_2_3_14_1_1_1 = yyv_InArgs;
yy_1_3_2_3_14_1_1_2 = yy_1_3_2_3_14_1_1_1;
if (yy_1_3_2_3_14_1_1_2[0] != 1) goto yyfl_312_2_1_3_2_3_14_1;
yy_1_3_2_3_14_1_1_2_1 = ((yy)yy_1_3_2_3_14_1_1_2[1]);
yy_1_3_2_3_14_1_1_2_2 = ((yy)yy_1_3_2_3_14_1_1_2[2]);
yy_1_3_2_3_14_1_1_2_3 = ((yy)yy_1_3_2_3_14_1_1_2[3]);
if (yy_1_3_2_3_14_1_1_2_3[0] != 2) goto yyfl_312_2_1_3_2_3_14_1;
yy_1_3_2_3_14_1_1_2_3_1 = ((yy)yy_1_3_2_3_14_1_1_2_3[1]);
yy_1_3_2_3_14_1_2_1 = yyv_InArgs;
yy_1_3_2_3_14_1_2_2 = ((yy)"");
ListInArgs(yy_1_3_2_3_14_1_2_1, yy_1_3_2_3_14_1_2_2, &yy_1_3_2_3_14_1_2_3);
yyv_Sep = yy_1_3_2_3_14_1_2_3;
yy_1_3_2_3_14_1_3_1 = ((yy)", 0");
s(yy_1_3_2_3_14_1_3_1);
goto yysl_312_2_1_3_2_3_14;
yyfl_312_2_1_3_2_3_14_1 : ;
yy_1_3_2_3_14_2_1_1 = yyv_InArgs;
yy_1_3_2_3_14_2_1_2 = ((yy)"");
ListInArgs(yy_1_3_2_3_14_2_1_1, yy_1_3_2_3_14_2_1_2, &yy_1_3_2_3_14_2_1_3);
yyv_Sep = yy_1_3_2_3_14_2_1_3;
goto yysl_312_2_1_3_2_3_14;
yysl_312_2_1_3_2_3_14 : ;
yyb = yysb;
}
{
yy yysb = yyb;
yy_1_3_2_3_15_1_1_1 = yyv_OutArgs;
yy_1_3_2_3_15_1_1_2 = yy_1_3_2_3_15_1_1_1;
if (yy_1_3_2_3_15_1_1_2[0] != 2) goto yyfl_312_2_1_3_2_3_15_1;
yy_1_3_2_3_15_1_1_2_1 = ((yy)yy_1_3_2_3_15_1_1_2[1]);
yy_1_3_2_3_15_1_2_1 = ((yy)", &yynull");
s(yy_1_3_2_3_15_1_2_1);
goto yysl_312_2_1_3_2_3_15;
yyfl_312_2_1_3_2_3_15_1 : ;
yy_1_3_2_3_15_2_1_1 = yyv_OutArgs;
yy_1_3_2_3_15_2_1_2 = yyv_Sep;
ListOutArgs(yy_1_3_2_3_15_2_1_1, yy_1_3_2_3_15_2_1_2);
goto yysl_312_2_1_3_2_3_15;
yysl_312_2_1_3_2_3_15 : ;
yyb = yysb;
}
yy_1_3_2_3_16_1 = ((yy)", ");
s(yy_1_3_2_3_16_1);
yy_1_3_2_3_17_1 = yyv_CodeName;
name(yy_1_3_2_3_17_1);
yy_1_3_2_3_18_1 = ((yy)");");
s(yy_1_3_2_3_18_1);
nl();
yy_1_3_2_3_20_1 = ((yy)"}");
s(yy_1_3_2_3_20_1);
nl();
yy_1_3_2_3_22_1 = yyv_OutArgs;
yy_1_3_2_3_22_2 = yyv_FormalOutArgs;
MatchArgs(yy_1_3_2_3_22_1, yy_1_3_2_3_22_2);
goto yysl_312_2_1_3_2;
yyfl_312_2_1_3_2_3 : ;
yy_1_3_2_4_1_1 = yyv_Class;
yy_1_3_2_4_1_2 = yy_1_3_2_4_1_1;
if (yy_1_3_2_4_1_2[0] != 5) goto yyfl_312_2_1_3_2_4;
yy_1_3_2_4_2_1 = yyv_InArgs;
yy_1_3_2_4_2_2 = yyv_FormalInArgs;
Build(yy_1_3_2_4_2_1, yy_1_3_2_4_2_2);
yy_1_3_2_4_3_1 = yyv_Class;
yy_1_3_2_4_3_2 = yyv_CodeName;
yy_1_3_2_4_3_3 = yyv_InArgs;
yy_1_3_2_4_3_4 = yyv_OutArgs;
Call(yy_1_3_2_4_3_1, yy_1_3_2_4_3_2, yy_1_3_2_4_3_3, yy_1_3_2_4_3_4);
yy_1_3_2_4_4_1 = yyv_OutArgs;
yy_1_3_2_4_4_2 = yyv_FormalOutArgs;
MatchArgs(yy_1_3_2_4_4_1, yy_1_3_2_4_4_2);
goto yysl_312_2_1_3_2;
yyfl_312_2_1_3_2_4 : ;
goto yyfl_312_2_1_3;
yysl_312_2_1_3_2 : ;
yyb = yysb;
}
goto yysl_312_2_1;
yyfl_312_2_1_3 : ;
yy_1_4_1_1 = ((yy)"'");
yy_1_4_1_2 = yyv_Predicate;
yy_1_4_1_3 = ((yy)"' not declared as predicate");
yy_1_4_1_4 = yyv_Pos;
MESSAGE1(yy_1_4_1_1, yy_1_4_1_2, yy_1_4_1_3, yy_1_4_1_4);
goto yysl_312_2_1;
yysl_312_2_1 : ;
yyb = yysb;
}
return;
yyfl_312_2 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_S;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_Tmp;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 8) goto yyfl_312_3;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_S = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_Tmp = yy_0_2;
return;
yyfl_312_3 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_A;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_Tmp;
yy yy_0_2;
yy yyv_Type;
yy yy_1;
yy yy_2_1;
yy yy_2_2;
yy yy_2_2_1;
yy yy_2_2_2;
yy yy_2_2_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 9) goto yyfl_312_4;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_A = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_Tmp = yy_0_2;
yyb = yyh;
yyh += 6; if (yyh > yyhx) yyExtend();
yy_1 = yyglov_id_POS;
if (yy_1 == (yy) yyu) yyErr(1,3256);
yyv_Type = yy_1;
yy_2_1 = yyv_A;
yy_2_2_1 = yyb + 4;
yy_2_2_1[0] = 2;
yy_2_2_2 = yyv_Type;
yy_2_2_3 = yyb + 5;
yy_2_2_3[0] = 2;
yy_2_2 = yyb + 0;
yy_2_2[0] = 1;
yy_2_2[1] = ((long)yy_2_2_1);
yy_2_2[2] = ((long)yy_2_2_2);
yy_2_2[3] = ((long)yy_2_2_3);
MatchArg(yy_2_1, yy_2_2);
return;
yyfl_312_4 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Target;
yy yy_0_1_1;
yy yyv_Pos;
yy yy_0_1_2;
yy yyv_Source;
yy yy_0_1_3;
yy yyv_Tmp;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy yyv_Type;
yy yy_1_3;
yy yy_2_1;
yy yy_2_2;
yy yy_2_2_1;
yy yy_2_2_2;
yy yy_2_2_3;
yy yy_3_1;
yy yyv_T;
yy yy_3_2;
yy yy_4_1;
yy yy_5_1;
yy yy_6_1;
yy yy_7_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_312_5;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Target = yy_0_1_1;
yyv_Pos = yy_0_1_2;
yyv_Source = yy_0_1_3;
yyv_Tmp = yy_0_2;
yyb = yyh;
yyh += 6; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_Target;
yy_1_2 = yyv_Pos;
CheckVariable(yy_1_1, yy_1_2, &yy_1_3);
yyv_Type = yy_1_3;
yy_2_1 = yyv_Source;
yy_2_2_1 = yyb + 4;
yy_2_2_1[0] = 2;
yy_2_2_2 = yyv_Type;
yy_2_2_3 = yyb + 5;
yy_2_2_3[0] = 2;
yy_2_2 = yyb + 0;
yy_2_2[0] = 1;
yy_2_2[1] = ((long)yy_2_2_1);
yy_2_2[2] = ((long)yy_2_2_2);
yy_2_2[3] = ((long)yy_2_2_3);
BuildArg(yy_2_1, yy_2_2);
yy_3_1 = yyv_Source;
GetUseArgTmp(yy_3_1, &yy_3_2);
yyv_T = yy_3_2;
yy_4_1 = yyv_Target;
glovarid(yy_4_1);
yy_5_1 = ((yy)" = ");
s(yy_5_1);
yy_6_1 = yyv_T;
tmp(yy_6_1);
yy_7_1 = ((yy)";");
s(yy_7_1);
nl();
return;
yyfl_312_5 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Var;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_Arg;
yy yy_0_1_3;
yy yyv_Tmp;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy yyv_Type;
yy yy_1_3;
yy yy_2_1;
yy yyv_T;
yy yy_2_2;
yy yy_3_1;
yy yy_4_1;
yy yy_5_1;
yy yy_6_1;
yy yy_8_1;
yy yyv_Line;
yy yy_8_2;
yy yy_9_1;
yy yy_10_1;
yy yy_11_1;
yy yy_12_1;
yy yy_13_1;
yy yy_15_1;
yy yy_15_2;
yy yy_15_2_1;
yy yy_15_2_2;
yy yy_15_2_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 3) goto yyfl_312_6;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Var = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_Arg = yy_0_1_3;
yyv_Tmp = yy_0_2;
yyb = yyh;
yyh += 6; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_Var;
yy_1_2 = yyv_P;
CheckVariable(yy_1_1, yy_1_2, &yy_1_3);
yyv_Type = yy_1_3;
yy_2_1 = yyv_Arg;
GetDefArgTmp(yy_2_1, &yy_2_2);
yyv_T = yy_2_2;
yy_3_1 = yyv_T;
tmp(yy_3_1);
yy_4_1 = ((yy)" = ");
s(yy_4_1);
yy_5_1 = yyv_Var;
glovarid(yy_5_1);
yy_6_1 = ((yy)";");
s(yy_6_1);
nl();
yy_8_1 = yyv_P;
PosToLineNumber(yy_8_1, &yy_8_2);
yyv_Line = yy_8_2;
yy_9_1 = ((yy)"if (");
s(yy_9_1);
yy_10_1 = yyv_T;
tmp(yy_10_1);
yy_11_1 = ((yy)" == (yy) yyu) yyErr(1,");
s(yy_11_1);
yy_12_1 = yyv_Line;
i(yy_12_1);
yy_13_1 = ((yy)");");
s(yy_13_1);
nl();
yy_15_1 = yyv_Arg;
yy_15_2_1 = yyb + 4;
yy_15_2_1[0] = 2;
yy_15_2_2 = yyv_Type;
yy_15_2_3 = yyb + 5;
yy_15_2_3[0] = 2;
yy_15_2 = yyb + 0;
yy_15_2[0] = 1;
yy_15_2[1] = ((long)yy_15_2_1);
yy_15_2[2] = ((long)yy_15_2_2);
yy_15_2[3] = ((long)yy_15_2_3);
MatchArg(yy_15_1, yy_15_2);
return;
yyfl_312_6 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Key;
yy yy_0_1_1;
yy yyv_Field;
yy yy_0_1_2;
yy yyv_Val;
yy yy_0_1_3;
yy yyv_Pos;
yy yy_0_1_4;
yy yyv_Tmp;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yy_1_4;
yy yyv_Type;
yy yy_1_5;
yy yyv_Offset;
yy yy_1_6;
yy yy_2_1;
yy yy_2_2;
yy yy_2_2_1;
yy yy_2_2_2;
yy yy_2_2_3;
yy yy_3_1;
yy yyv_T;
yy yy_3_2;
yy yy_4_1;
yy yy_5_1;
yy yy_6_1;
yy yy_7_1;
yy yy_8_1;
yy yy_9_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 5) goto yyfl_312_7;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yyv_Key = yy_0_1_1;
yyv_Field = yy_0_1_2;
yyv_Val = yy_0_1_3;
yyv_Pos = yy_0_1_4;
yyv_Tmp = yy_0_2;
yyb = yyh;
yyh += 7; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_Key;
yy_1_2 = yyv_Field;
yy_1_3 = yyb + 0;
yy_1_3[0] = 1;
yy_1_4 = yyv_Pos;
CheckTableField(yy_1_1, yy_1_2, yy_1_3, yy_1_4, &yy_1_5, &yy_1_6);
yyv_Type = yy_1_5;
yyv_Offset = yy_1_6;
yy_2_1 = yyv_Val;
yy_2_2_1 = yyb + 5;
yy_2_2_1[0] = 2;
yy_2_2_2 = yyv_Type;
yy_2_2_3 = yyb + 6;
yy_2_2_3[0] = 2;
yy_2_2 = yyb + 1;
yy_2_2[0] = 1;
yy_2_2[1] = ((long)yy_2_2_1);
yy_2_2[2] = ((long)yy_2_2_2);
yy_2_2[3] = ((long)yy_2_2_3);
BuildArg(yy_2_1, yy_2_2);
yy_3_1 = yyv_Val;
GetUseArgTmp(yy_3_1, &yy_3_2);
yyv_T = yy_3_2;
yy_4_1 = yyv_Key;
varid(yy_4_1);
yy_5_1 = ((yy)"[");
s(yy_5_1);
yy_6_1 = yyv_Offset;
i(yy_6_1);
yy_7_1 = ((yy)"] = (intptr_t) ");
s(yy_7_1);
yy_8_1 = yyv_T;
tmp(yy_8_1);
yy_9_1 = ((yy)";");
s(yy_9_1);
nl();
return;
yyfl_312_7 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Key;
yy yy_0_1_1;
yy yyv_Field;
yy yy_0_1_2;
yy yyv_Val;
yy yy_0_1_3;
yy yyv_Pos;
yy yy_0_1_4;
yy yyv_Tmp;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yy_1_4;
yy yyv_Type;
yy yy_1_5;
yy yyv_Offset;
yy yy_1_6;
yy yy_2_1;
yy yyv_T;
yy yy_2_2;
yy yy_3_1;
yy yy_4_1;
yy yy_5_1;
yy yy_6_1;
yy yy_7_1;
yy yy_8_1;
yy yy_10_1;
yy yyv_Line;
yy yy_10_2;
yy yy_11_1;
yy yy_12_1;
yy yy_13_1;
yy yy_14_1;
yy yy_15_1;
yy yy_17_1;
yy yy_17_2;
yy yy_17_2_1;
yy yy_17_2_2;
yy yy_17_2_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 6) goto yyfl_312_8;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yyv_Key = yy_0_1_1;
yyv_Field = yy_0_1_2;
yyv_Val = yy_0_1_3;
yyv_Pos = yy_0_1_4;
yyv_Tmp = yy_0_2;
yyb = yyh;
yyh += 7; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_Key;
yy_1_2 = yyv_Field;
yy_1_3 = yyb + 0;
yy_1_3[0] = 2;
yy_1_4 = yyv_Pos;
CheckTableField(yy_1_1, yy_1_2, yy_1_3, yy_1_4, &yy_1_5, &yy_1_6);
yyv_Type = yy_1_5;
yyv_Offset = yy_1_6;
yy_2_1 = yyv_Val;
GetDefArgTmp(yy_2_1, &yy_2_2);
yyv_T = yy_2_2;
yy_3_1 = yyv_T;
tmp(yy_3_1);
yy_4_1 = ((yy)" = (yy) ");
s(yy_4_1);
yy_5_1 = yyv_Key;
varid(yy_5_1);
yy_6_1 = ((yy)"[");
s(yy_6_1);
yy_7_1 = yyv_Offset;
i(yy_7_1);
yy_8_1 = ((yy)"];");
s(yy_8_1);
nl();
yy_10_1 = yyv_Pos;
PosToLineNumber(yy_10_1, &yy_10_2);
yyv_Line = yy_10_2;
yy_11_1 = ((yy)"if (");
s(yy_11_1);
yy_12_1 = yyv_T;
tmp(yy_12_1);
yy_13_1 = ((yy)" == (yy) yyu) yyErr(1,");
s(yy_13_1);
yy_14_1 = yyv_Line;
i(yy_14_1);
yy_15_1 = ((yy)");");
s(yy_15_1);
nl();
yy_17_1 = yyv_Val;
yy_17_2_1 = yyb + 5;
yy_17_2_1[0] = 2;
yy_17_2_2 = yyv_Type;
yy_17_2_3 = yyb + 6;
yy_17_2_3[0] = 2;
yy_17_2 = yyb + 1;
yy_17_2[0] = 1;
yy_17_2[1] = ((long)yy_17_2_1);
yy_17_2[2] = ((long)yy_17_2_2);
yy_17_2[3] = ((long)yy_17_2_3);
MatchArg(yy_17_1, yy_17_2);
return;
yyfl_312_8 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Type;
yy yy_0_1_1;
yy yyv_Key;
yy yy_0_1_2;
yy yyv_Pos;
yy yy_0_1_3;
yy yyv_Offset;
yy yy_0_1_4;
yy yyv_Tmp;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yy_1_3_1;
yy yy_1_4;
yy yy_2_1;
yy yyv_OffsetVal;
yy yy_2_2;
yy yy_3_1;
yy yy_4_1;
yy yy_5_1;
yy yy_6_1;
yy yy_8_1;
yy yy_8_2;
yy yyv_List;
yy yy_8_3;
yy yy_9_1;
yy yyv_N;
yy yy_9_2;
yy yy_10_1;
yy yy_10_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 4) goto yyfl_312_9;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yyv_Type = yy_0_1_1;
yyv_Key = yy_0_1_2;
yyv_Pos = yy_0_1_3;
yyv_Offset = yy_0_1_4;
yyv_Tmp = yy_0_2;
yy_1_1 = yyv_Key;
yy_1_2 = yyv_Type;
yy_1_3_1 = ((yy)1);
yy_1_3 = (yy)(-((long)yy_1_3_1));
yy_1_4 = yyv_Pos;
DefineLocalVar(yy_1_1, yy_1_2, yy_1_3, yy_1_4);
yy_2_1 = yyv_Offset;
GetINT(yy_2_1, &yy_2_2);
yyv_OffsetVal = yy_2_2;
yy_3_1 = yyv_Key;
varid(yy_3_1);
yy_4_1 = ((yy)" = yyb + ");
s(yy_4_1);
yy_5_1 = yyv_OffsetVal;
i(yy_5_1);
yy_6_1 = ((yy)";");
s(yy_6_1);
nl();
yy_8_1 = yyv_Type;
yy_8_2 = yyv_Pos;
GetTableFields(yy_8_1, yy_8_2, &yy_8_3);
yyv_List = yy_8_3;
yy_9_1 = yyv_List;
Length(yy_9_1, &yy_9_2);
yyv_N = yy_9_2;
yy_10_1 = yyv_N;
yy_10_2 = yyv_OffsetVal;
InitFields(yy_10_1, yy_10_2);
return;
yyfl_312_9 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Cases;
yy yy_0_1_1;
yy yyv_RefCommonVars;
yy yy_0_1_2;
yy yyv_P;
yy yy_0_1_3;
yy yyv_Prefix;
yy yy_0_2;
yy yy_1_1_1;
yy yy_1_1_2_1;
yy yy_1_1_2_2;
yy yy_2_1_1_1;
yy yy_2_1_1_2;
yy yy_2_1_1_2_1;
yy yy_2_1_1_2_2;
yy yy_2_1_1_2_3;
yy yy_2_1_2_1;
yy yy_2_1_2_2;
yy yyv_Old;
yy yy_3;
yy yy_4;
yy yyv_CommonVars;
yy yy_5;
yy yy_6_1;
yy yy_8_1;
yy yy_10_1;
yy yy_10_2;
yy yy_10_2_1;
yy yy_10_2_2;
yy yy_10_3;
yy yyv_TypedCommonVars;
yy yy_10_4;
yy yyv_MayFail;
yy yy_10_5;
yy yy_11_1;
yy yy_11_2;
yy yy_12_1_1_1;
yy yy_12_1_1_2;
yy yy_14_1;
yy yy_16_1;
yy yy_18_1;
yy yy_20;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 10) goto yyfl_312_10;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Cases = yy_0_1_1;
yyv_RefCommonVars = yy_0_1_2;
yyv_P = yy_0_1_3;
yyv_Prefix = yy_0_2;
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
{
yy yysb = yyb;
yy_1_1_1 = yyglov_insideChoiceRule;
if (yy_1_1_1 == (yy) yyu) yyErr(1,3301);
if (yy_1_1_1[0] != 1) goto yyfl_312_10_1_1;
yy_1_1_2_1 = ((yy)"disjunction not allowed inside CHOICE rule");
yy_1_1_2_2 = yyv_P;
MESSAGE(yy_1_1_2_1, yy_1_1_2_2);
goto yysl_312_10_1;
yyfl_312_10_1_1 : ;
goto yysl_312_10_1;
yysl_312_10_1 : ;
yyb = yysb;
}
{
yy yysb = yyb;
yy_2_1_1_1 = yyv_Cases;
yy_2_1_1_2 = yy_2_1_1_1;
if (yy_2_1_1_2[0] != 1) goto yyfl_312_10_2_1;
yy_2_1_1_2_1 = ((yy)yy_2_1_1_2[1]);
yy_2_1_1_2_2 = ((yy)yy_2_1_1_2[2]);
yy_2_1_1_2_3 = ((yy)yy_2_1_1_2[3]);
if (yy_2_1_1_2_3[0] != 2) goto yyfl_312_10_2_1;
yy_2_1_2_1 = ((yy)"at least two alternatives required");
yy_2_1_2_2 = yyv_P;
MESSAGE(yy_2_1_2_1, yy_2_1_2_2);
goto yysl_312_10_2;
yyfl_312_10_2_1 : ;
goto yysl_312_10_2;
yysl_312_10_2 : ;
yyb = yysb;
}
yy_3 = yyglov_currentSuccessLabel;
if (yy_3 == (yy) yyu) yyErr(1,3313);
yyv_Old = yy_3;
yy_4 = yyv_Prefix;
yyglov_currentSuccessLabel = yy_4;
yy_5 = (yy) yyv_RefCommonVars[1];
if (yy_5 == (yy) yyu) yyErr(1,3315);
yyv_CommonVars = yy_5;
yy_6_1 = ((yy)"{");
s(yy_6_1);
nl();
yy_8_1 = ((yy)"yy yysb = yyb;");
s(yy_8_1);
nl();
yy_10_1 = yyv_Cases;
yy_10_2_1 = ((yy)1);
yy_10_2_2 = yyv_Prefix;
yy_10_2 = yyb + 0;
yy_10_2[0] = 1;
yy_10_2[1] = ((long)yy_10_2_1);
yy_10_2[2] = ((long)yy_10_2_2);
yy_10_3 = yyv_CommonVars;
Code_Cases(yy_10_1, yy_10_2, yy_10_3, &yy_10_4, &yy_10_5);
yyv_TypedCommonVars = yy_10_4;
yyv_MayFail = yy_10_5;
yy_11_1 = yyv_TypedCommonVars;
yy_11_2 = yyv_P;
DeclareTypedVars(yy_11_1, yy_11_2);
{
yy yysb = yyb;
yy_12_1_1_1 = yyv_MayFail;
yy_12_1_1_2 = yy_12_1_1_1;
if (yy_12_1_1_2[0] != 1) goto yyfl_312_10_12_1;
Fail();
nl();
goto yysl_312_10_12;
yyfl_312_10_12_1 : ;
goto yysl_312_10_12;
yysl_312_10_12 : ;
yyb = yysb;
}
SuccessLabel();
yy_14_1 = ((yy)" : ;");
s(yy_14_1);
nl();
yy_16_1 = ((yy)"yyb = yysb;");
s(yy_16_1);
nl();
yy_18_1 = ((yy)"}");
s(yy_18_1);
nl();
yy_20 = yyv_Old;
yyglov_currentSuccessLabel = yy_20;
return;
yyfl_312_10 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Members;
yy yy_0_1_1;
yy yyv_Pos;
yy yy_0_1_2;
yy yyv_Prefix;
yy yy_0_2;
yy yy_1_1_1;
yy yy_1_1_2_1;
yy yy_1_1_2_2;
yy yy_2_1;
yy yy_2_2;
yy yy_2_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 11) goto yyfl_312_11;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_Members = yy_0_1_1;
yyv_Pos = yy_0_1_2;
yyv_Prefix = yy_0_2;
{
yy yysb = yyb;
yy_1_1_1 = yyglov_insideChoiceRule;
if (yy_1_1_1 == (yy) yyu) yyErr(1,3331);
if (yy_1_1_1[0] != 1) goto yyfl_312_11_1_1;
yy_1_1_2_1 = ((yy)"loop not allowed inside CHOICE rule");
yy_1_1_2_2 = yyv_Pos;
MESSAGE(yy_1_1_2_1, yy_1_1_2_2);
goto yysl_312_11_1;
yyfl_312_11_1_1 : ;
goto yysl_312_11_1;
yysl_312_11_1 : ;
yyb = yysb;
}
yy_2_1 = yyv_Members;
yy_2_2 = yyv_Pos;
yy_2_3 = yyv_Prefix;
Code_LoopBody(yy_2_1, yy_2_2, yy_2_3);
return;
yyfl_312_11 : ;
}
yyErr(2,3044);
}
int IsRelation(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yy_1_1;
yy yyv_Str;
yy yy_1_2;
yy yy_2_1_1_1;
yy yy_2_1_1_2;
yy yy_2_2_1_1;
yy yy_2_2_1_2;
yy yy_2_3_1_1;
yy yy_2_3_1_2;
yy yy_2_4_1_1;
yy yy_2_4_1_2;
yy yy_2_5_1_1;
yy yy_2_5_1_2;
yy yy_2_6_1_1;
yy yy_2_6_1_2;
yy_0_1 = yyin_1;
yyv_Id = yy_0_1;
yy_1_1 = yyv_Id;
id_to_string(yy_1_1, &yy_1_2);
yyv_Str = yy_1_2;
{
yy yysb = yyb;
yy_2_1_1_1 = yyv_Str;
yy_2_1_1_2 = ((yy)"eq");
if (strcmp((char *) yy_2_1_1_1, (char *) yy_2_1_1_2)  !=  0) goto yyfl_313_1_2_1;
goto yysl_313_1_2;
yyfl_313_1_2_1 : ;
yy_2_2_1_1 = yyv_Str;
yy_2_2_1_2 = ((yy)"ne");
if (strcmp((char *) yy_2_2_1_1, (char *) yy_2_2_1_2)  !=  0) goto yyfl_313_1_2_2;
goto yysl_313_1_2;
yyfl_313_1_2_2 : ;
yy_2_3_1_1 = yyv_Str;
yy_2_3_1_2 = ((yy)"lt");
if (strcmp((char *) yy_2_3_1_1, (char *) yy_2_3_1_2)  !=  0) goto yyfl_313_1_2_3;
goto yysl_313_1_2;
yyfl_313_1_2_3 : ;
yy_2_4_1_1 = yyv_Str;
yy_2_4_1_2 = ((yy)"le");
if (strcmp((char *) yy_2_4_1_1, (char *) yy_2_4_1_2)  !=  0) goto yyfl_313_1_2_4;
goto yysl_313_1_2;
yyfl_313_1_2_4 : ;
yy_2_5_1_1 = yyv_Str;
yy_2_5_1_2 = ((yy)"gt");
if (strcmp((char *) yy_2_5_1_1, (char *) yy_2_5_1_2)  !=  0) goto yyfl_313_1_2_5;
goto yysl_313_1_2;
yyfl_313_1_2_5 : ;
yy_2_6_1_1 = yyv_Str;
yy_2_6_1_2 = ((yy)"ge");
if (strcmp((char *) yy_2_6_1_1, (char *) yy_2_6_1_2)  !=  0) goto yyfl_313_1_2_6;
goto yysl_313_1_2;
yyfl_313_1_2_6 : ;
goto yyfl_313_1;
yysl_313_1_2 : ;
yyb = yysb;
}
return 1;
yyfl_313_1 : ;
}
return 0;
}
int IsIdent(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yyv_Str;
yy yy_0_2;
yy yy_1_1;
yy yyv_Str2;
yy yy_1_2;
yy yy_2_1;
yy yy_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Id = yy_0_1;
yyv_Str = yy_0_2;
yy_1_1 = yyv_Id;
id_to_string(yy_1_1, &yy_1_2);
yyv_Str2 = yy_1_2;
yy_2_1 = yyv_Str;
yy_2_2 = yyv_Str2;
if (strcmp((char *) yy_2_1, (char *) yy_2_2)  !=  0) goto yyfl_314_1;
return 1;
yyfl_314_1 : ;
}
return 0;
}
DetermineType(yyin_1, yyout_1)
yy yyin_1;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_Op;
yy yy_0_1_2;
yy yyv_Pos;
yy yy_0_1_3;
yy yyv_Left;
yy yy_0_1_4;
yy yyv_Right;
yy yy_0_1_5;
yy yy_0_2;
yy yyv_Type;
yy yy_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_315_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yy_0_1_5 = ((yy)yy_0_1[5]);
yyv_Tmp = yy_0_1_1;
yyv_Op = yy_0_1_2;
yyv_Pos = yy_0_1_3;
yyv_Left = yy_0_1_4;
yyv_Right = yy_0_1_5;
yy_1 = yyglov_id_INT;
if (yy_1 == (yy) yyu) yyErr(1,3355);
yyv_Type = yy_1;
yy_0_2 = yyv_Type;
*yyout_1 = yy_0_2;
return;
yyfl_315_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_Op;
yy yy_0_1_2;
yy yyv_Pos;
yy yy_0_1_3;
yy yyv_Arg;
yy yy_0_1_4;
yy yy_0_2;
yy yyv_Type;
yy yy_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_315_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yyv_Tmp = yy_0_1_1;
yyv_Op = yy_0_1_2;
yyv_Pos = yy_0_1_3;
yyv_Arg = yy_0_1_4;
yy_1 = yyglov_id_INT;
if (yy_1 == (yy) yyu) yyErr(1,3358);
yyv_Type = yy_1;
yy_0_2 = yyv_Type;
*yyout_1 = yy_0_2;
return;
yyfl_315_2 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_Offset;
yy yy_0_1_2;
yy yyv_Functor;
yy yy_0_1_3;
yy yyv_Pos;
yy yy_0_1_4;
yy yyv_Args;
yy yy_0_1_5;
yy yy_0_2;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yyv_Type;
yy yy_1_1_1_2_1;
yy yy_1_1_1_2_2;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_1_2_1_2_1;
yy yyv_Others;
yy yy_1_2_1_2_2;
yy yy_1_2_2_1;
yy yy_1_2_2_2;
yy yy_1_2_2_3;
yy yy_1_2_2_4;
yy yy_1_3_1_1;
yy yy_1_3_1_2;
yy yy_1_3_1_3;
yy yy_1_3_1_4;
yy yy_1_3_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 3) goto yyfl_315_3;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yy_0_1_5 = ((yy)yy_0_1[5]);
yyv_Tmp = yy_0_1_1;
yyv_Offset = yy_0_1_2;
yyv_Functor = yy_0_1_3;
yyv_Pos = yy_0_1_4;
yyv_Args = yy_0_1_5;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Functor;
if (! GetFunctorMeaning(yy_1_1_1_1, &yy_1_1_1_2)) goto yyfl_315_3_1_1;
if (yy_1_1_1_2[0] != 1) goto yyfl_315_3_1_1;
yy_1_1_1_2_1 = ((yy)yy_1_1_1_2[1]);
yy_1_1_1_2_2 = ((yy)yy_1_1_1_2[2]);
yyv_Type = yy_1_1_1_2_1;
if (yy_1_1_1_2_2[0] != 2) goto yyfl_315_3_1_1;
goto yysl_315_3_1;
yyfl_315_3_1_1 : ;
yy_1_2_1_1 = yyv_Functor;
if (! GetFunctorMeaning(yy_1_2_1_1, &yy_1_2_1_2)) goto yyfl_315_3_1_2;
if (yy_1_2_1_2[0] != 1) goto yyfl_315_3_1_2;
yy_1_2_1_2_1 = ((yy)yy_1_2_1_2[1]);
yy_1_2_1_2_2 = ((yy)yy_1_2_1_2[2]);
yyv_Type = yy_1_2_1_2_1;
yyv_Others = yy_1_2_1_2_2;
yy_1_2_2_1 = ((yy)"ambigous '");
yy_1_2_2_2 = yyv_Functor;
yy_1_2_2_3 = ((yy)"' requires type prefix");
yy_1_2_2_4 = yyv_Pos;
MESSAGE1(yy_1_2_2_1, yy_1_2_2_2, yy_1_2_2_3, yy_1_2_2_4);
goto yysl_315_3_1;
yyfl_315_3_1_2 : ;
yy_1_3_1_1 = ((yy)"unknown functor '");
yy_1_3_1_2 = yyv_Functor;
yy_1_3_1_3 = ((yy)"'");
yy_1_3_1_4 = yyv_Pos;
MESSAGE1(yy_1_3_1_1, yy_1_3_1_2, yy_1_3_1_3, yy_1_3_1_4);
yy_1_3_2 = yyglov_id_INT;
if (yy_1_3_2 == (yy) yyu) yyErr(1,3365);
yyv_Type = yy_1_3_2;
goto yysl_315_3_1;
yysl_315_3_1 : ;
yyb = yysb;
}
yy_0_2 = yyv_Type;
*yyout_1 = yy_0_2;
return;
yyfl_315_3 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Type;
yy yy_0_1_1;
yy yyv_Arg;
yy yy_0_1_2;
yy yyv_Pos;
yy yy_0_1_3;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 4) goto yyfl_315_4;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Type = yy_0_1_1;
yyv_Arg = yy_0_1_2;
yyv_Pos = yy_0_1_3;
yy_0_2 = yyv_Type;
*yyout_1 = yy_0_2;
return;
yyfl_315_4 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_Name;
yy yy_0_1_2;
yy yyv_Pos;
yy yy_0_1_3;
yy yy_0_2;
yy yy_1_1_1_1;
yy yyv_Type;
yy yy_1_1_1_2;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_1_2_1_3;
yy yy_1_2_1_4;
yy yy_1_2_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 5) goto yyfl_315_5;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Tmp = yy_0_1_1;
yyv_Name = yy_0_1_2;
yyv_Pos = yy_0_1_3;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Name;
if (! GetLocalMeaning(yy_1_1_1_1, &yy_1_1_1_2)) goto yyfl_315_5_1_1;
yyv_Type = yy_1_1_1_2;
goto yysl_315_5_1;
yyfl_315_5_1_1 : ;
yy_1_2_1_1 = ((yy)"'");
yy_1_2_1_2 = yyv_Name;
yy_1_2_1_3 = ((yy)"' has no value");
yy_1_2_1_4 = yyv_Pos;
MESSAGE1(yy_1_2_1_1, yy_1_2_1_2, yy_1_2_1_3, yy_1_2_1_4);
yy_1_2_2 = yyglov_id_INT;
if (yy_1_2_2 == (yy) yyu) yyErr(1,3373);
yyv_Type = yy_1_2_2;
goto yysl_315_5_1;
yysl_315_5_1 : ;
yyb = yysb;
}
yy_0_2 = yyv_Type;
*yyout_1 = yy_0_2;
return;
yyfl_315_5 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_Number;
yy yy_0_1_2;
yy yyv_Pos;
yy yy_0_1_3;
yy yy_0_2;
yy yyv_Type;
yy yy_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 6) goto yyfl_315_6;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Tmp = yy_0_1_1;
yyv_Number = yy_0_1_2;
yyv_Pos = yy_0_1_3;
yy_1 = yyglov_id_INT;
if (yy_1 == (yy) yyu) yyErr(1,3377);
yyv_Type = yy_1;
yy_0_2 = yyv_Type;
*yyout_1 = yy_0_2;
return;
yyfl_315_6 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_String;
yy yy_0_1_2;
yy yyv_Pos;
yy yy_0_1_3;
yy yy_0_2;
yy yyv_Type;
yy yy_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 7) goto yyfl_315_7;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Tmp = yy_0_1_1;
yyv_String = yy_0_1_2;
yyv_Pos = yy_0_1_3;
yy_1 = yyglov_id_STRING;
if (yy_1 == (yy) yyu) yyErr(1,3380);
yyv_Type = yy_1;
yy_0_2 = yyv_Type;
*yyout_1 = yy_0_2;
return;
yyfl_315_7 : ;
}
yyErr(2,3352);
}
NegIntRel(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yy_1_1_2_1;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_1_2_2_1;
yy yy_1_3_1_1;
yy yy_1_3_1_2;
yy yy_1_3_2_1;
yy yy_1_4_1_1;
yy yy_1_4_1_2;
yy yy_1_4_2_1;
yy yy_1_5_1_1;
yy yy_1_5_1_2;
yy yy_1_5_2_1;
yy yy_1_6_1_1;
yy yy_1_6_1_2;
yy yy_1_6_2_1;
yy_0_1 = yyin_1;
yyv_Id = yy_0_1;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Id;
yy_1_1_1_2 = ((yy)"eq");
if (! IsIdent(yy_1_1_1_1, yy_1_1_1_2)) goto yyfl_316_1_1_1;
yy_1_1_2_1 = ((yy)" != ");
s(yy_1_1_2_1);
goto yysl_316_1_1;
yyfl_316_1_1_1 : ;
yy_1_2_1_1 = yyv_Id;
yy_1_2_1_2 = ((yy)"ne");
if (! IsIdent(yy_1_2_1_1, yy_1_2_1_2)) goto yyfl_316_1_1_2;
yy_1_2_2_1 = ((yy)" == ");
s(yy_1_2_2_1);
goto yysl_316_1_1;
yyfl_316_1_1_2 : ;
yy_1_3_1_1 = yyv_Id;
yy_1_3_1_2 = ((yy)"lt");
if (! IsIdent(yy_1_3_1_1, yy_1_3_1_2)) goto yyfl_316_1_1_3;
yy_1_3_2_1 = ((yy)" >= ");
s(yy_1_3_2_1);
goto yysl_316_1_1;
yyfl_316_1_1_3 : ;
yy_1_4_1_1 = yyv_Id;
yy_1_4_1_2 = ((yy)"le");
if (! IsIdent(yy_1_4_1_1, yy_1_4_1_2)) goto yyfl_316_1_1_4;
yy_1_4_2_1 = ((yy)" > ");
s(yy_1_4_2_1);
goto yysl_316_1_1;
yyfl_316_1_1_4 : ;
yy_1_5_1_1 = yyv_Id;
yy_1_5_1_2 = ((yy)"gt");
if (! IsIdent(yy_1_5_1_1, yy_1_5_1_2)) goto yyfl_316_1_1_5;
yy_1_5_2_1 = ((yy)" <= ");
s(yy_1_5_2_1);
goto yysl_316_1_1;
yyfl_316_1_1_5 : ;
yy_1_6_1_1 = yyv_Id;
yy_1_6_1_2 = ((yy)"ge");
if (! IsIdent(yy_1_6_1_1, yy_1_6_1_2)) goto yyfl_316_1_1_6;
yy_1_6_2_1 = ((yy)" < ");
s(yy_1_6_2_1);
goto yysl_316_1_1;
yyfl_316_1_1_6 : ;
goto yyfl_316_1;
yysl_316_1_1 : ;
yyb = yysb;
}
return;
yyfl_316_1 : ;
}
yyErr(2,3382);
}
InitFields(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yyv_Count;
yy yy_0_1;
yy yyv_Offset;
yy yy_0_2;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yy_1_1_2_1;
yy yy_1_1_3_1;
yy yy_1_1_3_1_1;
yy yy_1_1_3_1_2;
yy yy_1_1_4_1;
yy yy_1_1_6_1;
yy yy_1_1_6_1_1;
yy yy_1_1_6_1_2;
yy yy_1_1_6_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Count = yy_0_1;
yyv_Offset = yy_0_2;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Count;
yy_1_1_1_2 = ((yy)0);
if ((long)yy_1_1_1_1 <= (long)yy_1_1_1_2) goto yyfl_317_1_1_1;
yy_1_1_2_1 = ((yy)"yyb[");
s(yy_1_1_2_1);
yy_1_1_3_1_1 = yyv_Offset;
yy_1_1_3_1_2 = yyv_Count;
yy_1_1_3_1 = (yy)(((long)yy_1_1_3_1_1)+((long)yy_1_1_3_1_2));
i(yy_1_1_3_1);
yy_1_1_4_1 = ((yy)"] = yyu;");
s(yy_1_1_4_1);
nl();
yy_1_1_6_1_1 = yyv_Count;
yy_1_1_6_1_2 = ((yy)1);
yy_1_1_6_1 = (yy)(((long)yy_1_1_6_1_1)-((long)yy_1_1_6_1_2));
yy_1_1_6_2 = yyv_Offset;
InitFields(yy_1_1_6_1, yy_1_1_6_2);
goto yysl_317_1_1;
yyfl_317_1_1_1 : ;
goto yysl_317_1_1;
yysl_317_1_1 : ;
yyb = yysb;
}
return;
}
}
CheckEmptyInArgs(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_P;
yy yy_0_1_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_318_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yyv_P = yy_0_1_1;
return;
yyfl_318_1 : ;
}
{
yy yyb;
yy yyv_InArgs;
yy yy_0_1;
yy yy_1_1;
yy yyv_Pos;
yy yy_1_2;
yy yy_2_1;
yy yy_2_2;
yy_0_1 = yyin_1;
yyv_InArgs = yy_0_1;
yy_1_1 = yyv_InArgs;
POS_USEARGLIST(yy_1_1, &yy_1_2);
yyv_Pos = yy_1_2;
yy_2_1 = ((yy)"Input arguments not allowed");
yy_2_2 = yyv_Pos;
MESSAGE(yy_2_1, yy_2_2);
return;
yyfl_318_2 : ;
}
yyErr(2,3401);
}
CheckSingleOutArg(yyin_1, yyout_1)
yy yyin_1;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Arg;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_P2;
yy yy_0_1_3_1;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_319_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Arg = yy_0_1_1;
yyv_P = yy_0_1_2;
if (yy_0_1_3[0] != 2) goto yyfl_319_1;
yy_0_1_3_1 = ((yy)yy_0_1_3[1]);
yyv_P2 = yy_0_1_3_1;
yy_0_2 = yyv_Arg;
*yyout_1 = yy_0_2;
return;
yyfl_319_1 : ;
}
{
yy yyb;
yy yyv_OutArgs;
yy yy_0_1;
yy yy_0_2;
yy yy_1_1;
yy yyv_Pos;
yy yy_1_2;
yy yy_2_1;
yy yy_2_2;
yy_0_1 = yyin_1;
yyv_OutArgs = yy_0_1;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_OutArgs;
POS_DEFARGLIST(yy_1_1, &yy_1_2);
yyv_Pos = yy_1_2;
yy_2_1 = ((yy)"One output argument required");
yy_2_2 = yyv_Pos;
MESSAGE(yy_2_1, yy_2_2);
yy_0_2 = yyb + 0;
yy_0_2[0] = 6;
*yyout_1 = yy_0_2;
return;
yyfl_319_2 : ;
}
yyErr(2,3409);
}
Code_Cases(yyin_1, yyin_2, yyin_3, yyout_1, yyout_2)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy *yyout_1;
yy *yyout_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_2;
yy yyv_N;
yy yy_0_2_1;
yy yyv_Tl;
yy yy_0_2_2;
yy yyv_CommonVars;
yy yy_0_3;
yy yy_0_4;
yy yy_0_5;
yy yy_1_1;
yy yy_1_2;
yy yy_1_2_1;
yy yy_1_2_2;
yy yy_1_3;
yy yyv_TypedCV;
yy yy_1_4;
yy yyv_MayFail;
yy yy_1_5;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 1) goto yyfl_320_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_H = yy_0_1_1;
yyv_P = yy_0_1_2;
if (yy_0_1_3[0] != 2) goto yyfl_320_1;
if (yy_0_2[0] != 1) goto yyfl_320_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yyv_N = yy_0_2_1;
yyv_Tl = yy_0_2_2;
yyv_CommonVars = yy_0_3;
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_H;
yy_1_2_1 = yyv_N;
yy_1_2_2 = yyv_Tl;
yy_1_2 = yyb + 0;
yy_1_2[0] = 1;
yy_1_2[1] = ((long)yy_1_2_1);
yy_1_2[2] = ((long)yy_1_2_2);
yy_1_3 = yyv_CommonVars;
Code_Case(yy_1_1, yy_1_2, yy_1_3, &yy_1_4, &yy_1_5);
yyv_TypedCV = yy_1_4;
yyv_MayFail = yy_1_5;
yy_0_4 = yyv_TypedCV;
yy_0_5 = yyv_MayFail;
*yyout_1 = yy_0_4;
*yyout_2 = yy_0_5;
return;
yyfl_320_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_T;
yy yy_0_1_3;
yy yy_0_2;
yy yyv_N;
yy yy_0_2_1;
yy yyv_Tl;
yy yy_0_2_2;
yy yyv_CommonVars;
yy yy_0_3;
yy yy_0_4;
yy yy_0_5;
yy yy_1_1;
yy yy_1_2;
yy yy_1_2_1;
yy yy_1_2_2;
yy yy_1_3;
yy yyv_TypedCV1;
yy yy_1_4;
yy yyv_MayFailHead;
yy yy_1_5;
yy yy_2_1_1_1;
yy yy_2_1_1_2;
yy yy_2_1_2_1;
yy yy_2_1_2_2;
yy yy_3_1;
yy yy_3_2;
yy yy_3_2_1;
yy yy_3_2_1_1;
yy yy_3_2_1_2;
yy yy_3_2_2;
yy yy_3_3;
yy yyv_TypedCV2;
yy yy_3_4;
yy yyv_MayFailTail;
yy yy_3_5;
yy yy_4_1;
yy yy_4_2;
yy yy_4_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 1) goto yyfl_320_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_H = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_T = yy_0_1_3;
if (yy_0_2[0] != 1) goto yyfl_320_2;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yyv_N = yy_0_2_1;
yyv_Tl = yy_0_2_2;
yyv_CommonVars = yy_0_3;
yyb = yyh;
yyh += 6; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_H;
yy_1_2_1 = yyv_N;
yy_1_2_2 = yyv_Tl;
yy_1_2 = yyb + 0;
yy_1_2[0] = 1;
yy_1_2[1] = ((long)yy_1_2_1);
yy_1_2[2] = ((long)yy_1_2_2);
yy_1_3 = yyv_CommonVars;
Code_Case(yy_1_1, yy_1_2, yy_1_3, &yy_1_4, &yy_1_5);
yyv_TypedCV1 = yy_1_4;
yyv_MayFailHead = yy_1_5;
{
yy yysb = yyb;
yy_2_1_1_1 = yyv_MayFailHead;
yy_2_1_1_2 = yy_2_1_1_1;
if (yy_2_1_1_2[0] != 2) goto yyfl_320_2_2_1;
yy_2_1_2_1 = ((yy)"alternative cannot be reached");
yy_2_1_2_2 = yyv_P;
MESSAGE(yy_2_1_2_1, yy_2_1_2_2);
goto yysl_320_2_2;
yyfl_320_2_2_1 : ;
goto yysl_320_2_2;
yysl_320_2_2 : ;
yyb = yysb;
}
yy_3_1 = yyv_T;
yy_3_2_1_1 = yyv_N;
yy_3_2_1_2 = ((yy)1);
yy_3_2_1 = (yy)(((long)yy_3_2_1_1)+((long)yy_3_2_1_2));
yy_3_2_2 = yyv_Tl;
yy_3_2 = yyb + 3;
yy_3_2[0] = 1;
yy_3_2[1] = ((long)yy_3_2_1);
yy_3_2[2] = ((long)yy_3_2_2);
yy_3_3 = yyv_CommonVars;
Code_Cases(yy_3_1, yy_3_2, yy_3_3, &yy_3_4, &yy_3_5);
yyv_TypedCV2 = yy_3_4;
yyv_MayFailTail = yy_3_5;
yy_4_1 = yyv_TypedCV1;
yy_4_2 = yyv_TypedCV2;
yy_4_3 = yyv_P;
CheckConsistent(yy_4_1, yy_4_2, yy_4_3);
yy_0_4 = yyv_TypedCV1;
yy_0_5 = yyv_MayFailTail;
*yyout_1 = yy_0_4;
*yyout_2 = yy_0_5;
return;
yyfl_320_2 : ;
}
yyErr(2,3417);
}
Code_Case(yyin_1, yyin_2, yyin_3, yyout_1, yyout_2)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy *yyout_1;
yy *yyout_2;
{
{
yy yyb;
yy yyv_Members;
yy yy_0_1;
yy yyv_Prefix;
yy yy_0_2;
yy yyv_CommonVar;
yy yy_0_3;
yy yy_0_4;
yy yy_0_5;
yy yyv_Old;
yy yy_1;
yy yyv_OldUsed;
yy yy_2;
yy yy_3;
yy yy_4;
yy yy_6_1;
yy yy_6_2;
yy yy_6_2_1;
yy yy_6_2_2;
yy yy_6_3;
yy yy_7_1;
yy yyv_TypedCommonVars;
yy yy_7_2;
yy yy_9_1;
yy yy_11_1;
yy yyv_MayFail;
yy yy_13;
yy yy_14_1_1_1;
yy yy_14_1_1_2;
yy yy_14_1_3_1;
yy yy_15;
yy yy_16;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Members = yy_0_1;
yyv_Prefix = yy_0_2;
yyv_CommonVar = yy_0_3;
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yy_1 = yyglov_currentFailLabel;
if (yy_1 == (yy) yyu) yyErr(1,3438);
yyv_Old = yy_1;
yy_2 = yyglov_currentFailLabelUsed;
if (yy_2 == (yy) yyu) yyErr(1,3439);
yyv_OldUsed = yy_2;
yy_3 = yyv_Prefix;
yyglov_currentFailLabel = yy_3;
yy_4 = yyb + 0;
yy_4[0] = 2;
yyglov_currentFailLabelUsed = yy_4;
BeginLocalScope();
yy_6_1 = yyv_Members;
yy_6_2_1 = ((yy)1);
yy_6_2_2 = yyv_Prefix;
yy_6_2 = yyb + 1;
yy_6_2[0] = 1;
yy_6_2[1] = ((long)yy_6_2_1);
yy_6_2[2] = ((long)yy_6_2_2);
yy_6_3 = ((yy)0);
Code_Members(yy_6_1, yy_6_2, yy_6_3);
yy_7_1 = yyv_CommonVar;
BuildTypedVars(yy_7_1, &yy_7_2);
yyv_TypedCommonVars = yy_7_2;
EndLocalScope();
yy_9_1 = ((yy)"goto ");
s(yy_9_1);
SuccessLabel();
yy_11_1 = ((yy)";");
s(yy_11_1);
nl();
yy_13 = yyglov_currentFailLabelUsed;
if (yy_13 == (yy) yyu) yyErr(1,3447);
yyv_MayFail = yy_13;
{
yy yysb = yyb;
yy_14_1_1_1 = yyv_MayFail;
yy_14_1_1_2 = yy_14_1_1_1;
if (yy_14_1_1_2[0] != 1) goto yyfl_321_1_14_1;
FailLabel();
yy_14_1_3_1 = ((yy)" : ;");
s(yy_14_1_3_1);
nl();
goto yysl_321_1_14;
yyfl_321_1_14_1 : ;
goto yysl_321_1_14;
yysl_321_1_14 : ;
yyb = yysb;
}
yy_15 = yyv_Old;
yyglov_currentFailLabel = yy_15;
yy_16 = yyv_OldUsed;
yyglov_currentFailLabelUsed = yy_16;
yy_0_4 = yyv_TypedCommonVars;
yy_0_5 = yyv_MayFail;
*yyout_1 = yy_0_4;
*yyout_2 = yy_0_5;
return;
}
}
Code_LoopBody(yyin_1, yyin_2, yyin_3)
yy yyin_1;
yy yyin_2;
yy yyin_3;
{
{
yy yyb;
yy yyv_Members;
yy yy_0_1;
yy yyv_Pos;
yy yy_0_2;
yy yyv_Prefix;
yy yy_0_3;
yy yyv_Old;
yy yy_1;
yy yyv_OldSL;
yy yy_2;
yy yyv_OldUsed;
yy yy_3;
yy yy_4;
yy yy_5;
yy yy_6;
yy yy_8_1;
yy yy_11_1;
yy yy_11_2;
yy yy_11_2_1;
yy yy_11_2_2;
yy yy_11_3;
yy yy_13_1;
yy yy_15_1;
yy yyv_MayFail;
yy yy_17;
yy yy_18_1_1_1;
yy yy_18_1_1_2;
yy yy_18_1_3_1;
yy yy_18_2_1_1;
yy yy_18_2_1_2;
yy yy_19;
yy yy_20;
yy yy_21;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Members = yy_0_1;
yyv_Pos = yy_0_2;
yyv_Prefix = yy_0_3;
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yy_1 = yyglov_currentFailLabel;
if (yy_1 == (yy) yyu) yyErr(1,3457);
yyv_Old = yy_1;
yy_2 = yyglov_currentSuccessLabel;
if (yy_2 == (yy) yyu) yyErr(1,3458);
yyv_OldSL = yy_2;
yy_3 = yyglov_currentFailLabelUsed;
if (yy_3 == (yy) yyu) yyErr(1,3459);
yyv_OldUsed = yy_3;
yy_4 = yyv_Prefix;
yyglov_currentFailLabel = yy_4;
yy_5 = yyv_Prefix;
yyglov_currentSuccessLabel = yy_5;
yy_6 = yyb + 0;
yy_6[0] = 2;
yyglov_currentFailLabelUsed = yy_6;
SuccessLabel();
yy_8_1 = ((yy)" : ;");
s(yy_8_1);
nl();
BeginLocalScope();
yy_11_1 = yyv_Members;
yy_11_2_1 = ((yy)1);
yy_11_2_2 = yyv_Prefix;
yy_11_2 = yyb + 1;
yy_11_2[0] = 1;
yy_11_2[1] = ((long)yy_11_2_1);
yy_11_2[2] = ((long)yy_11_2_2);
yy_11_3 = ((yy)0);
Code_Members(yy_11_1, yy_11_2, yy_11_3);
EndLocalScope();
yy_13_1 = ((yy)"goto ");
s(yy_13_1);
SuccessLabel();
yy_15_1 = ((yy)";");
s(yy_15_1);
nl();
yy_17 = yyglov_currentFailLabelUsed;
if (yy_17 == (yy) yyu) yyErr(1,3468);
yyv_MayFail = yy_17;
{
yy yysb = yyb;
yy_18_1_1_1 = yyv_MayFail;
yy_18_1_1_2 = yy_18_1_1_1;
if (yy_18_1_1_2[0] != 1) goto yyfl_322_1_18_1;
FailLabel();
yy_18_1_3_1 = ((yy)" : ;");
s(yy_18_1_3_1);
nl();
goto yysl_322_1_18;
yyfl_322_1_18_1 : ;
yy_18_2_1_1 = ((yy)"infinite loop");
yy_18_2_1_2 = yyv_Pos;
MESSAGE(yy_18_2_1_1, yy_18_2_1_2);
goto yysl_322_1_18;
yysl_322_1_18 : ;
yyb = yysb;
}
yy_19 = yyv_Old;
yyglov_currentFailLabel = yy_19;
yy_20 = yyv_OldSL;
yyglov_currentSuccessLabel = yy_20;
yy_21 = yyv_OldUsed;
yyglov_currentFailLabelUsed = yy_21;
return;
}
}
Call(yyin_1, yyin_2, yyin_3, yyin_4)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy yyin_4;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Name;
yy yy_0_2;
yy yyv_InArgs;
yy yy_0_3;
yy yyv_OutArgs;
yy yy_0_4;
yy yy_1_1;
yy yy_2_1;
yy yyv_N;
yy yy_3;
yy yy_4_1;
yy yy_4_2;
yy yyv_Sep;
yy yy_4_3;
yy yy_5_1;
yy yy_5_2;
yy yy_6_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
if (yy_0_1[0] != 1) goto yyfl_323_1;
yyv_Name = yy_0_2;
yyv_InArgs = yy_0_3;
yyv_OutArgs = yy_0_4;
yy_1_1 = yyv_Name;
name(yy_1_1);
yy_2_1 = ((yy)"(");
s(yy_2_1);
yy_3 = yyglov_currentRuleNumber;
if (yy_3 == (yy) yyu) yyErr(1,3482);
yyv_N = yy_3;
yy_4_1 = yyv_InArgs;
yy_4_2 = ((yy)"");
ListInArgs(yy_4_1, yy_4_2, &yy_4_3);
yyv_Sep = yy_4_3;
yy_5_1 = yyv_OutArgs;
yy_5_2 = yyv_Sep;
ListOutArgs(yy_5_1, yy_5_2);
yy_6_1 = ((yy)");");
s(yy_6_1);
nl();
return;
yyfl_323_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Name;
yy yy_0_2;
yy yyv_InArgs;
yy yy_0_3;
yy yyv_OutArgs;
yy yy_0_4;
yy yy_1_1;
yy yy_2_1;
yy yy_3_1;
yy yyv_N;
yy yy_4;
yy yy_5_1;
yy yy_5_2;
yy yyv_Sep;
yy yy_5_3;
yy yy_6_1;
yy yy_6_2;
yy yy_7_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
if (yy_0_1[0] != 2) goto yyfl_323_2;
yyv_Name = yy_0_2;
yyv_InArgs = yy_0_3;
yyv_OutArgs = yy_0_4;
yy_1_1 = ((yy)"if (! ");
s(yy_1_1);
yy_2_1 = yyv_Name;
name(yy_2_1);
yy_3_1 = ((yy)"(");
s(yy_3_1);
yy_4 = yyglov_currentRuleNumber;
if (yy_4 == (yy) yyu) yyErr(1,3488);
yyv_N = yy_4;
yy_5_1 = yyv_InArgs;
yy_5_2 = ((yy)"");
ListInArgs(yy_5_1, yy_5_2, &yy_5_3);
yyv_Sep = yy_5_3;
yy_6_1 = yyv_OutArgs;
yy_6_2 = yyv_Sep;
ListOutArgs(yy_6_1, yy_6_2);
yy_7_1 = ((yy)")) ");
s(yy_7_1);
Fail();
nl();
return;
yyfl_323_2 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Name;
yy yy_0_2;
yy yyv_InArgs;
yy yy_0_3;
yy yyv_OutArgs;
yy yy_0_4;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yyv_Arg;
yy yy_1_1_1_2_1;
yy yy_1_1_1_2_2;
yy yy_1_1_1_2_3;
yy yy_1_1_2_1;
yy yyv_Type;
yy yy_1_1_2_2;
yy yyv_L;
yy yy_1_1_3;
yy yy_1_1_4_1;
yy yy_1_1_4_2;
yy yyv_Index;
yy yy_1_1_4_3;
yy yyv_Predicates;
yy yy_1_1_5;
yy yy_1_1_6_1;
yy yy_1_1_6_2;
yy yyv_Predicate;
yy yy_1_1_6_2_1;
yy yy_1_1_7_1;
yy yy_1_1_7_2;
yy yy_1_1_7_3;
yy yyv_Offset;
yy yy_1_1_7_4;
yy yy_1_1_8_1;
yy yyv_Tmp;
yy yy_1_1_8_2;
yy yy_1_1_9_1_1;
yy yy_1_1_9_1_2_1;
yy yy_1_1_9_1_4_1;
yy yy_1_1_9_1_6_1;
yy yy_1_1_9_1_8_1;
yy yy_1_1_9_1_9_1;
yy yy_1_1_9_1_10_1;
yy yy_1_1_9_1_11_1;
yy yy_1_1_9_1_12_1;
yy yy_1_1_9_1_14_1;
yy yy_1_1_9_1_15_1;
yy yy_1_1_9_1_16_1;
yy yy_1_1_9_1_18_1;
yy yy_1_1_9_1_20_1;
yy yy_1_1_9_1_22_1;
yy yy_1_1_9_1_23_1;
yy yy_1_1_9_1_23_2;
yy yyv_Sep;
yy yy_1_1_9_1_23_3;
yy yy_1_1_9_1_24_1;
yy yy_1_1_9_1_24_2;
yy yy_1_1_9_1_25_1;
yy yy_1_1_9_1_27_1;
yy yy_1_1_9_1_29_1;
yy yy_1_1_9_2_1;
yy yyv_Pathes;
yy yy_1_1_9_2_2;
yy yy_1_1_9_2_3_1;
yy yy_1_1_9_2_3_2;
yy yy_1_1_9_2_3_2_1;
yy yy_1_1_9_2_3_2_1_1;
yy yyv_V;
yy yy_1_1_9_2_3_2_1_2;
yy yyv_VarPos;
yy yy_1_1_9_2_3_2_1_3;
yy yy_1_1_9_2_3_2_2;
yy yy_1_1_9_2_3_2_3;
yy yy_1_1_9_2_4_1;
yy yy_1_1_9_2_4_2;
yy yy_1_1_9_2_4_3;
yy yyv_Path;
yy yy_1_1_9_2_4_4;
yy yy_1_1_9_2_5_1;
yy yy_1_1_9_2_7_1;
yy yy_1_1_9_2_8_1;
yy yy_1_1_9_2_9_1;
yy yy_1_1_9_2_10_1;
yy yy_1_1_9_2_11_1;
yy yy_1_1_9_2_13_1;
yy yy_1_1_9_2_14_1;
yy yy_1_1_9_2_15_1;
yy yy_1_1_9_2_15_2;
yy yy_1_1_9_2_15_3;
yy yy_1_1_9_2_16_1;
yy yy_1_1_9_2_16_2;
yy yy_1_1_9_2_17_1;
yy yy_1_1_9_2_19_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
if (yy_0_1[0] != 5) goto yyfl_323_3;
yyv_Name = yy_0_2;
yyv_InArgs = yy_0_3;
yyv_OutArgs = yy_0_4;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_InArgs;
yy_1_1_1_2 = yy_1_1_1_1;
if (yy_1_1_1_2[0] != 1) goto yyfl_323_3_1_1;
yy_1_1_1_2_1 = ((yy)yy_1_1_1_2[1]);
yy_1_1_1_2_2 = ((yy)yy_1_1_1_2[2]);
yy_1_1_1_2_3 = ((yy)yy_1_1_1_2[3]);
yyv_Arg = yy_1_1_1_2_1;
yy_1_1_2_1 = yyv_Arg;
DetermineType(yy_1_1_2_1, &yy_1_1_2_2);
yyv_Type = yy_1_1_2_2;
yy_1_1_3 = yyglov_choice_Types;
if (yy_1_1_3 == (yy) yyu) yyErr(1,3497);
yyv_L = yy_1_1_3;
yy_1_1_4_1 = yyv_L;
yy_1_1_4_2 = yyv_Type;
LookupChoiceType(yy_1_1_4_1, yy_1_1_4_2, &yy_1_1_4_3);
yyv_Index = yy_1_1_4_3;
yy_1_1_5 = (yy) yyv_Index[1];
if (yy_1_1_5 == (yy) yyu) yyErr(1,3499);
yyv_Predicates = yy_1_1_5;
yy_1_1_6_1 = yyv_Name;
yy_1_1_6_2 = yy_1_1_6_1;
if (yy_1_1_6_2[0] != 1) goto yyfl_323_3_1_1;
yy_1_1_6_2_1 = ((yy)yy_1_1_6_2[1]);
yyv_Predicate = yy_1_1_6_2_1;
yy_1_1_7_1 = yyv_Predicate;
yy_1_1_7_2 = yyv_Predicates;
yy_1_1_7_3 = ((yy)1);
GetCntlOffset(yy_1_1_7_1, yy_1_1_7_2, yy_1_1_7_3, &yy_1_1_7_4);
yyv_Offset = yy_1_1_7_4;
yy_1_1_8_1 = yyv_Arg;
GetUseArgTmp(yy_1_1_8_1, &yy_1_1_8_2);
yyv_Tmp = yy_1_1_8_2;
{
yy yysb = yyb;
yy_1_1_9_1_1 = yyglov_insideChoiceRule;
if (yy_1_1_9_1_1 == (yy) yyu) yyErr(1,3504);
if (yy_1_1_9_1_1[0] != 2) goto yyfl_323_3_1_1_9_1;
yy_1_1_9_1_2_1 = ((yy)"{");
s(yy_1_1_9_1_2_1);
nl();
yy_1_1_9_1_4_1 = ((yy)"yy yyCntl; yysave yyCntlSaved; int (* yyfun) ();");
s(yy_1_1_9_1_4_1);
nl();
yy_1_1_9_1_6_1 = ((yy)"yyBeginChoice(&yyCntlSaved);");
s(yy_1_1_9_1_6_1);
nl();
yy_1_1_9_1_8_1 = ((yy)"yyTry_");
s(yy_1_1_9_1_8_1);
yy_1_1_9_1_9_1 = yyv_Type;
id(yy_1_1_9_1_9_1);
yy_1_1_9_1_10_1 = ((yy)"(");
s(yy_1_1_9_1_10_1);
yy_1_1_9_1_11_1 = yyv_Tmp;
tmp(yy_1_1_9_1_11_1);
yy_1_1_9_1_12_1 = ((yy)",&yyCntl);");
s(yy_1_1_9_1_12_1);
nl();
yy_1_1_9_1_14_1 = ((yy)"yyfun = (int (*) ()) yyCntl[");
s(yy_1_1_9_1_14_1);
yy_1_1_9_1_15_1 = yyv_Offset;
i(yy_1_1_9_1_15_1);
yy_1_1_9_1_16_1 = ((yy)"];");
s(yy_1_1_9_1_16_1);
nl();
yy_1_1_9_1_18_1 = ((yy)"if (yyfun == (int (*) ()) 0) { yyEndChoice(yyCntlSaved); ");
s(yy_1_1_9_1_18_1);
Fail();
yy_1_1_9_1_20_1 = ((yy)" }");
s(yy_1_1_9_1_20_1);
nl();
yy_1_1_9_1_22_1 = ((yy)"yyfun (yyCntl, ");
s(yy_1_1_9_1_22_1);
yy_1_1_9_1_23_1 = yyv_InArgs;
yy_1_1_9_1_23_2 = ((yy)"");
ListInArgs(yy_1_1_9_1_23_1, yy_1_1_9_1_23_2, &yy_1_1_9_1_23_3);
yyv_Sep = yy_1_1_9_1_23_3;
yy_1_1_9_1_24_1 = yyv_OutArgs;
yy_1_1_9_1_24_2 = yyv_Sep;
ListOutArgs(yy_1_1_9_1_24_1, yy_1_1_9_1_24_2);
yy_1_1_9_1_25_1 = ((yy)");");
s(yy_1_1_9_1_25_1);
nl();
yy_1_1_9_1_27_1 = ((yy)"yyEndChoice(yyCntlSaved);");
s(yy_1_1_9_1_27_1);
nl();
yy_1_1_9_1_29_1 = ((yy)"}");
s(yy_1_1_9_1_29_1);
nl();
goto yysl_323_3_1_1_9;
yyfl_323_3_1_1_9_1 : ;
yy_1_1_9_2_1 = yyglov_insideChoiceRule;
if (yy_1_1_9_2_1 == (yy) yyu) yyErr(1,3520);
if (yy_1_1_9_2_1[0] != 1) goto yyfl_323_3_1_1_9_2;
yy_1_1_9_2_2 = yyglov_pathes;
if (yy_1_1_9_2_2 == (yy) yyu) yyErr(1,3522);
yyv_Pathes = yy_1_1_9_2_2;
yy_1_1_9_2_3_1 = yyv_InArgs;
yy_1_1_9_2_3_2 = yy_1_1_9_2_3_1;
if (yy_1_1_9_2_3_2[0] != 1) goto yyfl_323_3_1_1_9_2;
yy_1_1_9_2_3_2_1 = ((yy)yy_1_1_9_2_3_2[1]);
yy_1_1_9_2_3_2_2 = ((yy)yy_1_1_9_2_3_2[2]);
yy_1_1_9_2_3_2_3 = ((yy)yy_1_1_9_2_3_2[3]);
if (yy_1_1_9_2_3_2_1[0] != 5) goto yyfl_323_3_1_1_9_2;
yy_1_1_9_2_3_2_1_1 = ((yy)yy_1_1_9_2_3_2_1[1]);
yy_1_1_9_2_3_2_1_2 = ((yy)yy_1_1_9_2_3_2_1[2]);
yy_1_1_9_2_3_2_1_3 = ((yy)yy_1_1_9_2_3_2_1[3]);
yyv_V = yy_1_1_9_2_3_2_1_2;
yyv_VarPos = yy_1_1_9_2_3_2_1_3;
yy_1_1_9_2_4_1 = yyv_V;
yy_1_1_9_2_4_2 = yyv_Pathes;
yy_1_1_9_2_4_3 = yyv_VarPos;
LookupPath(yy_1_1_9_2_4_1, yy_1_1_9_2_4_2, yy_1_1_9_2_4_3, &yy_1_1_9_2_4_4);
yyv_Path = yy_1_1_9_2_4_4;
yy_1_1_9_2_5_1 = ((yy)"{ int (* yyfun) ();");
s(yy_1_1_9_2_5_1);
nl();
yy_1_1_9_2_7_1 = ((yy)"yyfun = (int (*) ()) ((yy) ");
s(yy_1_1_9_2_7_1);
yy_1_1_9_2_8_1 = yyv_Path;
cntlpath(yy_1_1_9_2_8_1);
yy_1_1_9_2_9_1 = ((yy)") [");
s(yy_1_1_9_2_9_1);
yy_1_1_9_2_10_1 = yyv_Offset;
i(yy_1_1_9_2_10_1);
yy_1_1_9_2_11_1 = ((yy)"];");
s(yy_1_1_9_2_11_1);
nl();
yy_1_1_9_2_13_1 = ((yy)"yyfun (");
s(yy_1_1_9_2_13_1);
yy_1_1_9_2_14_1 = yyv_Path;
cntlpath(yy_1_1_9_2_14_1);
yy_1_1_9_2_15_1 = yyv_InArgs;
yy_1_1_9_2_15_2 = ((yy)", ");
ListInArgs(yy_1_1_9_2_15_1, yy_1_1_9_2_15_2, &yy_1_1_9_2_15_3);
yyv_Sep = yy_1_1_9_2_15_3;
yy_1_1_9_2_16_1 = yyv_OutArgs;
yy_1_1_9_2_16_2 = yyv_Sep;
ListOutArgs(yy_1_1_9_2_16_1, yy_1_1_9_2_16_2);
yy_1_1_9_2_17_1 = ((yy)");");
s(yy_1_1_9_2_17_1);
nl();
yy_1_1_9_2_19_1 = ((yy)"}");
s(yy_1_1_9_2_19_1);
nl();
goto yysl_323_3_1_1_9;
yyfl_323_3_1_1_9_2 : ;
goto yyfl_323_3_1_1;
yysl_323_3_1_1_9 : ;
yyb = yysb;
}
goto yysl_323_3_1;
yyfl_323_3_1_1 : ;
goto yysl_323_3_1;
yysl_323_3_1 : ;
yyb = yysb;
}
return;
yyfl_323_3 : ;
}
yyErr(2,3478);
}
MakePath(yyin_1, yyin_2, yyout_1)
yy yyin_1;
yy yyin_2;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_Name;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_Var;
yy yy_0_2;
yy yy_0_3;
yy yy_1_1;
yy yy_1_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 5) goto yyfl_324_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Name = yy_0_1_2;
yyv_Var = yy_0_2;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_Name;
yy_1_2 = yyv_Var;
if (! yyeq_IDENT(yy_1_1, yy_1_2)) goto yyfl_324_1;
yy_0_3 = yyb + 0;
yy_0_3[0] = 2;
*yyout_1 = yy_0_3;
return;
yyfl_324_1 : ;
}
yyErr(2,3541);
}
Fail()
{
{
yy yyb;
yy yy_1_1;
yy yy_3_1;
yy yy_4;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_1_1 = ((yy)"goto ");
s(yy_1_1);
FailLabel();
yy_3_1 = ((yy)";");
s(yy_3_1);
yy_4 = yyb + 0;
yy_4[0] = 1;
yyglov_currentFailLabelUsed = yy_4;
return;
}
}
FailLabel()
{
{
yy yyb;
yy yyv_P;
yy yy_1;
yy yyv_R;
yy yy_2;
yy yyv_L;
yy yy_3;
yy yy_4_1;
yy yy_5_1;
yy yy_6_1;
yy yy_7_1;
yy yy_8_1;
yy_1 = yyglov_currentProcNumber;
if (yy_1 == (yy) yyu) yyErr(1,3555);
yyv_P = yy_1;
yy_2 = yyglov_currentRuleNumber;
if (yy_2 == (yy) yyu) yyErr(1,3556);
yyv_R = yy_2;
yy_3 = yyglov_currentFailLabel;
if (yy_3 == (yy) yyu) yyErr(1,3557);
yyv_L = yy_3;
yy_4_1 = ((yy)"yyfl_");
s(yy_4_1);
yy_5_1 = yyv_P;
i(yy_5_1);
yy_6_1 = ((yy)"_");
s(yy_6_1);
yy_7_1 = yyv_R;
i(yy_7_1);
yy_8_1 = yyv_L;
tmpval(yy_8_1);
return;
}
}
SuccessLabel()
{
{
yy yyb;
yy yyv_P;
yy yy_1;
yy yyv_R;
yy yy_2;
yy yyv_L;
yy yy_3;
yy yy_4_1;
yy yy_5_1;
yy yy_6_1;
yy yy_7_1;
yy yy_8_1;
yy_1 = yyglov_currentProcNumber;
if (yy_1 == (yy) yyu) yyErr(1,3563);
yyv_P = yy_1;
yy_2 = yyglov_currentRuleNumber;
if (yy_2 == (yy) yyu) yyErr(1,3564);
yyv_R = yy_2;
yy_3 = yyglov_currentSuccessLabel;
if (yy_3 == (yy) yyu) yyErr(1,3565);
yyv_L = yy_3;
yy_4_1 = ((yy)"yysl_");
s(yy_4_1);
yy_5_1 = yyv_P;
i(yy_5_1);
yy_6_1 = ((yy)"_");
s(yy_6_1);
yy_7_1 = yyv_R;
i(yy_7_1);
yy_8_1 = yyv_L;
tmpval(yy_8_1);
return;
}
}
ListInArgs(yyin_1, yyin_2, yyout_1)
yy yyin_1;
yy yyin_2;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_T;
yy yy_0_1_3;
yy yyv_Sep;
yy yy_0_2;
yy yy_0_3;
yy yy_1_1;
yy yyv_Tmp;
yy yy_1_2;
yy yy_2_1;
yy yy_3_1;
yy yy_4_1;
yy yy_4_2;
yy yyv_NextSep;
yy yy_4_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_328_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_H = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_T = yy_0_1_3;
yyv_Sep = yy_0_2;
yy_1_1 = yyv_H;
GetUseArgTmp(yy_1_1, &yy_1_2);
yyv_Tmp = yy_1_2;
yy_2_1 = yyv_Sep;
s(yy_2_1);
yy_3_1 = yyv_Tmp;
tmp(yy_3_1);
yy_4_1 = yyv_T;
yy_4_2 = ((yy)", ");
ListInArgs(yy_4_1, yy_4_2, &yy_4_3);
yyv_NextSep = yy_4_3;
yy_0_3 = yyv_NextSep;
*yyout_1 = yy_0_3;
return;
yyfl_328_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_P;
yy yy_0_1_1;
yy yyv_Sep;
yy yy_0_2;
yy yy_0_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_328_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yyv_P = yy_0_1_1;
yyv_Sep = yy_0_2;
yy_0_3 = yyv_Sep;
*yyout_1 = yy_0_3;
return;
yyfl_328_2 : ;
}
yyErr(2,3568);
}
ListOutArgs(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_T;
yy yy_0_1_3;
yy yyv_Sep;
yy yy_0_2;
yy yy_1_1;
yy yyv_Tmp;
yy yy_1_2;
yy yy_2_1;
yy yy_3_1;
yy yy_4_1;
yy yy_5_1;
yy yy_5_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_329_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_H = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_T = yy_0_1_3;
yyv_Sep = yy_0_2;
yy_1_1 = yyv_H;
GetDefArgTmp(yy_1_1, &yy_1_2);
yyv_Tmp = yy_1_2;
yy_2_1 = yyv_Sep;
s(yy_2_1);
yy_3_1 = ((yy)"&");
s(yy_3_1);
yy_4_1 = yyv_Tmp;
tmp(yy_4_1);
yy_5_1 = yyv_T;
yy_5_2 = ((yy)", ");
ListOutArgs(yy_5_1, yy_5_2);
return;
yyfl_329_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_P;
yy yy_0_1_1;
yy yyv_Sep;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_329_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yyv_P = yy_0_1_1;
yyv_Sep = yy_0_2;
return;
yyfl_329_2 : ;
}
yyErr(2,3577);
}
int IsTable(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yy_1_1;
yy yy_1_2;
yy yy_1_2_1;
yy_0_1 = yyin_1;
yyv_Id = yy_0_1;
yy_1_1 = yyv_Id;
if (! GetGlobalMeaning(yy_1_1, &yy_1_2)) goto yyfl_330_1;
if (yy_1_2[0] != 3) goto yyfl_330_1;
yy_1_2_1 = ((yy)yy_1_2[1]);
return 1;
yyfl_330_1 : ;
}
return 0;
}
int IsPredicate(yyin_1, yyin_2, yyin_3, yyout_1, yyout_2, yyout_3, yyout_4)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy *yyout_1;
yy *yyout_2;
yy *yyout_3;
yy *yyout_4;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yyv_InArgs;
yy yy_0_2;
yy yyv_Pos;
yy yy_0_3;
yy yy_0_4;
yy yy_0_5;
yy yy_0_6;
yy yy_0_7;
yy yy_1_1;
yy yyv_Head;
yy yy_1_2;
yy yy_2_1;
yy yy_2_2;
yy yyv_CodeName;
yy yy_2_2_1;
yy yyv_Class;
yy yy_2_2_2;
yy yy_2_2_3;
yy yyv_In;
yy yy_2_2_3_1;
yy yyv_Out;
yy yy_2_2_3_2;
yy yyv_Rules;
yy yy_2_2_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Id = yy_0_1;
yyv_InArgs = yy_0_2;
yyv_Pos = yy_0_3;
yy_1_1 = yyv_Id;
if (! GetGlobalMeaning(yy_1_1, &yy_1_2)) goto yyfl_331_1;
yyv_Head = yy_1_2;
yy_2_1 = yyv_Head;
yy_2_2 = yy_2_1;
if (yy_2_2[0] != 4) goto yyfl_331_1;
yy_2_2_1 = ((yy)yy_2_2[1]);
yy_2_2_2 = ((yy)yy_2_2[2]);
yy_2_2_3 = ((yy)yy_2_2[3]);
yy_2_2_4 = ((yy)yy_2_2[4]);
yyv_CodeName = yy_2_2_1;
yyv_Class = yy_2_2_2;
if (yy_2_2_3[0] != 1) goto yyfl_331_1;
yy_2_2_3_1 = ((yy)yy_2_2_3[1]);
yy_2_2_3_2 = ((yy)yy_2_2_3[2]);
yyv_In = yy_2_2_3_1;
yyv_Out = yy_2_2_3_2;
yyv_Rules = yy_2_2_4;
yy_0_4 = yyv_CodeName;
yy_0_5 = yyv_Class;
yy_0_6 = yyv_In;
yy_0_7 = yyv_Out;
*yyout_1 = yy_0_4;
*yyout_2 = yy_0_5;
*yyout_3 = yy_0_6;
*yyout_4 = yy_0_7;
return 1;
yyfl_331_1 : ;
}
return 0;
}
int IdentIsInList(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yy_0_2;
yy yyv_Head;
yy yy_0_2_1;
yy yyv_Tail;
yy yy_0_2_2;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Id = yy_0_1;
if (yy_0_2[0] != 1) goto yyfl_332_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yyv_Head = yy_0_2_1;
yyv_Tail = yy_0_2_2;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Id;
yy_1_1_1_2 = yyv_Head;
if (! yyeq_IDENT(yy_1_1_1_1, yy_1_1_1_2)) goto yyfl_332_1_1_1;
goto yysl_332_1_1;
yyfl_332_1_1_1 : ;
yy_1_2_1_1 = yyv_Id;
yy_1_2_1_2 = yyv_Tail;
if (! IdentIsInList(yy_1_2_1_1, yy_1_2_1_2)) goto yyfl_332_1_1_2;
goto yysl_332_1_1;
yyfl_332_1_1_2 : ;
goto yyfl_332_1;
yysl_332_1_1 : ;
yyb = yysb;
}
return 1;
yyfl_332_1 : ;
}
return 0;
}
int IsType(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yy_1_1;
yy yy_1_2;
yy yy_1_2_1;
yy yyv_Functors;
yy yy_1_2_2;
yy_0_1 = yyin_1;
yyv_Id = yy_0_1;
yy_1_1 = yyv_Id;
if (! GetGlobalMeaning(yy_1_1, &yy_1_2)) goto yyfl_333_1;
if (yy_1_2[0] != 1) goto yyfl_333_1;
yy_1_2_1 = ((yy)yy_1_2[1]);
yy_1_2_2 = ((yy)yy_1_2[2]);
yyv_Functors = yy_1_2_2;
return 1;
yyfl_333_1 : ;
}
return 0;
}
int IsVariable(yyin_1, yyout_1)
yy yyin_1;
yy *yyout_1;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy yyv_Type;
yy yy_1_2_1;
yy yyv_P;
yy yy_1_2_2;
yy_0_1 = yyin_1;
yyv_Id = yy_0_1;
yy_1_1 = yyv_Id;
if (! GetGlobalMeaning(yy_1_1, &yy_1_2)) goto yyfl_334_1;
if (yy_1_2[0] != 2) goto yyfl_334_1;
yy_1_2_1 = ((yy)yy_1_2[1]);
yy_1_2_2 = ((yy)yy_1_2[2]);
yyv_Type = yy_1_2_1;
yyv_P = yy_1_2_2;
yy_0_2 = yyv_Type;
*yyout_1 = yy_0_2;
return 1;
yyfl_334_1 : ;
}
return 0;
}
CheckVariable(yyin_1, yyin_2, yyout_1)
yy yyin_1;
yy yyin_2;
yy *yyout_1;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yyv_P;
yy yy_0_2;
yy yy_0_3;
yy yy_1_1;
yy yyv_Type;
yy yy_1_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Id = yy_0_1;
yyv_P = yy_0_2;
yy_1_1 = yyv_Id;
if (! IsVariable(yy_1_1, &yy_1_2)) goto yyfl_335_1;
yyv_Type = yy_1_2;
yy_0_3 = yyv_Type;
*yyout_1 = yy_0_3;
return;
yyfl_335_1 : ;
}
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yyv_P;
yy yy_0_2;
yy yy_0_3;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yy_1_4;
yy yyv_Error;
yy yy_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Id = yy_0_1;
yyv_P = yy_0_2;
yy_1_1 = ((yy)"'");
yy_1_2 = yyv_Id;
yy_1_3 = ((yy)"' is not a global variable");
yy_1_4 = yyv_P;
MESSAGE1(yy_1_1, yy_1_2, yy_1_3, yy_1_4);
yy_2 = yyglov_errorId;
if (yy_2 == (yy) yyu) yyErr(1,3621);
yyv_Error = yy_2;
yy_0_3 = yyv_Error;
*yyout_1 = yy_0_3;
return;
yyfl_335_2 : ;
}
yyErr(2,3615);
}
CheckType(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yyv_P;
yy yy_0_2;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yy_1_1_1_2_1;
yy yyv_Type;
yy yy_1_1_1_2_2;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yyv_List;
yy yy_1_2_1_2_1;
yy yy_1_3_1_1;
yy yy_1_3_1_2;
yy yy_1_3_1_3;
yy yy_1_3_1_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Id = yy_0_1;
yyv_P = yy_0_2;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Id;
if (! GetGlobalMeaning(yy_1_1_1_1, &yy_1_1_1_2)) goto yyfl_336_1_1_1;
if (yy_1_1_1_2[0] != 1) goto yyfl_336_1_1_1;
yy_1_1_1_2_1 = ((yy)yy_1_1_1_2[1]);
yy_1_1_1_2_2 = ((yy)yy_1_1_1_2[2]);
yyv_Type = yy_1_1_1_2_2;
goto yysl_336_1_1;
yyfl_336_1_1_1 : ;
yy_1_2_1_1 = yyv_Id;
if (! GetGlobalMeaning(yy_1_2_1_1, &yy_1_2_1_2)) goto yyfl_336_1_1_2;
if (yy_1_2_1_2[0] != 3) goto yyfl_336_1_1_2;
yy_1_2_1_2_1 = ((yy)yy_1_2_1_2[1]);
yyv_List = yy_1_2_1_2_1;
goto yysl_336_1_1;
yyfl_336_1_1_2 : ;
yy_1_3_1_1 = ((yy)"'");
yy_1_3_1_2 = yyv_Id;
yy_1_3_1_3 = ((yy)"' not declared as type");
yy_1_3_1_4 = yyv_P;
MESSAGE1(yy_1_3_1_1, yy_1_3_1_2, yy_1_3_1_3, yy_1_3_1_4);
goto yysl_336_1_1;
yysl_336_1_1 : ;
yyb = yysb;
}
return;
}
}
CheckTableField(yyin_1, yyin_2, yyin_3, yyin_4, yyout_1, yyout_2)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy yyin_4;
yy *yyout_1;
yy *yyout_2;
{
{
yy yyb;
yy yyv_Key;
yy yy_0_1;
yy yyv_Field;
yy yy_0_2;
yy yyv_ReqAccess;
yy yy_0_3;
yy yyv_Pos;
yy yy_0_4;
yy yy_0_5;
yy yy_0_6;
yy yy_1_1_1_1;
yy yyv_KeyType;
yy yy_1_1_1_2;
yy yy_1_1_2_1;
yy yy_1_1_2_2;
yy yy_1_1_2_2_1;
yy yyv_FunctorSpecs;
yy yy_1_1_2_2_2;
yy yy_1_1_3_1_1_1;
yy yy_1_1_3_2_1_1;
yy yy_1_1_3_2_1_2;
yy yy_1_1_3_2_1_3;
yy yy_1_1_3_2_1_4;
yy yy_1_1_4_1;
yy yyv_FunctorCode;
yy yy_1_1_4_2;
yy yy_1_1_5_1_1_1;
yy yy_1_1_5_1_1_2;
yy yy_1_1_5_1_1_2_1;
yy yy_1_1_5_1_2_1_1_1;
yy yy_1_1_5_1_2_1_1_2;
yy yy_1_1_5_1_2_1_2_1;
yy yy_1_1_5_1_2_1_2_2;
yy yy_1_1_5_1_2_1_2_3;
yy yy_1_1_5_1_2_1_2_4;
yy yy_1_1_5_1_3_1;
yy yy_1_1_5_1_3_2;
yy yy_1_1_5_1_3_3;
yy yy_1_1_5_1_3_4;
yy yyv_Type;
yy yy_1_1_5_1_3_5;
yy yyv_Offset;
yy yy_1_1_5_1_3_6;
yy yyv_Access;
yy yy_1_1_5_1_3_7;
yy yy_1_1_5_2_1_1;
yy yy_1_1_5_2_1_2;
yy yy_1_1_5_2_1_3;
yy yyv_FunctorSpec;
yy yy_1_1_5_2_1_4;
yy yy_1_1_5_2_2_1;
yy yy_1_1_5_2_2_1_1;
yy yy_1_1_5_2_2_1_2;
yy yyv_Fields;
yy yy_1_1_5_2_2_1_3;
yy yy_1_1_5_2_2_2;
yy yy_1_1_5_2_3_1;
yy yy_1_1_5_2_3_2;
yy yy_1_1_5_2_3_3;
yy yy_1_1_5_2_3_4;
yy yy_1_1_5_2_3_5;
yy yy_1_1_5_2_3_6;
yy yy_1_1_5_2_3_7;
yy yy_1_1_5_2_4_1_1_1;
yy yy_1_1_5_2_4_1_1_2;
yy yy_1_1_5_2_4_1_2_1;
yy yy_1_1_5_2_4_1_2_2;
yy yy_1_1_5_2_4_1_3_1;
yy yy_1_1_5_2_4_1_3_2;
yy yy_1_1_5_2_4_1_3_3;
yy yy_1_1_5_2_4_1_3_4;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yyv_Table;
yy yy_1_2_1_3;
yy yy_1_2_2_1;
yy yy_1_2_2_2;
yy yy_1_2_2_3;
yy yy_1_2_2_4;
yy yy_1_2_2_5;
yy yy_1_2_2_6;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
yyv_Key = yy_0_1;
yyv_Field = yy_0_2;
yyv_ReqAccess = yy_0_3;
yyv_Pos = yy_0_4;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Key;
if (! GetLocalMeaning(yy_1_1_1_1, &yy_1_1_1_2)) goto yyfl_337_1_1_1;
yyv_KeyType = yy_1_1_1_2;
yy_1_1_2_1 = yyv_KeyType;
if (! GetGlobalMeaning(yy_1_1_2_1, &yy_1_1_2_2)) goto yyfl_337_1_1_1;
if (yy_1_1_2_2[0] != 1) goto yyfl_337_1_1_1;
yy_1_1_2_2_1 = ((yy)yy_1_1_2_2[1]);
yy_1_1_2_2_2 = ((yy)yy_1_1_2_2[2]);
yyv_FunctorSpecs = yy_1_1_2_2_2;
{
yy yysb = yyb;
yy_1_1_3_1_1_1 = yyv_Key;
if (! HasLocalMeaning(yy_1_1_3_1_1_1)) goto yyfl_337_1_1_1_3_1;
goto yysl_337_1_1_1_3;
yyfl_337_1_1_1_3_1 : ;
yy_1_1_3_2_1_1 = ((yy)"'");
yy_1_1_3_2_1_2 = yyv_Key;
yy_1_1_3_2_1_3 = ((yy)"' has no value");
yy_1_1_3_2_1_4 = yyv_Pos;
MESSAGE1(yy_1_1_3_2_1_1, yy_1_1_3_2_1_2, yy_1_1_3_2_1_3, yy_1_1_3_2_1_4);
goto yysl_337_1_1_1_3;
yysl_337_1_1_1_3 : ;
yyb = yysb;
}
yy_1_1_4_1 = yyv_Key;
if (! GetLocalMeaning2(yy_1_1_4_1, &yy_1_1_4_2)) goto yyfl_337_1_1_1;
yyv_FunctorCode = yy_1_1_4_2;
{
yy yysb = yyb;
yy_1_1_5_1_1_1 = yyv_FunctorCode;
yy_1_1_5_1_1_2_1 = ((yy)1);
yy_1_1_5_1_1_2 = (yy)(-((long)yy_1_1_5_1_1_2_1));
if ((long)yy_1_1_5_1_1_1 != (long)yy_1_1_5_1_1_2) goto yyfl_337_1_1_1_5_1;
{
yy yysb = yyb;
yy_1_1_5_1_2_1_1_2 = yyv_FunctorSpecs;
yy_1_1_5_1_2_1_1_1 = yy_1_1_5_1_2_1_1_2;
if (yy_1_1_5_1_2_1_1_1[0] != 2) goto yyfl_337_1_1_1_5_1_2_1;
yy_1_1_5_1_2_1_2_1 = ((yy)"type '");
yy_1_1_5_1_2_1_2_2 = yyv_KeyType;
yy_1_1_5_1_2_1_2_3 = ((yy)"' has no fields");
yy_1_1_5_1_2_1_2_4 = yyv_Pos;
MESSAGE1(yy_1_1_5_1_2_1_2_1, yy_1_1_5_1_2_1_2_2, yy_1_1_5_1_2_1_2_3, yy_1_1_5_1_2_1_2_4);
goto yysl_337_1_1_1_5_1_2;
yyfl_337_1_1_1_5_1_2_1 : ;
goto yysl_337_1_1_1_5_1_2;
yysl_337_1_1_1_5_1_2 : ;
yyb = yysb;
}
yy_1_1_5_1_3_1 = yyv_FunctorSpecs;
yy_1_1_5_1_3_2 = yyv_Field;
yy_1_1_5_1_3_3 = yyv_KeyType;
yy_1_1_5_1_3_4 = yyv_Pos;
LookupFieldForAllFunctors(yy_1_1_5_1_3_1, yy_1_1_5_1_3_2, yy_1_1_5_1_3_3, yy_1_1_5_1_3_4, &yy_1_1_5_1_3_5, &yy_1_1_5_1_3_6, &yy_1_1_5_1_3_7);
yyv_Type = yy_1_1_5_1_3_5;
yyv_Offset = yy_1_1_5_1_3_6;
yyv_Access = yy_1_1_5_1_3_7;
goto yysl_337_1_1_1_5;
yyfl_337_1_1_1_5_1 : ;
yy_1_1_5_2_1_1 = yyv_FunctorSpecs;
yy_1_1_5_2_1_2 = yyv_FunctorCode;
yy_1_1_5_2_1_3 = ((yy)1);
GetFunctorSpecForCode(yy_1_1_5_2_1_1, yy_1_1_5_2_1_2, yy_1_1_5_2_1_3, &yy_1_1_5_2_1_4);
yyv_FunctorSpec = yy_1_1_5_2_1_4;
yy_1_1_5_2_2_2 = yyv_FunctorSpec;
yy_1_1_5_2_2_1 = yy_1_1_5_2_2_2;
if (yy_1_1_5_2_2_1[0] != 1) goto yyfl_337_1_1_1_5_2;
yy_1_1_5_2_2_1_1 = ((yy)yy_1_1_5_2_2_1[1]);
yy_1_1_5_2_2_1_2 = ((yy)yy_1_1_5_2_2_1[2]);
yy_1_1_5_2_2_1_3 = ((yy)yy_1_1_5_2_2_1[3]);
yyv_Fields = yy_1_1_5_2_2_1_3;
yy_1_1_5_2_3_1 = yyv_Fields;
yy_1_1_5_2_3_2 = yyv_Field;
yy_1_1_5_2_3_3 = ((yy)1);
yy_1_1_5_2_3_4 = yyv_Pos;
LookupNamedField(yy_1_1_5_2_3_1, yy_1_1_5_2_3_2, yy_1_1_5_2_3_3, yy_1_1_5_2_3_4, &yy_1_1_5_2_3_5, &yy_1_1_5_2_3_6, &yy_1_1_5_2_3_7);
yyv_Type = yy_1_1_5_2_3_5;
yyv_Offset = yy_1_1_5_2_3_6;
yyv_Access = yy_1_1_5_2_3_7;
{
yy yysb = yyb;
yy_1_1_5_2_4_1_1_2 = yyv_ReqAccess;
yy_1_1_5_2_4_1_1_1 = yy_1_1_5_2_4_1_1_2;
if (yy_1_1_5_2_4_1_1_1[0] != 1) goto yyfl_337_1_1_1_5_2_4_1;
yy_1_1_5_2_4_1_2_2 = yyv_Access;
yy_1_1_5_2_4_1_2_1 = yy_1_1_5_2_4_1_2_2;
if (yy_1_1_5_2_4_1_2_1[0] != 2) goto yyfl_337_1_1_1_5_2_4_1;
yy_1_1_5_2_4_1_3_1 = ((yy)"Field '");
yy_1_1_5_2_4_1_3_2 = yyv_Field;
yy_1_1_5_2_4_1_3_3 = ((yy)"' is not variable");
yy_1_1_5_2_4_1_3_4 = yyv_Pos;
MESSAGE1(yy_1_1_5_2_4_1_3_1, yy_1_1_5_2_4_1_3_2, yy_1_1_5_2_4_1_3_3, yy_1_1_5_2_4_1_3_4);
goto yysl_337_1_1_1_5_2_4;
yyfl_337_1_1_1_5_2_4_1 : ;
goto yysl_337_1_1_1_5_2_4;
yysl_337_1_1_1_5_2_4 : ;
yyb = yysb;
}
goto yysl_337_1_1_1_5;
yyfl_337_1_1_1_5_2 : ;
goto yyfl_337_1_1_1;
yysl_337_1_1_1_5 : ;
yyb = yysb;
}
goto yysl_337_1_1;
yyfl_337_1_1_1 : ;
yy_1_2_1_1 = yyv_Key;
yy_1_2_1_2 = yyv_Pos;
CheckKey(yy_1_2_1_1, yy_1_2_1_2, &yy_1_2_1_3);
yyv_Table = yy_1_2_1_3;
yy_1_2_2_1 = yyv_Field;
yy_1_2_2_2 = yyv_Table;
yy_1_2_2_3 = yyv_Pos;
yy_1_2_2_4 = ((yy)1);
CheckField(yy_1_2_2_1, yy_1_2_2_2, yy_1_2_2_3, yy_1_2_2_4, &yy_1_2_2_5, &yy_1_2_2_6);
yyv_Type = yy_1_2_2_5;
yyv_Offset = yy_1_2_2_6;
goto yysl_337_1_1;
yysl_337_1_1 : ;
yyb = yysb;
}
yy_0_5 = yyv_Type;
yy_0_6 = yyv_Offset;
*yyout_1 = yy_0_5;
*yyout_2 = yy_0_6;
return;
}
}
LookupFieldForAllFunctors(yyin_1, yyin_2, yyin_3, yyin_4, yyout_1, yyout_2, yyout_3)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy yyin_4;
yy *yyout_1;
yy *yyout_2;
yy *yyout_3;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Head;
yy yy_0_1_1;
yy yyv_Tail;
yy yy_0_1_2;
yy yyv_Field;
yy yy_0_2;
yy yyv_KeyType;
yy yy_0_3;
yy yyv_Pos;
yy yy_0_4;
yy yy_0_5;
yy yy_0_6;
yy yy_0_7;
yy yy_1_1;
yy yy_1_1_1;
yy yy_1_1_2;
yy yyv_ArgSpecs;
yy yy_1_1_3;
yy yy_1_2;
yy yy_2_1;
yy yy_2_2;
yy yy_2_3;
yy yy_2_4;
yy yyv_Type1;
yy yy_2_5;
yy yyv_Offset1;
yy yy_2_6;
yy yyv_Access1;
yy yy_2_7;
yy yy_3_1_1_1;
yy yy_3_1_1_2;
yy yyv_Type;
yy yy_3_1_2_1;
yy yy_3_1_2_2;
yy yyv_Offset;
yy yy_3_1_3_1;
yy yy_3_1_3_2;
yy yyv_Access;
yy yy_3_1_4_1;
yy yy_3_1_4_2;
yy yy_3_2_1_1;
yy yy_3_2_1_2;
yy yy_3_2_1_3;
yy yy_3_2_1_4;
yy yyv_Type2;
yy yy_3_2_1_5;
yy yyv_Offset2;
yy yy_3_2_1_6;
yy yyv_Access2;
yy yy_3_2_1_7;
yy yy_3_2_2_1_1_1;
yy yy_3_2_2_1_1_2;
yy yy_3_2_2_1_2_1;
yy yy_3_2_2_1_2_2;
yy yy_3_2_2_1_2_3;
yy yy_3_2_2_1_2_4;
yy yy_3_2_2_1_2_5;
yy yy_3_2_2_1_2_6;
yy yy_3_2_3_1;
yy yy_3_2_3_2;
yy yy_3_2_4_1_1_1;
yy yy_3_2_4_1_1_2;
yy yy_3_2_4_1_2_1;
yy yy_3_2_4_1_2_2;
yy yy_3_2_4_1_2_3;
yy yy_3_2_4_1_2_4;
yy yy_3_2_4_1_2_5;
yy yy_3_2_4_1_2_6;
yy yy_3_2_5_1;
yy yy_3_2_5_2;
yy yy_3_2_6_1_1_1;
yy yy_3_2_6_1_1_2;
yy yy_3_2_6_1_2_1;
yy yy_3_2_6_1_2_2;
yy yy_3_2_6_1_3_1;
yy yy_3_2_6_1_3_2;
yy yy_3_2_6_2_1_1;
yy yy_3_2_6_2_1_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
if (yy_0_1[0] != 1) goto yyfl_338_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_Head = yy_0_1_1;
yyv_Tail = yy_0_1_2;
yyv_Field = yy_0_2;
yyv_KeyType = yy_0_3;
yyv_Pos = yy_0_4;
yy_1_2 = yyv_Head;
yy_1_1 = yy_1_2;
if (yy_1_1[0] != 1) goto yyfl_338_1;
yy_1_1_1 = ((yy)yy_1_1[1]);
yy_1_1_2 = ((yy)yy_1_1[2]);
yy_1_1_3 = ((yy)yy_1_1[3]);
yyv_ArgSpecs = yy_1_1_3;
yy_2_1 = yyv_ArgSpecs;
yy_2_2 = yyv_Field;
yy_2_3 = ((yy)1);
yy_2_4 = yyv_Pos;
LookupNamedField(yy_2_1, yy_2_2, yy_2_3, yy_2_4, &yy_2_5, &yy_2_6, &yy_2_7);
yyv_Type1 = yy_2_5;
yyv_Offset1 = yy_2_6;
yyv_Access1 = yy_2_7;
{
yy yysb = yyb;
yy_3_1_1_2 = yyv_Tail;
yy_3_1_1_1 = yy_3_1_1_2;
if (yy_3_1_1_1[0] != 2) goto yyfl_338_1_3_1;
yy_3_1_2_2 = yyv_Type1;
yy_3_1_2_1 = yy_3_1_2_2;
yyv_Type = yy_3_1_2_1;
yy_3_1_3_2 = yyv_Offset1;
yy_3_1_3_1 = yy_3_1_3_2;
yyv_Offset = yy_3_1_3_1;
yy_3_1_4_2 = yyv_Access1;
yy_3_1_4_1 = yy_3_1_4_2;
yyv_Access = yy_3_1_4_1;
goto yysl_338_1_3;
yyfl_338_1_3_1 : ;
yy_3_2_1_1 = yyv_Tail;
yy_3_2_1_2 = yyv_Field;
yy_3_2_1_3 = yyv_KeyType;
yy_3_2_1_4 = yyv_Pos;
LookupFieldForAllFunctors(yy_3_2_1_1, yy_3_2_1_2, yy_3_2_1_3, yy_3_2_1_4, &yy_3_2_1_5, &yy_3_2_1_6, &yy_3_2_1_7);
yyv_Type2 = yy_3_2_1_5;
yyv_Offset2 = yy_3_2_1_6;
yyv_Access2 = yy_3_2_1_7;
{
yy yysb = yyb;
yy_3_2_2_1_1_1 = yyv_Type1;
yy_3_2_2_1_1_2 = yyv_Type2;
if (yyeq_IDENT(yy_3_2_2_1_1_1, yy_3_2_2_1_1_2)) goto yyfl_338_1_3_2_2_1;
yy_3_2_2_1_2_1 = ((yy)"'");
yy_3_2_2_1_2_2 = yyv_Field;
yy_3_2_2_1_2_3 = ((yy)"' has different types in '");
yy_3_2_2_1_2_4 = yyv_KeyType;
yy_3_2_2_1_2_5 = ((yy)"' alternatives");
yy_3_2_2_1_2_6 = yyv_Pos;
MESSAGE2(yy_3_2_2_1_2_1, yy_3_2_2_1_2_2, yy_3_2_2_1_2_3, yy_3_2_2_1_2_4, yy_3_2_2_1_2_5, yy_3_2_2_1_2_6);
goto yysl_338_1_3_2_2;
yyfl_338_1_3_2_2_1 : ;
goto yysl_338_1_3_2_2;
yysl_338_1_3_2_2 : ;
yyb = yysb;
}
yy_3_2_3_2 = yyv_Type1;
yy_3_2_3_1 = yy_3_2_3_2;
yyv_Type = yy_3_2_3_1;
{
yy yysb = yyb;
yy_3_2_4_1_1_1 = yyv_Offset1;
yy_3_2_4_1_1_2 = yyv_Offset2;
if ((long)yy_3_2_4_1_1_1 == (long)yy_3_2_4_1_1_2) goto yyfl_338_1_3_2_4_1;
yy_3_2_4_1_2_1 = ((yy)"'");
yy_3_2_4_1_2_2 = yyv_Field;
yy_3_2_4_1_2_3 = ((yy)"' has different positions in '");
yy_3_2_4_1_2_4 = yyv_KeyType;
yy_3_2_4_1_2_5 = ((yy)"' alternatives");
yy_3_2_4_1_2_6 = yyv_Pos;
MESSAGE2(yy_3_2_4_1_2_1, yy_3_2_4_1_2_2, yy_3_2_4_1_2_3, yy_3_2_4_1_2_4, yy_3_2_4_1_2_5, yy_3_2_4_1_2_6);
goto yysl_338_1_3_2_4;
yyfl_338_1_3_2_4_1 : ;
goto yysl_338_1_3_2_4;
yysl_338_1_3_2_4 : ;
yyb = yysb;
}
yy_3_2_5_2 = yyv_Offset1;
yy_3_2_5_1 = yy_3_2_5_2;
yyv_Offset = yy_3_2_5_1;
{
yy yysb = yyb;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_3_2_6_1_1_2 = yyv_Access1;
yy_3_2_6_1_1_1 = yy_3_2_6_1_1_2;
if (yy_3_2_6_1_1_1[0] != 1) goto yyfl_338_1_3_2_6_1;
yy_3_2_6_1_2_2 = yyv_Access2;
yy_3_2_6_1_2_1 = yy_3_2_6_1_2_2;
if (yy_3_2_6_1_2_1[0] != 1) goto yyfl_338_1_3_2_6_1;
yy_3_2_6_1_3_2 = yyb + 0;
yy_3_2_6_1_3_2[0] = 1;
yy_3_2_6_1_3_1 = yy_3_2_6_1_3_2;
yyv_Access = yy_3_2_6_1_3_1;
goto yysl_338_1_3_2_6;
yyfl_338_1_3_2_6_1 : ;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_3_2_6_2_1_2 = yyb + 0;
yy_3_2_6_2_1_2[0] = 2;
yy_3_2_6_2_1_1 = yy_3_2_6_2_1_2;
yyv_Access = yy_3_2_6_2_1_1;
goto yysl_338_1_3_2_6;
yysl_338_1_3_2_6 : ;
yyb = yysb;
}
goto yysl_338_1_3;
yysl_338_1_3 : ;
yyb = yysb;
}
yy_0_5 = yyv_Type;
yy_0_6 = yyv_Offset;
yy_0_7 = yyv_Access;
*yyout_1 = yy_0_5;
*yyout_2 = yy_0_6;
*yyout_3 = yy_0_7;
return;
yyfl_338_1 : ;
}
yyErr(2,3669);
}
GetFunctorSpecForCode(yyin_1, yyin_2, yyin_3, yyout_1)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_T;
yy yy_0_1_2;
yy yyv_N;
yy yy_0_2;
yy yyv_Start;
yy yy_0_3;
yy yy_0_4;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yyv_Spec;
yy yy_1_1_2_1;
yy yy_1_1_2_2;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_1_2_1_3;
yy yy_1_2_1_3_1;
yy yy_1_2_1_3_2;
yy yy_1_2_1_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 1) goto yyfl_339_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_H = yy_0_1_1;
yyv_T = yy_0_1_2;
yyv_N = yy_0_2;
yyv_Start = yy_0_3;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_N;
yy_1_1_1_2 = yyv_Start;
if ((long)yy_1_1_1_1 != (long)yy_1_1_1_2) goto yyfl_339_1_1_1;
yy_1_1_2_2 = yyv_H;
yy_1_1_2_1 = yy_1_1_2_2;
yyv_Spec = yy_1_1_2_1;
goto yysl_339_1_1;
yyfl_339_1_1_1 : ;
yy_1_2_1_1 = yyv_T;
yy_1_2_1_2 = yyv_N;
yy_1_2_1_3_1 = yyv_Start;
yy_1_2_1_3_2 = ((yy)1);
yy_1_2_1_3 = (yy)(((long)yy_1_2_1_3_1)+((long)yy_1_2_1_3_2));
GetFunctorSpecForCode(yy_1_2_1_1, yy_1_2_1_2, yy_1_2_1_3, &yy_1_2_1_4);
yyv_Spec = yy_1_2_1_4;
goto yysl_339_1_1;
yysl_339_1_1 : ;
yyb = yysb;
}
yy_0_4 = yyv_Spec;
*yyout_1 = yy_0_4;
return;
yyfl_339_1 : ;
}
yyErr(2,3707);
}
LookupNamedField(yyin_1, yyin_2, yyin_3, yyin_4, yyout_1, yyout_2, yyout_3)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy yyin_4;
yy *yyout_1;
yy *yyout_2;
yy *yyout_3;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_T;
yy yy_0_1_3;
yy yyv_Id;
yy yy_0_2;
yy yyv_N;
yy yy_0_3;
yy yyv_Pos;
yy yy_0_4;
yy yy_0_5;
yy yy_0_6;
yy yy_0_7;
yy yy_1_1_1_1;
yy yy_1_1_1_1_1;
yy yyv_Id1;
yy yy_1_1_1_1_1_1;
yy yyv_Type;
yy yy_1_1_1_1_2;
yy yyv_Access;
yy yy_1_1_1_1_3;
yy yy_1_1_1_2;
yy yy_1_1_2_1;
yy yy_1_1_2_2;
yy yyv_Offset;
yy yy_1_1_3_1;
yy yy_1_1_3_2;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_1_2_1_3;
yy yy_1_2_1_3_1;
yy yy_1_2_1_3_2;
yy yy_1_2_1_4;
yy yy_1_2_1_5;
yy yy_1_2_1_6;
yy yy_1_2_1_7;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
if (yy_0_1[0] != 1) goto yyfl_340_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_H = yy_0_1_1;
yyv_T = yy_0_1_3;
yyv_Id = yy_0_2;
yyv_N = yy_0_3;
yyv_Pos = yy_0_4;
{
yy yysb = yyb;
yy_1_1_1_2 = yyv_H;
yy_1_1_1_1 = yy_1_1_1_2;
if (yy_1_1_1_1[0] != 1) goto yyfl_340_1_1_1;
yy_1_1_1_1_1 = ((yy)yy_1_1_1_1[1]);
yy_1_1_1_1_2 = ((yy)yy_1_1_1_1[2]);
yy_1_1_1_1_3 = ((yy)yy_1_1_1_1[3]);
if (yy_1_1_1_1_1[0] != 1) goto yyfl_340_1_1_1;
yy_1_1_1_1_1_1 = ((yy)yy_1_1_1_1_1[1]);
yyv_Id1 = yy_1_1_1_1_1_1;
yyv_Type = yy_1_1_1_1_2;
yyv_Access = yy_1_1_1_1_3;
yy_1_1_2_1 = yyv_Id1;
yy_1_1_2_2 = yyv_Id;
if (! yyeq_IDENT(yy_1_1_2_1, yy_1_1_2_2)) goto yyfl_340_1_1_1;
yy_1_1_3_2 = yyv_N;
yy_1_1_3_1 = yy_1_1_3_2;
yyv_Offset = yy_1_1_3_1;
goto yysl_340_1_1;
yyfl_340_1_1_1 : ;
yy_1_2_1_1 = yyv_T;
yy_1_2_1_2 = yyv_Id;
yy_1_2_1_3_1 = yyv_N;
yy_1_2_1_3_2 = ((yy)1);
yy_1_2_1_3 = (yy)(((long)yy_1_2_1_3_1)+((long)yy_1_2_1_3_2));
yy_1_2_1_4 = yyv_Pos;
LookupNamedField(yy_1_2_1_1, yy_1_2_1_2, yy_1_2_1_3, yy_1_2_1_4, &yy_1_2_1_5, &yy_1_2_1_6, &yy_1_2_1_7);
yyv_Type = yy_1_2_1_5;
yyv_Offset = yy_1_2_1_6;
yyv_Access = yy_1_2_1_7;
goto yysl_340_1_1;
yysl_340_1_1 : ;
yyb = yysb;
}
yy_0_5 = yyv_Type;
yy_0_6 = yyv_Offset;
yy_0_7 = yyv_Access;
*yyout_1 = yy_0_5;
*yyout_2 = yy_0_6;
*yyout_3 = yy_0_7;
return;
yyfl_340_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Id;
yy yy_0_2;
yy yyv_N;
yy yy_0_3;
yy yyv_Pos;
yy yy_0_4;
yy yy_0_5;
yy yy_0_6;
yy yy_0_7;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yy_1_4;
yy yy_2_1;
yy yyv_Error;
yy yy_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
if (yy_0_1[0] != 2) goto yyfl_340_2;
yyv_Id = yy_0_2;
yyv_N = yy_0_3;
yyv_Pos = yy_0_4;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_1_1 = ((yy)"invalid field name '");
yy_1_2 = yyv_Id;
yy_1_3 = ((yy)"'");
yy_1_4 = yyv_Pos;
MESSAGE1(yy_1_1, yy_1_2, yy_1_3, yy_1_4);
yy_2_1 = ((yy)"ERROR");
string_to_id(yy_2_1, &yy_2_2);
yyv_Error = yy_2_2;
yy_0_5 = yyv_Error;
yy_0_6 = ((yy)0);
yy_0_7 = yyb + 0;
yy_0_7[0] = 2;
*yyout_1 = yy_0_5;
*yyout_2 = yy_0_6;
*yyout_3 = yy_0_7;
return;
yyfl_340_2 : ;
}
yyErr(2,3713);
}
CheckKey(yyin_1, yyin_2, yyout_1)
yy yyin_1;
yy yyin_2;
yy *yyout_1;
{
{
yy yyb;
yy yyv_Key;
yy yy_0_1;
yy yyv_Pos;
yy yy_0_2;
yy yy_0_3;
yy yy_1_1;
yy yy_2_1;
yy yyv_Meaning;
yy yy_2_2;
yy yy_3_1;
yy yy_3_2;
yy yyv_List;
yy yy_3_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Key = yy_0_1;
yyv_Pos = yy_0_2;
yy_1_1 = yyv_Key;
if (! HasLocalMeaning(yy_1_1)) goto yyfl_341_1;
yy_2_1 = yyv_Key;
if (! GetLocalMeaning(yy_2_1, &yy_2_2)) goto yyfl_341_1;
yyv_Meaning = yy_2_2;
yy_3_1 = yyv_Meaning;
yy_3_2 = yyv_Pos;
GetTableFields(yy_3_1, yy_3_2, &yy_3_3);
yyv_List = yy_3_3;
yy_0_3 = yyv_List;
*yyout_1 = yy_0_3;
return;
yyfl_341_1 : ;
}
{
yy yyb;
yy yyv_Key;
yy yy_0_1;
yy yyv_Pos;
yy yy_0_2;
yy yy_0_3;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yy_1_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Key = yy_0_1;
yyv_Pos = yy_0_2;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_1_1 = ((yy)"'");
yy_1_2 = yyv_Key;
yy_1_3 = ((yy)"' has no value");
yy_1_4 = yyv_Pos;
MESSAGE1(yy_1_1, yy_1_2, yy_1_3, yy_1_4);
yy_0_3 = yyb + 0;
yy_0_3[0] = 2;
*yyout_1 = yy_0_3;
return;
yyfl_341_2 : ;
}
yyErr(2,3729);
}
GetTableFields(yyin_1, yyin_2, yyout_1)
yy yyin_1;
yy yyin_2;
yy *yyout_1;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yyv_Pos;
yy yy_0_2;
yy yy_0_3;
yy yy_1_1;
yy yy_1_2;
yy yyv_List;
yy yy_1_2_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Id = yy_0_1;
yyv_Pos = yy_0_2;
yy_1_1 = yyv_Id;
if (! GetGlobalMeaning(yy_1_1, &yy_1_2)) goto yyfl_342_1;
if (yy_1_2[0] != 3) goto yyfl_342_1;
yy_1_2_1 = ((yy)yy_1_2[1]);
yyv_List = yy_1_2_1;
yy_0_3 = yyv_List;
*yyout_1 = yy_0_3;
return;
yyfl_342_1 : ;
}
{
yy yyb;
yy yyv_Type;
yy yy_0_1;
yy yyv_Pos;
yy yy_0_2;
yy yy_0_3;
yy yy_1_1;
yy yy_1_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Type = yy_0_1;
yyv_Pos = yy_0_2;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_1_1 = ((yy)"Not defined as table index");
yy_1_2 = yyv_Pos;
MESSAGE(yy_1_1, yy_1_2);
yy_0_3 = yyb + 0;
yy_0_3[0] = 2;
*yyout_1 = yy_0_3;
return;
yyfl_342_2 : ;
}
yyErr(2,3738);
}
CheckField(yyin_1, yyin_2, yyin_3, yyin_4, yyout_1, yyout_2)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy yyin_4;
yy *yyout_1;
yy *yyout_2;
{
{
yy yyb;
yy yyv_I1;
yy yy_0_1;
yy yy_0_2;
yy yy_0_2_1;
yy yyv_I2;
yy yy_0_2_1_1;
yy yyv_Type;
yy yy_0_2_1_2;
yy yy_0_2_1_3;
yy yyv_Tail;
yy yy_0_2_2;
yy yyv_P;
yy yy_0_3;
yy yyv_N;
yy yy_0_4;
yy yy_0_5;
yy yy_0_6;
yy yy_1_1;
yy yy_1_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
yyv_I1 = yy_0_1;
if (yy_0_2[0] != 1) goto yyfl_343_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
if (yy_0_2_1[0] != 1) goto yyfl_343_1;
yy_0_2_1_1 = ((yy)yy_0_2_1[1]);
yy_0_2_1_2 = ((yy)yy_0_2_1[2]);
yy_0_2_1_3 = ((yy)yy_0_2_1[3]);
yyv_I2 = yy_0_2_1_1;
yyv_Type = yy_0_2_1_2;
yyv_Tail = yy_0_2_2;
yyv_P = yy_0_3;
yyv_N = yy_0_4;
yy_1_1 = yyv_I1;
yy_1_2 = yyv_I2;
if (! yyeq_IDENT(yy_1_1, yy_1_2)) goto yyfl_343_1;
yy_0_5 = yyv_Type;
yy_0_6 = yyv_N;
*yyout_1 = yy_0_5;
*yyout_2 = yy_0_6;
return;
yyfl_343_1 : ;
}
{
yy yyb;
yy yyv_I1;
yy yy_0_1;
yy yy_0_2;
yy yy_0_2_1;
yy yyv_I2;
yy yy_0_2_1_1;
yy yyv_Type2;
yy yy_0_2_1_2;
yy yy_0_2_1_3;
yy yyv_Tail;
yy yy_0_2_2;
yy yyv_P;
yy yy_0_3;
yy yyv_N;
yy yy_0_4;
yy yy_0_5;
yy yy_0_6;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yy_1_4;
yy yy_1_4_1;
yy yy_1_4_2;
yy yyv_Type;
yy yy_1_5;
yy yyv_N2;
yy yy_1_6;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
yyv_I1 = yy_0_1;
if (yy_0_2[0] != 1) goto yyfl_343_2;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
if (yy_0_2_1[0] != 1) goto yyfl_343_2;
yy_0_2_1_1 = ((yy)yy_0_2_1[1]);
yy_0_2_1_2 = ((yy)yy_0_2_1[2]);
yy_0_2_1_3 = ((yy)yy_0_2_1[3]);
yyv_I2 = yy_0_2_1_1;
yyv_Type2 = yy_0_2_1_2;
yyv_Tail = yy_0_2_2;
yyv_P = yy_0_3;
yyv_N = yy_0_4;
yy_1_1 = yyv_I1;
yy_1_2 = yyv_Tail;
yy_1_3 = yyv_P;
yy_1_4_1 = yyv_N;
yy_1_4_2 = ((yy)1);
yy_1_4 = (yy)(((long)yy_1_4_1)+((long)yy_1_4_2));
CheckField(yy_1_1, yy_1_2, yy_1_3, yy_1_4, &yy_1_5, &yy_1_6);
yyv_Type = yy_1_5;
yyv_N2 = yy_1_6;
yy_0_5 = yyv_Type;
yy_0_6 = yyv_N2;
*yyout_1 = yy_0_5;
*yyout_2 = yy_0_6;
return;
yyfl_343_2 : ;
}
{
yy yyb;
yy yyv_I;
yy yy_0_1;
yy yy_0_2;
yy yyv_P;
yy yy_0_3;
yy yyv_N;
yy yy_0_4;
yy yy_0_5;
yy yy_0_6;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yy_1_4;
yy yyv_Error;
yy yy_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
yyv_I = yy_0_1;
if (yy_0_2[0] != 2) goto yyfl_343_3;
yyv_P = yy_0_3;
yyv_N = yy_0_4;
yy_1_1 = ((yy)"invalid field name '");
yy_1_2 = yyv_I;
yy_1_3 = ((yy)"'");
yy_1_4 = yyv_P;
MESSAGE1(yy_1_1, yy_1_2, yy_1_3, yy_1_4);
yy_2 = yyglov_errorId;
if (yy_2 == (yy) yyu) yyErr(1,3754);
yyv_Error = yy_2;
yy_0_5 = yyv_Error;
yy_0_6 = yyv_N;
*yyout_1 = yy_0_5;
*yyout_2 = yy_0_6;
return;
yyfl_343_3 : ;
}
yyErr(2,3745);
}
CheckForUndefinedFields(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_T;
yy yy_0_1_3;
yy yy_1_1;
yy yyv_Tmp;
yy yy_1_2;
yy yy_2_1;
yy yyv_Line;
yy yy_2_2;
yy yy_3_1;
yy yy_4_1;
yy yy_5_1;
yy yy_6_1;
yy yy_7_1;
yy yy_9_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_344_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_H = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_T = yy_0_1_3;
yy_1_1 = yyv_H;
GetDefArgTmp(yy_1_1, &yy_1_2);
yyv_Tmp = yy_1_2;
yy_2_1 = yyv_P;
PosToLineNumber(yy_2_1, &yy_2_2);
yyv_Line = yy_2_2;
yy_3_1 = ((yy)"if (");
s(yy_3_1);
yy_4_1 = yyv_Tmp;
tmp(yy_4_1);
yy_5_1 = ((yy)" == (yy) yyu) yyErr(1,");
s(yy_5_1);
yy_6_1 = yyv_Line;
i(yy_6_1);
yy_7_1 = ((yy)");");
s(yy_7_1);
nl();
yy_9_1 = yyv_T;
CheckForUndefinedFields(yy_9_1);
return;
yyfl_344_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_344_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
return;
yyfl_344_2 : ;
}
yyErr(2,3758);
}
Match(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_345_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
return;
yyfl_345_1 : ;
}
{
yy yyb;
yy yyv_DL;
yy yy_0_1;
yy yyv_AL;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_DL = yy_0_1;
yyv_AL = yy_0_2;
yy_1_1 = yyv_DL;
yy_1_2 = yyv_AL;
MatchArgs(yy_1_1, yy_1_2);
return;
yyfl_345_2 : ;
}
yyErr(2,3773);
}
MatchArgs(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_T;
yy yy_0_1_3;
yy yy_0_2;
yy yyv_FH;
yy yy_0_2_1;
yy yyv_P2;
yy yy_0_2_2;
yy yyv_FT;
yy yy_0_2_3;
yy yy_1_1;
yy yy_1_2;
yy yy_2_1;
yy yy_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_346_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_H = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_T = yy_0_1_3;
if (yy_0_2[0] != 1) goto yyfl_346_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yy_0_2_3 = ((yy)yy_0_2[3]);
yyv_FH = yy_0_2_1;
yyv_P2 = yy_0_2_2;
yyv_FT = yy_0_2_3;
yy_1_1 = yyv_H;
yy_1_2 = yyv_FH;
MatchArg(yy_1_1, yy_1_2);
yy_2_1 = yyv_T;
yy_2_2 = yyv_FT;
MatchArgs(yy_2_1, yy_2_2);
return;
yyfl_346_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_P;
yy yy_0_1_1;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_346_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yyv_P = yy_0_1_1;
if (yy_0_2[0] != 2) goto yyfl_346_2;
return;
yyfl_346_2 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_P;
yy yy_0_1_1;
yy yyv_Formals;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_346_3;
yy_0_1_1 = ((yy)yy_0_1[1]);
yyv_P = yy_0_1_1;
yyv_Formals = yy_0_2;
yy_1_1 = ((yy)"Wrong number of arguments");
yy_1_2 = yyv_P;
MESSAGE(yy_1_1, yy_1_2);
return;
yyfl_346_3 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_T;
yy yy_0_1_3;
yy yyv_Formals;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_346_4;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_H = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_T = yy_0_1_3;
yyv_Formals = yy_0_2;
yy_1_1 = ((yy)"Wrong number of arguments");
yy_1_2 = yyv_P;
MESSAGE(yy_1_1, yy_1_2);
return;
yyfl_346_4 : ;
}
yyErr(2,3778);
}
MatchArg(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yyv_Term;
yy yy_0_1;
yy yy_0_2;
yy yyv_Name;
yy yy_0_2_1;
yy yyv_ExpType;
yy yy_0_2_2;
yy yy_0_2_3;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yyv_Tmp;
yy yy_1_1_1_2_1;
yy yyv_Functor;
yy yy_1_1_1_2_2;
yy yyv_P;
yy yy_1_1_1_2_3;
yy yyv_Args;
yy yy_1_1_1_2_4;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yyv_Tp;
yy yy_1_2_1_2_1;
yy yy_1_2_1_2_2;
yy yy_1_2_1_2_3;
yy yy_1_2_1_2_4;
yy yy_1_2_1_2_5;
yy yy_1_2_2_1_1_1;
yy yy_1_2_2_1_1_2;
yy yy_1_2_2_2_1_1;
yy yy_1_2_2_2_1_2;
yy yy_1_2_2_2_1_3;
yy yy_1_2_2_2_1_4;
yy yy_1_2_2_2_1_5;
yy yy_1_2_2_2_1_6;
yy yy_2_1;
yy yy_2_2;
yy yy_2_3;
yy yyv_Code;
yy yy_2_4;
yy yyv_ArgSpecs;
yy yy_2_5;
yy yy_3_1;
yy yy_4_1;
yy yy_5_1;
yy yy_6_1;
yy yy_7_1;
yy yy_10_1;
yy yy_10_2;
yy yy_10_3;
yy yy_11_1;
yy yy_11_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Term = yy_0_1;
if (yy_0_2[0] != 1) goto yyfl_348_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yy_0_2_3 = ((yy)yy_0_2[3]);
yyv_Name = yy_0_2_1;
yyv_ExpType = yy_0_2_2;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Term;
yy_1_1_1_2 = yy_1_1_1_1;
if (yy_1_1_1_2[0] != 1) goto yyfl_348_1_1_1;
yy_1_1_1_2_1 = ((yy)yy_1_1_1_2[1]);
yy_1_1_1_2_2 = ((yy)yy_1_1_1_2[2]);
yy_1_1_1_2_3 = ((yy)yy_1_1_1_2[3]);
yy_1_1_1_2_4 = ((yy)yy_1_1_1_2[4]);
yyv_Tmp = yy_1_1_1_2_1;
yyv_Functor = yy_1_1_1_2_2;
yyv_P = yy_1_1_1_2_3;
yyv_Args = yy_1_1_1_2_4;
goto yysl_348_1_1;
yyfl_348_1_1_1 : ;
yy_1_2_1_1 = yyv_Term;
yy_1_2_1_2 = yy_1_2_1_1;
if (yy_1_2_1_2[0] != 2) goto yyfl_348_1_1_2;
yy_1_2_1_2_1 = ((yy)yy_1_2_1_2[1]);
yy_1_2_1_2_2 = ((yy)yy_1_2_1_2[2]);
yy_1_2_1_2_3 = ((yy)yy_1_2_1_2[3]);
yy_1_2_1_2_4 = ((yy)yy_1_2_1_2[4]);
yy_1_2_1_2_5 = ((yy)yy_1_2_1_2[5]);
yyv_Tp = yy_1_2_1_2_1;
yyv_Tmp = yy_1_2_1_2_2;
yyv_Functor = yy_1_2_1_2_3;
yyv_P = yy_1_2_1_2_4;
yyv_Args = yy_1_2_1_2_5;
{
yy yysb = yyb;
yy_1_2_2_1_1_1 = yyv_Tp;
yy_1_2_2_1_1_2 = yyv_ExpType;
if (! yyeq_IDENT(yy_1_2_2_1_1_1, yy_1_2_2_1_1_2)) goto yyfl_348_1_1_2_2_1;
goto yysl_348_1_1_2_2;
yyfl_348_1_1_2_2_1 : ;
yy_1_2_2_2_1_1 = ((yy)"'");
yy_1_2_2_2_1_2 = yyv_Tp;
yy_1_2_2_2_1_3 = ((yy)"', where '");
yy_1_2_2_2_1_4 = yyv_ExpType;
yy_1_2_2_2_1_5 = ((yy)"' is expected");
yy_1_2_2_2_1_6 = yyv_P;
MESSAGE2(yy_1_2_2_2_1_1, yy_1_2_2_2_1_2, yy_1_2_2_2_1_3, yy_1_2_2_2_1_4, yy_1_2_2_2_1_5, yy_1_2_2_2_1_6);
goto yysl_348_1_1_2_2;
yysl_348_1_1_2_2 : ;
yyb = yysb;
}
goto yysl_348_1_1;
yyfl_348_1_1_2 : ;
goto yyfl_348_1;
yysl_348_1_1 : ;
yyb = yysb;
}
yy_2_1 = yyv_Functor;
yy_2_2 = yyv_ExpType;
yy_2_3 = yyv_P;
ApplyFunctor(yy_2_1, yy_2_2, yy_2_3, &yy_2_4, &yy_2_5);
yyv_Code = yy_2_4;
yyv_ArgSpecs = yy_2_5;
yy_3_1 = ((yy)"if (");
s(yy_3_1);
yy_4_1 = yyv_Tmp;
tmp(yy_4_1);
yy_5_1 = ((yy)"[0] != ");
s(yy_5_1);
yy_6_1 = yyv_Code;
i(yy_6_1);
yy_7_1 = ((yy)") ");
s(yy_7_1);
Fail();
nl();
yy_10_1 = yyv_Args;
yy_10_2 = yyv_Tmp;
yy_10_3 = ((yy)1);
AssignDefArgs(yy_10_1, yy_10_2, yy_10_3);
yy_11_1 = yyv_Args;
yy_11_2 = yyv_ArgSpecs;
Match(yy_11_1, yy_11_2);
return;
yyfl_348_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_Name;
yy yy_0_1_2;
yy yyv_P;
yy yy_0_1_3;
yy yy_0_2;
yy yyv_Name2;
yy yy_0_2_1;
yy yyv_ExpType;
yy yy_0_2_2;
yy yy_0_2_3;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yy_1_3_1;
yy yy_1_4;
yy yy_2_1;
yy yy_3_1;
yy yy_4_1;
yy yy_5_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 3) goto yyfl_348_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Tmp = yy_0_1_1;
yyv_Name = yy_0_1_2;
yyv_P = yy_0_1_3;
if (yy_0_2[0] != 1) goto yyfl_348_2;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yy_0_2_3 = ((yy)yy_0_2[3]);
yyv_Name2 = yy_0_2_1;
yyv_ExpType = yy_0_2_2;
yy_1_1 = yyv_Name;
yy_1_2 = yyv_ExpType;
yy_1_3_1 = ((yy)1);
yy_1_3 = (yy)(-((long)yy_1_3_1));
yy_1_4 = yyv_P;
DefineLocalVar(yy_1_1, yy_1_2, yy_1_3, yy_1_4);
yy_2_1 = yyv_Name;
varid(yy_2_1);
yy_3_1 = ((yy)" = ");
s(yy_3_1);
yy_4_1 = yyv_Tmp;
tmp(yy_4_1);
yy_5_1 = ((yy)";");
s(yy_5_1);
nl();
return;
yyfl_348_2 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_Name;
yy yy_0_1_2;
yy yyv_P;
yy yy_0_1_3;
yy yyv_Arg;
yy yy_0_1_4;
yy yy_0_2;
yy yyv_Name2;
yy yy_0_2_1;
yy yyv_ExpType;
yy yy_0_2_2;
yy yyv_Access;
yy yy_0_2_3;
yy yy_1_1_1_1_1_1;
yy yy_1_1_1_1_1_2;
yy yy_1_1_1_1_1_2_1;
yy yyv_Functor;
yy yy_1_1_1_1_1_2_2;
yy yyv_FunctorPos;
yy yy_1_1_1_1_1_2_3;
yy yy_1_1_1_1_1_2_4;
yy yy_1_1_1_1_2_1;
yy yy_1_1_1_1_2_2;
yy yyv_FunctorCode;
yy yy_1_1_1_1_2_3;
yy yy_1_1_1_1_2_4;
yy yy_1_1_1_1_3_1;
yy yy_1_1_1_1_3_2;
yy yy_1_1_1_1_3_3;
yy yy_1_1_1_1_3_4;
yy yy_1_1_1_2_1_1;
yy yy_1_1_1_2_1_2;
yy yyv_IDENT;
yy yy_1_1_1_2_1_2_1;
yy yy_1_1_1_2_1_2_2;
yy yy_1_1_1_2_1_2_3;
yy yy_1_1_1_2_1_2_4;
yy yy_1_1_1_2_1_2_5;
yy yy_1_1_1_2_2_1;
yy yy_1_1_1_2_2_2;
yy yy_1_1_1_2_2_3;
yy yy_1_1_1_2_2_4;
yy yy_1_1_1_2_3_1;
yy yy_1_1_1_2_3_2;
yy yy_1_1_1_2_3_3;
yy yy_1_1_1_2_3_4;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_1_2_1_3;
yy yy_1_2_1_3_1;
yy yy_1_2_1_4;
yy yy_2_1;
yy yyv_ArgTmp;
yy yy_2_2;
yy yy_3_1;
yy yy_4_1;
yy yy_5_1;
yy yy_6_1;
yy yy_8_1;
yy yy_9_1;
yy yy_10_1;
yy yy_11_1;
yy yy_13_1;
yy yy_13_2;
yy yy_13_2_1;
yy yy_13_2_2;
yy yy_13_2_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 4) goto yyfl_348_3;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yyv_Tmp = yy_0_1_1;
yyv_Name = yy_0_1_2;
yyv_P = yy_0_1_3;
yyv_Arg = yy_0_1_4;
if (yy_0_2[0] != 1) goto yyfl_348_3;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yy_0_2_3 = ((yy)yy_0_2[3]);
yyv_Name2 = yy_0_2_1;
yyv_ExpType = yy_0_2_2;
yyv_Access = yy_0_2_3;
yyb = yyh;
yyh += 5; if (yyh > yyhx) yyExtend();
{
yy yysb = yyb;
{
yy yysb = yyb;
yy_1_1_1_1_1_1 = yyv_Arg;
yy_1_1_1_1_1_2 = yy_1_1_1_1_1_1;
if (yy_1_1_1_1_1_2[0] != 1) goto yyfl_348_3_1_1_1_1;
yy_1_1_1_1_1_2_1 = ((yy)yy_1_1_1_1_1_2[1]);
yy_1_1_1_1_1_2_2 = ((yy)yy_1_1_1_1_1_2[2]);
yy_1_1_1_1_1_2_3 = ((yy)yy_1_1_1_1_1_2[3]);
yy_1_1_1_1_1_2_4 = ((yy)yy_1_1_1_1_1_2[4]);
yyv_Functor = yy_1_1_1_1_1_2_2;
yyv_FunctorPos = yy_1_1_1_1_1_2_3;
yy_1_1_1_1_2_1 = yyv_Functor;
yy_1_1_1_1_2_2 = yyv_ExpType;
GetFunctorCode(yy_1_1_1_1_2_1, yy_1_1_1_1_2_2, &yy_1_1_1_1_2_3, &yy_1_1_1_1_2_4);
yyv_FunctorCode = yy_1_1_1_1_2_3;
yy_1_1_1_1_3_1 = yyv_Name;
yy_1_1_1_1_3_2 = yyv_ExpType;
yy_1_1_1_1_3_3 = yyv_FunctorCode;
yy_1_1_1_1_3_4 = yyv_FunctorPos;
DefineLocalVar(yy_1_1_1_1_3_1, yy_1_1_1_1_3_2, yy_1_1_1_1_3_3, yy_1_1_1_1_3_4);
goto yysl_348_3_1_1_1;
yyfl_348_3_1_1_1_1 : ;
yy_1_1_1_2_1_1 = yyv_Arg;
yy_1_1_1_2_1_2 = yy_1_1_1_2_1_1;
if (yy_1_1_1_2_1_2[0] != 2) goto yyfl_348_3_1_1_1_2;
yy_1_1_1_2_1_2_1 = ((yy)yy_1_1_1_2_1_2[1]);
yy_1_1_1_2_1_2_2 = ((yy)yy_1_1_1_2_1_2[2]);
yy_1_1_1_2_1_2_3 = ((yy)yy_1_1_1_2_1_2[3]);
yy_1_1_1_2_1_2_4 = ((yy)yy_1_1_1_2_1_2[4]);
yy_1_1_1_2_1_2_5 = ((yy)yy_1_1_1_2_1_2[5]);
yyv_IDENT = yy_1_1_1_2_1_2_1;
yyv_Functor = yy_1_1_1_2_1_2_3;
yyv_FunctorPos = yy_1_1_1_2_1_2_4;
yy_1_1_1_2_2_1 = yyv_Functor;
yy_1_1_1_2_2_2 = yyv_ExpType;
GetFunctorCode(yy_1_1_1_2_2_1, yy_1_1_1_2_2_2, &yy_1_1_1_2_2_3, &yy_1_1_1_2_2_4);
yyv_FunctorCode = yy_1_1_1_2_2_3;
yy_1_1_1_2_3_1 = yyv_Name;
yy_1_1_1_2_3_2 = yyv_ExpType;
yy_1_1_1_2_3_3 = yyv_FunctorCode;
yy_1_1_1_2_3_4 = yyv_FunctorPos;
DefineLocalVar(yy_1_1_1_2_3_1, yy_1_1_1_2_3_2, yy_1_1_1_2_3_3, yy_1_1_1_2_3_4);
goto yysl_348_3_1_1_1;
yyfl_348_3_1_1_1_2 : ;
goto yyfl_348_3_1_1;
yysl_348_3_1_1_1 : ;
yyb = yysb;
}
goto yysl_348_3_1;
yyfl_348_3_1_1 : ;
yy_1_2_1_1 = yyv_Name;
yy_1_2_1_2 = yyv_ExpType;
yy_1_2_1_3_1 = ((yy)1);
yy_1_2_1_3 = (yy)(-((long)yy_1_2_1_3_1));
yy_1_2_1_4 = yyv_P;
DefineLocalVar(yy_1_2_1_1, yy_1_2_1_2, yy_1_2_1_3, yy_1_2_1_4);
goto yysl_348_3_1;
yysl_348_3_1 : ;
yyb = yysb;
}
yy_2_1 = yyv_Arg;
GetDefArgTmp(yy_2_1, &yy_2_2);
yyv_ArgTmp = yy_2_2;
yy_3_1 = yyv_ArgTmp;
tmp(yy_3_1);
yy_4_1 = ((yy)" = ");
s(yy_4_1);
yy_5_1 = yyv_Tmp;
tmp(yy_5_1);
yy_6_1 = ((yy)";");
s(yy_6_1);
nl();
yy_8_1 = yyv_Name;
varid(yy_8_1);
yy_9_1 = ((yy)" = ");
s(yy_9_1);
yy_10_1 = yyv_Tmp;
tmp(yy_10_1);
yy_11_1 = ((yy)";");
s(yy_11_1);
nl();
yy_13_1 = yyv_Arg;
yy_13_2_1 = yyb + 4;
yy_13_2_1[0] = 2;
yy_13_2_2 = yyv_ExpType;
yy_13_2_3 = yyv_Access;
yy_13_2 = yyb + 0;
yy_13_2[0] = 1;
yy_13_2[1] = ((long)yy_13_2_1);
yy_13_2[2] = ((long)yy_13_2_2);
yy_13_2[3] = ((long)yy_13_2_3);
MatchArg(yy_13_1, yy_13_2);
return;
yyfl_348_3 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yy_0_2;
yy yyv_Name;
yy yy_0_2_1;
yy yyv_ExpType;
yy yy_0_2_2;
yy yyv_Access;
yy yy_0_2_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 5) goto yyfl_348_4;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_Tmp = yy_0_1_1;
yyv_P = yy_0_1_2;
if (yy_0_2[0] != 1) goto yyfl_348_4;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yy_0_2_3 = ((yy)yy_0_2[3]);
yyv_Name = yy_0_2_1;
yyv_ExpType = yy_0_2_2;
yyv_Access = yy_0_2_3;
return;
yyfl_348_4 : ;
}
yyErr(2,3796);
}
FieldsToArgSpecs(yyin_1, yyin_2, yyout_1)
yy yyin_1;
yy yyin_2;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_Name;
yy yy_0_1_1_1;
yy yyv_Type;
yy yy_0_1_1_2;
yy yy_0_1_1_3;
yy yyv_T;
yy yy_0_1_2;
yy yyv_P;
yy yy_0_2;
yy yy_0_3;
yy yy_0_3_1;
yy yy_0_3_1_1;
yy yy_0_3_1_1_1;
yy yy_0_3_1_2;
yy yy_0_3_1_3;
yy yy_0_3_2;
yy yy_0_3_3;
yy yy_1_1;
yy yy_1_2;
yy yyv_T2;
yy yy_1_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_349_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
if (yy_0_1_1[0] != 1) goto yyfl_349_1;
yy_0_1_1_1 = ((yy)yy_0_1_1[1]);
yy_0_1_1_2 = ((yy)yy_0_1_1[2]);
yy_0_1_1_3 = ((yy)yy_0_1_1[3]);
yyv_Name = yy_0_1_1_1;
yyv_Type = yy_0_1_1_2;
yyv_T = yy_0_1_2;
yyv_P = yy_0_2;
yyb = yyh;
yyh += 11; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_T;
yy_1_2 = yyv_P;
FieldsToArgSpecs(yy_1_1, yy_1_2, &yy_1_3);
yyv_T2 = yy_1_3;
yy_0_3_1_1_1 = yyv_Name;
yy_0_3_1_1 = yyb + 8;
yy_0_3_1_1[0] = 1;
yy_0_3_1_1[1] = ((long)yy_0_3_1_1_1);
yy_0_3_1_2 = yyv_Type;
yy_0_3_1_3 = yyb + 10;
yy_0_3_1_3[0] = 1;
yy_0_3_1 = yyb + 4;
yy_0_3_1[0] = 1;
yy_0_3_1[1] = ((long)yy_0_3_1_1);
yy_0_3_1[2] = ((long)yy_0_3_1_2);
yy_0_3_1[3] = ((long)yy_0_3_1_3);
yy_0_3_2 = yyv_P;
yy_0_3_3 = yyv_T2;
yy_0_3 = yyb + 0;
yy_0_3[0] = 1;
yy_0_3[1] = ((long)yy_0_3_1);
yy_0_3[2] = ((long)yy_0_3_2);
yy_0_3[3] = ((long)yy_0_3_3);
*yyout_1 = yy_0_3;
return;
yyfl_349_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_P;
yy yy_0_2;
yy yy_0_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_349_2;
yyv_P = yy_0_2;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_0_3 = yyb + 0;
yy_0_3[0] = 2;
*yyout_1 = yy_0_3;
return;
yyfl_349_2 : ;
}
yyErr(2,3840);
}
AssignDefArgs(yyin_1, yyin_2, yyin_3)
yy yyin_1;
yy yyin_2;
yy yyin_3;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_T;
yy yy_0_1_3;
yy yyv_Tmp;
yy yy_0_2;
yy yyv_N;
yy yy_0_3;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yy_2_1;
yy yy_2_2;
yy yy_2_3;
yy yy_2_3_1;
yy yy_2_3_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 1) goto yyfl_350_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_H = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_T = yy_0_1_3;
yyv_Tmp = yy_0_2;
yyv_N = yy_0_3;
yy_1_1 = yyv_H;
yy_1_2 = yyv_Tmp;
yy_1_3 = yyv_N;
AssignDefArg(yy_1_1, yy_1_2, yy_1_3);
yy_2_1 = yyv_T;
yy_2_2 = yyv_Tmp;
yy_2_3_1 = yyv_N;
yy_2_3_2 = ((yy)1);
yy_2_3 = (yy)(((long)yy_2_3_1)+((long)yy_2_3_2));
AssignDefArgs(yy_2_1, yy_2_2, yy_2_3);
return;
yyfl_350_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_P;
yy yy_0_1_1;
yy yyv_Tmp;
yy yy_0_2;
yy yyv_N;
yy yy_0_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 2) goto yyfl_350_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yyv_P = yy_0_1_1;
yyv_Tmp = yy_0_2;
yyv_N = yy_0_3;
return;
yyfl_350_2 : ;
}
yyErr(2,3849);
}
AssignDefArg(yyin_1, yyin_2, yyin_3)
yy yyin_1;
yy yyin_2;
yy yyin_3;
{
{
yy yyb;
yy yyv_Arg;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_2;
yy yyv_N;
yy yy_0_3;
yy yy_1_1;
yy yyv_ArgTmp;
yy yy_1_2;
yy yy_2_1;
yy yy_3_1;
yy yy_4_1;
yy yy_5_1;
yy yy_6_1;
yy yy_7_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Arg = yy_0_1;
yyv_Tmp = yy_0_2;
yyv_N = yy_0_3;
yy_1_1 = yyv_Arg;
GetDefArgTmp(yy_1_1, &yy_1_2);
yyv_ArgTmp = yy_1_2;
yy_2_1 = yyv_ArgTmp;
tmp(yy_2_1);
yy_3_1 = ((yy)" = ((yy)");
s(yy_3_1);
yy_4_1 = yyv_Tmp;
tmp(yy_4_1);
yy_5_1 = ((yy)"[");
s(yy_5_1);
yy_6_1 = yyv_N;
i(yy_6_1);
yy_7_1 = ((yy)"]);");
s(yy_7_1);
nl();
return;
}
}
ApplyFunctor(yyin_1, yyin_2, yyin_3, yyout_1, yyout_2)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy *yyout_1;
yy *yyout_2;
{
{
yy yyb;
yy yyv_Functor;
yy yy_0_1;
yy yyv_Type;
yy yy_0_2;
yy yyv_P;
yy yy_0_3;
yy yy_0_4;
yy yy_0_5;
yy yy_1_1;
yy yyv_FunctorSpecs;
yy yy_1_2;
yy yy_2_1;
yy yy_2_2;
yy yy_2_3;
yy yyv_Code;
yy yy_2_4;
yy yyv_Args;
yy yy_2_5;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Functor = yy_0_1;
yyv_Type = yy_0_2;
yyv_P = yy_0_3;
yy_1_1 = yyv_Type;
if (! IsNamedType(yy_1_1, &yy_1_2)) goto yyfl_352_1;
yyv_FunctorSpecs = yy_1_2;
yy_2_1 = yyv_Functor;
yy_2_2 = yyv_FunctorSpecs;
yy_2_3 = ((yy)1);
if (! LookupFunctor(yy_2_1, yy_2_2, yy_2_3, &yy_2_4, &yy_2_5)) goto yyfl_352_1;
yyv_Code = yy_2_4;
yyv_Args = yy_2_5;
yy_0_4 = yyv_Code;
yy_0_5 = yyv_Args;
*yyout_1 = yy_0_4;
*yyout_2 = yy_0_5;
return;
yyfl_352_1 : ;
}
{
yy yyb;
yy yyv_Functor;
yy yy_0_1;
yy yyv_Type;
yy yy_0_2;
yy yyv_P;
yy yy_0_3;
yy yy_0_4;
yy yy_0_5;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yy_1_4;
yy yy_1_5;
yy yy_1_6;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Functor = yy_0_1;
yyv_Type = yy_0_2;
yyv_P = yy_0_3;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_1_1 = ((yy)"'");
yy_1_2 = yyv_Functor;
yy_1_3 = ((yy)"' not defined as functor for '");
yy_1_4 = yyv_Type;
yy_1_5 = ((yy)"'");
yy_1_6 = yyv_P;
MESSAGE2(yy_1_1, yy_1_2, yy_1_3, yy_1_4, yy_1_5, yy_1_6);
yy_0_4 = ((yy)0);
yy_0_5 = yyb + 0;
yy_0_5[0] = 2;
*yyout_1 = yy_0_4;
*yyout_2 = yy_0_5;
return;
yyfl_352_2 : ;
}
yyErr(2,3864);
}
GetFunctorCode(yyin_1, yyin_2, yyout_1, yyout_2)
yy yyin_1;
yy yyin_2;
yy *yyout_1;
yy *yyout_2;
{
{
yy yyb;
yy yyv_Functor;
yy yy_0_1;
yy yyv_Type;
yy yy_0_2;
yy yy_0_3;
yy yy_0_4;
yy yy_1_1_1_1;
yy yyv_FunctorSpecs;
yy yy_1_1_1_2;
yy yy_1_1_2_1;
yy yy_1_1_2_2;
yy yy_1_1_2_3;
yy yyv_Code;
yy yy_1_1_2_4;
yy yyv_Args;
yy yy_1_1_2_5;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_1_2_2_1;
yy yy_1_2_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Functor = yy_0_1;
yyv_Type = yy_0_2;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Type;
if (! IsNamedType(yy_1_1_1_1, &yy_1_1_1_2)) goto yyfl_353_1_1_1;
yyv_FunctorSpecs = yy_1_1_1_2;
yy_1_1_2_1 = yyv_Functor;
yy_1_1_2_2 = yyv_FunctorSpecs;
yy_1_1_2_3 = ((yy)1);
if (! LookupFunctor(yy_1_1_2_1, yy_1_1_2_2, yy_1_1_2_3, &yy_1_1_2_4, &yy_1_1_2_5)) goto yyfl_353_1_1_1;
yyv_Code = yy_1_1_2_4;
yyv_Args = yy_1_1_2_5;
goto yysl_353_1_1;
yyfl_353_1_1_1 : ;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_1_2_1_1 = ((yy)0);
yy_1_2_1_2 = yy_1_2_1_1;
yyv_Code = yy_1_2_1_2;
yy_1_2_2_1 = yyb + 0;
yy_1_2_2_1[0] = 2;
yy_1_2_2_2 = yy_1_2_2_1;
yyv_Args = yy_1_2_2_2;
goto yysl_353_1_1;
yysl_353_1_1 : ;
yyb = yysb;
}
yy_0_3 = yyv_Code;
yy_0_4 = yyv_Args;
*yyout_1 = yy_0_3;
*yyout_2 = yy_0_4;
return;
}
}
int LookupFunctor(yyin_1, yyin_2, yyin_3, yyout_1, yyout_2)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy *yyout_1;
yy *yyout_2;
{
{
yy yyb;
yy yyv_Functor;
yy yy_0_1;
yy yy_0_2;
yy yy_0_2_1;
yy yyv_F;
yy yy_0_2_1_1;
yy yyv_P;
yy yy_0_2_1_2;
yy yyv_Args;
yy yy_0_2_1_3;
yy yyv_Tail;
yy yy_0_2_2;
yy yyv_FirstCode;
yy yy_0_3;
yy yy_0_4;
yy yy_0_5;
yy yy_1_1;
yy yy_1_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Functor = yy_0_1;
if (yy_0_2[0] != 1) goto yyfl_354_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
if (yy_0_2_1[0] != 1) goto yyfl_354_1;
yy_0_2_1_1 = ((yy)yy_0_2_1[1]);
yy_0_2_1_2 = ((yy)yy_0_2_1[2]);
yy_0_2_1_3 = ((yy)yy_0_2_1[3]);
yyv_F = yy_0_2_1_1;
yyv_P = yy_0_2_1_2;
yyv_Args = yy_0_2_1_3;
yyv_Tail = yy_0_2_2;
yyv_FirstCode = yy_0_3;
yy_1_1 = yyv_Functor;
yy_1_2 = yyv_F;
if (! yyeq_IDENT(yy_1_1, yy_1_2)) goto yyfl_354_1;
yy_0_4 = yyv_FirstCode;
yy_0_5 = yyv_Args;
*yyout_1 = yy_0_4;
*yyout_2 = yy_0_5;
return 1;
yyfl_354_1 : ;
}
{
yy yyb;
yy yyv_Functor;
yy yy_0_1;
yy yy_0_2;
yy yyv_Head;
yy yy_0_2_1;
yy yyv_Tail;
yy yy_0_2_2;
yy yyv_FirstCode;
yy yy_0_3;
yy yy_0_4;
yy yy_0_5;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yy_1_3_1;
yy yy_1_3_2;
yy yyv_Code;
yy yy_1_4;
yy yyv_Args;
yy yy_1_5;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Functor = yy_0_1;
if (yy_0_2[0] != 1) goto yyfl_354_2;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yyv_Head = yy_0_2_1;
yyv_Tail = yy_0_2_2;
yyv_FirstCode = yy_0_3;
yy_1_1 = yyv_Functor;
yy_1_2 = yyv_Tail;
yy_1_3_1 = yyv_FirstCode;
yy_1_3_2 = ((yy)1);
yy_1_3 = (yy)(((long)yy_1_3_1)+((long)yy_1_3_2));
if (! LookupFunctor(yy_1_1, yy_1_2, yy_1_3, &yy_1_4, &yy_1_5)) goto yyfl_354_2;
yyv_Code = yy_1_4;
yyv_Args = yy_1_5;
yy_0_4 = yyv_Code;
yy_0_5 = yyv_Args;
*yyout_1 = yy_0_4;
*yyout_2 = yy_0_5;
return 1;
yyfl_354_2 : ;
}
return 0;
}
int IsNamedType(yyin_1, yyout_1)
yy yyin_1;
yy *yyout_1;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy yy_1_2_1;
yy yyv_Args;
yy yy_1_2_2;
yy_0_1 = yyin_1;
yyv_Id = yy_0_1;
yy_1_1 = yyv_Id;
if (! GetGlobalMeaning(yy_1_1, &yy_1_2)) goto yyfl_355_1;
if (yy_1_2[0] != 1) goto yyfl_355_1;
yy_1_2_1 = ((yy)yy_1_2[1]);
yy_1_2_2 = ((yy)yy_1_2[2]);
yyv_Args = yy_1_2_2;
yy_0_2 = yyv_Args;
*yyout_1 = yy_0_2;
return 1;
yyfl_355_1 : ;
}
return 0;
}
Build(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_T;
yy yy_0_1_3;
yy yy_0_2;
yy yyv_FH;
yy yy_0_2_1;
yy yyv_P2;
yy yy_0_2_2;
yy yyv_FT;
yy yy_0_2_3;
yy yy_1_1;
yy yy_1_2;
yy yy_2_1;
yy yy_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_356_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_H = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_T = yy_0_1_3;
if (yy_0_2[0] != 1) goto yyfl_356_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yy_0_2_3 = ((yy)yy_0_2[3]);
yyv_FH = yy_0_2_1;
yyv_P2 = yy_0_2_2;
yyv_FT = yy_0_2_3;
yy_1_1 = yyv_H;
yy_1_2 = yyv_FH;
BuildArg(yy_1_1, yy_1_2);
yy_2_1 = yyv_T;
yy_2_2 = yyv_FT;
Build(yy_2_1, yy_2_2);
return;
yyfl_356_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_P;
yy yy_0_1_1;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_356_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yyv_P = yy_0_1_1;
if (yy_0_2[0] != 2) goto yyfl_356_2;
return;
yyfl_356_2 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_P;
yy yy_0_1_1;
yy yyv_ArgSpecList;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_356_3;
yy_0_1_1 = ((yy)yy_0_1[1]);
yyv_P = yy_0_1_1;
yyv_ArgSpecList = yy_0_2;
yy_1_1 = ((yy)"Wrong number of arguments");
yy_1_2 = yyv_P;
MESSAGE(yy_1_1, yy_1_2);
return;
yyfl_356_3 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_T;
yy yy_0_1_3;
yy yyv_ArgSpecList;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_356_4;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_H = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_T = yy_0_1_3;
yyv_ArgSpecList = yy_0_2;
yy_1_1 = ((yy)"Wrong number of arguments");
yy_1_2 = yyv_P;
MESSAGE(yy_1_1, yy_1_2);
return;
yyfl_356_4 : ;
}
yyErr(2,3901);
}
BuildArg(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_Op;
yy yy_0_1_2;
yy yyv_P;
yy yy_0_1_3;
yy yyv_Left;
yy yy_0_1_4;
yy yyv_Right;
yy yy_0_1_5;
yy yy_0_2;
yy yyv_Name;
yy yy_0_2_1;
yy yyv_Expected;
yy yy_0_2_2;
yy yy_0_2_3;
yy yy_1_1;
yy yy_1_2;
yy yyv_T_ID_INT;
yy yy_2;
yy yy_3_1;
yy yy_3_2;
yy yy_3_2_1;
yy yy_3_2_2;
yy yy_3_2_3;
yy yy_4_1;
yy yy_4_2;
yy yy_4_2_1;
yy yy_4_2_2;
yy yy_4_2_3;
yy yy_5_1;
yy yyv_L;
yy yy_5_2;
yy yy_6_1;
yy yyv_R;
yy yy_6_2;
yy yy_7_1;
yy yy_8_1;
yy yy_9_1;
yy yy_10_1;
yy yy_11_1;
yy yy_12_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_357_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yy_0_1_5 = ((yy)yy_0_1[5]);
yyv_Tmp = yy_0_1_1;
yyv_Op = yy_0_1_2;
yyv_P = yy_0_1_3;
yyv_Left = yy_0_1_4;
yyv_Right = yy_0_1_5;
if (yy_0_2[0] != 1) goto yyfl_357_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yy_0_2_3 = ((yy)yy_0_2[3]);
yyv_Name = yy_0_2_1;
yyv_Expected = yy_0_2_2;
yyb = yyh;
yyh += 12; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_Expected;
yy_1_2 = yyv_P;
CheckInt(yy_1_1, yy_1_2);
yy_2 = yyglov_id_INT;
if (yy_2 == (yy) yyu) yyErr(1,3917);
yyv_T_ID_INT = yy_2;
yy_3_1 = yyv_Left;
yy_3_2_1 = yyb + 4;
yy_3_2_1[0] = 2;
yy_3_2_2 = yyv_T_ID_INT;
yy_3_2_3 = yyb + 5;
yy_3_2_3[0] = 2;
yy_3_2 = yyb + 0;
yy_3_2[0] = 1;
yy_3_2[1] = ((long)yy_3_2_1);
yy_3_2[2] = ((long)yy_3_2_2);
yy_3_2[3] = ((long)yy_3_2_3);
BuildArg(yy_3_1, yy_3_2);
yy_4_1 = yyv_Right;
yy_4_2_1 = yyb + 10;
yy_4_2_1[0] = 2;
yy_4_2_2 = yyv_T_ID_INT;
yy_4_2_3 = yyb + 11;
yy_4_2_3[0] = 2;
yy_4_2 = yyb + 6;
yy_4_2[0] = 1;
yy_4_2[1] = ((long)yy_4_2_1);
yy_4_2[2] = ((long)yy_4_2_2);
yy_4_2[3] = ((long)yy_4_2_3);
BuildArg(yy_4_1, yy_4_2);
yy_5_1 = yyv_Left;
GetUseArgTmp(yy_5_1, &yy_5_2);
yyv_L = yy_5_2;
yy_6_1 = yyv_Right;
GetUseArgTmp(yy_6_1, &yy_6_2);
yyv_R = yy_6_2;
yy_7_1 = yyv_Tmp;
tmp(yy_7_1);
yy_8_1 = ((yy)" = (yy)(");
s(yy_8_1);
yy_9_1 = yyv_L;
tmp_i(yy_9_1);
yy_10_1 = yyv_Op;
binop(yy_10_1);
yy_11_1 = yyv_R;
tmp_i(yy_11_1);
yy_12_1 = ((yy)");");
s(yy_12_1);
nl();
return;
yyfl_357_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_Op;
yy yy_0_1_2;
yy yyv_P;
yy yy_0_1_3;
yy yyv_Arg;
yy yy_0_1_4;
yy yy_0_2;
yy yyv_Name;
yy yy_0_2_1;
yy yyv_Expected;
yy yy_0_2_2;
yy yy_0_2_3;
yy yy_1_1;
yy yy_1_2;
yy yyv_T_ID_INT;
yy yy_2;
yy yy_3_1;
yy yy_3_2;
yy yy_3_2_1;
yy yy_3_2_2;
yy yy_3_2_3;
yy yy_4_1;
yy yyv_X;
yy yy_4_2;
yy yy_5_1;
yy yy_6_1;
yy yy_7_1;
yy yy_8_1;
yy yy_9_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_357_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yyv_Tmp = yy_0_1_1;
yyv_Op = yy_0_1_2;
yyv_P = yy_0_1_3;
yyv_Arg = yy_0_1_4;
if (yy_0_2[0] != 1) goto yyfl_357_2;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yy_0_2_3 = ((yy)yy_0_2[3]);
yyv_Name = yy_0_2_1;
yyv_Expected = yy_0_2_2;
yyb = yyh;
yyh += 6; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_Expected;
yy_1_2 = yyv_P;
CheckInt(yy_1_1, yy_1_2);
yy_2 = yyglov_id_INT;
if (yy_2 == (yy) yyu) yyErr(1,3926);
yyv_T_ID_INT = yy_2;
yy_3_1 = yyv_Arg;
yy_3_2_1 = yyb + 4;
yy_3_2_1[0] = 2;
yy_3_2_2 = yyv_T_ID_INT;
yy_3_2_3 = yyb + 5;
yy_3_2_3[0] = 2;
yy_3_2 = yyb + 0;
yy_3_2[0] = 1;
yy_3_2[1] = ((long)yy_3_2_1);
yy_3_2[2] = ((long)yy_3_2_2);
yy_3_2[3] = ((long)yy_3_2_3);
BuildArg(yy_3_1, yy_3_2);
yy_4_1 = yyv_Arg;
GetUseArgTmp(yy_4_1, &yy_4_2);
yyv_X = yy_4_2;
yy_5_1 = yyv_Tmp;
tmp(yy_5_1);
yy_6_1 = ((yy)" = (yy)(");
s(yy_6_1);
yy_7_1 = yyv_Op;
monop(yy_7_1);
yy_8_1 = yyv_X;
tmp_i(yy_8_1);
yy_9_1 = ((yy)");");
s(yy_9_1);
nl();
return;
yyfl_357_2 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Type;
yy yy_0_1_1;
yy yyv_Term;
yy yy_0_1_2;
yy yyv_P;
yy yy_0_1_3;
yy yyv_Spec;
yy yy_0_2;
yy yy_0_2_1;
yy yyv_Name;
yy yy_0_2_1_1;
yy yyv_ExpType;
yy yy_0_2_1_2;
yy yy_0_2_1_3;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_1_2_1_3;
yy yy_1_2_1_4;
yy yy_1_2_1_5;
yy yy_1_2_1_6;
yy yy_2_1;
yy yy_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 4) goto yyfl_357_3;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Type = yy_0_1_1;
yyv_Term = yy_0_1_2;
yyv_P = yy_0_1_3;
yy_0_2_1 = yy_0_2;
yyv_Spec = yy_0_2;
if (yy_0_2_1[0] != 1) goto yyfl_357_3;
yy_0_2_1_1 = ((yy)yy_0_2_1[1]);
yy_0_2_1_2 = ((yy)yy_0_2_1[2]);
yy_0_2_1_3 = ((yy)yy_0_2_1[3]);
yyv_Name = yy_0_2_1_1;
yyv_ExpType = yy_0_2_1_2;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Type;
yy_1_1_1_2 = yyv_ExpType;
if (! yyeq_IDENT(yy_1_1_1_1, yy_1_1_1_2)) goto yyfl_357_3_1_1;
goto yysl_357_3_1;
yyfl_357_3_1_1 : ;
yy_1_2_1_1 = ((yy)"'");
yy_1_2_1_2 = yyv_ExpType;
yy_1_2_1_3 = ((yy)"' expected instead of '");
yy_1_2_1_4 = yyv_Type;
yy_1_2_1_5 = ((yy)"'");
yy_1_2_1_6 = yyv_P;
MESSAGE2(yy_1_2_1_1, yy_1_2_1_2, yy_1_2_1_3, yy_1_2_1_4, yy_1_2_1_5, yy_1_2_1_6);
goto yysl_357_3_1;
yysl_357_3_1 : ;
yyb = yysb;
}
yy_2_1 = yyv_Term;
yy_2_2 = yyv_Spec;
BuildArg(yy_2_1, yy_2_2);
return;
yyfl_357_3 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_Offset;
yy yy_0_1_2;
yy yyv_Functor;
yy yy_0_1_3;
yy yyv_P;
yy yy_0_1_4;
yy yyv_Args;
yy yy_0_1_5;
yy yy_0_2;
yy yyv_Name;
yy yy_0_2_1;
yy yyv_ExpType;
yy yy_0_2_2;
yy yy_0_2_3;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yyv_Code;
yy yy_1_4;
yy yyv_FunctorArgSpecs;
yy yy_1_5;
yy yy_2_1;
yy yy_2_2;
yy yy_3_1;
yy yyv_OffsetVal;
yy yy_3_2;
yy yy_4_1;
yy yy_5_1;
yy yy_6_1;
yy yy_7_1;
yy yy_9_1;
yy yyv_NArgs;
yy yy_9_2;
yy yy_10_1;
yy yy_11_1;
yy yy_12_1;
yy yy_13_1;
yy yy_15_1;
yy yy_15_2;
yy yy_15_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 3) goto yyfl_357_4;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yy_0_1_5 = ((yy)yy_0_1[5]);
yyv_Tmp = yy_0_1_1;
yyv_Offset = yy_0_1_2;
yyv_Functor = yy_0_1_3;
yyv_P = yy_0_1_4;
yyv_Args = yy_0_1_5;
if (yy_0_2[0] != 1) goto yyfl_357_4;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yy_0_2_3 = ((yy)yy_0_2[3]);
yyv_Name = yy_0_2_1;
yyv_ExpType = yy_0_2_2;
yy_1_1 = yyv_Functor;
yy_1_2 = yyv_ExpType;
yy_1_3 = yyv_P;
ApplyFunctor(yy_1_1, yy_1_2, yy_1_3, &yy_1_4, &yy_1_5);
yyv_Code = yy_1_4;
yyv_FunctorArgSpecs = yy_1_5;
yy_2_1 = yyv_Args;
yy_2_2 = yyv_FunctorArgSpecs;
Build(yy_2_1, yy_2_2);
yy_3_1 = yyv_Offset;
GetINT(yy_3_1, &yy_3_2);
yyv_OffsetVal = yy_3_2;
yy_4_1 = yyv_Tmp;
tmp(yy_4_1);
yy_5_1 = ((yy)" = yyb + ");
s(yy_5_1);
yy_6_1 = yyv_OffsetVal;
i(yy_6_1);
yy_7_1 = ((yy)";");
s(yy_7_1);
nl();
yy_9_1 = yyv_Args;
CountFunctorArgs(yy_9_1, &yy_9_2);
yyv_NArgs = yy_9_2;
yy_10_1 = yyv_Tmp;
tmp(yy_10_1);
yy_11_1 = ((yy)"[0] = ");
s(yy_11_1);
yy_12_1 = yyv_Code;
i(yy_12_1);
yy_13_1 = ((yy)";");
s(yy_13_1);
nl();
yy_15_1 = yyv_Args;
yy_15_2 = yyv_Tmp;
yy_15_3 = ((yy)1);
AssignSons(yy_15_1, yy_15_2, yy_15_3);
return;
yyfl_357_4 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_Name;
yy yy_0_1_2;
yy yyv_P;
yy yy_0_1_3;
yy yy_0_2;
yy yyv_Name2;
yy yy_0_2_1;
yy yyv_ExpType;
yy yy_0_2_2;
yy yy_0_2_3;
yy yy_1_1_1_1;
yy yy_1_1_2_1;
yy yyv_VarType;
yy yy_1_1_2_2;
yy yy_1_1_3_1;
yy yyv_FunctorCode;
yy yy_1_1_3_2;
yy yy_1_1_4_1_1_1;
yy yy_1_1_4_1_1_2;
yy yy_1_1_4_2_1_1;
yy yy_1_1_4_2_1_2;
yy yy_1_1_4_2_1_3;
yy yy_1_1_4_2_1_4;
yy yy_1_1_4_2_1_5;
yy yy_1_1_4_2_1_6;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_1_2_1_3;
yy yy_1_2_1_4;
yy yy_2_1;
yy yy_3_1;
yy yy_4_1;
yy yy_5_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 5) goto yyfl_357_5;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Tmp = yy_0_1_1;
yyv_Name = yy_0_1_2;
yyv_P = yy_0_1_3;
if (yy_0_2[0] != 1) goto yyfl_357_5;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yy_0_2_3 = ((yy)yy_0_2[3]);
yyv_Name2 = yy_0_2_1;
yyv_ExpType = yy_0_2_2;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Name;
if (! HasLocalMeaning(yy_1_1_1_1)) goto yyfl_357_5_1_1;
yy_1_1_2_1 = yyv_Name;
if (! GetLocalMeaning(yy_1_1_2_1, &yy_1_1_2_2)) goto yyfl_357_5_1_1;
yyv_VarType = yy_1_1_2_2;
yy_1_1_3_1 = yyv_Name;
if (! GetLocalMeaning2(yy_1_1_3_1, &yy_1_1_3_2)) goto yyfl_357_5_1_1;
yyv_FunctorCode = yy_1_1_3_2;
{
yy yysb = yyb;
yy_1_1_4_1_1_1 = yyv_ExpType;
yy_1_1_4_1_1_2 = yyv_VarType;
if (! yyeq_IDENT(yy_1_1_4_1_1_1, yy_1_1_4_1_1_2)) goto yyfl_357_5_1_1_4_1;
goto yysl_357_5_1_1_4;
yyfl_357_5_1_1_4_1 : ;
yy_1_1_4_2_1_1 = ((yy)"Formal type '");
yy_1_1_4_2_1_2 = yyv_ExpType;
yy_1_1_4_2_1_3 = ((yy)"', actual type '");
yy_1_1_4_2_1_4 = yyv_VarType;
yy_1_1_4_2_1_5 = ((yy)"'");
yy_1_1_4_2_1_6 = yyv_P;
MESSAGE2(yy_1_1_4_2_1_1, yy_1_1_4_2_1_2, yy_1_1_4_2_1_3, yy_1_1_4_2_1_4, yy_1_1_4_2_1_5, yy_1_1_4_2_1_6);
goto yysl_357_5_1_1_4;
yysl_357_5_1_1_4 : ;
yyb = yysb;
}
goto yysl_357_5_1;
yyfl_357_5_1_1 : ;
yy_1_2_1_1 = ((yy)"'");
yy_1_2_1_2 = yyv_Name;
yy_1_2_1_3 = ((yy)"' is undefined");
yy_1_2_1_4 = yyv_P;
MESSAGE1(yy_1_2_1_1, yy_1_2_1_2, yy_1_2_1_3, yy_1_2_1_4);
goto yysl_357_5_1;
yysl_357_5_1 : ;
yyb = yysb;
}
yy_2_1 = yyv_Tmp;
tmp(yy_2_1);
yy_3_1 = ((yy)" = ");
s(yy_3_1);
yy_4_1 = yyv_Name;
varid(yy_4_1);
yy_5_1 = ((yy)";");
s(yy_5_1);
nl();
return;
yyfl_357_5 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_Number;
yy yy_0_1_2;
yy yyv_P;
yy yy_0_1_3;
yy yy_0_2;
yy yyv_Name;
yy yy_0_2_1;
yy yyv_Type;
yy yy_0_2_2;
yy yy_0_2_3;
yy yy_1_1;
yy yy_1_2;
yy yy_2_1;
yy yy_3_1;
yy yy_4_1;
yy yy_5_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 6) goto yyfl_357_6;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Tmp = yy_0_1_1;
yyv_Number = yy_0_1_2;
yyv_P = yy_0_1_3;
if (yy_0_2[0] != 1) goto yyfl_357_6;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yy_0_2_3 = ((yy)yy_0_2[3]);
yyv_Name = yy_0_2_1;
yyv_Type = yy_0_2_2;
yy_1_1 = yyv_Type;
yy_1_2 = yyv_P;
CheckInt(yy_1_1, yy_1_2);
yy_2_1 = yyv_Tmp;
tmp(yy_2_1);
yy_3_1 = ((yy)" = ((yy)");
s(yy_3_1);
yy_4_1 = yyv_Number;
i(yy_4_1);
yy_5_1 = ((yy)");");
s(yy_5_1);
nl();
return;
yyfl_357_6 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_String;
yy yy_0_1_2;
yy yyv_P;
yy yy_0_1_3;
yy yy_0_2;
yy yyv_Name;
yy yy_0_2_1;
yy yyv_Type;
yy yy_0_2_2;
yy yy_0_2_3;
yy yy_1_1;
yy yy_1_2;
yy yy_2_1;
yy yy_3_1;
yy yy_4_1;
yy yy_5_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 7) goto yyfl_357_7;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Tmp = yy_0_1_1;
yyv_String = yy_0_1_2;
yyv_P = yy_0_1_3;
if (yy_0_2[0] != 1) goto yyfl_357_7;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yy_0_2_3 = ((yy)yy_0_2[3]);
yyv_Name = yy_0_2_1;
yyv_Type = yy_0_2_2;
yy_1_1 = yyv_Type;
yy_1_2 = yyv_P;
CheckString(yy_1_1, yy_1_2);
yy_2_1 = yyv_Tmp;
tmp(yy_2_1);
yy_3_1 = ((yy)" = ((yy)");
s(yy_3_1);
yy_4_1 = yyv_String;
qu_s(yy_4_1);
yy_5_1 = ((yy)");");
s(yy_5_1);
nl();
return;
yyfl_357_7 : ;
}
yyErr(2,3913);
}
CountFunctorArgs(yyin_1, yyout_1)
yy yyin_1;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_USEARG;
yy yy_0_1_1;
yy yyv_POS;
yy yy_0_1_2;
yy yyv_USEARGLIST;
yy yy_0_1_3;
yy yy_0_2;
yy yy_0_2_1;
yy yy_0_2_2;
yy yy_1_1;
yy yyv_N;
yy yy_1_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_358_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_USEARG = yy_0_1_1;
yyv_POS = yy_0_1_2;
yyv_USEARGLIST = yy_0_1_3;
yy_1_1 = yyv_USEARGLIST;
CountFunctorArgs(yy_1_1, &yy_1_2);
yyv_N = yy_1_2;
yy_0_2_1 = yyv_N;
yy_0_2_2 = ((yy)1);
yy_0_2 = (yy)(((long)yy_0_2_1)+((long)yy_0_2_2));
*yyout_1 = yy_0_2;
return;
yyfl_358_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_POS;
yy yy_0_1_1;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_358_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yyv_POS = yy_0_1_1;
yy_0_2 = ((yy)0);
*yyout_1 = yy_0_2;
return;
yyfl_358_2 : ;
}
yyErr(2,3974);
}
CheckInt(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yyv_P;
yy yy_0_2;
yy yyv_T_ID_INT;
yy yy_1;
yy yy_2_1;
yy yy_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Id = yy_0_1;
yyv_P = yy_0_2;
yy_1 = yyglov_id_INT;
if (yy_1 == (yy) yyu) yyErr(1,3981);
yyv_T_ID_INT = yy_1;
yy_2_1 = yyv_Id;
yy_2_2 = yyv_T_ID_INT;
if (! yyeq_IDENT(yy_2_1, yy_2_2)) goto yyfl_359_1;
return;
yyfl_359_1 : ;
}
{
yy yyb;
yy yyv_Other;
yy yy_0_1;
yy yyv_P;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Other = yy_0_1;
yyv_P = yy_0_2;
yy_1_1 = ((yy)"Formal type must be INT");
yy_1_2 = yyv_P;
MESSAGE(yy_1_1, yy_1_2);
return;
yyfl_359_2 : ;
}
yyErr(2,3978);
}
CheckString(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yyv_P;
yy yy_0_2;
yy yyv_T_ID_STRING;
yy yy_1;
yy yy_2_1;
yy yy_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Id = yy_0_1;
yyv_P = yy_0_2;
yy_1 = yyglov_id_STRING;
if (yy_1 == (yy) yyu) yyErr(1,3991);
yyv_T_ID_STRING = yy_1;
yy_2_1 = yyv_Id;
yy_2_2 = yyv_T_ID_STRING;
if (! yyeq_IDENT(yy_2_1, yy_2_2)) goto yyfl_360_1;
return;
yyfl_360_1 : ;
}
{
yy yyb;
yy yyv_Other;
yy yy_0_1;
yy yyv_P;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Other = yy_0_1;
yyv_P = yy_0_2;
yy_1_1 = ((yy)"Formal type must be STRING");
yy_1_2 = yyv_P;
MESSAGE(yy_1_1, yy_1_2);
return;
yyfl_360_2 : ;
}
yyErr(2,3987);
}
AssignSons(yyin_1, yyin_2, yyin_3)
yy yyin_1;
yy yyin_2;
yy yyin_3;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_T;
yy yy_0_1_3;
yy yyv_Tmp;
yy yy_0_2;
yy yyv_N;
yy yy_0_3;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yy_2_1;
yy yy_2_2;
yy yy_2_3;
yy yy_2_3_1;
yy yy_2_3_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 1) goto yyfl_361_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_H = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_T = yy_0_1_3;
yyv_Tmp = yy_0_2;
yyv_N = yy_0_3;
yy_1_1 = yyv_H;
yy_1_2 = yyv_Tmp;
yy_1_3 = yyv_N;
AssignSon(yy_1_1, yy_1_2, yy_1_3);
yy_2_1 = yyv_T;
yy_2_2 = yyv_Tmp;
yy_2_3_1 = yyv_N;
yy_2_3_2 = ((yy)1);
yy_2_3 = (yy)(((long)yy_2_3_1)+((long)yy_2_3_2));
AssignSons(yy_2_1, yy_2_2, yy_2_3);
return;
yyfl_361_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_P;
yy yy_0_1_1;
yy yyv_Tmp;
yy yy_0_2;
yy yyv_N;
yy yy_0_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 2) goto yyfl_361_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yyv_P = yy_0_1_1;
yyv_Tmp = yy_0_2;
yyv_N = yy_0_3;
return;
yyfl_361_2 : ;
}
yyErr(2,3998);
}
AssignSon(yyin_1, yyin_2, yyin_3)
yy yyin_1;
yy yyin_2;
yy yyin_3;
{
{
yy yyb;
yy yyv_Arg;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_2;
yy yyv_N;
yy yy_0_3;
yy yy_1_1;
yy yyv_UseArgTmp;
yy yy_1_2;
yy yy_2_1;
yy yy_3_1;
yy yy_4_1;
yy yy_5_1;
yy yy_6_1;
yy yy_7_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Arg = yy_0_1;
yyv_Tmp = yy_0_2;
yyv_N = yy_0_3;
yy_1_1 = yyv_Arg;
GetUseArgTmp(yy_1_1, &yy_1_2);
yyv_UseArgTmp = yy_1_2;
yy_2_1 = yyv_Tmp;
tmp(yy_2_1);
yy_3_1 = ((yy)"[");
s(yy_3_1);
yy_4_1 = yyv_N;
i(yy_4_1);
yy_5_1 = ((yy)"] = ");
s(yy_5_1);
yy_6_1 = yyv_UseArgTmp;
tmp_i(yy_6_1);
yy_7_1 = ((yy)";");
s(yy_7_1);
nl();
return;
}
}
DefineLocalVar(yyin_1, yyin_2, yyin_3, yyin_4)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy yyin_4;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yyv_Type;
yy yy_0_2;
yy yyv_FunctorCode;
yy yy_0_3;
yy yyv_P;
yy yy_0_4;
yy yy_1_1;
yy yy_2_1;
yy yy_2_2;
yy yy_2_3;
yy yy_2_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
yyv_Id = yy_0_1;
yyv_Type = yy_0_2;
yyv_FunctorCode = yy_0_3;
yyv_P = yy_0_4;
yy_1_1 = yyv_Id;
if (! HasLocalMeaning(yy_1_1)) goto yyfl_363_1;
yy_2_1 = ((yy)"'");
yy_2_2 = yyv_Id;
yy_2_3 = ((yy)"' is defined more than once");
yy_2_4 = yyv_P;
MESSAGE1(yy_2_1, yy_2_2, yy_2_3, yy_2_4);
return;
yyfl_363_1 : ;
}
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yyv_Type;
yy yy_0_2;
yy yyv_FunctorCode;
yy yy_0_3;
yy yyv_P;
yy yy_0_4;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yyv_L;
yy yy_2;
yy yy_3;
yy yy_3_1;
yy yy_3_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
yyv_Id = yy_0_1;
yyv_Type = yy_0_2;
yyv_FunctorCode = yy_0_3;
yyv_P = yy_0_4;
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_Id;
yy_1_2 = yyv_Type;
yy_1_3 = yyv_FunctorCode;
DefLocalMeaning(yy_1_1, yy_1_2, yy_1_3);
yy_2 = yyglov_localVars;
if (yy_2 == (yy) yyu) yyErr(1,4024);
yyv_L = yy_2;
yy_3_1 = yyv_Id;
yy_3_2 = yyv_L;
yy_3 = yyb + 0;
yy_3[0] = 1;
yy_3[1] = ((long)yy_3_1);
yy_3[2] = ((long)yy_3_2);
yyglov_localVars = yy_3;
return;
yyfl_363_2 : ;
}
yyErr(2,4016);
}
ApplyLocalVar(yyin_1, yyin_2, yyin_3)
yy yyin_1;
yy yyin_2;
yy yyin_3;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yyv_ExpType;
yy yy_0_2;
yy yyv_P;
yy yy_0_3;
yy yy_1_1;
yy yy_2_1;
yy yyv_VarType;
yy yy_2_2;
yy yy_3_1;
yy yy_3_2;
yy yy_3_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Id = yy_0_1;
yyv_ExpType = yy_0_2;
yyv_P = yy_0_3;
yy_1_1 = yyv_Id;
if (! HasLocalMeaning(yy_1_1)) goto yyfl_364_1;
yy_2_1 = yyv_Id;
if (! GetLocalMeaning(yy_2_1, &yy_2_2)) goto yyfl_364_1;
yyv_VarType = yy_2_2;
yy_3_1 = yyv_ExpType;
yy_3_2 = yyv_VarType;
yy_3_3 = yyv_P;
CheckTypeOfLocalVar(yy_3_1, yy_3_2, yy_3_3);
return;
yyfl_364_1 : ;
}
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yyv_Type;
yy yy_0_2;
yy yyv_P;
yy yy_0_3;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yy_1_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Id = yy_0_1;
yyv_Type = yy_0_2;
yyv_P = yy_0_3;
yy_1_1 = ((yy)"'");
yy_1_2 = yyv_Id;
yy_1_3 = ((yy)"' is undefined");
yy_1_4 = yyv_P;
MESSAGE1(yy_1_1, yy_1_2, yy_1_3, yy_1_4);
return;
yyfl_364_2 : ;
}
yyErr(2,4027);
}
CheckEqType(yyin_1, yyin_2, yyin_3, yyin_4)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy yyin_4;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yyv_T1;
yy yy_0_2;
yy yyv_T2;
yy yy_0_3;
yy yyv_P;
yy yy_0_4;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_1_2_1_3;
yy yy_1_2_1_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
yyv_Id = yy_0_1;
yyv_T1 = yy_0_2;
yyv_T2 = yy_0_3;
yyv_P = yy_0_4;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_T1;
yy_1_1_1_2 = yyv_T2;
if (! yyeq_IDENT(yy_1_1_1_1, yy_1_1_1_2)) goto yyfl_365_1_1_1;
goto yysl_365_1_1;
yyfl_365_1_1_1 : ;
yy_1_2_1_1 = ((yy)"'");
yy_1_2_1_2 = yyv_Id;
yy_1_2_1_3 = ((yy)"' defined with different types");
yy_1_2_1_4 = yyv_P;
MESSAGE1(yy_1_2_1_1, yy_1_2_1_2, yy_1_2_1_3, yy_1_2_1_4);
goto yysl_365_1_1;
yysl_365_1_1 : ;
yyb = yysb;
}
return;
}
}
CheckTypeOfLocalVar(yyin_1, yyin_2, yyin_3)
yy yyin_1;
yy yyin_2;
yy yyin_3;
{
{
yy yyb;
yy yyv_Type1;
yy yy_0_1;
yy yyv_Type2;
yy yy_0_2;
yy yyv_P;
yy yy_0_3;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_1_2_1_3;
yy yy_1_2_1_4;
yy yy_1_2_1_5;
yy yy_1_2_1_6;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Type1 = yy_0_1;
yyv_Type2 = yy_0_2;
yyv_P = yy_0_3;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Type1;
yy_1_1_1_2 = yyv_Type2;
if (! yyeq_IDENT(yy_1_1_1_1, yy_1_1_1_2)) goto yyfl_366_1_1_1;
goto yysl_366_1_1;
yyfl_366_1_1_1 : ;
yy_1_2_1_1 = ((yy)"Formal type '");
yy_1_2_1_2 = yyv_Type1;
yy_1_2_1_3 = ((yy)"', actual type '");
yy_1_2_1_4 = yyv_Type2;
yy_1_2_1_5 = ((yy)"'");
yy_1_2_1_6 = yyv_P;
MESSAGE2(yy_1_2_1_1, yy_1_2_1_2, yy_1_2_1_3, yy_1_2_1_4, yy_1_2_1_5, yy_1_2_1_6);
goto yysl_366_1_1;
yysl_366_1_1 : ;
yyb = yysb;
}
return;
}
}
DefYYSTYPE()
{
{
yy yyb;
yy yyv_N;
yy yy_1;
yy yy_2_1;
yy yy_3_1;
yy yy_3_1_1;
yy yy_3_1_2;
yy yy_4_1;
yy yy_6_1;
yy yy_8_1;
yy_1 = yyglov_maxAttr;
if (yy_1 == (yy) yyu) yyErr(1,4057);
yyv_N = yy_1;
yy_2_1 = ((yy)"#include <stdint.h>\ntypedef struct {intptr_t attr[");
s(yy_2_1);
yy_3_1_1 = yyv_N;
yy_3_1_2 = ((yy)1);
yy_3_1 = (yy)(((long)yy_3_1_1)+((long)yy_3_1_2));
i(yy_3_1);
yy_4_1 = ((yy)"];} yyATTRIBUTES;");
s(yy_4_1);
nl();
yy_6_1 = ((yy)"#define YYSTYPE yyATTRIBUTES");
s(yy_6_1);
nl();
yy_8_1 = ((yy)"extern YYSTYPE yylval;");
s(yy_8_1);
nl();
nl();
return;
}
}
Prelude()
{
{
yy yyb;
yy yy_1_1;
yy yy_3_1;
yy yy_5_1;
yy yy_7_1;
yy yy_9_1;
yy yyv_F;
yy yy_11_1;
yy yy_12_1;
yy yy_14_1;
yy yy_16_1;
yy yy_17_1;
yy yy_18_1;
yy yy_20_1;
/* --PATCH-- */ yy_1_1 = ((yy)
/* --PATCH-- */             "#include <stdio.h>\n"
/* --PATCH-- */             "#include <string.h>\n"
/* --PATCH-- */             "#include <stdlib.h>\n"
/* --PATCH-- */				"#include <stdint.h>\n"
/* --PATCH-- */             "#undef min\n"
/* --PATCH-- */             "#pragma clang diagnostic ignored \"-Wimplicit-int\"\n"
/* --PATCH-- */             "#pragma clang diagnostic ignored \"-Wimplicit-function-declaration\"\n"
/* --PATCH-- */             "typedef intptr_t * yy;");
s(yy_1_1);
nl();
/* --PATCH-- */ yy_3_1 = ((yy)"#define yyu INTPTR_MIN");
s(yy_3_1);
nl();
yy_5_1 = ((yy)"static yy yynull;");
s(yy_5_1);
nl();
yy_7_1 = ((yy)"extern yy yyh;");
s(yy_7_1);
nl();
yy_9_1 = ((yy)"extern yy yyhx;");
s(yy_9_1);
nl();
GetSourceName(&yy_11_1);
yyv_F = yy_11_1;
/* --PATCH-- */ yy_12_1 = ((yy)"static void yyErr(n,l)");
s(yy_12_1);
nl();
yy_14_1 = ((yy)"{");
s(yy_14_1);
nl();
/* --PATCH-- */ yy_16_1 = ((yy)"yyAbort(n,");
s(yy_16_1);
yy_17_1 = yyv_F;
qu_s(yy_17_1);
yy_18_1 = ((yy)", l);");
s(yy_18_1);
nl();
yy_20_1 = ((yy)"}");
s(yy_20_1);
nl();
return;
}
}
monop(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_369_1;
return;
yyfl_369_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_1_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_369_2;
yy_1_1 = ((yy)"-");
s(yy_1_1);
return;
yyfl_369_2 : ;
}
yyErr(2,4085);
}
binop(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yy_1_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_370_1;
yy_1_1 = ((yy)"+");
s(yy_1_1);
return;
yyfl_370_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_1_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_370_2;
yy_1_1 = ((yy)"-");
s(yy_1_1);
return;
yyfl_370_2 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_1_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 3) goto yyfl_370_3;
yy_1_1 = ((yy)"*");
s(yy_1_1);
return;
yyfl_370_3 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_1_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 4) goto yyfl_370_4;
yy_1_1 = ((yy)"/");
s(yy_1_1);
return;
yyfl_370_4 : ;
}
yyErr(2,4092);
}
attr(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yyv_Tmp;
yy yy_0_1;
yy yyv_N;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy yyv_ArgNo;
yy yy_1_2_1;
yy yy_1_2_2;
yy yyv_MemberNo;
yy yy_1_2_2_1;
yy yy_1_2_2_2;
yy yy_2_1;
yy yy_3_1;
yy yy_4_1;
yy yy_5_1;
yy yy_6_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Tmp = yy_0_1;
yyv_N = yy_0_2;
yy_1_1 = yyv_Tmp;
GetTempo(yy_1_1, &yy_1_2);
if (yy_1_2[0] != 1) goto yyfl_371_1;
yy_1_2_1 = ((yy)yy_1_2[1]);
yy_1_2_2 = ((yy)yy_1_2[2]);
yyv_ArgNo = yy_1_2_1;
if (yy_1_2_2[0] != 1) goto yyfl_371_1;
yy_1_2_2_1 = ((yy)yy_1_2_2[1]);
yy_1_2_2_2 = ((yy)yy_1_2_2[2]);
yyv_MemberNo = yy_1_2_2_1;
if (yy_1_2_2_2[0] != 2) goto yyfl_371_1;
yy_2_1 = ((yy)"$");
s(yy_2_1);
yy_3_1 = yyv_N;
i(yy_3_1);
yy_4_1 = ((yy)".attr[");
s(yy_4_1);
yy_5_1 = yyv_ArgNo;
i(yy_5_1);
yy_6_1 = ((yy)"]");
s(yy_6_1);
return;
yyfl_371_1 : ;
}
yyErr(2,4103);
}
tmp(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yyv_Tmp;
yy yy_0_1;
yy yy_1_1;
yy yyv_Val;
yy yy_1_2;
yy yy_2_1;
yy yy_3_1;
yy_0_1 = yyin_1;
yyv_Tmp = yy_0_1;
yy_1_1 = yyv_Tmp;
GetTempo(yy_1_1, &yy_1_2);
yyv_Val = yy_1_2;
yy_2_1 = ((yy)"yy");
s(yy_2_1);
yy_3_1 = yyv_Val;
tmpval(yy_3_1);
return;
}
}
tmpval(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_N;
yy yy_0_1_1;
yy yyv_Tl;
yy yy_0_1_2;
yy yy_1_1;
yy yy_2_1;
yy yy_3_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_373_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_N = yy_0_1_1;
yyv_Tl = yy_0_1_2;
yy_1_1 = yyv_Tl;
tmpval(yy_1_1);
yy_2_1 = ((yy)"_");
s(yy_2_1);
yy_3_1 = yyv_N;
i(yy_3_1);
return;
yyfl_373_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_373_2;
return;
yyfl_373_2 : ;
}
yyErr(2,4115);
}
tmp_i(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yyv_T;
yy yy_0_1;
yy yy_1_1;
yy yy_2_1;
yy yy_3_1;
yy_0_1 = yyin_1;
yyv_T = yy_0_1;
yy_1_1 = ((yy)"((intptr_t)");
s(yy_1_1);
yy_2_1 = yyv_T;
tmp(yy_2_1);
yy_3_1 = ((yy)")");
s(yy_3_1);
return;
}
}
id(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yy_1_1;
yy yyv_Str;
yy yy_1_2;
yy yy_2_1;
yy_0_1 = yyin_1;
yyv_Id = yy_0_1;
yy_1_1 = yyv_Id;
id_to_string(yy_1_1, &yy_1_2);
yyv_Str = yy_1_2;
yy_2_1 = yyv_Str;
s(yy_2_1);
return;
}
}
name(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Id;
yy yy_0_1_1;
yy yy_1_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_376_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yyv_Id = yy_0_1_1;
yy_1_1 = yyv_Id;
id(yy_1_1);
return;
yyfl_376_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_1_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_376_2;
yy_1_1 = ((yy)"yygoal");
s(yy_1_1);
return;
yyfl_376_2 : ;
}
yyErr(2,4136);
}
glovarid(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yy_1_1;
yy yyv_Str;
yy yy_1_2;
yy yy_2_1;
yy yy_3_1;
yy_0_1 = yyin_1;
yyv_Id = yy_0_1;
yy_1_1 = yyv_Id;
id_to_string(yy_1_1, &yy_1_2);
yyv_Str = yy_1_2;
yy_2_1 = ((yy)"yyglov_");
s(yy_2_1);
yy_3_1 = yyv_Str;
s(yy_3_1);
return;
}
}
varid(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yy_1_1;
yy yyv_Str;
yy yy_1_2;
yy yy_2_1;
yy yy_3_1;
yy_0_1 = yyin_1;
yyv_Id = yy_0_1;
yy_1_1 = yyv_Id;
id_to_string(yy_1_1, &yy_1_2);
yyv_Str = yy_1_2;
yy_2_1 = ((yy)"yyv_");
s(yy_2_1);
yy_3_1 = yyv_Str;
s(yy_3_1);
return;
}
}
NewTempo(yyout_1)
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Tempo;
yyb = yyh;
yyh += 2; if (yyh > yyhx) yyExtend();
yyv_Tempo = yyb + 0;
yyb[1] = yyu;
yy_0_1 = yyv_Tempo;
*yyout_1 = yy_0_1;
return;
}
}
SetTempo(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yyv_RefTempo;
yy yy_0_1;
yy yyv_Tempo;
yy yy_0_2;
yy yy_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_RefTempo = yy_0_1;
yyv_Tempo = yy_0_2;
yy_1 = yyv_Tempo;
yyv_RefTempo[1] = (long) yy_1;
return;
}
}
GetTempo(yyin_1, yyout_1)
yy yyin_1;
yy *yyout_1;
{
{
yy yyb;
yy yyv_RefTempo;
yy yy_0_1;
yy yy_0_2;
yy yyv_Tempo;
yy yy_1;
yy_0_1 = yyin_1;
yyv_RefTempo = yy_0_1;
yy_1 = (yy) yyv_RefTempo[1];
if (yy_1 == (yy) yyu) yyErr(1,4174);
yyv_Tempo = yy_1;
yy_0_2 = yyv_Tempo;
*yyout_1 = yy_0_2;
return;
}
}
NewINT(yyout_1)
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_I;
yyb = yyh;
yyh += 2; if (yyh > yyhx) yyExtend();
yyv_I = yyb + 0;
yyb[1] = yyu;
yy_0_1 = yyv_I;
*yyout_1 = yy_0_1;
return;
}
}
SetINT(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yyv_RefINT;
yy yy_0_1;
yy yyv_INT;
yy yy_0_2;
yy yy_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_RefINT = yy_0_1;
yyv_INT = yy_0_2;
yy_1 = yyv_INT;
yyv_RefINT[1] = (long) yy_1;
return;
}
}
GetINT(yyin_1, yyout_1)
yy yyin_1;
yy *yyout_1;
{
{
yy yyb;
yy yyv_RefINT;
yy yy_0_1;
yy yy_0_2;
yy yyv_INT;
yy yy_1;
yy_0_1 = yyin_1;
yyv_RefINT = yy_0_1;
yy_1 = (yy) yyv_RefINT[1];
if (yy_1 == (yy) yyu) yyErr(1,4189);
yyv_INT = yy_1;
yy_0_2 = yyv_INT;
*yyout_1 = yy_0_2;
return;
}
}
GetUseArgTmp(yyin_1, yyout_1)
yy yyin_1;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_Op;
yy yy_0_1_2;
yy yyv_P;
yy yy_0_1_3;
yy yyv_Left;
yy yy_0_1_4;
yy yyv_Right;
yy yy_0_1_5;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_387_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yy_0_1_5 = ((yy)yy_0_1[5]);
yyv_Tmp = yy_0_1_1;
yyv_Op = yy_0_1_2;
yyv_P = yy_0_1_3;
yyv_Left = yy_0_1_4;
yyv_Right = yy_0_1_5;
yy_0_2 = yyv_Tmp;
*yyout_1 = yy_0_2;
return;
yyfl_387_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_Op;
yy yy_0_1_2;
yy yyv_P;
yy yy_0_1_3;
yy yyv_Arg;
yy yy_0_1_4;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_387_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yyv_Tmp = yy_0_1_1;
yyv_Op = yy_0_1_2;
yyv_P = yy_0_1_3;
yyv_Arg = yy_0_1_4;
yy_0_2 = yyv_Tmp;
*yyout_1 = yy_0_2;
return;
yyfl_387_2 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_Term;
yy yy_0_1_2;
yy yy_0_1_3;
yy yy_0_2;
yy yy_1_1;
yy yyv_Tmp;
yy yy_1_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 4) goto yyfl_387_3;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Term = yy_0_1_2;
yy_1_1 = yyv_Term;
GetUseArgTmp(yy_1_1, &yy_1_2);
yyv_Tmp = yy_1_2;
yy_0_2 = yyv_Tmp;
*yyout_1 = yy_0_2;
return;
yyfl_387_3 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_Offset;
yy yy_0_1_2;
yy yyv_Functor;
yy yy_0_1_3;
yy yyv_P;
yy yy_0_1_4;
yy yyv_Args;
yy yy_0_1_5;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 3) goto yyfl_387_4;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yy_0_1_5 = ((yy)yy_0_1[5]);
yyv_Tmp = yy_0_1_1;
yyv_Offset = yy_0_1_2;
yyv_Functor = yy_0_1_3;
yyv_P = yy_0_1_4;
yyv_Args = yy_0_1_5;
yy_0_2 = yyv_Tmp;
*yyout_1 = yy_0_2;
return;
yyfl_387_4 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_Name;
yy yy_0_1_2;
yy yyv_P;
yy yy_0_1_3;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 5) goto yyfl_387_5;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Tmp = yy_0_1_1;
yyv_Name = yy_0_1_2;
yyv_P = yy_0_1_3;
yy_0_2 = yyv_Tmp;
*yyout_1 = yy_0_2;
return;
yyfl_387_5 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_Number;
yy yy_0_1_2;
yy yyv_P;
yy yy_0_1_3;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 6) goto yyfl_387_6;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Tmp = yy_0_1_1;
yyv_Number = yy_0_1_2;
yyv_P = yy_0_1_3;
yy_0_2 = yyv_Tmp;
*yyout_1 = yy_0_2;
return;
yyfl_387_6 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_String;
yy yy_0_1_2;
yy yyv_P;
yy yy_0_1_3;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 7) goto yyfl_387_7;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Tmp = yy_0_1_1;
yyv_String = yy_0_1_2;
yyv_P = yy_0_1_3;
yy_0_2 = yyv_Tmp;
*yyout_1 = yy_0_2;
return;
yyfl_387_7 : ;
}
yyErr(2,4191);
}
GetDefArgTmp(yyin_1, yyout_1)
yy yyin_1;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_Functor;
yy yy_0_1_2;
yy yyv_P;
yy yy_0_1_3;
yy yyv_Args;
yy yy_0_1_4;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_388_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yyv_Tmp = yy_0_1_1;
yyv_Functor = yy_0_1_2;
yyv_P = yy_0_1_3;
yyv_Args = yy_0_1_4;
yy_0_2 = yyv_Tmp;
*yyout_1 = yy_0_2;
return;
yyfl_388_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tp;
yy yy_0_1_1;
yy yyv_Tmp;
yy yy_0_1_2;
yy yyv_Functor;
yy yy_0_1_3;
yy yyv_P;
yy yy_0_1_4;
yy yyv_Args;
yy yy_0_1_5;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_388_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yy_0_1_5 = ((yy)yy_0_1[5]);
yyv_Tp = yy_0_1_1;
yyv_Tmp = yy_0_1_2;
yyv_Functor = yy_0_1_3;
yyv_P = yy_0_1_4;
yyv_Args = yy_0_1_5;
yy_0_2 = yyv_Tmp;
*yyout_1 = yy_0_2;
return;
yyfl_388_2 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_Name;
yy yy_0_1_2;
yy yyv_P;
yy yy_0_1_3;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 3) goto yyfl_388_3;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Tmp = yy_0_1_1;
yyv_Name = yy_0_1_2;
yyv_P = yy_0_1_3;
yy_0_2 = yyv_Tmp;
*yyout_1 = yy_0_2;
return;
yyfl_388_3 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_Name;
yy yy_0_1_2;
yy yyv_P;
yy yy_0_1_3;
yy yyv_Arg;
yy yy_0_1_4;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 4) goto yyfl_388_4;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yyv_Tmp = yy_0_1_1;
yyv_Name = yy_0_1_2;
yyv_P = yy_0_1_3;
yyv_Arg = yy_0_1_4;
yy_0_2 = yyv_Tmp;
*yyout_1 = yy_0_2;
return;
yyfl_388_4 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 5) goto yyfl_388_5;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_Tmp = yy_0_1_1;
yyv_P = yy_0_1_2;
yy_0_2 = yyv_Tmp;
*yyout_1 = yy_0_2;
return;
yyfl_388_5 : ;
}
yyErr(2,4202);
}
POS_USEARGLIST(yyin_1, yyout_1)
yy yyin_1;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_USEARG;
yy yy_0_1_1;
yy yyv_POS;
yy yy_0_1_2;
yy yyv_USEARGLIST;
yy yy_0_1_3;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_389_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_USEARG = yy_0_1_1;
yyv_POS = yy_0_1_2;
yyv_USEARGLIST = yy_0_1_3;
yy_0_2 = yyv_POS;
*yyout_1 = yy_0_2;
return;
yyfl_389_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_POS;
yy yy_0_1_1;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_389_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yyv_POS = yy_0_1_1;
yy_0_2 = yyv_POS;
*yyout_1 = yy_0_2;
return;
yyfl_389_2 : ;
}
yyErr(2,4213);
}
POS_DEFARGLIST(yyin_1, yyout_1)
yy yyin_1;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_DEFARG;
yy yy_0_1_1;
yy yyv_POS;
yy yy_0_1_2;
yy yyv_DEFARGLIST;
yy yy_0_1_3;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_390_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_DEFARG = yy_0_1_1;
yyv_POS = yy_0_1_2;
yyv_DEFARGLIST = yy_0_1_3;
yy_0_2 = yyv_POS;
*yyout_1 = yy_0_2;
return;
yyfl_390_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_POS;
yy yy_0_1_1;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_390_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yyv_POS = yy_0_1_1;
yy_0_2 = yyv_POS;
*yyout_1 = yy_0_2;
return;
yyfl_390_2 : ;
}
yyErr(2,4220);
}
POS_MEMBER(yyin_1, yyout_1)
yy yyin_1;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_DefArg;
yy yy_0_1_1;
yy yyv_UseArg;
yy yy_0_1_2;
yy yyv_Pos;
yy yy_0_1_3;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_391_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_DefArg = yy_0_1_1;
yyv_UseArg = yy_0_1_2;
yyv_Pos = yy_0_1_3;
yy_0_2 = yyv_Pos;
*yyout_1 = yy_0_2;
return;
yyfl_391_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Var;
yy yy_0_1_1;
yy yyv_Pos;
yy yy_0_1_2;
yy yyv_Expression;
yy yy_0_1_3;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_391_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Var = yy_0_1_1;
yyv_Pos = yy_0_1_2;
yyv_Expression = yy_0_1_3;
yy_0_2 = yyv_Pos;
*yyout_1 = yy_0_2;
return;
yyfl_391_2 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Var;
yy yy_0_1_1;
yy yyv_Pos;
yy yy_0_1_2;
yy yyv_Value;
yy yy_0_1_3;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 3) goto yyfl_391_3;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Var = yy_0_1_1;
yyv_Pos = yy_0_1_2;
yyv_Value = yy_0_1_3;
yy_0_2 = yyv_Pos;
*yyout_1 = yy_0_2;
return;
yyfl_391_3 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Type;
yy yy_0_1_1;
yy yyv_Key;
yy yy_0_1_2;
yy yyv_Pos;
yy yy_0_1_3;
yy yyv_Offset;
yy yy_0_1_4;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 4) goto yyfl_391_4;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yyv_Type = yy_0_1_1;
yyv_Key = yy_0_1_2;
yyv_Pos = yy_0_1_3;
yyv_Offset = yy_0_1_4;
yy_0_2 = yyv_Pos;
*yyout_1 = yy_0_2;
return;
yyfl_391_4 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Key;
yy yy_0_1_1;
yy yyv_Field;
yy yy_0_1_2;
yy yyv_Expression;
yy yy_0_1_3;
yy yyv_Pos;
yy yy_0_1_4;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 5) goto yyfl_391_5;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yyv_Key = yy_0_1_1;
yyv_Field = yy_0_1_2;
yyv_Expression = yy_0_1_3;
yyv_Pos = yy_0_1_4;
yy_0_2 = yyv_Pos;
*yyout_1 = yy_0_2;
return;
yyfl_391_5 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Key;
yy yy_0_1_1;
yy yyv_Field;
yy yy_0_1_2;
yy yyv_Value;
yy yy_0_1_3;
yy yyv_Pos;
yy yy_0_1_4;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 6) goto yyfl_391_6;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yyv_Key = yy_0_1_1;
yyv_Field = yy_0_1_2;
yyv_Value = yy_0_1_3;
yyv_Pos = yy_0_1_4;
yy_0_2 = yyv_Pos;
*yyout_1 = yy_0_2;
return;
yyfl_391_6 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Predicate;
yy yy_0_1_1;
yy yyv_Pos;
yy yy_0_1_2;
yy yyv_InArgs;
yy yy_0_1_3;
yy yyv_OutArgs;
yy yy_0_1_4;
yy yyv_Offset;
yy yy_0_1_5;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 7) goto yyfl_391_7;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yy_0_1_5 = ((yy)yy_0_1[5]);
yyv_Predicate = yy_0_1_1;
yyv_Pos = yy_0_1_2;
yyv_InArgs = yy_0_1_3;
yyv_OutArgs = yy_0_1_4;
yyv_Offset = yy_0_1_5;
yy_0_2 = yyv_Pos;
*yyout_1 = yy_0_2;
return;
yyfl_391_7 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_S;
yy yy_0_1_1;
yy yyv_Pos;
yy yy_0_1_2;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 8) goto yyfl_391_8;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_S = yy_0_1_1;
yyv_Pos = yy_0_1_2;
yy_0_2 = yyv_Pos;
*yyout_1 = yy_0_2;
return;
yyfl_391_8 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_A;
yy yy_0_1_1;
yy yyv_Pos;
yy yy_0_1_2;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 9) goto yyfl_391_9;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_A = yy_0_1_1;
yyv_Pos = yy_0_1_2;
yy_0_2 = yyv_Pos;
*yyout_1 = yy_0_2;
return;
yyfl_391_9 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Cases;
yy yy_0_1_1;
yy yyv_CommonVars;
yy yy_0_1_2;
yy yyv_Pos;
yy yy_0_1_3;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 10) goto yyfl_391_10;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Cases = yy_0_1_1;
yyv_CommonVars = yy_0_1_2;
yyv_Pos = yy_0_1_3;
yy_0_2 = yyv_Pos;
*yyout_1 = yy_0_2;
return;
yyfl_391_10 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Members;
yy yy_0_1_1;
yy yyv_Pos;
yy yy_0_1_2;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 11) goto yyfl_391_11;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_Members = yy_0_1_1;
yyv_Pos = yy_0_1_2;
yy_0_2 = yyv_Pos;
*yyout_1 = yy_0_2;
return;
yyfl_391_11 : ;
}
yyErr(2,4226);
}
CodeChoiceProcedures()
{
{
yy yyb;
yy yyv_Decls;
yy yy_1;
yy yy_2_1_1_1;
yy yy_2_1_1_2;
yy yy_2_2_2_1;
yy yyv_Types;
yy yy_2_2_3;
yy yy_2_2_4_1;
yy yy_2_2_5_1;
yy_1 = yyglov_choice_Declarations;
if (yy_1 == (yy) yyu) yyErr(1,4271);
yyv_Decls = yy_1;
{
yy yysb = yyb;
yy_2_1_1_1 = yyv_Decls;
yy_2_1_1_2 = yy_2_1_1_1;
if (yy_2_1_1_2[0] != 2) goto yyfl_399_1_2_1;
goto yysl_399_1_2;
yyfl_399_1_2_1 : ;
ChoicePrelude();
yy_2_2_2_1 = yyv_Decls;
SelectChoiceTypes(yy_2_2_2_1);
yy_2_2_3 = yyglov_choice_Types;
if (yy_2_2_3 == (yy) yyu) yyErr(1,4277);
yyv_Types = yy_2_2_3;
yy_2_2_4_1 = yyv_Decls;
CodeChoiceRuleProcs_Decls(yy_2_2_4_1);
yy_2_2_5_1 = yyv_Types;
CodeChoiceTypes(yy_2_2_5_1);
goto yysl_399_1_2;
yysl_399_1_2 : ;
yyb = yysb;
}
return;
}
}
ChoicePrelude()
{
{
yy yyb;
yy yy_1_1;
yy yy_3_1;
yy yy_5_1;
yy yy_7_1;
yy yy_9_1;
yy_1_1 = ((yy)"yy yyDummy;");
s(yy_1_1);
nl();
yy_3_1 = ((yy)"static intptr_t yyInfin = 123456789;");
s(yy_3_1);
nl();
yy_5_1 = ((yy)"extern yy yyAllocCntl();");
s(yy_5_1);
nl();
yy_7_1 = ((yy)"extern yy yySetCntl();");
s(yy_7_1);
nl();
yy_9_1 = ((yy)"typedef struct {yy firstblock; yy curblock; yy curpos;} yysave;");
s(yy_9_1);
nl();
return;
}
}
SelectChoiceTypes(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Head;
yy yy_0_1_1;
yy yyv_Tail;
yy yy_0_1_2;
yy yy_1_1;
yy yy_1_2;
yy yyv_Id;
yy yy_1_2_1;
yy yyv_P;
yy yy_1_2_2;
yy yy_1_2_3;
yy yyv_Name;
yy yy_1_2_3_1;
yy yy_1_2_3_2;
yy yy_1_2_3_3;
yy yyv_InArgs;
yy yy_1_2_3_3_1;
yy yyv_OutArgs;
yy yy_1_2_3_3_2;
yy yyv_Rules;
yy yy_1_2_3_4;
yy yy_2_1_1_1;
yy yy_2_1_1_2;
yy yy_2_1_2_1;
yy yy_2_1_2_2;
yy yy_2_2_1_1;
yy yy_2_2_1_2;
yy yy_2_2_1_2_1;
yy yy_2_2_1_2_1_1;
yy yyv_Type;
yy yy_2_2_1_2_1_2;
yy yy_2_2_1_2_1_3;
yy yy_2_2_1_2_2;
yy yy_2_2_1_2_3;
yy yy_2_2_2_1_1_1;
yy yyv_ArgTypeMeaning;
yy yy_2_2_2_1_1_2;
yy yy_2_2_2_1_2_1_1_1;
yy yy_2_2_2_1_2_1_1_2;
yy yy_2_2_2_1_2_1_1_2_1;
yy yy_2_2_2_1_2_1_1_2_2;
yy yy_2_2_2_1_2_1_1_2_2_1;
yy yy_2_2_2_1_2_1_1_2_2_2;
yy yy_2_2_2_1_2_2_1_1;
yy yy_2_2_2_1_2_2_1_2;
yy yy_2_2_2_1_2_2_1_3;
yy yy_2_2_2_1_2_2_1_4;
yy yy_2_2_2_2_1_1;
yy yy_2_2_2_2_1_2;
yy yy_2_2_2_2_1_3;
yy yy_2_2_2_2_1_4;
yy yy_2_2_3_1;
yy yy_2_2_3_2;
yy yyv_L;
yy yy_2_2_4;
yy yy_2_2_5_1;
yy yy_2_2_5_2;
yy yyv_Index;
yy yy_2_2_5_3;
yy yy_2_2_6_1;
yy yy_2_2_6_2;
yy yy_3_1_1_1;
yy yy_3_1_1_2;
yy yy_3_1_2_1;
yy yy_3_1_2_2;
yy yy_3_1_2_3;
yy yy_3_1_2_4;
yy yy_4_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_401_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_Head = yy_0_1_1;
yyv_Tail = yy_0_1_2;
yy_1_1 = yyv_Head;
yy_1_2 = yy_1_1;
if (yy_1_2[0] != 2) goto yyfl_401_1;
yy_1_2_1 = ((yy)yy_1_2[1]);
yy_1_2_2 = ((yy)yy_1_2[2]);
yy_1_2_3 = ((yy)yy_1_2[3]);
yyv_Id = yy_1_2_1;
yyv_P = yy_1_2_2;
if (yy_1_2_3[0] != 4) goto yyfl_401_1;
yy_1_2_3_1 = ((yy)yy_1_2_3[1]);
yy_1_2_3_2 = ((yy)yy_1_2_3[2]);
yy_1_2_3_3 = ((yy)yy_1_2_3[3]);
yy_1_2_3_4 = ((yy)yy_1_2_3[4]);
yyv_Name = yy_1_2_3_1;
if (yy_1_2_3_2[0] != 5) goto yyfl_401_1;
if (yy_1_2_3_3[0] != 1) goto yyfl_401_1;
yy_1_2_3_3_1 = ((yy)yy_1_2_3_3[1]);
yy_1_2_3_3_2 = ((yy)yy_1_2_3_3[2]);
yyv_InArgs = yy_1_2_3_3_1;
yyv_OutArgs = yy_1_2_3_3_2;
yyv_Rules = yy_1_2_3_4;
{
yy yysb = yyb;
yy_2_1_1_1 = yyv_InArgs;
yy_2_1_1_2 = yy_2_1_1_1;
if (yy_2_1_1_2[0] != 2) goto yyfl_401_1_2_1;
yy_2_1_2_1 = ((yy)"CHOICE predicate requires at least one input argument");
yy_2_1_2_2 = yyv_P;
MESSAGE(yy_2_1_2_1, yy_2_1_2_2);
goto yysl_401_1_2;
yyfl_401_1_2_1 : ;
yy_2_2_1_1 = yyv_InArgs;
yy_2_2_1_2 = yy_2_2_1_1;
if (yy_2_2_1_2[0] != 1) goto yyfl_401_1_2_2;
yy_2_2_1_2_1 = ((yy)yy_2_2_1_2[1]);
yy_2_2_1_2_2 = ((yy)yy_2_2_1_2[2]);
yy_2_2_1_2_3 = ((yy)yy_2_2_1_2[3]);
if (yy_2_2_1_2_1[0] != 1) goto yyfl_401_1_2_2;
yy_2_2_1_2_1_1 = ((yy)yy_2_2_1_2_1[1]);
yy_2_2_1_2_1_2 = ((yy)yy_2_2_1_2_1[2]);
yy_2_2_1_2_1_3 = ((yy)yy_2_2_1_2_1[3]);
yyv_Type = yy_2_2_1_2_1_2;
{
yy yysb = yyb;
yy_2_2_2_1_1_1 = yyv_Type;
if (! GetGlobalMeaning(yy_2_2_2_1_1_1, &yy_2_2_2_1_1_2)) goto yyfl_401_1_2_2_2_1;
yyv_ArgTypeMeaning = yy_2_2_2_1_1_2;
{
yy yysb = yyb;
yy_2_2_2_1_2_1_1_1 = yyv_ArgTypeMeaning;
yy_2_2_2_1_2_1_1_2 = yy_2_2_2_1_2_1_1_1;
if (yy_2_2_2_1_2_1_1_2[0] != 1) goto yyfl_401_1_2_2_2_1_2_1;
yy_2_2_2_1_2_1_1_2_1 = ((yy)yy_2_2_2_1_2_1_1_2[1]);
yy_2_2_2_1_2_1_1_2_2 = ((yy)yy_2_2_2_1_2_1_1_2[2]);
if (yy_2_2_2_1_2_1_1_2_2[0] != 1) goto yyfl_401_1_2_2_2_1_2_1;
yy_2_2_2_1_2_1_1_2_2_1 = ((yy)yy_2_2_2_1_2_1_1_2_2[1]);
yy_2_2_2_1_2_1_1_2_2_2 = ((yy)yy_2_2_2_1_2_1_1_2_2[2]);
goto yysl_401_1_2_2_2_1_2;
yyfl_401_1_2_2_2_1_2_1 : ;
yy_2_2_2_1_2_2_1_1 = ((yy)"'");
yy_2_2_2_1_2_2_1_2 = yyv_Type;
yy_2_2_2_1_2_2_1_3 = ((yy)"' cannot be used as type of primary CHOICE argument");
yy_2_2_2_1_2_2_1_4 = yyv_P;
MESSAGE1(yy_2_2_2_1_2_2_1_1, yy_2_2_2_1_2_2_1_2, yy_2_2_2_1_2_2_1_3, yy_2_2_2_1_2_2_1_4);
goto yysl_401_1_2_2_2_1_2;
yysl_401_1_2_2_2_1_2 : ;
yyb = yysb;
}
goto yysl_401_1_2_2_2;
yyfl_401_1_2_2_2_1 : ;
yy_2_2_2_2_1_1 = ((yy)"'");
yy_2_2_2_2_1_2 = yyv_Type;
yy_2_2_2_2_1_3 = ((yy)"' is not defined as type");
yy_2_2_2_2_1_4 = yyv_P;
MESSAGE1(yy_2_2_2_2_1_1, yy_2_2_2_2_1_2, yy_2_2_2_2_1_3, yy_2_2_2_2_1_4);
goto yysl_401_1_2_2_2;
yysl_401_1_2_2_2 : ;
yyb = yysb;
}
yy_2_2_3_1 = yyv_InArgs;
yy_2_2_3_2 = yyv_OutArgs;
CheckFormalParams(yy_2_2_3_1, yy_2_2_3_2);
yy_2_2_4 = yyglov_choice_Types;
if (yy_2_2_4 == (yy) yyu) yyErr(1,4307);
yyv_L = yy_2_2_4;
yy_2_2_5_1 = yyv_L;
yy_2_2_5_2 = yyv_Type;
LookupChoiceType(yy_2_2_5_1, yy_2_2_5_2, &yy_2_2_5_3);
yyv_Index = yy_2_2_5_3;
yy_2_2_6_1 = yyv_Index;
yy_2_2_6_2 = yyv_Rules;
EnterChoiceRules(yy_2_2_6_1, yy_2_2_6_2);
goto yysl_401_1_2;
yyfl_401_1_2_2 : ;
goto yyfl_401_1;
yysl_401_1_2 : ;
yyb = yysb;
}
{
yy yysb = yyb;
yy_3_1_1_1 = yyv_Rules;
yy_3_1_1_2 = yy_3_1_1_1;
if (yy_3_1_1_2[0] != 2) goto yyfl_401_1_3_1;
yy_3_1_2_1 = ((yy)"no rules for CHOICE predicate '");
yy_3_1_2_2 = yyv_Id;
yy_3_1_2_3 = ((yy)"'");
yy_3_1_2_4 = yyv_P;
MESSAGE1(yy_3_1_2_1, yy_3_1_2_2, yy_3_1_2_3, yy_3_1_2_4);
goto yysl_401_1_3;
yyfl_401_1_3_1 : ;
goto yysl_401_1_3;
yysl_401_1_3 : ;
yyb = yysb;
}
yy_4_1 = yyv_Tail;
SelectChoiceTypes(yy_4_1);
return;
yyfl_401_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_401_2;
return;
yyfl_401_2 : ;
}
yyErr(2,4291);
}
LookupChoiceType(yyin_1, yyin_2, yyout_1)
yy yyin_1;
yy yyin_2;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_T;
yy yy_0_1_1;
yy yyv_I;
yy yy_0_1_2;
yy yyv_Tail;
yy yy_0_1_3;
yy yyv_Type;
yy yy_0_2;
yy yy_0_3;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yy_1_1_2_1;
yy yyv_Index;
yy yy_1_1_2_2;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_1_2_1_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_402_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_T = yy_0_1_1;
yyv_I = yy_0_1_2;
yyv_Tail = yy_0_1_3;
yyv_Type = yy_0_2;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Type;
yy_1_1_1_2 = yyv_T;
if (! yyeq_IDENT(yy_1_1_1_1, yy_1_1_1_2)) goto yyfl_402_1_1_1;
yy_1_1_2_1 = yyv_I;
yy_1_1_2_2 = yy_1_1_2_1;
yyv_Index = yy_1_1_2_2;
goto yysl_402_1_1;
yyfl_402_1_1_1 : ;
yy_1_2_1_1 = yyv_Tail;
yy_1_2_1_2 = yyv_Type;
LookupChoiceType(yy_1_2_1_1, yy_1_2_1_2, &yy_1_2_1_3);
yyv_Index = yy_1_2_1_3;
goto yysl_402_1_1;
yysl_402_1_1 : ;
yyb = yysb;
}
yy_0_3 = yyv_Index;
*yyout_1 = yy_0_3;
return;
yyfl_402_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Type;
yy yy_0_2;
yy yy_0_3;
yy yyv_Index;
yy yy_2;
yy yy_3;
yy yy_4;
yy yyv_L;
yy yy_5;
yy yy_6;
yy yy_6_1;
yy yy_6_2;
yy yy_6_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_402_2;
yyv_Type = yy_0_2;
yyb = yyh;
yyh += 11; if (yyh > yyhx) yyExtend();
yyv_Index = yyb + 0;
yyb[3] = yyu;
yyb[2] = yyu;
yyb[1] = yyu;
yy_2 = yyb + 4;
yy_2[0] = 2;
yyv_Index[1] = (long) yy_2;
yy_3 = yyb + 5;
yy_3[0] = 2;
yyv_Index[2] = (long) yy_3;
yy_4 = yyb + 6;
yy_4[0] = 2;
yyv_Index[3] = (long) yy_4;
yy_5 = yyglov_choice_Types;
if (yy_5 == (yy) yyu) yyErr(1,4333);
yyv_L = yy_5;
yy_6_1 = yyv_Type;
yy_6_2 = yyv_Index;
yy_6_3 = yyv_L;
yy_6 = yyb + 7;
yy_6[0] = 1;
yy_6[1] = ((long)yy_6_1);
yy_6[2] = ((long)yy_6_2);
yy_6[3] = ((long)yy_6_3);
yyglov_choice_Types = yy_6;
yy_0_3 = yyv_Index;
*yyout_1 = yy_0_3;
return;
yyfl_402_2 : ;
}
yyErr(2,4320);
}
EnterChoiceRules(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yyv_TypeIndex;
yy yy_0_1;
yy yy_0_2;
yy yyv_Rule;
yy yy_0_2_1;
yy yy_0_2_1_1;
yy yy_0_2_1_1_1;
yy yyv_Predicate;
yy yy_0_2_1_1_1_1;
yy yyv_PosOfId;
yy yy_0_2_1_1_1_2;
yy yyv_InArgs;
yy yy_0_2_1_1_1_3;
yy yy_0_2_1_1_1_4;
yy yy_0_2_1_1_2;
yy yy_0_2_1_1_3;
yy yyv_Rules;
yy yy_0_2_2;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yyv_Arg;
yy yy_1_1_1_2_1;
yy yy_1_1_1_2_2;
yy yy_1_1_1_2_3;
yy yy_1_1_2_1_1_1;
yy yy_1_1_2_1_1_2;
yy yyv_Tmp;
yy yy_1_1_2_1_1_2_1;
yy yyv_Name;
yy yy_1_1_2_1_1_2_2;
yy yyv_PosOfVar;
yy yy_1_1_2_1_1_2_3;
yy yyv_ChainRules;
yy yy_1_1_2_1_2;
yy yy_1_1_2_1_3;
yy yy_1_1_2_1_3_1;
yy yy_1_1_2_1_3_2;
yy yy_1_1_2_2_1_1;
yy yy_1_1_2_2_1_2;
yy yy_1_1_2_2_1_2_1;
yy yyv_Functor;
yy yy_1_1_2_2_1_2_2;
yy yyv_PosOfFunctor;
yy yy_1_1_2_2_1_2_3;
yy yyv_Args;
yy yy_1_1_2_2_1_2_4;
yy yyv_FunList;
yy yy_1_1_2_2_2;
yy yy_1_1_2_2_3_1_1_1;
yy yy_1_1_2_2_3_1_1_2;
yy yyv_FunctorIndex;
yy yy_1_1_2_2_3_1_1_3;
yy yyv_OldRules;
yy yy_1_1_2_2_3_1_2;
yy yy_1_1_2_2_3_1_3;
yy yy_1_1_2_2_3_1_3_1;
yy yy_1_1_2_2_3_1_3_2;
yy yy_1_1_2_2_3_2_2;
yy yy_1_1_2_2_3_2_2_1;
yy yy_1_1_2_2_3_2_2_2;
yy yyv_FL;
yy yy_1_1_2_2_3_2_3;
yy yy_1_1_2_2_3_2_4;
yy yy_1_1_2_2_3_2_4_1;
yy yy_1_1_2_2_3_2_4_2;
yy yy_1_1_2_2_3_2_4_3;
yy yy_1_1_2_3_1_1;
yy yy_1_1_2_3_1_2;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yyv_PList;
yy yy_2;
yy yy_3_1_1_1;
yy yy_3_1_1_2;
yy yy_3_2_1;
yy yy_3_2_1_1;
yy yy_3_2_1_2;
yy yy_4_1;
yy yy_4_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_TypeIndex = yy_0_1;
if (yy_0_2[0] != 1) goto yyfl_403_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yy_0_2_1_1 = yy_0_2_1;
yyv_Rule = yy_0_2_1;
if (yy_0_2_1_1[0] != 1) goto yyfl_403_1;
yy_0_2_1_1_1 = ((yy)yy_0_2_1_1[1]);
yy_0_2_1_1_2 = ((yy)yy_0_2_1_1[2]);
yy_0_2_1_1_3 = ((yy)yy_0_2_1_1[3]);
if (yy_0_2_1_1_1[0] != 1) goto yyfl_403_1;
yy_0_2_1_1_1_1 = ((yy)yy_0_2_1_1_1[1]);
yy_0_2_1_1_1_2 = ((yy)yy_0_2_1_1_1[2]);
yy_0_2_1_1_1_3 = ((yy)yy_0_2_1_1_1[3]);
yy_0_2_1_1_1_4 = ((yy)yy_0_2_1_1_1[4]);
yyv_Predicate = yy_0_2_1_1_1_1;
yyv_PosOfId = yy_0_2_1_1_1_2;
yyv_InArgs = yy_0_2_1_1_1_3;
yyv_Rules = yy_0_2_2;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_InArgs;
yy_1_1_1_2 = yy_1_1_1_1;
if (yy_1_1_1_2[0] != 1) goto yyfl_403_1_1_1;
yy_1_1_1_2_1 = ((yy)yy_1_1_1_2[1]);
yy_1_1_1_2_2 = ((yy)yy_1_1_1_2[2]);
yy_1_1_1_2_3 = ((yy)yy_1_1_1_2[3]);
yyv_Arg = yy_1_1_1_2_1;
{
yy yysb = yyb;
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yy_1_1_2_1_1_1 = yyv_Arg;
yy_1_1_2_1_1_2 = yy_1_1_2_1_1_1;
if (yy_1_1_2_1_1_2[0] != 3) goto yyfl_403_1_1_1_2_1;
yy_1_1_2_1_1_2_1 = ((yy)yy_1_1_2_1_1_2[1]);
yy_1_1_2_1_1_2_2 = ((yy)yy_1_1_2_1_1_2[2]);
yy_1_1_2_1_1_2_3 = ((yy)yy_1_1_2_1_1_2[3]);
yyv_Tmp = yy_1_1_2_1_1_2_1;
yyv_Name = yy_1_1_2_1_1_2_2;
yyv_PosOfVar = yy_1_1_2_1_1_2_3;
yy_1_1_2_1_2 = (yy) yyv_TypeIndex[3];
if (yy_1_1_2_1_2 == (yy) yyu) yyErr(1,4341);
yyv_ChainRules = yy_1_1_2_1_2;
yy_1_1_2_1_3_1 = yyv_Rule;
yy_1_1_2_1_3_2 = yyv_ChainRules;
yy_1_1_2_1_3 = yyb + 0;
yy_1_1_2_1_3[0] = 1;
yy_1_1_2_1_3[1] = ((long)yy_1_1_2_1_3_1);
yy_1_1_2_1_3[2] = ((long)yy_1_1_2_1_3_2);
yyv_TypeIndex[3] = (long) yy_1_1_2_1_3;
goto yysl_403_1_1_1_2;
yyfl_403_1_1_1_2_1 : ;
yy_1_1_2_2_1_1 = yyv_Arg;
yy_1_1_2_2_1_2 = yy_1_1_2_2_1_1;
if (yy_1_1_2_2_1_2[0] != 1) goto yyfl_403_1_1_1_2_2;
yy_1_1_2_2_1_2_1 = ((yy)yy_1_1_2_2_1_2[1]);
yy_1_1_2_2_1_2_2 = ((yy)yy_1_1_2_2_1_2[2]);
yy_1_1_2_2_1_2_3 = ((yy)yy_1_1_2_2_1_2[3]);
yy_1_1_2_2_1_2_4 = ((yy)yy_1_1_2_2_1_2[4]);
yyv_Tmp = yy_1_1_2_2_1_2_1;
yyv_Functor = yy_1_1_2_2_1_2_2;
yyv_PosOfFunctor = yy_1_1_2_2_1_2_3;
yyv_Args = yy_1_1_2_2_1_2_4;
yy_1_1_2_2_2 = (yy) yyv_TypeIndex[2];
if (yy_1_1_2_2_2 == (yy) yyu) yyErr(1,4345);
yyv_FunList = yy_1_1_2_2_2;
{
yy yysb = yyb;
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yy_1_1_2_2_3_1_1_1 = yyv_Functor;
yy_1_1_2_2_3_1_1_2 = yyv_FunList;
if (! LookupChoiceFunctor(yy_1_1_2_2_3_1_1_1, yy_1_1_2_2_3_1_1_2, &yy_1_1_2_2_3_1_1_3)) goto yyfl_403_1_1_1_2_2_3_1;
yyv_FunctorIndex = yy_1_1_2_2_3_1_1_3;
yy_1_1_2_2_3_1_2 = (yy) yyv_FunctorIndex[1];
if (yy_1_1_2_2_3_1_2 == (yy) yyu) yyErr(1,4347);
yyv_OldRules = yy_1_1_2_2_3_1_2;
yy_1_1_2_2_3_1_3_1 = yyv_Rule;
yy_1_1_2_2_3_1_3_2 = yyv_OldRules;
yy_1_1_2_2_3_1_3 = yyb + 0;
yy_1_1_2_2_3_1_3[0] = 1;
yy_1_1_2_2_3_1_3[1] = ((long)yy_1_1_2_2_3_1_3_1);
yy_1_1_2_2_3_1_3[2] = ((long)yy_1_1_2_2_3_1_3_2);
yyv_FunctorIndex[1] = (long) yy_1_1_2_2_3_1_3;
goto yysl_403_1_1_1_2_2_3;
yyfl_403_1_1_1_2_2_3_1 : ;
yyb = yyh;
yyh += 10; if (yyh > yyhx) yyExtend();
yyv_FunctorIndex = yyb + 0;
yyb[1] = yyu;
yy_1_1_2_2_3_2_2_1 = yyv_Rule;
yy_1_1_2_2_3_2_2_2 = yyb + 5;
yy_1_1_2_2_3_2_2_2[0] = 2;
yy_1_1_2_2_3_2_2 = yyb + 2;
yy_1_1_2_2_3_2_2[0] = 1;
yy_1_1_2_2_3_2_2[1] = ((long)yy_1_1_2_2_3_2_2_1);
yy_1_1_2_2_3_2_2[2] = ((long)yy_1_1_2_2_3_2_2_2);
yyv_FunctorIndex[1] = (long) yy_1_1_2_2_3_2_2;
yy_1_1_2_2_3_2_3 = (yy) yyv_TypeIndex[2];
if (yy_1_1_2_2_3_2_3 == (yy) yyu) yyErr(1,4354);
yyv_FL = yy_1_1_2_2_3_2_3;
yy_1_1_2_2_3_2_4_1 = yyv_Functor;
yy_1_1_2_2_3_2_4_2 = yyv_FunctorIndex;
yy_1_1_2_2_3_2_4_3 = yyv_FL;
yy_1_1_2_2_3_2_4 = yyb + 6;
yy_1_1_2_2_3_2_4[0] = 1;
yy_1_1_2_2_3_2_4[1] = ((long)yy_1_1_2_2_3_2_4_1);
yy_1_1_2_2_3_2_4[2] = ((long)yy_1_1_2_2_3_2_4_2);
yy_1_1_2_2_3_2_4[3] = ((long)yy_1_1_2_2_3_2_4_3);
yyv_TypeIndex[2] = (long) yy_1_1_2_2_3_2_4;
goto yysl_403_1_1_1_2_2_3;
yysl_403_1_1_1_2_2_3 : ;
yyb = yysb;
}
goto yysl_403_1_1_1_2;
yyfl_403_1_1_1_2_2 : ;
yy_1_1_2_3_1_1 = ((yy)"primary CHOICE argument must be term or variable");
yy_1_1_2_3_1_2 = yyv_PosOfId;
MESSAGE(yy_1_1_2_3_1_1, yy_1_1_2_3_1_2);
goto yysl_403_1_1_1_2;
yysl_403_1_1_1_2 : ;
yyb = yysb;
}
goto yysl_403_1_1;
yyfl_403_1_1_1 : ;
yy_1_2_1_1 = ((yy)"invalid number of arguments");
yy_1_2_1_2 = yyv_PosOfId;
MESSAGE(yy_1_2_1_1, yy_1_2_1_2);
goto yysl_403_1_1;
yysl_403_1_1 : ;
yyb = yysb;
}
yy_2 = (yy) yyv_TypeIndex[1];
if (yy_2 == (yy) yyu) yyErr(1,4362);
yyv_PList = yy_2;
{
yy yysb = yyb;
yy_3_1_1_1 = yyv_Predicate;
yy_3_1_1_2 = yyv_PList;
if (! IsInPredicateList(yy_3_1_1_1, yy_3_1_1_2)) goto yyfl_403_1_3_1;
goto yysl_403_1_3;
yyfl_403_1_3_1 : ;
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yy_3_2_1_1 = yyv_Predicate;
yy_3_2_1_2 = yyv_PList;
yy_3_2_1 = yyb + 0;
yy_3_2_1[0] = 1;
yy_3_2_1[1] = ((long)yy_3_2_1_1);
yy_3_2_1[2] = ((long)yy_3_2_1_2);
yyv_TypeIndex[1] = (long) yy_3_2_1;
goto yysl_403_1_3;
yysl_403_1_3 : ;
yyb = yysb;
}
yy_4_1 = yyv_TypeIndex;
yy_4_2 = yyv_Rules;
EnterChoiceRules(yy_4_1, yy_4_2);
return;
yyfl_403_1 : ;
}
{
yy yyb;
yy yyv_Index;
yy yy_0_1;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Index = yy_0_1;
if (yy_0_2[0] != 2) goto yyfl_403_2;
return;
yyfl_403_2 : ;
}
yyErr(2,4336);
}
int LookupChoiceFunctor(yyin_1, yyin_2, yyout_1)
yy yyin_1;
yy yyin_2;
yy *yyout_1;
{
{
yy yyb;
yy yyv_Functor;
yy yy_0_1;
yy yy_0_2;
yy yyv_F;
yy yy_0_2_1;
yy yyv_I;
yy yy_0_2_2;
yy yyv_Tail;
yy yy_0_2_3;
yy yy_0_3;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yy_1_1_2_1;
yy yyv_Index;
yy yy_1_1_2_2;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_1_2_1_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Functor = yy_0_1;
if (yy_0_2[0] != 1) goto yyfl_404_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yy_0_2_3 = ((yy)yy_0_2[3]);
yyv_F = yy_0_2_1;
yyv_I = yy_0_2_2;
yyv_Tail = yy_0_2_3;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Functor;
yy_1_1_1_2 = yyv_F;
if (! yyeq_IDENT(yy_1_1_1_1, yy_1_1_1_2)) goto yyfl_404_1_1_1;
yy_1_1_2_1 = yyv_I;
yy_1_1_2_2 = yy_1_1_2_1;
yyv_Index = yy_1_1_2_2;
goto yysl_404_1_1;
yyfl_404_1_1_1 : ;
yy_1_2_1_1 = yyv_Functor;
yy_1_2_1_2 = yyv_Tail;
if (! LookupChoiceFunctor(yy_1_2_1_1, yy_1_2_1_2, &yy_1_2_1_3)) goto yyfl_404_1_1_2;
yyv_Index = yy_1_2_1_3;
goto yysl_404_1_1;
yyfl_404_1_1_2 : ;
goto yyfl_404_1;
yysl_404_1_1 : ;
yyb = yysb;
}
yy_0_3 = yyv_Index;
*yyout_1 = yy_0_3;
return 1;
yyfl_404_1 : ;
}
return 0;
}
CodeChoiceTypes(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Type;
yy yy_0_1_1;
yy yyv_Index;
yy yy_0_1_2;
yy yyv_Tail;
yy yy_0_1_3;
yy yy_1_1;
yy yy_1_2;
yy yy_2_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_405_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Type = yy_0_1_1;
yyv_Index = yy_0_1_2;
yyv_Tail = yy_0_1_3;
yy_1_1 = yyv_Type;
yy_1_2 = yyv_Index;
CodeChoiceType(yy_1_1, yy_1_2);
yy_2_1 = yyv_Tail;
CodeChoiceTypes(yy_2_1);
return;
yyfl_405_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_405_2;
return;
yyfl_405_2 : ;
}
yyErr(2,4381);
}
CodeChoiceType(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yyv_Type;
yy yy_0_1;
yy yyv_Index;
yy yy_0_2;
yy yy_1_1;
yy yy_2_1;
yy yy_3_1;
yy yy_5_1;
yy yy_7_1;
yy yy_9_1;
yy yy_11_1;
yy yy_13_1;
yy yy_15_1;
yy yy_17;
yy yyv_PredicateList;
yy yy_18;
yy yy_19_1;
yy yy_20_1;
yy yyv_TopFunctors;
yy yy_22;
yy yy_23_1;
yy yy_23_2;
yy yy_23_2_1;
yy yyv_Functors;
yy yy_23_2_2;
yy yy_24_1;
yy yy_24_2;
yy yy_24_3;
yy yy_25_1;
yy yyv_CR;
yy yy_27;
yy yy_28_1_1_1;
yy yy_28_1_1_2;
yy yy_28_2_1_1;
yy yy_28_2_3_1;
yy yy_28_2_5_1;
yy yy_28_2_5_2;
yy yy_28_2_5_3;
yy yy_28_2_6_1;
yy yy_29_1;
yy yy_29_2;
yy yy_30_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Type = yy_0_1;
yyv_Index = yy_0_2;
yy_1_1 = ((yy)"yyTry_");
s(yy_1_1);
yy_2_1 = yyv_Type;
id(yy_2_1);
yy_3_1 = ((yy)"(yyArg, yyRefCntl)");
s(yy_3_1);
nl();
yy_5_1 = ((yy)"yy yyArg; yy *yyRefCntl;");
s(yy_5_1);
nl();
yy_7_1 = ((yy)"{");
s(yy_7_1);
nl();
yy_9_1 = ((yy)"yy yyCntl;");
s(yy_9_1);
nl();
yy_11_1 = ((yy)"yy yyCntlPtr;");
s(yy_11_1);
nl();
yy_13_1 = ((yy)"intptr_t yycost;");
s(yy_13_1);
nl();
yy_15_1 = ((yy)"intptr_t yyapplied;");
s(yy_15_1);
nl();
yy_17 = yyv_Index;
yyglov_current_choice_type = yy_17;
yy_18 = (yy) yyv_Index[1];
if (yy_18 == (yy) yyu) yyErr(1,4399);
yyv_PredicateList = yy_18;
yy_19_1 = yyv_PredicateList;
CodeInitCntl(yy_19_1);
yy_20_1 = ((yy)"switch(yyArg[0]){");
s(yy_20_1);
nl();
yy_22 = (yy) yyv_Index[2];
if (yy_22 == (yy) yyu) yyErr(1,4402);
yyv_TopFunctors = yy_22;
yy_23_1 = yyv_Type;
if (! GetGlobalMeaning(yy_23_1, &yy_23_2)) goto yyfl_407_1;
if (yy_23_2[0] != 1) goto yyfl_407_1;
yy_23_2_1 = ((yy)yy_23_2[1]);
yy_23_2_2 = ((yy)yy_23_2[2]);
yyv_Functors = yy_23_2_2;
yy_24_1 = yyv_Functors;
yy_24_2 = yyv_TopFunctors;
yy_24_3 = yyv_Type;
CodeNonTopFunctorCases(yy_24_1, yy_24_2, yy_24_3);
yy_25_1 = ((yy)"}");
s(yy_25_1);
nl();
yy_27 = (yy) yyv_Index[3];
if (yy_27 == (yy) yyu) yyErr(1,4406);
yyv_CR = yy_27;
{
yy yysb = yyb;
yy_28_1_1_1 = yyv_CR;
yy_28_1_1_2 = yy_28_1_1_1;
if (yy_28_1_1_2[0] != 2) goto yyfl_407_1_28_1;
goto yysl_407_1_28;
yyfl_407_1_28_1 : ;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_28_2_1_1 = ((yy)"do {");
s(yy_28_2_1_1);
nl();
yy_28_2_3_1 = ((yy)"yyapplied = 0;");
s(yy_28_2_3_1);
nl();
yy_28_2_5_1 = yyv_Type;
yy_28_2_5_2 = yyv_CR;
yy_28_2_5_3 = yyb + 0;
yy_28_2_5_3[0] = 1;
CodeTryRules(yy_28_2_5_1, yy_28_2_5_2, yy_28_2_5_3);
yy_28_2_6_1 = ((yy)"} while(yyapplied);");
s(yy_28_2_6_1);
nl();
goto yysl_407_1_28;
yysl_407_1_28 : ;
yyb = yysb;
}
yy_29_1 = yyv_PredicateList;
yy_29_2 = ((yy)0);
CodeBuildCntlPredicateInfo(yy_29_1, yy_29_2);
yy_30_1 = ((yy)"}");
s(yy_30_1);
nl();
return;
yyfl_407_1 : ;
}
yyErr(2,4389);
}
CodeBuildCntl()
{
{
yy yyb;
yy yyv_Index;
yy yy_1;
yy yyv_Predicates;
yy yy_2;
yy yy_3_1;
yy yy_3_2;
yy_1 = yyglov_current_choice_type;
if (yy_1 == (yy) yyu) yyErr(1,4419);
yyv_Index = yy_1;
yy_2 = (yy) yyv_Index[1];
if (yy_2 == (yy) yyu) yyErr(1,4420);
yyv_Predicates = yy_2;
yy_3_1 = yyv_Predicates;
yy_3_2 = ((yy)1);
CodeBuildCntlPredicateInfo(yy_3_1, yy_3_2);
return;
}
}
LengthPredicateList(yyin_1, yyout_1)
yy yyin_1;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Head;
yy yy_0_1_1;
yy yyv_Tail;
yy yy_0_1_2;
yy yy_0_2;
yy yy_0_2_1;
yy yy_0_2_2;
yy yy_1_1;
yy yyv_N;
yy yy_1_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_409_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_Head = yy_0_1_1;
yyv_Tail = yy_0_1_2;
yy_1_1 = yyv_Tail;
LengthPredicateList(yy_1_1, &yy_1_2);
yyv_N = yy_1_2;
yy_0_2_1 = yyv_N;
yy_0_2_2 = ((yy)1);
yy_0_2 = (yy)(((long)yy_0_2_1)+((long)yy_0_2_2));
*yyout_1 = yy_0_2;
return;
yyfl_409_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_409_2;
yy_0_2 = ((yy)0);
*yyout_1 = yy_0_2;
return;
yyfl_409_2 : ;
}
yyErr(2,4423);
}
CodeBuildCntlPredicateInfo(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Predicate;
yy yy_0_1_1;
yy yyv_Tail;
yy yy_0_1_2;
yy yyv_Offset;
yy yy_0_2;
yy yy_1_1;
yy yy_2_1;
yy yy_3_1;
yy yy_4_1;
yy yy_5_1;
yy yy_7_1;
yy yy_8_1;
yy yy_8_1_1;
yy yy_8_1_2;
yy yy_9_1;
yy yy_10_1;
yy yy_11_1;
yy yy_13_1;
yy yy_13_2;
yy yy_13_2_1;
yy yy_13_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_410_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_Predicate = yy_0_1_1;
yyv_Tail = yy_0_1_2;
yyv_Offset = yy_0_2;
yy_1_1 = ((yy)"yyCntl[");
s(yy_1_1);
yy_2_1 = yyv_Offset;
i(yy_2_1);
yy_3_1 = ((yy)"] = yyCostFor_");
s(yy_3_1);
yy_4_1 = yyv_Predicate;
id(yy_4_1);
yy_5_1 = ((yy)";");
s(yy_5_1);
nl();
yy_7_1 = ((yy)"yyCntl[");
s(yy_7_1);
yy_8_1_1 = yyv_Offset;
yy_8_1_2 = ((yy)1);
yy_8_1 = (yy)(((long)yy_8_1_1)+((long)yy_8_1_2));
i(yy_8_1);
yy_9_1 = ((yy)"] = yyRuleFor_");
s(yy_9_1);
yy_10_1 = yyv_Predicate;
id(yy_10_1);
yy_11_1 = ((yy)";");
s(yy_11_1);
nl();
yy_13_1 = yyv_Tail;
yy_13_2_1 = yyv_Offset;
yy_13_2_2 = ((yy)2);
yy_13_2 = (yy)(((long)yy_13_2_1)+((long)yy_13_2_2));
CodeBuildCntlPredicateInfo(yy_13_1, yy_13_2);
return;
yyfl_410_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_410_2;
return;
yyfl_410_2 : ;
}
yyErr(2,4428);
}
CodeBuildCntlSons(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_Tail;
yy yy_0_1_3;
yy yyv_N;
yy yy_0_2;
yy yy_1_1;
yy yy_2_1;
yy yy_3_1;
yy yy_4_1;
yy yy_5_1;
yy yy_7_1;
yy yy_7_2;
yy yy_7_2_1;
yy yy_7_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_411_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Tail = yy_0_1_3;
yyv_N = yy_0_2;
yy_1_1 = ((yy)"yyCntl[-");
s(yy_1_1);
yy_2_1 = yyv_N;
i(yy_2_1);
yy_3_1 = ((yy)"] = (intptr_t) yyCntl_");
s(yy_3_1);
yy_4_1 = yyv_N;
i(yy_4_1);
yy_5_1 = ((yy)";");
s(yy_5_1);
nl();
yy_7_1 = yyv_Tail;
yy_7_2_1 = yyv_N;
yy_7_2_2 = ((yy)1);
yy_7_2 = (yy)(((long)yy_7_2_1)+((long)yy_7_2_2));
CodeBuildCntlSons(yy_7_1, yy_7_2);
return;
yyfl_411_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_411_2;
return;
yyfl_411_2 : ;
}
yyErr(2,4435);
}
int IsInPredicateList(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yy_0_2;
yy yyv_Id2;
yy yy_0_2_1;
yy yyv_Tail;
yy yy_0_2_2;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Id = yy_0_1;
if (yy_0_2[0] != 1) goto yyfl_412_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yyv_Id2 = yy_0_2_1;
yyv_Tail = yy_0_2_2;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Id;
yy_1_1_1_2 = yyv_Id2;
if (! yyeq_IDENT(yy_1_1_1_1, yy_1_1_1_2)) goto yyfl_412_1_1_1;
goto yysl_412_1_1;
yyfl_412_1_1_1 : ;
yy_1_2_1_1 = yyv_Id;
yy_1_2_1_2 = yyv_Tail;
if (! IsInPredicateList(yy_1_2_1_1, yy_1_2_1_2)) goto yyfl_412_1_1_2;
goto yysl_412_1_1;
yyfl_412_1_1_2 : ;
goto yyfl_412_1;
yysl_412_1_1 : ;
yyb = yysb;
}
return 1;
yyfl_412_1 : ;
}
return 0;
}
CodeInitCntl(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Predicate;
yy yy_0_1_1;
yy yyv_Tail;
yy yy_0_1_2;
yy yy_1_1;
yy yy_2_1;
yy yy_3_1;
yy yy_5_1;
yy yy_6_1;
yy yy_7_1;
yy yy_9_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_414_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_Predicate = yy_0_1_1;
yyv_Tail = yy_0_1_2;
yy_1_1 = ((yy)"intptr_t yyRuleFor_");
s(yy_1_1);
yy_2_1 = yyv_Predicate;
id(yy_2_1);
yy_3_1 = ((yy)" = 0;");
s(yy_3_1);
nl();
yy_5_1 = ((yy)"intptr_t yyCostFor_");
s(yy_5_1);
yy_6_1 = yyv_Predicate;
id(yy_6_1);
yy_7_1 = ((yy)" = yyInfin;");
s(yy_7_1);
nl();
yy_9_1 = yyv_Tail;
CodeInitCntl(yy_9_1);
return;
yyfl_414_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_414_2;
return;
yyfl_414_2 : ;
}
yyErr(2,4451);
}
CodeTopFunctorCases(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Functor;
yy yy_0_1_1;
yy yyv_Index;
yy yy_0_1_2;
yy yyv_Tail;
yy yy_0_1_3;
yy yyv_Type;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy yyv_Code;
yy yy_1_3;
yy yyv_ArgSpecList;
yy yy_1_4;
yy yy_2_1;
yy yy_3_1;
yy yy_4_1;
yy yy_6_1;
yy yy_8_1;
yy yy_8_2;
yy yy_9_1;
yy yy_9_2;
yy yy_10_1;
yy yyv_NSons;
yy yy_10_2;
yy yyv_TypeIndex;
yy yy_11;
yy yyv_Predicates;
yy yy_12;
yy yy_13_1;
yy yyv_NPred;
yy yy_13_2;
yy yy_14_1;
yy yy_15_1;
yy yy_15_1_1;
yy yy_15_1_1_1;
yy yy_15_1_1_2;
yy yy_15_1_2;
yy yy_16_1;
yy yy_18_1;
yy yy_19_1;
yy yy_20_1;
yy yy_22_1;
yy yy_24_1;
yy yy_24_2;
yy yyv_R;
yy yy_25;
yy yy_26_1;
yy yy_26_2;
yy yy_26_3;
yy yy_27_1;
yy yy_29_1;
yy yy_31_1;
yy yy_31_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_415_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Functor = yy_0_1_1;
yyv_Index = yy_0_1_2;
yyv_Tail = yy_0_1_3;
yyv_Type = yy_0_2;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_Functor;
yy_1_2 = yyv_Type;
GetFunctorCode(yy_1_1, yy_1_2, &yy_1_3, &yy_1_4);
yyv_Code = yy_1_3;
yyv_ArgSpecList = yy_1_4;
yy_2_1 = ((yy)"case ");
s(yy_2_1);
yy_3_1 = yyv_Code;
i(yy_3_1);
yy_4_1 = ((yy)":");
s(yy_4_1);
nl();
yy_6_1 = ((yy)"{");
s(yy_6_1);
nl();
yy_8_1 = yyv_ArgSpecList;
yy_8_2 = ((yy)1);
DeclareSonCntl(yy_8_1, yy_8_2);
yy_9_1 = yyv_ArgSpecList;
yy_9_2 = ((yy)1);
CodeTrySons(yy_9_1, yy_9_2);
yy_10_1 = yyv_ArgSpecList;
LengthArgSpecList(yy_10_1, &yy_10_2);
yyv_NSons = yy_10_2;
yy_11 = yyglov_current_choice_type;
if (yy_11 == (yy) yyu) yyErr(1,4466);
yyv_TypeIndex = yy_11;
yy_12 = (yy) yyv_TypeIndex[1];
if (yy_12 == (yy) yyu) yyErr(1,4467);
yyv_Predicates = yy_12;
yy_13_1 = yyv_Predicates;
LengthPredicateList(yy_13_1, &yy_13_2);
yyv_NPred = yy_13_2;
yy_14_1 = ((yy)"yyCntlPtr = yyAllocCntl(");
s(yy_14_1);
yy_15_1_1_1 = ((yy)2);
yy_15_1_1_2 = yyv_NPred;
yy_15_1_1 = (yy)(((long)yy_15_1_1_1)*((long)yy_15_1_1_2));
yy_15_1_2 = yyv_NSons;
yy_15_1 = (yy)(((long)yy_15_1_1)+((long)yy_15_1_2));
i(yy_15_1);
yy_16_1 = ((yy)");");
s(yy_16_1);
nl();
yy_18_1 = ((yy)"yyCntl = yyCntlPtr+");
s(yy_18_1);
yy_19_1 = yyv_NSons;
i(yy_19_1);
yy_20_1 = ((yy)";");
s(yy_20_1);
nl();
yy_22_1 = ((yy)"*yyRefCntl = yyCntl;");
s(yy_22_1);
nl();
yy_24_1 = yyv_ArgSpecList;
yy_24_2 = ((yy)1);
CodeBuildCntlSons(yy_24_1, yy_24_2);
yy_25 = (yy) yyv_Index[1];
if (yy_25 == (yy) yyu) yyErr(1,4474);
yyv_R = yy_25;
yy_26_1 = yyv_Type;
yy_26_2 = yyv_R;
yy_26_3 = yyb + 0;
yy_26_3[0] = 2;
CodeTryRules(yy_26_1, yy_26_2, yy_26_3);
yy_27_1 = ((yy)"}");
s(yy_27_1);
nl();
yy_29_1 = ((yy)"break;");
s(yy_29_1);
nl();
yy_31_1 = yyv_Tail;
yy_31_2 = yyv_Type;
CodeTopFunctorCases(yy_31_1, yy_31_2);
return;
yyfl_415_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Type;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_415_2;
yyv_Type = yy_0_2;
return;
yyfl_415_2 : ;
}
yyErr(2,4458);
}
CodeNonTopFunctorCases(yyin_1, yyin_2, yyin_3)
yy yyin_1;
yy yyin_2;
yy yyin_3;
{
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_Functor;
yy yy_0_1_1_1;
yy yy_0_1_1_2;
yy yy_0_1_1_3;
yy yyv_Tail;
yy yy_0_1_2;
yy yyv_TopF;
yy yy_0_2;
yy yyv_Type;
yy yy_0_3;
yy yy_1_1;
yy yy_1_2;
yy yyv_Code;
yy yy_1_3;
yy yyv_ArgSpecList;
yy yy_1_4;
yy yy_2_1;
yy yy_3_1;
yy yy_4_1;
yy yy_6_1;
yy yy_8_1;
yy yy_8_2;
yy yy_9_1;
yy yy_9_2;
yy yy_10_1;
yy yyv_NSons;
yy yy_10_2;
yy yyv_TypeIndex;
yy yy_11;
yy yyv_Predicates;
yy yy_12;
yy yy_13_1;
yy yyv_NPred;
yy yy_13_2;
yy yy_14_1;
yy yy_15_1;
yy yy_15_1_1;
yy yy_15_1_1_1;
yy yy_15_1_1_2;
yy yy_15_1_2;
yy yy_16_1;
yy yy_18_1;
yy yy_19_1;
yy yy_20_1;
yy yy_22_1;
yy yy_24_1;
yy yy_24_2;
yy yy_25_1_1_1;
yy yy_25_1_1_2;
yy yyv_Index;
yy yy_25_1_1_3;
yy yyv_R;
yy yy_25_1_2;
yy yy_25_1_3_1;
yy yy_25_1_3_2;
yy yy_25_1_3_3;
yy yy_26_1;
yy yy_28_1;
yy yy_30_1;
yy yy_30_2;
yy yy_30_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 1) goto yyfl_416_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
if (yy_0_1_1[0] != 1) goto yyfl_416_1;
yy_0_1_1_1 = ((yy)yy_0_1_1[1]);
yy_0_1_1_2 = ((yy)yy_0_1_1[2]);
yy_0_1_1_3 = ((yy)yy_0_1_1[3]);
yyv_Functor = yy_0_1_1_1;
yyv_Tail = yy_0_1_2;
yyv_TopF = yy_0_2;
yyv_Type = yy_0_3;
yy_1_1 = yyv_Functor;
yy_1_2 = yyv_Type;
GetFunctorCode(yy_1_1, yy_1_2, &yy_1_3, &yy_1_4);
yyv_Code = yy_1_3;
yyv_ArgSpecList = yy_1_4;
yy_2_1 = ((yy)"case ");
s(yy_2_1);
yy_3_1 = yyv_Code;
i(yy_3_1);
yy_4_1 = ((yy)":");
s(yy_4_1);
nl();
yy_6_1 = ((yy)"{");
s(yy_6_1);
nl();
yy_8_1 = yyv_ArgSpecList;
yy_8_2 = ((yy)1);
DeclareSonCntl(yy_8_1, yy_8_2);
yy_9_1 = yyv_ArgSpecList;
yy_9_2 = ((yy)1);
CodeTrySons(yy_9_1, yy_9_2);
yy_10_1 = yyv_ArgSpecList;
LengthArgSpecList(yy_10_1, &yy_10_2);
yyv_NSons = yy_10_2;
yy_11 = yyglov_current_choice_type;
if (yy_11 == (yy) yyu) yyErr(1,4489);
yyv_TypeIndex = yy_11;
yy_12 = (yy) yyv_TypeIndex[1];
if (yy_12 == (yy) yyu) yyErr(1,4490);
yyv_Predicates = yy_12;
yy_13_1 = yyv_Predicates;
LengthPredicateList(yy_13_1, &yy_13_2);
yyv_NPred = yy_13_2;
yy_14_1 = ((yy)"yyCntlPtr = yyAllocCntl(");
s(yy_14_1);
yy_15_1_1_1 = ((yy)2);
yy_15_1_1_2 = yyv_NPred;
yy_15_1_1 = (yy)(((long)yy_15_1_1_1)*((long)yy_15_1_1_2));
yy_15_1_2 = yyv_NSons;
yy_15_1 = (yy)(((long)yy_15_1_1)+((long)yy_15_1_2));
i(yy_15_1);
yy_16_1 = ((yy)");");
s(yy_16_1);
nl();
yy_18_1 = ((yy)"yyCntl = yyCntlPtr+");
s(yy_18_1);
yy_19_1 = yyv_NSons;
i(yy_19_1);
yy_20_1 = ((yy)";");
s(yy_20_1);
nl();
yy_22_1 = ((yy)"*yyRefCntl = yyCntl;");
s(yy_22_1);
nl();
yy_24_1 = yyv_ArgSpecList;
yy_24_2 = ((yy)1);
CodeBuildCntlSons(yy_24_1, yy_24_2);
{
yy yysb = yyb;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_25_1_1_1 = yyv_Functor;
yy_25_1_1_2 = yyv_TopF;
if (! IsInChoiceFunctorList(yy_25_1_1_1, yy_25_1_1_2, &yy_25_1_1_3)) goto yyfl_416_1_25_1;
yyv_Index = yy_25_1_1_3;
yy_25_1_2 = (yy) yyv_Index[1];
if (yy_25_1_2 == (yy) yyu) yyErr(1,4498);
yyv_R = yy_25_1_2;
yy_25_1_3_1 = yyv_Type;
yy_25_1_3_2 = yyv_R;
yy_25_1_3_3 = yyb + 0;
yy_25_1_3_3[0] = 2;
CodeTryRules(yy_25_1_3_1, yy_25_1_3_2, yy_25_1_3_3);
goto yysl_416_1_25;
yyfl_416_1_25_1 : ;
goto yysl_416_1_25;
yysl_416_1_25 : ;
yyb = yysb;
}
yy_26_1 = ((yy)"}");
s(yy_26_1);
nl();
yy_28_1 = ((yy)"break;");
s(yy_28_1);
nl();
yy_30_1 = yyv_Tail;
yy_30_2 = yyv_TopF;
yy_30_3 = yyv_Type;
CodeNonTopFunctorCases(yy_30_1, yy_30_2, yy_30_3);
return;
yyfl_416_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_0_2;
yy yy_0_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 2) goto yyfl_416_2;
return;
yyfl_416_2 : ;
}
yyErr(2,4481);
}
int IsInChoiceFunctorList(yyin_1, yyin_2, yyout_1)
yy yyin_1;
yy yyin_2;
yy *yyout_1;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yy_0_2;
yy yyv_Id2;
yy yy_0_2_1;
yy yyv_X;
yy yy_0_2_2;
yy yyv_Tail;
yy yy_0_2_3;
yy yy_0_3;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yy_1_1_2_1;
yy yyv_Index;
yy yy_1_1_2_2;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_1_2_1_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Id = yy_0_1;
if (yy_0_2[0] != 1) goto yyfl_417_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yy_0_2_3 = ((yy)yy_0_2[3]);
yyv_Id2 = yy_0_2_1;
yyv_X = yy_0_2_2;
yyv_Tail = yy_0_2_3;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Id;
yy_1_1_1_2 = yyv_Id2;
if (! yyeq_IDENT(yy_1_1_1_1, yy_1_1_1_2)) goto yyfl_417_1_1_1;
yy_1_1_2_1 = yyv_X;
yy_1_1_2_2 = yy_1_1_2_1;
yyv_Index = yy_1_1_2_2;
goto yysl_417_1_1;
yyfl_417_1_1_1 : ;
yy_1_2_1_1 = yyv_Id;
yy_1_2_1_2 = yyv_Tail;
if (! IsInChoiceFunctorList(yy_1_2_1_1, yy_1_2_1_2, &yy_1_2_1_3)) goto yyfl_417_1_1_2;
yyv_Index = yy_1_2_1_3;
goto yysl_417_1_1;
yyfl_417_1_1_2 : ;
goto yyfl_417_1;
yysl_417_1_1 : ;
yyb = yysb;
}
yy_0_3 = yyv_Index;
*yyout_1 = yy_0_3;
return 1;
yyfl_417_1 : ;
}
return 0;
}
LengthArgSpecList(yyin_1, yyout_1)
yy yyin_1;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_Tail;
yy yy_0_1_3;
yy yy_0_2;
yy yy_0_2_1;
yy yy_0_2_2;
yy yy_1_1;
yy yyv_N;
yy yy_1_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_418_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Tail = yy_0_1_3;
yy_1_1 = yyv_Tail;
LengthArgSpecList(yy_1_1, &yy_1_2);
yyv_N = yy_1_2;
yy_0_2_1 = yyv_N;
yy_0_2_2 = ((yy)1);
yy_0_2 = (yy)(((long)yy_0_2_1)+((long)yy_0_2_2));
*yyout_1 = yy_0_2;
return;
yyfl_418_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_0_2;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_418_2;
yy_0_2 = ((yy)0);
*yyout_1 = yy_0_2;
return;
yyfl_418_2 : ;
}
yyErr(2,4515);
}
DeclareSonCntl(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_1_1;
yy yy_0_1_1_2;
yy yy_0_1_1_3;
yy yy_0_1_2;
yy yyv_Tail;
yy yy_0_1_3;
yy yyv_N;
yy yy_0_2;
yy yy_1_1;
yy yy_2_1;
yy yy_3_1;
yy yy_5_1;
yy yy_5_2;
yy yy_5_2_1;
yy yy_5_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_419_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
if (yy_0_1_1[0] != 1) goto yyfl_419_1;
yy_0_1_1_1 = ((yy)yy_0_1_1[1]);
yy_0_1_1_2 = ((yy)yy_0_1_1[2]);
yy_0_1_1_3 = ((yy)yy_0_1_1[3]);
yyv_Tail = yy_0_1_3;
yyv_N = yy_0_2;
yy_1_1 = ((yy)"yy yyCntl_");
s(yy_1_1);
yy_2_1 = yyv_N;
i(yy_2_1);
yy_3_1 = ((yy)";");
s(yy_3_1);
nl();
yy_5_1 = yyv_Tail;
yy_5_2_1 = yyv_N;
yy_5_2_2 = ((yy)1);
yy_5_2 = (yy)(((long)yy_5_2_1)+((long)yy_5_2_2));
DeclareSonCntl(yy_5_1, yy_5_2);
return;
yyfl_419_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_N;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_419_2;
yyv_N = yy_0_2;
return;
yyfl_419_2 : ;
}
yyErr(2,4520);
}
CodeTrySons(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_1_1_1;
yy yyv_Type;
yy yy_0_1_1_2;
yy yy_0_1_1_3;
yy yy_0_1_2;
yy yyv_Tail;
yy yy_0_1_3;
yy yyv_N;
yy yy_0_2;
yy yyv_List;
yy yy_1;
yy yy_2_1_1_1;
yy yy_2_1_1_2;
yy yy_2_1_2_1;
yy yy_2_1_3_1;
yy yy_2_1_4_1;
yy yy_2_1_5_1;
yy yy_2_1_6_1;
yy yy_2_1_7_1;
yy yy_2_1_8_1;
yy yy_3_1;
yy yy_3_2;
yy yy_3_2_1;
yy yy_3_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_420_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
if (yy_0_1_1[0] != 1) goto yyfl_420_1;
yy_0_1_1_1 = ((yy)yy_0_1_1[1]);
yy_0_1_1_2 = ((yy)yy_0_1_1[2]);
yy_0_1_1_3 = ((yy)yy_0_1_1[3]);
yyv_Type = yy_0_1_1_2;
yyv_Tail = yy_0_1_3;
yyv_N = yy_0_2;
yy_1 = yyglov_choice_Types;
if (yy_1 == (yy) yyu) yyErr(1,4528);
yyv_List = yy_1;
{
yy yysb = yyb;
yy_2_1_1_1 = yyv_Type;
yy_2_1_1_2 = yyv_List;
if (! IsInChoiceTypeList(yy_2_1_1_1, yy_2_1_1_2)) goto yyfl_420_1_2_1;
yy_2_1_2_1 = ((yy)"yyTry_");
s(yy_2_1_2_1);
yy_2_1_3_1 = yyv_Type;
id(yy_2_1_3_1);
yy_2_1_4_1 = ((yy)"(yyArg[");
s(yy_2_1_4_1);
yy_2_1_5_1 = yyv_N;
i(yy_2_1_5_1);
yy_2_1_6_1 = ((yy)"], &yyCntl_");
s(yy_2_1_6_1);
yy_2_1_7_1 = yyv_N;
i(yy_2_1_7_1);
yy_2_1_8_1 = ((yy)");");
s(yy_2_1_8_1);
nl();
goto yysl_420_1_2;
yyfl_420_1_2_1 : ;
goto yysl_420_1_2;
yysl_420_1_2 : ;
yyb = yysb;
}
yy_3_1 = yyv_Tail;
yy_3_2_1 = yyv_N;
yy_3_2_2 = ((yy)1);
yy_3_2 = (yy)(((long)yy_3_2_1)+((long)yy_3_2_2));
CodeTrySons(yy_3_1, yy_3_2);
return;
yyfl_420_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_N;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_420_2;
yyv_N = yy_0_2;
return;
yyfl_420_2 : ;
}
yyErr(2,4526);
}
CodeTryRules(yyin_1, yyin_2, yyin_3)
yy yyin_1;
yy yyin_2;
yy yyin_3;
{
{
yy yyb;
yy yyv_Type;
yy yy_0_1;
yy yy_0_2;
yy yyv_H;
yy yy_0_2_1;
yy yyv_T;
yy yy_0_2_2;
yy yyv_ChainRule;
yy yy_0_3;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yy_2_1;
yy yy_2_2;
yy yy_2_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Type = yy_0_1;
if (yy_0_2[0] != 1) goto yyfl_421_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yyv_H = yy_0_2_1;
yyv_T = yy_0_2_2;
yyv_ChainRule = yy_0_3;
yy_1_1 = yyv_Type;
yy_1_2 = yyv_T;
yy_1_3 = yyv_ChainRule;
CodeTryRules(yy_1_1, yy_1_2, yy_1_3);
yy_2_1 = yyv_Type;
yy_2_2 = yyv_H;
yy_2_3 = yyv_ChainRule;
CodeTryRule(yy_2_1, yy_2_2, yy_2_3);
return;
yyfl_421_1 : ;
}
{
yy yyb;
yy yyv_Type;
yy yy_0_1;
yy yyv_H;
yy yy_0_2;
yy yyv_ChainRule;
yy yy_0_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Type = yy_0_1;
yyv_H = yy_0_2;
yyv_ChainRule = yy_0_3;
return;
yyfl_421_2 : ;
}
yyErr(2,4537);
}
CodeTryRule(yyin_1, yyin_2, yyin_3)
yy yyin_1;
yy yyin_2;
yy yyin_3;
{
{
yy yyb;
yy yyv_Type;
yy yy_0_1;
yy yy_0_2;
yy yy_0_2_1;
yy yyv_Predicate;
yy yy_0_2_1_1;
yy yyv_Pos;
yy yy_0_2_1_2;
yy yyv_In;
yy yy_0_2_1_3;
yy yyv_Out;
yy yy_0_2_1_4;
yy yyv_Rhs;
yy yy_0_2_2;
yy yyv_Cost;
yy yy_0_2_3;
yy yyv_ChainRule;
yy yy_0_3;
yy yy_2_1;
yy yyv_Line;
yy yy_2_2;
yy yy_3;
yy yy_4_1;
yy yy_6_1;
yy yy_6_2;
yy yyv_Arg;
yy yy_6_2_1;
yy yy_6_2_2;
yy yy_6_2_3;
yy yy_7_1;
yy yy_7_2;
yy yy_7_2_1;
yy yy_7_2_2;
yy yy_7_2_3;
yy yy_7_3;
yy yy_7_4;
yy yyv_Pathes;
yy yy_7_5;
yy yy_8_1;
yy yy_8_2;
yy yyv_MemberList;
yy yy_8_2_1;
yy yyv_Space;
yy yy_8_2_2;
yy yy_9_1;
yy yy_9_2;
yy yy_9_3;
yy yyv_ChoiceMembers;
yy yy_9_4;
yy yy_10_1;
yy yyv_LineOfRule;
yy yy_10_2;
yy yy_11_1;
yy yy_11_2;
yy yy_11_3;
yy yy_11_4;
yy yy_11_5;
yy yyv_TryLabel;
yy yy_12;
yy yy_13_1;
yy yy_14_1;
yy yy_15_1;
yy yy_17_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Type = yy_0_1;
if (yy_0_2[0] != 1) goto yyfl_423_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yy_0_2_3 = ((yy)yy_0_2[3]);
if (yy_0_2_1[0] != 1) goto yyfl_423_1;
yy_0_2_1_1 = ((yy)yy_0_2_1[1]);
yy_0_2_1_2 = ((yy)yy_0_2_1[2]);
yy_0_2_1_3 = ((yy)yy_0_2_1[3]);
yy_0_2_1_4 = ((yy)yy_0_2_1[4]);
yyv_Predicate = yy_0_2_1_1;
yyv_Pos = yy_0_2_1_2;
yyv_In = yy_0_2_1_3;
yyv_Out = yy_0_2_1_4;
yyv_Rhs = yy_0_2_2;
yyv_Cost = yy_0_2_3;
yyv_ChainRule = yy_0_3;
yyb = yyh;
yyh += 9; if (yyh > yyhx) yyExtend();
BeginLocalScope();
yy_2_1 = yyv_Pos;
PosToLineNumber(yy_2_1, &yy_2_2);
yyv_Line = yy_2_2;
yy_3 = yyv_Line;
yyglov_tryLabel = yy_3;
yy_4_1 = ((yy)"{");
s(yy_4_1);
nl();
yy_6_1 = yyv_In;
yy_6_2 = yy_6_1;
if (yy_6_2[0] != 1) goto yyfl_423_1;
yy_6_2_1 = ((yy)yy_6_2[1]);
yy_6_2_2 = ((yy)yy_6_2[2]);
yy_6_2_3 = ((yy)yy_6_2[3]);
yyv_Arg = yy_6_2_1;
yy_7_1 = yyv_Arg;
yy_7_2_1 = yyb + 4;
yy_7_2_1[0] = 2;
yy_7_2_2 = yyv_Type;
yy_7_2_3 = yyb + 5;
yy_7_2_3[0] = 2;
yy_7_2 = yyb + 0;
yy_7_2[0] = 1;
yy_7_2[1] = ((long)yy_7_2_1);
yy_7_2[2] = ((long)yy_7_2_2);
yy_7_2[3] = ((long)yy_7_2_3);
yy_7_3 = yyb + 6;
yy_7_3[0] = 2;
yy_7_4 = yyb + 7;
yy_7_4[0] = 2;
GetPathes(yy_7_1, yy_7_2, yy_7_3, yy_7_4, &yy_7_5);
yyv_Pathes = yy_7_5;
yy_8_1 = yyv_Rhs;
yy_8_2 = yy_8_1;
if (yy_8_2[0] != 1) goto yyfl_423_1;
yy_8_2_1 = ((yy)yy_8_2[1]);
yy_8_2_2 = ((yy)yy_8_2[2]);
yyv_MemberList = yy_8_2_1;
yyv_Space = yy_8_2_2;
yy_9_1 = yyv_MemberList;
yy_9_2 = yyv_Pathes;
yy_9_3 = yyb + 8;
yy_9_3[0] = 2;
CodeTryConditions(yy_9_1, yy_9_2, yy_9_3, &yy_9_4);
yyv_ChoiceMembers = yy_9_4;
yy_10_1 = yyv_Pos;
PosToLineNumber(yy_10_1, &yy_10_2);
yyv_LineOfRule = yy_10_2;
yy_11_1 = yyv_Predicate;
yy_11_2 = yyv_Cost;
yy_11_3 = yyv_LineOfRule;
yy_11_4 = yyv_ChoiceMembers;
yy_11_5 = yyv_ChainRule;
CodeCostUpdate(yy_11_1, yy_11_2, yy_11_3, yy_11_4, yy_11_5);
yy_12 = yyglov_tryLabel;
if (yy_12 == (yy) yyu) yyErr(1,4564);
yyv_TryLabel = yy_12;
yy_13_1 = ((yy)"yyL_");
s(yy_13_1);
yy_14_1 = yyv_TryLabel;
i(yy_14_1);
yy_15_1 = ((yy)":;");
s(yy_15_1);
nl();
yy_17_1 = ((yy)"}");
s(yy_17_1);
nl();
EndLocalScope();
return;
yyfl_423_1 : ;
}
yyErr(2,4546);
}
argpath(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Path;
yy yy_0_1_1;
yy yyv_N;
yy yy_0_1_2;
yy yy_1_1;
yy yy_2_1;
yy yy_3_1;
yy yy_4_1;
yy yy_5_1;
yy yy_6_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_426_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_Path = yy_0_1_1;
yyv_N = yy_0_1_2;
yy_1_1 = ((yy)"((yy)");
s(yy_1_1);
yy_2_1 = yyv_Path;
argpath(yy_2_1);
yy_3_1 = ((yy)")");
s(yy_3_1);
yy_4_1 = ((yy)"[");
s(yy_4_1);
yy_5_1 = yyv_N;
i(yy_5_1);
yy_6_1 = ((yy)"]");
s(yy_6_1);
return;
yyfl_426_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_1_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_426_2;
yy_1_1 = ((yy)"yyArg");
s(yy_1_1);
return;
yyfl_426_2 : ;
}
yyErr(2,4578);
}
cntlpath(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yy_1_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_427_1;
yy_1_1 = ((yy)"yyCntl");
s(yy_1_1);
return;
yyfl_427_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Path;
yy yy_0_1_1;
yy yyv_N;
yy yy_0_1_2;
yy yy_1_1;
yy yy_2_1;
yy yy_3_1;
yy yy_4_1;
yy yy_5_1;
yy yy_6_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_427_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_Path = yy_0_1_1;
yyv_N = yy_0_1_2;
yy_1_1 = ((yy)"((yy)");
s(yy_1_1);
yy_2_1 = yyv_Path;
cntlpath(yy_2_1);
yy_3_1 = ((yy)")");
s(yy_3_1);
yy_4_1 = ((yy)"[-");
s(yy_4_1);
yy_5_1 = yyv_N;
i(yy_5_1);
yy_6_1 = ((yy)"]");
s(yy_6_1);
return;
yyfl_427_2 : ;
}
yyErr(2,4585);
}
ulcntlpath(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_N;
yy yy_0_1_2;
yy yy_1_1;
yy yy_2_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_428_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
if (yy_0_1_1[0] != 2) goto yyfl_428_1;
yyv_N = yy_0_1_2;
yy_1_1 = ((yy)"yyCntl_");
s(yy_1_1);
yy_2_1 = yyv_N;
i(yy_2_1);
return;
yyfl_428_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Path;
yy yy_0_1_1;
yy yyv_N;
yy yy_0_1_2;
yy yy_1_1;
yy yy_2_1;
yy yy_3_1;
yy yy_4_1;
yy yy_5_1;
yy yy_6_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_428_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_Path = yy_0_1_1;
yyv_N = yy_0_1_2;
yy_1_1 = ((yy)"((yy)");
s(yy_1_1);
yy_2_1 = yyv_Path;
ulcntlpath(yy_2_1);
yy_3_1 = ((yy)")");
s(yy_3_1);
yy_4_1 = ((yy)"[-");
s(yy_4_1);
yy_5_1 = yyv_N;
i(yy_5_1);
yy_6_1 = ((yy)"]");
s(yy_6_1);
return;
yyfl_428_2 : ;
}
yyErr(2,4592);
}
LookupPath(yyin_1, yyin_2, yyin_3, yyout_1)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy *yyout_1;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yy_0_2;
yy yyv_ThisId;
yy yy_0_2_1;
yy yyv_ThisPath;
yy yy_0_2_2;
yy yyv_Tail;
yy yy_0_2_3;
yy yyv_Pos;
yy yy_0_3;
yy yy_0_4;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yy_1_1_2_1;
yy yyv_Path;
yy yy_1_1_2_2;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_1_2_1_3;
yy yy_1_2_1_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Id = yy_0_1;
if (yy_0_2[0] != 1) goto yyfl_429_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yy_0_2_3 = ((yy)yy_0_2[3]);
yyv_ThisId = yy_0_2_1;
yyv_ThisPath = yy_0_2_2;
yyv_Tail = yy_0_2_3;
yyv_Pos = yy_0_3;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Id;
yy_1_1_1_2 = yyv_ThisId;
if (! yyeq_IDENT(yy_1_1_1_1, yy_1_1_1_2)) goto yyfl_429_1_1_1;
yy_1_1_2_1 = yyv_ThisPath;
yy_1_1_2_2 = yy_1_1_2_1;
yyv_Path = yy_1_1_2_2;
goto yysl_429_1_1;
yyfl_429_1_1_1 : ;
yy_1_2_1_1 = yyv_Id;
yy_1_2_1_2 = yyv_Tail;
yy_1_2_1_3 = yyv_Pos;
LookupPath(yy_1_2_1_1, yy_1_2_1_2, yy_1_2_1_3, &yy_1_2_1_4);
yyv_Path = yy_1_2_1_4;
goto yysl_429_1_1;
yysl_429_1_1 : ;
yyb = yysb;
}
yy_0_4 = yyv_Path;
*yyout_1 = yy_0_4;
return;
yyfl_429_1 : ;
}
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yy_0_2;
yy yyv_Pos;
yy yy_0_3;
yy yy_0_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Id = yy_0_1;
if (yy_0_2[0] != 2) goto yyfl_429_2;
yyv_Pos = yy_0_3;
yyb = yyh;
yyh += 1; if (yyh > yyhx) yyExtend();
yy_0_4 = yyb + 0;
yy_0_4[0] = 2;
*yyout_1 = yy_0_4;
return;
yyfl_429_2 : ;
}
yyErr(2,4599);
}
GetPathes(yyin_1, yyin_2, yyin_3, yyin_4, yyout_1)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy yyin_4;
yy *yyout_1;
{
{
yy yyb;
yy yyv_Term;
yy yy_0_1;
yy yy_0_2;
yy yy_0_2_1;
yy yyv_ExpType;
yy yy_0_2_2;
yy yy_0_2_3;
yy yyv_Path;
yy yy_0_3;
yy yyv_VarPathes1;
yy yy_0_4;
yy yy_0_5;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yyv_Tmp;
yy yy_1_1_1_2_1;
yy yyv_Functor;
yy yy_1_1_1_2_2;
yy yyv_P;
yy yy_1_1_1_2_3;
yy yyv_Args;
yy yy_1_1_1_2_4;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yyv_Tp;
yy yy_1_2_1_2_1;
yy yy_1_2_1_2_2;
yy yy_1_2_1_2_3;
yy yy_1_2_1_2_4;
yy yy_1_2_1_2_5;
yy yy_2_1;
yy yy_2_2;
yy yy_2_3;
yy yyv_Code;
yy yy_2_4;
yy yyv_ArgSpecs;
yy yy_2_5;
yy yyv_TryLabel;
yy yy_3;
yy yy_4_1;
yy yy_5_1;
yy yy_6_1;
yy yy_7_1;
yy yy_8_1;
yy yy_9_1;
yy yy_10_1;
yy yy_12_1;
yy yy_12_2;
yy yy_12_3;
yy yy_12_3_1;
yy yy_12_3_2;
yy yy_12_4;
yy yyv_VarPathes2;
yy yy_12_5;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
yyv_Term = yy_0_1;
if (yy_0_2[0] != 1) goto yyfl_430_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yy_0_2_3 = ((yy)yy_0_2[3]);
yyv_ExpType = yy_0_2_2;
yyv_Path = yy_0_3;
yyv_VarPathes1 = yy_0_4;
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Term;
yy_1_1_1_2 = yy_1_1_1_1;
if (yy_1_1_1_2[0] != 1) goto yyfl_430_1_1_1;
yy_1_1_1_2_1 = ((yy)yy_1_1_1_2[1]);
yy_1_1_1_2_2 = ((yy)yy_1_1_1_2[2]);
yy_1_1_1_2_3 = ((yy)yy_1_1_1_2[3]);
yy_1_1_1_2_4 = ((yy)yy_1_1_1_2[4]);
yyv_Tmp = yy_1_1_1_2_1;
yyv_Functor = yy_1_1_1_2_2;
yyv_P = yy_1_1_1_2_3;
yyv_Args = yy_1_1_1_2_4;
goto yysl_430_1_1;
yyfl_430_1_1_1 : ;
yy_1_2_1_1 = yyv_Term;
yy_1_2_1_2 = yy_1_2_1_1;
if (yy_1_2_1_2[0] != 2) goto yyfl_430_1_1_2;
yy_1_2_1_2_1 = ((yy)yy_1_2_1_2[1]);
yy_1_2_1_2_2 = ((yy)yy_1_2_1_2[2]);
yy_1_2_1_2_3 = ((yy)yy_1_2_1_2[3]);
yy_1_2_1_2_4 = ((yy)yy_1_2_1_2[4]);
yy_1_2_1_2_5 = ((yy)yy_1_2_1_2[5]);
yyv_Tp = yy_1_2_1_2_1;
yyv_Tmp = yy_1_2_1_2_2;
yyv_Functor = yy_1_2_1_2_3;
yyv_P = yy_1_2_1_2_4;
yyv_Args = yy_1_2_1_2_5;
goto yysl_430_1_1;
yyfl_430_1_1_2 : ;
goto yyfl_430_1;
yysl_430_1_1 : ;
yyb = yysb;
}
yy_2_1 = yyv_Functor;
yy_2_2 = yyv_ExpType;
yy_2_3 = yyv_P;
ApplyFunctor(yy_2_1, yy_2_2, yy_2_3, &yy_2_4, &yy_2_5);
yyv_Code = yy_2_4;
yyv_ArgSpecs = yy_2_5;
yy_3 = yyglov_tryLabel;
if (yy_3 == (yy) yyu) yyErr(1,4615);
yyv_TryLabel = yy_3;
yy_4_1 = ((yy)"if (((yy)");
s(yy_4_1);
yy_5_1 = yyv_Path;
argpath(yy_5_1);
yy_6_1 = ((yy)")[0] != ");
s(yy_6_1);
yy_7_1 = yyv_Code;
i(yy_7_1);
yy_8_1 = ((yy)") goto yyL_");
s(yy_8_1);
yy_9_1 = yyv_TryLabel;
i(yy_9_1);
yy_10_1 = ((yy)";");
s(yy_10_1);
nl();
yy_12_1 = yyv_Args;
yy_12_2 = yyv_ArgSpecs;
yy_12_3_1 = yyv_Path;
yy_12_3_2 = ((yy)1);
yy_12_3 = yyb + 0;
yy_12_3[0] = 1;
yy_12_3[1] = ((long)yy_12_3_1);
yy_12_3[2] = ((long)yy_12_3_2);
yy_12_4 = yyv_VarPathes1;
GetPathes_Args(yy_12_1, yy_12_2, yy_12_3, yy_12_4, &yy_12_5);
yyv_VarPathes2 = yy_12_5;
yy_0_5 = yyv_VarPathes2;
*yyout_1 = yy_0_5;
return;
yyfl_430_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_Name;
yy yy_0_1_2;
yy yyv_P;
yy yy_0_1_3;
yy yy_0_2;
yy yy_0_2_1;
yy yyv_ExpType;
yy yy_0_2_2;
yy yy_0_2_3;
yy yyv_Path;
yy yy_0_3;
yy yyv_VarPathes1;
yy yy_0_4;
yy yy_0_5;
yy yy_0_5_1;
yy yy_0_5_2;
yy yy_0_5_3;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yy_1_3_1;
yy yy_1_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
if (yy_0_1[0] != 3) goto yyfl_430_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Tmp = yy_0_1_1;
yyv_Name = yy_0_1_2;
yyv_P = yy_0_1_3;
if (yy_0_2[0] != 1) goto yyfl_430_2;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yy_0_2_3 = ((yy)yy_0_2[3]);
yyv_ExpType = yy_0_2_2;
yyv_Path = yy_0_3;
yyv_VarPathes1 = yy_0_4;
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_Name;
yy_1_2 = yyv_ExpType;
yy_1_3_1 = ((yy)1);
yy_1_3 = (yy)(-((long)yy_1_3_1));
yy_1_4 = yyv_P;
DefineLocalVar(yy_1_1, yy_1_2, yy_1_3, yy_1_4);
yy_0_5_1 = yyv_Name;
yy_0_5_2 = yyv_Path;
yy_0_5_3 = yyv_VarPathes1;
yy_0_5 = yyb + 0;
yy_0_5[0] = 1;
yy_0_5[1] = ((long)yy_0_5_1);
yy_0_5[2] = ((long)yy_0_5_2);
yy_0_5[3] = ((long)yy_0_5_3);
*yyout_1 = yy_0_5;
return;
yyfl_430_2 : ;
}
yyErr(2,4609);
}
GetPathes_Args(yyin_1, yyin_2, yyin_3, yyin_4, yyout_1)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy yyin_4;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_T;
yy yy_0_1_3;
yy yy_0_2;
yy yyv_FH;
yy yy_0_2_1;
yy yyv_P2;
yy yy_0_2_2;
yy yyv_FT;
yy yy_0_2_3;
yy yyv_Path;
yy yy_0_3;
yy yy_0_3_1;
yy yyv_Pre;
yy yy_0_3_1_1;
yy yyv_N;
yy yy_0_3_1_2;
yy yyv_VP1;
yy yy_0_4;
yy yy_0_5;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yy_1_4;
yy yyv_VP2;
yy yy_1_5;
yy yy_2_1;
yy yy_2_2;
yy yy_2_3;
yy yy_2_3_1;
yy yy_2_3_2;
yy yy_2_3_2_1;
yy yy_2_3_2_2;
yy yy_2_4;
yy yyv_VP3;
yy yy_2_5;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
if (yy_0_1[0] != 1) goto yyfl_431_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_H = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_T = yy_0_1_3;
if (yy_0_2[0] != 1) goto yyfl_431_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yy_0_2_3 = ((yy)yy_0_2[3]);
yyv_FH = yy_0_2_1;
yyv_P2 = yy_0_2_2;
yyv_FT = yy_0_2_3;
yy_0_3_1 = yy_0_3;
yyv_Path = yy_0_3;
if (yy_0_3_1[0] != 1) goto yyfl_431_1;
yy_0_3_1_1 = ((yy)yy_0_3_1[1]);
yy_0_3_1_2 = ((yy)yy_0_3_1[2]);
yyv_Pre = yy_0_3_1_1;
yyv_N = yy_0_3_1_2;
yyv_VP1 = yy_0_4;
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_H;
yy_1_2 = yyv_FH;
yy_1_3 = yyv_Path;
yy_1_4 = yyv_VP1;
GetPathes(yy_1_1, yy_1_2, yy_1_3, yy_1_4, &yy_1_5);
yyv_VP2 = yy_1_5;
yy_2_1 = yyv_T;
yy_2_2 = yyv_FT;
yy_2_3_1 = yyv_Pre;
yy_2_3_2_1 = yyv_N;
yy_2_3_2_2 = ((yy)1);
yy_2_3_2 = (yy)(((long)yy_2_3_2_1)+((long)yy_2_3_2_2));
yy_2_3 = yyb + 0;
yy_2_3[0] = 1;
yy_2_3[1] = ((long)yy_2_3_1);
yy_2_3[2] = ((long)yy_2_3_2);
yy_2_4 = yyv_VP2;
GetPathes_Args(yy_2_1, yy_2_2, yy_2_3, yy_2_4, &yy_2_5);
yyv_VP3 = yy_2_5;
yy_0_5 = yyv_VP3;
*yyout_1 = yy_0_5;
return;
yyfl_431_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_P;
yy yy_0_1_1;
yy yy_0_2;
yy yy_0_3;
yy yyv_VP;
yy yy_0_4;
yy yy_0_5;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
if (yy_0_1[0] != 2) goto yyfl_431_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yyv_P = yy_0_1_1;
if (yy_0_2[0] != 2) goto yyfl_431_2;
yyv_VP = yy_0_4;
yy_0_5 = yyv_VP;
*yyout_1 = yy_0_5;
return;
yyfl_431_2 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_P;
yy yy_0_1_1;
yy yyv_Formals;
yy yy_0_2;
yy yy_0_3;
yy yyv_VP;
yy yy_0_4;
yy yy_0_5;
yy yy_1_1;
yy yy_1_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
if (yy_0_1[0] != 2) goto yyfl_431_3;
yy_0_1_1 = ((yy)yy_0_1[1]);
yyv_P = yy_0_1_1;
yyv_Formals = yy_0_2;
yyv_VP = yy_0_4;
yy_1_1 = ((yy)"Wrong number of arguments");
yy_1_2 = yyv_P;
MESSAGE(yy_1_1, yy_1_2);
yy_0_5 = yyv_VP;
*yyout_1 = yy_0_5;
return;
yyfl_431_3 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_T;
yy yy_0_1_3;
yy yyv_Formals;
yy yy_0_2;
yy yy_0_3;
yy yyv_VP;
yy yy_0_4;
yy yy_0_5;
yy yy_1_1;
yy yy_1_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
if (yy_0_1[0] != 1) goto yyfl_431_4;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_H = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_T = yy_0_1_3;
yyv_Formals = yy_0_2;
yyv_VP = yy_0_4;
yy_1_1 = ((yy)"Wrong number of arguments");
yy_1_2 = yyv_P;
MESSAGE(yy_1_1, yy_1_2);
yy_0_5 = yyv_VP;
*yyout_1 = yy_0_5;
return;
yyfl_431_4 : ;
}
yyErr(2,4624);
}
MakePathes(yyin_1, yyin_2, yyin_3, yyout_1)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_Functor;
yy yy_0_1_2;
yy yyv_P;
yy yy_0_1_3;
yy yyv_Args;
yy yy_0_1_4;
yy yyv_Path;
yy yy_0_2;
yy yyv_VarPathes1;
yy yy_0_3;
yy yy_0_4;
yy yy_1_1;
yy yy_1_2;
yy yy_1_2_1;
yy yy_1_2_2;
yy yy_1_3;
yy yyv_VarPathes2;
yy yy_1_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 1) goto yyfl_432_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yyv_Tmp = yy_0_1_1;
yyv_Functor = yy_0_1_2;
yyv_P = yy_0_1_3;
yyv_Args = yy_0_1_4;
yyv_Path = yy_0_2;
yyv_VarPathes1 = yy_0_3;
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_Args;
yy_1_2_1 = yyv_Path;
yy_1_2_2 = ((yy)1);
yy_1_2 = yyb + 0;
yy_1_2[0] = 1;
yy_1_2[1] = ((long)yy_1_2_1);
yy_1_2[2] = ((long)yy_1_2_2);
yy_1_3 = yyv_VarPathes1;
MakePathes_Args(yy_1_1, yy_1_2, yy_1_3, &yy_1_4);
yyv_VarPathes2 = yy_1_4;
yy_0_4 = yyv_VarPathes2;
*yyout_1 = yy_0_4;
return;
yyfl_432_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tp;
yy yy_0_1_1;
yy yyv_Tmp;
yy yy_0_1_2;
yy yyv_Functor;
yy yy_0_1_3;
yy yyv_P;
yy yy_0_1_4;
yy yyv_Args;
yy yy_0_1_5;
yy yyv_Path;
yy yy_0_2;
yy yyv_VarPathes1;
yy yy_0_3;
yy yy_0_4;
yy yy_1_1;
yy yy_1_2;
yy yy_1_2_1;
yy yy_1_2_2;
yy yy_1_3;
yy yyv_VarPathes2;
yy yy_1_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 2) goto yyfl_432_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yy_0_1_5 = ((yy)yy_0_1[5]);
yyv_Tp = yy_0_1_1;
yyv_Tmp = yy_0_1_2;
yyv_Functor = yy_0_1_3;
yyv_P = yy_0_1_4;
yyv_Args = yy_0_1_5;
yyv_Path = yy_0_2;
yyv_VarPathes1 = yy_0_3;
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_Args;
yy_1_2_1 = yyv_Path;
yy_1_2_2 = ((yy)1);
yy_1_2 = yyb + 0;
yy_1_2[0] = 1;
yy_1_2[1] = ((long)yy_1_2_1);
yy_1_2[2] = ((long)yy_1_2_2);
yy_1_3 = yyv_VarPathes1;
MakePathes_Args(yy_1_1, yy_1_2, yy_1_3, &yy_1_4);
yyv_VarPathes2 = yy_1_4;
yy_0_4 = yyv_VarPathes2;
*yyout_1 = yy_0_4;
return;
yyfl_432_2 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Tmp;
yy yy_0_1_1;
yy yyv_Name;
yy yy_0_1_2;
yy yyv_P;
yy yy_0_1_3;
yy yyv_Path;
yy yy_0_2;
yy yyv_VarPathes1;
yy yy_0_3;
yy yy_0_4;
yy yy_0_4_1;
yy yy_0_4_2;
yy yy_0_4_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 3) goto yyfl_432_3;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Tmp = yy_0_1_1;
yyv_Name = yy_0_1_2;
yyv_P = yy_0_1_3;
yyv_Path = yy_0_2;
yyv_VarPathes1 = yy_0_3;
yyb = yyh;
yyh += 4; if (yyh > yyhx) yyExtend();
yy_0_4_1 = yyv_Name;
yy_0_4_2 = yyv_Path;
yy_0_4_3 = yyv_VarPathes1;
yy_0_4 = yyb + 0;
yy_0_4[0] = 1;
yy_0_4[1] = ((long)yy_0_4_1);
yy_0_4[2] = ((long)yy_0_4_2);
yy_0_4[3] = ((long)yy_0_4_3);
*yyout_1 = yy_0_4;
return;
yyfl_432_3 : ;
}
yyErr(2,4640);
}
MakePathes_Args(yyin_1, yyin_2, yyin_3, yyout_1)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yyv_T;
yy yy_0_1_3;
yy yyv_Path;
yy yy_0_2;
yy yy_0_2_1;
yy yyv_Pre;
yy yy_0_2_1_1;
yy yyv_N;
yy yy_0_2_1_2;
yy yyv_VP1;
yy yy_0_3;
yy yy_0_4;
yy yy_1_1;
yy yy_1_2;
yy yy_1_3;
yy yyv_VP2;
yy yy_1_4;
yy yy_2_1;
yy yy_2_2;
yy yy_2_2_1;
yy yy_2_2_2;
yy yy_2_2_2_1;
yy yy_2_2_2_2;
yy yy_2_3;
yy yyv_VP3;
yy yy_2_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 1) goto yyfl_433_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_H = yy_0_1_1;
yyv_P = yy_0_1_2;
yyv_T = yy_0_1_3;
yy_0_2_1 = yy_0_2;
yyv_Path = yy_0_2;
if (yy_0_2_1[0] != 1) goto yyfl_433_1;
yy_0_2_1_1 = ((yy)yy_0_2_1[1]);
yy_0_2_1_2 = ((yy)yy_0_2_1[2]);
yyv_Pre = yy_0_2_1_1;
yyv_N = yy_0_2_1_2;
yyv_VP1 = yy_0_3;
yyb = yyh;
yyh += 3; if (yyh > yyhx) yyExtend();
yy_1_1 = yyv_H;
yy_1_2 = yyv_Path;
yy_1_3 = yyv_VP1;
MakePathes(yy_1_1, yy_1_2, yy_1_3, &yy_1_4);
yyv_VP2 = yy_1_4;
yy_2_1 = yyv_T;
yy_2_2_1 = yyv_Pre;
yy_2_2_2_1 = yyv_N;
yy_2_2_2_2 = ((yy)1);
yy_2_2_2 = (yy)(((long)yy_2_2_2_1)+((long)yy_2_2_2_2));
yy_2_2 = yyb + 0;
yy_2_2[0] = 1;
yy_2_2[1] = ((long)yy_2_2_1);
yy_2_2[2] = ((long)yy_2_2_2);
yy_2_3 = yyv_VP2;
MakePathes_Args(yy_2_1, yy_2_2, yy_2_3, &yy_2_4);
yyv_VP3 = yy_2_4;
yy_0_4 = yyv_VP3;
*yyout_1 = yy_0_4;
return;
yyfl_433_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_P;
yy yy_0_1_1;
yy yy_0_2;
yy yyv_VP;
yy yy_0_3;
yy yy_0_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 2) goto yyfl_433_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yyv_P = yy_0_1_1;
yyv_VP = yy_0_3;
yy_0_4 = yyv_VP;
*yyout_1 = yy_0_4;
return;
yyfl_433_2 : ;
}
yyErr(2,4651);
}
CodeTryConditions(yyin_1, yyin_2, yyin_3, yyout_1)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Head;
yy yy_0_1_1;
yy yyv_Tail;
yy yy_0_1_2;
yy yyv_Pathes;
yy yy_0_2;
yy yyv_OldList;
yy yy_0_3;
yy yy_0_4;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yyv_Predicate;
yy yy_1_1_1_2_1;
yy yyv_Pos;
yy yy_1_1_1_2_2;
yy yyv_InArgs;
yy yy_1_1_1_2_3;
yy yyv_OutArgs;
yy yy_1_1_1_2_4;
yy yyv_Offset;
yy yy_1_1_1_2_5;
yy yy_1_1_2_1_1_1;
yy yy_1_1_2_1_1_2;
yy yy_1_1_2_1_2_1_1_1;
yy yy_1_1_2_1_2_1_2_1_1_1;
yy yy_1_1_2_1_2_1_2_1_1_2;
yy yy_1_1_2_1_2_1_2_1_1_2_1;
yy yy_1_1_2_1_2_1_2_1_1_2_2;
yy yy_1_1_2_1_2_1_2_1_1_2_3;
yy yy_1_1_2_1_2_1_2_1_1_2_3_1;
yy yy_1_1_2_1_2_1_2_1_1_2_3_2;
yy yy_1_1_2_1_2_1_2_1_1_2_3_3;
yy yy_1_1_2_1_2_1_2_1_1_2_3_3_1;
yy yy_1_1_2_1_2_1_2_2_1_1;
yy yy_1_1_2_1_2_1_2_2_1_2;
yy yy_1_1_2_1_2_1_3_1;
yy yy_1_1_2_1_2_1_3_2;
yy yyv_A1;
yy yy_1_1_2_1_2_1_3_2_1;
yy yyv_ArgPos1;
yy yy_1_1_2_1_2_1_3_2_2;
yy yy_1_1_2_1_2_1_3_2_3;
yy yyv_A2;
yy yy_1_1_2_1_2_1_3_2_3_1;
yy yyv_ArgPos2;
yy yy_1_1_2_1_2_1_3_2_3_2;
yy yy_1_1_2_1_2_1_3_2_3_3;
yy yy_1_1_2_1_2_1_3_2_3_3_1;
yy yy_1_1_2_1_2_1_4_1_1_1;
yy yy_1_1_2_1_2_1_4_1_1_2;
yy yyv_ArgTmp;
yy yy_1_1_2_1_2_1_4_1_1_2_1;
yy yyv_Arg1;
yy yy_1_1_2_1_2_1_4_1_1_2_2;
yy yyv_VarPos;
yy yy_1_1_2_1_2_1_4_1_1_2_3;
yy yy_1_1_2_1_2_1_4_1_2_1_1_1;
yy yyv_Type;
yy yy_1_1_2_1_2_1_4_1_2_1_1_2;
yy yy_1_1_2_1_2_1_4_1_2_2_1_1;
yy yy_1_1_2_1_2_1_4_1_2_2_1_2;
yy yy_1_1_2_1_2_1_4_1_2_2_1_3;
yy yy_1_1_2_1_2_1_4_1_2_2_1_4;
yy yy_1_1_2_1_2_1_4_1_2_2_2;
yy yy_1_1_2_1_2_1_4_2_1_1;
yy yy_1_1_2_1_2_1_4_2_1_2;
yy yy_1_1_2_1_2_1_4_2_1_2_1;
yy yyv_Number;
yy yy_1_1_2_1_2_1_4_2_1_2_2;
yy yy_1_1_2_1_2_1_4_2_1_2_3;
yy yy_1_1_2_1_2_1_4_2_2;
yy yy_1_1_2_1_2_1_4_3_1_1;
yy yy_1_1_2_1_2_1_4_3_1_2;
yy yy_1_1_2_1_2_1_4_3_1_2_1;
yy yyv_String;
yy yy_1_1_2_1_2_1_4_3_1_2_2;
yy yy_1_1_2_1_2_1_4_3_1_2_3;
yy yy_1_1_2_1_2_1_4_3_2;
yy yy_1_1_2_1_2_1_4_4_1_1;
yy yy_1_1_2_1_2_1_4_4_1_2;
yy yy_1_1_2_1_2_1_4_4_2;
yy yyv_TryLabel;
yy yy_1_1_2_1_2_1_5;
yy yy_1_1_2_1_2_1_6_1_1_1;
yy yy_1_1_2_1_2_1_6_1_1_2;
yy yy_1_1_2_1_2_1_6_1_2_1;
yy yy_1_1_2_1_2_1_6_1_3_1;
yy yy_1_1_2_1_2_1_6_1_3_2;
yy yy_1_1_2_1_2_1_6_1_3_3;
yy yy_1_1_2_1_2_1_6_1_4_1;
yy yy_1_1_2_1_2_1_6_1_5_1;
yy yy_1_1_2_1_2_1_6_1_5_2;
yy yy_1_1_2_1_2_1_6_1_5_3;
yy yy_1_1_2_1_2_1_6_1_6_1;
yy yy_1_1_2_1_2_1_6_1_7_1;
yy yy_1_1_2_1_2_1_6_1_8_1;
yy yy_1_1_2_1_2_1_6_2_1_1;
yy yy_1_1_2_1_2_1_6_2_1_2;
yy yy_1_1_2_1_2_1_6_2_2_1;
yy yy_1_1_2_1_2_1_6_2_3_1;
yy yy_1_1_2_1_2_1_6_2_3_2;
yy yy_1_1_2_1_2_1_6_2_3_3;
yy yy_1_1_2_1_2_1_6_2_4_1;
yy yy_1_1_2_1_2_1_6_2_5_1;
yy yy_1_1_2_1_2_1_6_2_5_2;
yy yy_1_1_2_1_2_1_6_2_5_3;
yy yy_1_1_2_1_2_1_6_2_6_1;
yy yy_1_1_2_1_2_1_6_2_7_1;
yy yy_1_1_2_1_2_1_6_2_8_1;
yy yy_1_1_2_1_2_1_6_2_9_1;
yy yy_1_1_2_1_2_1_6_2_10_1;
yy yy_1_1_2_1_2_1_6_3_1_1_1_1;
yy yy_1_1_2_1_2_1_6_3_1_1_1_2;
yy yy_1_1_2_1_2_1_6_3_1_1_2_1;
yy yy_1_1_2_1_2_1_6_3_1_1_3_1;
yy yy_1_1_2_1_2_1_6_3_1_1_4_1;
yy yy_1_1_2_1_2_1_6_3_1_1_5_1;
yy yy_1_1_2_1_2_1_6_3_1_1_5_2;
yy yy_1_1_2_1_2_1_6_3_1_1_5_3;
yy yy_1_1_2_1_2_1_6_3_1_1_6_1;
yy yy_1_1_2_1_2_1_6_3_1_1_7_1;
yy yy_1_1_2_1_2_1_6_3_1_1_7_2;
yy yy_1_1_2_1_2_1_6_3_1_1_7_3;
yy yy_1_1_2_1_2_1_6_3_1_1_8_1;
yy yy_1_1_2_1_2_1_6_3_1_1_9_1;
yy yy_1_1_2_1_2_1_6_3_1_1_10_1;
yy yy_1_1_2_1_2_1_6_3_1_2_1_1;
yy yy_1_1_2_1_2_1_6_3_1_2_1_2;
yy yy_1_1_2_1_2_1_6_3_1_2_2_1;
yy yy_1_1_2_1_2_1_6_3_1_2_3_1;
yy yy_1_1_2_1_2_1_6_3_1_2_4_1;
yy yy_1_1_2_1_2_1_6_3_1_2_5_1;
yy yy_1_1_2_1_2_1_6_3_1_2_5_2;
yy yy_1_1_2_1_2_1_6_3_1_2_5_3;
yy yy_1_1_2_1_2_1_6_3_1_2_6_1;
yy yy_1_1_2_1_2_1_6_3_1_2_7_1;
yy yy_1_1_2_1_2_1_6_3_1_2_7_2;
yy yy_1_1_2_1_2_1_6_3_1_2_7_3;
yy yy_1_1_2_1_2_1_6_3_1_2_8_1;
yy yy_1_1_2_1_2_1_6_3_1_2_9_1;
yy yy_1_1_2_1_2_1_6_3_1_2_10_1;
yy yy_1_1_2_1_2_1_6_3_1_3_1_1;
yy yy_1_1_2_1_2_1_6_3_1_3_1_2;
yy yy_1_1_2_1_2_1_6_3_1_3_1_3;
yy yy_1_1_2_1_2_1_6_3_1_3_1_4;
yy yy_1_1_2_1_3_1;
yy yyv_NewList;
yy yy_1_1_2_1_3_2;
yy yy_1_1_2_2_1_1;
yy yy_1_1_2_2_1_2;
yy yy_1_1_2_2_1_3;
yy yyv_CodeName;
yy yy_1_1_2_2_1_4;
yy yyv_Class;
yy yy_1_1_2_2_1_5;
yy yyv_FormalInArgs;
yy yy_1_1_2_2_1_6;
yy yyv_FormalOutArgs;
yy yy_1_1_2_2_1_7;
yy yy_1_1_2_2_2_1_1_1;
yy yy_1_1_2_2_2_1_1_2;
yy yy_1_1_2_2_2_1_2_1;
yy yy_1_1_2_2_2_1_3_1;
yy yy_1_1_2_2_2_1_4_1;
yy yy_1_1_2_2_2_1_5_1;
yy yy_1_1_2_2_2_1_5_2;
yy yy_1_1_2_2_2_1_5_3;
yy yyv_Sep;
yy yy_1_1_2_2_2_1_5_4;
yy yy_1_1_2_2_2_1_6_1;
yy yy_1_1_2_2_2_1_6_2;
yy yy_1_1_2_2_2_1_7;
yy yy_1_1_2_2_2_1_8_1;
yy yy_1_1_2_2_2_1_9_1;
yy yy_1_1_2_2_2_1_10_1;
yy yy_1_1_2_2_2_1_12_1;
yy yy_1_1_2_2_2_1_12_2;
yy yy_1_1_2_2_2_2_1_1;
yy yy_1_1_2_2_2_2_1_2;
yy yy_1_1_2_2_2_2_2_1_1_1;
yy yy_1_1_2_2_2_2_2_1_1_2;
yy yyv_Arg;
yy yy_1_1_2_2_2_2_2_1_1_2_1;
yy yyv_ArgPos;
yy yy_1_1_2_2_2_2_2_1_1_2_2;
yy yy_1_1_2_2_2_2_2_1_1_2_3;
yy yy_1_1_2_2_2_2_2_1_2_1_1_1;
yy yy_1_1_2_2_2_2_2_1_2_1_1_2;
yy yyv_Tmp;
yy yy_1_1_2_2_2_2_2_1_2_1_1_2_1;
yy yyv_Name;
yy yy_1_1_2_2_2_2_2_1_2_1_1_2_2;
yy yy_1_1_2_2_2_2_2_1_2_1_1_2_3;
yy yy_1_1_2_2_2_2_2_1_2_1_2_1_1_1;
yy yy_1_1_2_2_2_2_2_1_2_1_2_1_1_2;
yy yy_1_1_2_2_2_2_2_1_2_1_2_2_1_1;
yy yy_1_1_2_2_2_2_2_1_2_1_2_2_1_2;
yy yy_1_1_2_2_2_2_2_1_2_1_2_2_1_3;
yy yy_1_1_2_2_2_2_2_1_2_1_2_2_1_4;
yy yy_1_1_2_2_2_2_2_1_2_1_2_2_2;
yy yy_1_1_2_2_2_2_2_1_2_1_3_1;
yy yy_1_1_2_2_2_2_2_1_2_1_3_2;
yy yy_1_1_2_2_2_2_2_1_2_1_3_3;
yy yyv_Path;
yy yy_1_1_2_2_2_2_2_1_2_1_3_4;
yy yyv_TypeList;
yy yy_1_1_2_2_2_2_2_1_2_1_4;
yy yy_1_1_2_2_2_2_2_1_2_1_5_1;
yy yy_1_1_2_2_2_2_2_1_2_1_5_2;
yy yyv_ChoiceTypeIndex;
yy yy_1_1_2_2_2_2_2_1_2_1_5_3;
yy yy_1_1_2_2_2_2_2_1_2_1_6_1;
yy yy_1_1_2_2_2_2_2_1_2_1_6_1_1;
yy yy_1_1_2_2_2_2_2_1_2_1_6_1_2;
yy yy_1_1_2_2_2_2_2_1_2_1_6_1_3;
yy yy_1_1_2_2_2_2_2_1_2_1_6_1_4;
yy yy_1_1_2_2_2_2_2_1_2_1_6_1_5;
yy yy_1_1_2_2_2_2_2_1_2_1_6_2;
yy yy_1_1_2_2_2_2_2_1_2_2_1_1;
yy yy_1_1_2_2_2_2_2_1_2_2_1_2;
yy yy_1_1_2_2_2_2_2_1_2_2_2_1;
yy yy_1_1_2_2_2_2_2_1_2_2_2_2;
yy yy_1_1_2_2_2_2_2_2_1_1;
yy yy_1_1_2_2_2_2_2_2_1_2;
yy yy_1_1_2_2_2_2_2_2_1_2_1;
yy yy_1_1_2_2_2_2_2_2_2_1;
yy yy_1_1_2_2_2_2_2_2_2_2;
yy yy_1_1_2_2_2_2_2_2_3_1;
yy yy_1_1_2_2_2_2_2_2_3_2;
yy yy_1_1_2_2_2_3_1_1;
yy yy_1_1_2_2_2_3_1_2;
yy yy_1_1_2_3_1_1;
yy yy_1_1_2_3_1_2;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_1_2_2_1;
yy yy_1_2_2_2;
yy yy_1_2_3_1;
yy yy_1_2_3_2;
yy yy_2_1;
yy yy_2_2;
yy yy_2_3;
yy yyv_List;
yy yy_2_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 1) goto yyfl_435_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yyv_Head = yy_0_1_1;
yyv_Tail = yy_0_1_2;
yyv_Pathes = yy_0_2;
yyv_OldList = yy_0_3;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Head;
yy_1_1_1_2 = yy_1_1_1_1;
if (yy_1_1_1_2[0] != 7) goto yyfl_435_1_1_1;
yy_1_1_1_2_1 = ((yy)yy_1_1_1_2[1]);
yy_1_1_1_2_2 = ((yy)yy_1_1_1_2[2]);
yy_1_1_1_2_3 = ((yy)yy_1_1_1_2[3]);
yy_1_1_1_2_4 = ((yy)yy_1_1_1_2[4]);
yy_1_1_1_2_5 = ((yy)yy_1_1_1_2[5]);
yyv_Predicate = yy_1_1_1_2_1;
yyv_Pos = yy_1_1_1_2_2;
yyv_InArgs = yy_1_1_1_2_3;
yyv_OutArgs = yy_1_1_1_2_4;
yyv_Offset = yy_1_1_1_2_5;
{
yy yysb = yyb;
yy_1_1_2_1_1_1 = yyv_Predicate;
if (! GetGlobalMeaning(yy_1_1_2_1_1_1, &yy_1_1_2_1_1_2)) goto yyfl_435_1_1_1_2_1;
if (yy_1_1_2_1_1_2[0] != 5) goto yyfl_435_1_1_1_2_1;
{
yy yysb = yyb;
yy_1_1_2_1_2_1_1_1 = yyv_Predicate;
if (! IsRelation(yy_1_1_2_1_2_1_1_1)) goto yyfl_435_1_1_1_2_1_2_1;
{
yy yysb = yyb;
yy_1_1_2_1_2_1_2_1_1_1 = yyv_InArgs;
yy_1_1_2_1_2_1_2_1_1_2 = yy_1_1_2_1_2_1_2_1_1_1;
if (yy_1_1_2_1_2_1_2_1_1_2[0] != 1) goto yyfl_435_1_1_1_2_1_2_1_2_1;
yy_1_1_2_1_2_1_2_1_1_2_1 = ((yy)yy_1_1_2_1_2_1_2_1_1_2[1]);
yy_1_1_2_1_2_1_2_1_1_2_2 = ((yy)yy_1_1_2_1_2_1_2_1_1_2[2]);
yy_1_1_2_1_2_1_2_1_1_2_3 = ((yy)yy_1_1_2_1_2_1_2_1_1_2[3]);
if (yy_1_1_2_1_2_1_2_1_1_2_3[0] != 1) goto yyfl_435_1_1_1_2_1_2_1_2_1;
yy_1_1_2_1_2_1_2_1_1_2_3_1 = ((yy)yy_1_1_2_1_2_1_2_1_1_2_3[1]);
yy_1_1_2_1_2_1_2_1_1_2_3_2 = ((yy)yy_1_1_2_1_2_1_2_1_1_2_3[2]);
yy_1_1_2_1_2_1_2_1_1_2_3_3 = ((yy)yy_1_1_2_1_2_1_2_1_1_2_3[3]);
if (yy_1_1_2_1_2_1_2_1_1_2_3_3[0] != 2) goto yyfl_435_1_1_1_2_1_2_1_2_1;
yy_1_1_2_1_2_1_2_1_1_2_3_3_1 = ((yy)yy_1_1_2_1_2_1_2_1_1_2_3_3[1]);
goto yysl_435_1_1_1_2_1_2_1_2;
yyfl_435_1_1_1_2_1_2_1_2_1 : ;
yy_1_1_2_1_2_1_2_2_1_1 = ((yy)"invalid number of arguments");
yy_1_1_2_1_2_1_2_2_1_2 = yyv_Pos;
MESSAGE(yy_1_1_2_1_2_1_2_2_1_1, yy_1_1_2_1_2_1_2_2_1_2);
goto yysl_435_1_1_1_2_1_2_1_2;
yysl_435_1_1_1_2_1_2_1_2 : ;
yyb = yysb;
}
yy_1_1_2_1_2_1_3_1 = yyv_InArgs;
yy_1_1_2_1_2_1_3_2 = yy_1_1_2_1_2_1_3_1;
if (yy_1_1_2_1_2_1_3_2[0] != 1) goto yyfl_435_1_1_1_2_1_2_1;
yy_1_1_2_1_2_1_3_2_1 = ((yy)yy_1_1_2_1_2_1_3_2[1]);
yy_1_1_2_1_2_1_3_2_2 = ((yy)yy_1_1_2_1_2_1_3_2[2]);
yy_1_1_2_1_2_1_3_2_3 = ((yy)yy_1_1_2_1_2_1_3_2[3]);
yyv_A1 = yy_1_1_2_1_2_1_3_2_1;
yyv_ArgPos1 = yy_1_1_2_1_2_1_3_2_2;
if (yy_1_1_2_1_2_1_3_2_3[0] != 1) goto yyfl_435_1_1_1_2_1_2_1;
yy_1_1_2_1_2_1_3_2_3_1 = ((yy)yy_1_1_2_1_2_1_3_2_3[1]);
yy_1_1_2_1_2_1_3_2_3_2 = ((yy)yy_1_1_2_1_2_1_3_2_3[2]);
yy_1_1_2_1_2_1_3_2_3_3 = ((yy)yy_1_1_2_1_2_1_3_2_3[3]);
yyv_A2 = yy_1_1_2_1_2_1_3_2_3_1;
yyv_ArgPos2 = yy_1_1_2_1_2_1_3_2_3_2;
if (yy_1_1_2_1_2_1_3_2_3_3[0] != 2) goto yyfl_435_1_1_1_2_1_2_1;
yy_1_1_2_1_2_1_3_2_3_3_1 = ((yy)yy_1_1_2_1_2_1_3_2_3_3[1]);
{
yy yysb = yyb;
yy_1_1_2_1_2_1_4_1_1_1 = yyv_A1;
yy_1_1_2_1_2_1_4_1_1_2 = yy_1_1_2_1_2_1_4_1_1_1;
if (yy_1_1_2_1_2_1_4_1_1_2[0] != 5) goto yyfl_435_1_1_1_2_1_2_1_4_1;
yy_1_1_2_1_2_1_4_1_1_2_1 = ((yy)yy_1_1_2_1_2_1_4_1_1_2[1]);
yy_1_1_2_1_2_1_4_1_1_2_2 = ((yy)yy_1_1_2_1_2_1_4_1_1_2[2]);
yy_1_1_2_1_2_1_4_1_1_2_3 = ((yy)yy_1_1_2_1_2_1_4_1_1_2[3]);
yyv_ArgTmp = yy_1_1_2_1_2_1_4_1_1_2_1;
yyv_Arg1 = yy_1_1_2_1_2_1_4_1_1_2_2;
yyv_VarPos = yy_1_1_2_1_2_1_4_1_1_2_3;
{
yy yysb = yyb;
yy_1_1_2_1_2_1_4_1_2_1_1_1 = yyv_Arg1;
if (! GetLocalMeaning(yy_1_1_2_1_2_1_4_1_2_1_1_1, &yy_1_1_2_1_2_1_4_1_2_1_1_2)) goto yyfl_435_1_1_1_2_1_2_1_4_1_2_1;
yyv_Type = yy_1_1_2_1_2_1_4_1_2_1_1_2;
goto yysl_435_1_1_1_2_1_2_1_4_1_2;
yyfl_435_1_1_1_2_1_2_1_4_1_2_1 : ;
yy_1_1_2_1_2_1_4_1_2_2_1_1 = ((yy)"'");
yy_1_1_2_1_2_1_4_1_2_2_1_2 = yyv_Arg1;
yy_1_1_2_1_2_1_4_1_2_2_1_3 = ((yy)"' is not (component of) primary CHOICE argument");
yy_1_1_2_1_2_1_4_1_2_2_1_4 = yyv_VarPos;
MESSAGE1(yy_1_1_2_1_2_1_4_1_2_2_1_1, yy_1_1_2_1_2_1_4_1_2_2_1_2, yy_1_1_2_1_2_1_4_1_2_2_1_3, yy_1_1_2_1_2_1_4_1_2_2_1_4);
yy_1_1_2_1_2_1_4_1_2_2_2 = yyglov_errorId;
if (yy_1_1_2_1_2_1_4_1_2_2_2 == (yy) yyu) yyErr(1,4687);
yyv_Type = yy_1_1_2_1_2_1_4_1_2_2_2;
goto yysl_435_1_1_1_2_1_2_1_4_1_2;
yysl_435_1_1_1_2_1_2_1_4_1_2 : ;
yyb = yysb;
}
goto yysl_435_1_1_1_2_1_2_1_4;
yyfl_435_1_1_1_2_1_2_1_4_1 : ;
yy_1_1_2_1_2_1_4_2_1_1 = yyv_A1;
yy_1_1_2_1_2_1_4_2_1_2 = yy_1_1_2_1_2_1_4_2_1_1;
if (yy_1_1_2_1_2_1_4_2_1_2[0] != 6) goto yyfl_435_1_1_1_2_1_2_1_4_2;
yy_1_1_2_1_2_1_4_2_1_2_1 = ((yy)yy_1_1_2_1_2_1_4_2_1_2[1]);
yy_1_1_2_1_2_1_4_2_1_2_2 = ((yy)yy_1_1_2_1_2_1_4_2_1_2[2]);
yy_1_1_2_1_2_1_4_2_1_2_3 = ((yy)yy_1_1_2_1_2_1_4_2_1_2[3]);
yyv_Number = yy_1_1_2_1_2_1_4_2_1_2_2;
yy_1_1_2_1_2_1_4_2_2 = yyglov_id_INT;
if (yy_1_1_2_1_2_1_4_2_2 == (yy) yyu) yyErr(1,4691);
yyv_Type = yy_1_1_2_1_2_1_4_2_2;
goto yysl_435_1_1_1_2_1_2_1_4;
yyfl_435_1_1_1_2_1_2_1_4_2 : ;
yy_1_1_2_1_2_1_4_3_1_1 = yyv_A1;
yy_1_1_2_1_2_1_4_3_1_2 = yy_1_1_2_1_2_1_4_3_1_1;
if (yy_1_1_2_1_2_1_4_3_1_2[0] != 7) goto yyfl_435_1_1_1_2_1_2_1_4_3;
yy_1_1_2_1_2_1_4_3_1_2_1 = ((yy)yy_1_1_2_1_2_1_4_3_1_2[1]);
yy_1_1_2_1_2_1_4_3_1_2_2 = ((yy)yy_1_1_2_1_2_1_4_3_1_2[2]);
yy_1_1_2_1_2_1_4_3_1_2_3 = ((yy)yy_1_1_2_1_2_1_4_3_1_2[3]);
yyv_String = yy_1_1_2_1_2_1_4_3_1_2_2;
yy_1_1_2_1_2_1_4_3_2 = yyglov_id_STRING;
if (yy_1_1_2_1_2_1_4_3_2 == (yy) yyu) yyErr(1,4694);
yyv_Type = yy_1_1_2_1_2_1_4_3_2;
goto yysl_435_1_1_1_2_1_2_1_4;
yyfl_435_1_1_1_2_1_2_1_4_3 : ;
yy_1_1_2_1_2_1_4_4_1_1 = ((yy)"illegal input for condition in CHOICE rule");
yy_1_1_2_1_2_1_4_4_1_2 = yyv_ArgPos1;
MESSAGE(yy_1_1_2_1_2_1_4_4_1_1, yy_1_1_2_1_2_1_4_4_1_2);
yy_1_1_2_1_2_1_4_4_2 = yyglov_errorId;
if (yy_1_1_2_1_2_1_4_4_2 == (yy) yyu) yyErr(1,4697);
yyv_Type = yy_1_1_2_1_2_1_4_4_2;
goto yysl_435_1_1_1_2_1_2_1_4;
yysl_435_1_1_1_2_1_2_1_4 : ;
yyb = yysb;
}
yy_1_1_2_1_2_1_5 = yyglov_tryLabel;
if (yy_1_1_2_1_2_1_5 == (yy) yyu) yyErr(1,4700);
yyv_TryLabel = yy_1_1_2_1_2_1_5;
{
yy yysb = yyb;
yy_1_1_2_1_2_1_6_1_1_1 = yyv_Type;
yy_1_1_2_1_2_1_6_1_1_2 = ((yy)"INT");
if (! IsIdent(yy_1_1_2_1_2_1_6_1_1_1, yy_1_1_2_1_2_1_6_1_1_2)) goto yyfl_435_1_1_1_2_1_2_1_6_1;
yy_1_1_2_1_2_1_6_1_2_1 = ((yy)"if (");
s(yy_1_1_2_1_2_1_6_1_2_1);
yy_1_1_2_1_2_1_6_1_3_1 = yyv_A1;
yy_1_1_2_1_2_1_6_1_3_2 = yyv_ArgPos1;
yy_1_1_2_1_2_1_6_1_3_3 = yyv_Pathes;
CodeTryConditionArg(yy_1_1_2_1_2_1_6_1_3_1, yy_1_1_2_1_2_1_6_1_3_2, yy_1_1_2_1_2_1_6_1_3_3);
yy_1_1_2_1_2_1_6_1_4_1 = yyv_Predicate;
NegIntRel(yy_1_1_2_1_2_1_6_1_4_1);
yy_1_1_2_1_2_1_6_1_5_1 = yyv_A2;
yy_1_1_2_1_2_1_6_1_5_2 = yyv_ArgPos2;
yy_1_1_2_1_2_1_6_1_5_3 = yyv_Pathes;
CodeTryConditionArg(yy_1_1_2_1_2_1_6_1_5_1, yy_1_1_2_1_2_1_6_1_5_2, yy_1_1_2_1_2_1_6_1_5_3);
yy_1_1_2_1_2_1_6_1_6_1 = ((yy)") goto yyL_");
s(yy_1_1_2_1_2_1_6_1_6_1);
yy_1_1_2_1_2_1_6_1_7_1 = yyv_TryLabel;
i(yy_1_1_2_1_2_1_6_1_7_1);
yy_1_1_2_1_2_1_6_1_8_1 = ((yy)";");
s(yy_1_1_2_1_2_1_6_1_8_1);
nl();
goto yysl_435_1_1_1_2_1_2_1_6;
yyfl_435_1_1_1_2_1_2_1_6_1 : ;
yy_1_1_2_1_2_1_6_2_1_1 = yyv_Type;
yy_1_1_2_1_2_1_6_2_1_2 = ((yy)"STRING");
if (! IsIdent(yy_1_1_2_1_2_1_6_2_1_1, yy_1_1_2_1_2_1_6_2_1_2)) goto yyfl_435_1_1_1_2_1_2_1_6_2;
yy_1_1_2_1_2_1_6_2_2_1 = ((yy)"if (strcmp((char *) ");
s(yy_1_1_2_1_2_1_6_2_2_1);
yy_1_1_2_1_2_1_6_2_3_1 = yyv_A1;
yy_1_1_2_1_2_1_6_2_3_2 = yyv_ArgPos1;
yy_1_1_2_1_2_1_6_2_3_3 = yyv_Pathes;
CodeTryConditionArg(yy_1_1_2_1_2_1_6_2_3_1, yy_1_1_2_1_2_1_6_2_3_2, yy_1_1_2_1_2_1_6_2_3_3);
yy_1_1_2_1_2_1_6_2_4_1 = ((yy)", (char *) ");
s(yy_1_1_2_1_2_1_6_2_4_1);
yy_1_1_2_1_2_1_6_2_5_1 = yyv_A2;
yy_1_1_2_1_2_1_6_2_5_2 = yyv_ArgPos2;
yy_1_1_2_1_2_1_6_2_5_3 = yyv_Pathes;
CodeTryConditionArg(yy_1_1_2_1_2_1_6_2_5_1, yy_1_1_2_1_2_1_6_2_5_2, yy_1_1_2_1_2_1_6_2_5_3);
yy_1_1_2_1_2_1_6_2_6_1 = ((yy)") ");
s(yy_1_1_2_1_2_1_6_2_6_1);
yy_1_1_2_1_2_1_6_2_7_1 = yyv_Predicate;
NegIntRel(yy_1_1_2_1_2_1_6_2_7_1);
yy_1_1_2_1_2_1_6_2_8_1 = ((yy)" 0) goto yyL_");
s(yy_1_1_2_1_2_1_6_2_8_1);
yy_1_1_2_1_2_1_6_2_9_1 = yyv_TryLabel;
i(yy_1_1_2_1_2_1_6_2_9_1);
yy_1_1_2_1_2_1_6_2_10_1 = ((yy)";");
s(yy_1_1_2_1_2_1_6_2_10_1);
nl();
goto yysl_435_1_1_1_2_1_2_1_6;
yyfl_435_1_1_1_2_1_2_1_6_2 : ;
{
yy yysb = yyb;
yy_1_1_2_1_2_1_6_3_1_1_1_1 = yyv_Predicate;
yy_1_1_2_1_2_1_6_3_1_1_1_2 = ((yy)"eq");
if (! IsIdent(yy_1_1_2_1_2_1_6_3_1_1_1_1, yy_1_1_2_1_2_1_6_3_1_1_1_2)) goto yyfl_435_1_1_1_2_1_2_1_6_3_1_1;
yy_1_1_2_1_2_1_6_3_1_1_2_1 = ((yy)"if (! yyeq_");
s(yy_1_1_2_1_2_1_6_3_1_1_2_1);
yy_1_1_2_1_2_1_6_3_1_1_3_1 = yyv_Type;
id(yy_1_1_2_1_2_1_6_3_1_1_3_1);
yy_1_1_2_1_2_1_6_3_1_1_4_1 = ((yy)"(");
s(yy_1_1_2_1_2_1_6_3_1_1_4_1);
yy_1_1_2_1_2_1_6_3_1_1_5_1 = yyv_A1;
yy_1_1_2_1_2_1_6_3_1_1_5_2 = yyv_ArgPos1;
yy_1_1_2_1_2_1_6_3_1_1_5_3 = yyv_Pathes;
CodeTryConditionArg(yy_1_1_2_1_2_1_6_3_1_1_5_1, yy_1_1_2_1_2_1_6_3_1_1_5_2, yy_1_1_2_1_2_1_6_3_1_1_5_3);
yy_1_1_2_1_2_1_6_3_1_1_6_1 = ((yy)", ");
s(yy_1_1_2_1_2_1_6_3_1_1_6_1);
yy_1_1_2_1_2_1_6_3_1_1_7_1 = yyv_A2;
yy_1_1_2_1_2_1_6_3_1_1_7_2 = yyv_ArgPos2;
yy_1_1_2_1_2_1_6_3_1_1_7_3 = yyv_Pathes;
CodeTryConditionArg(yy_1_1_2_1_2_1_6_3_1_1_7_1, yy_1_1_2_1_2_1_6_3_1_1_7_2, yy_1_1_2_1_2_1_6_3_1_1_7_3);
yy_1_1_2_1_2_1_6_3_1_1_8_1 = ((yy)")) goto yyL_");
s(yy_1_1_2_1_2_1_6_3_1_1_8_1);
yy_1_1_2_1_2_1_6_3_1_1_9_1 = yyv_TryLabel;
i(yy_1_1_2_1_2_1_6_3_1_1_9_1);
yy_1_1_2_1_2_1_6_3_1_1_10_1 = ((yy)";");
s(yy_1_1_2_1_2_1_6_3_1_1_10_1);
nl();
goto yysl_435_1_1_1_2_1_2_1_6_3_1;
yyfl_435_1_1_1_2_1_2_1_6_3_1_1 : ;
yy_1_1_2_1_2_1_6_3_1_2_1_1 = yyv_Predicate;
yy_1_1_2_1_2_1_6_3_1_2_1_2 = ((yy)"ne");
if (! IsIdent(yy_1_1_2_1_2_1_6_3_1_2_1_1, yy_1_1_2_1_2_1_6_3_1_2_1_2)) goto yyfl_435_1_1_1_2_1_2_1_6_3_1_2;
yy_1_1_2_1_2_1_6_3_1_2_2_1 = ((yy)"if (yyeq_");
s(yy_1_1_2_1_2_1_6_3_1_2_2_1);
yy_1_1_2_1_2_1_6_3_1_2_3_1 = yyv_Type;
id(yy_1_1_2_1_2_1_6_3_1_2_3_1);
yy_1_1_2_1_2_1_6_3_1_2_4_1 = ((yy)"(");
s(yy_1_1_2_1_2_1_6_3_1_2_4_1);
yy_1_1_2_1_2_1_6_3_1_2_5_1 = yyv_A1;
yy_1_1_2_1_2_1_6_3_1_2_5_2 = yyv_ArgPos1;
yy_1_1_2_1_2_1_6_3_1_2_5_3 = yyv_Pathes;
CodeTryConditionArg(yy_1_1_2_1_2_1_6_3_1_2_5_1, yy_1_1_2_1_2_1_6_3_1_2_5_2, yy_1_1_2_1_2_1_6_3_1_2_5_3);
yy_1_1_2_1_2_1_6_3_1_2_6_1 = ((yy)", ");
s(yy_1_1_2_1_2_1_6_3_1_2_6_1);
yy_1_1_2_1_2_1_6_3_1_2_7_1 = yyv_A2;
yy_1_1_2_1_2_1_6_3_1_2_7_2 = yyv_ArgPos2;
yy_1_1_2_1_2_1_6_3_1_2_7_3 = yyv_Pathes;
CodeTryConditionArg(yy_1_1_2_1_2_1_6_3_1_2_7_1, yy_1_1_2_1_2_1_6_3_1_2_7_2, yy_1_1_2_1_2_1_6_3_1_2_7_3);
yy_1_1_2_1_2_1_6_3_1_2_8_1 = ((yy)")) goto yyL_");
s(yy_1_1_2_1_2_1_6_3_1_2_8_1);
yy_1_1_2_1_2_1_6_3_1_2_9_1 = yyv_TryLabel;
i(yy_1_1_2_1_2_1_6_3_1_2_9_1);
yy_1_1_2_1_2_1_6_3_1_2_10_1 = ((yy)";");
s(yy_1_1_2_1_2_1_6_3_1_2_10_1);
nl();
goto yysl_435_1_1_1_2_1_2_1_6_3_1;
yyfl_435_1_1_1_2_1_2_1_6_3_1_2 : ;
yy_1_1_2_1_2_1_6_3_1_3_1_1 = ((yy)"relation not defined for type '");
yy_1_1_2_1_2_1_6_3_1_3_1_2 = yyv_Type;
yy_1_1_2_1_2_1_6_3_1_3_1_3 = ((yy)"'");
yy_1_1_2_1_2_1_6_3_1_3_1_4 = yyv_Pos;
MESSAGE1(yy_1_1_2_1_2_1_6_3_1_3_1_1, yy_1_1_2_1_2_1_6_3_1_3_1_2, yy_1_1_2_1_2_1_6_3_1_3_1_3, yy_1_1_2_1_2_1_6_3_1_3_1_4);
goto yysl_435_1_1_1_2_1_2_1_6_3_1;
yysl_435_1_1_1_2_1_2_1_6_3_1 : ;
yyb = yysb;
}
goto yysl_435_1_1_1_2_1_2_1_6;
yysl_435_1_1_1_2_1_2_1_6 : ;
yyb = yysb;
}
goto yysl_435_1_1_1_2_1_2;
yyfl_435_1_1_1_2_1_2_1 : ;
goto yysl_435_1_1_1_2_1_2;
yysl_435_1_1_1_2_1_2 : ;
yyb = yysb;
}
yy_1_1_2_1_3_1 = yyv_OldList;
yy_1_1_2_1_3_2 = yy_1_1_2_1_3_1;
yyv_NewList = yy_1_1_2_1_3_2;
goto yysl_435_1_1_1_2;
yyfl_435_1_1_1_2_1 : ;
yy_1_1_2_2_1_1 = yyv_Predicate;
yy_1_1_2_2_1_2 = yyv_InArgs;
yy_1_1_2_2_1_3 = yyv_Pos;
if (! IsPredicate(yy_1_1_2_2_1_1, yy_1_1_2_2_1_2, yy_1_1_2_2_1_3, &yy_1_1_2_2_1_4, &yy_1_1_2_2_1_5, &yy_1_1_2_2_1_6, &yy_1_1_2_2_1_7)) goto yyfl_435_1_1_1_2_2;
yyv_CodeName = yy_1_1_2_2_1_4;
yyv_Class = yy_1_1_2_2_1_5;
yyv_FormalInArgs = yy_1_1_2_2_1_6;
yyv_FormalOutArgs = yy_1_1_2_2_1_7;
{
yy yysb = yyb;
yy_1_1_2_2_2_1_1_1 = yyv_Class;
yy_1_1_2_2_2_1_1_2 = yy_1_1_2_2_2_1_1_1;
if (yy_1_1_2_2_2_1_1_2[0] != 2) goto yyfl_435_1_1_1_2_2_2_1;
yy_1_1_2_2_2_1_2_1 = ((yy)"if (!");
s(yy_1_1_2_2_2_1_2_1);
yy_1_1_2_2_2_1_3_1 = yyv_Predicate;
id(yy_1_1_2_2_2_1_3_1);
yy_1_1_2_2_2_1_4_1 = ((yy)"(");
s(yy_1_1_2_2_2_1_4_1);
yy_1_1_2_2_2_1_5_1 = yyv_InArgs;
yy_1_1_2_2_2_1_5_2 = yyv_Pathes;
yy_1_1_2_2_2_1_5_3 = ((yy)"");
CodeTryConditionArgs(yy_1_1_2_2_2_1_5_1, yy_1_1_2_2_2_1_5_2, yy_1_1_2_2_2_1_5_3, &yy_1_1_2_2_2_1_5_4);
yyv_Sep = yy_1_1_2_2_2_1_5_4;
yy_1_1_2_2_2_1_6_1 = yyv_OutArgs;
yy_1_1_2_2_2_1_6_2 = yyv_Sep;
CodeTryConditionOutArgs(yy_1_1_2_2_2_1_6_1, yy_1_1_2_2_2_1_6_2);
yy_1_1_2_2_2_1_7 = yyglov_tryLabel;
if (yy_1_1_2_2_2_1_7 == (yy) yyu) yyErr(1,4744);
yyv_TryLabel = yy_1_1_2_2_2_1_7;
yy_1_1_2_2_2_1_8_1 = ((yy)")) goto yyL_");
s(yy_1_1_2_2_2_1_8_1);
yy_1_1_2_2_2_1_9_1 = yyv_TryLabel;
i(yy_1_1_2_2_2_1_9_1);
yy_1_1_2_2_2_1_10_1 = ((yy)";");
s(yy_1_1_2_2_2_1_10_1);
nl();
yy_1_1_2_2_2_1_12_1 = yyv_OldList;
yy_1_1_2_2_2_1_12_2 = yy_1_1_2_2_2_1_12_1;
yyv_NewList = yy_1_1_2_2_2_1_12_2;
goto yysl_435_1_1_1_2_2_2;
yyfl_435_1_1_1_2_2_2_1 : ;
yy_1_1_2_2_2_2_1_1 = yyv_Class;
yy_1_1_2_2_2_2_1_2 = yy_1_1_2_2_2_2_1_1;
if (yy_1_1_2_2_2_2_1_2[0] != 5) goto yyfl_435_1_1_1_2_2_2_2;
{
yy yysb = yyb;
yy_1_1_2_2_2_2_2_1_1_1 = yyv_InArgs;
yy_1_1_2_2_2_2_2_1_1_2 = yy_1_1_2_2_2_2_2_1_1_1;
if (yy_1_1_2_2_2_2_2_1_1_2[0] != 1) goto yyfl_435_1_1_1_2_2_2_2_2_1;
yy_1_1_2_2_2_2_2_1_1_2_1 = ((yy)yy_1_1_2_2_2_2_2_1_1_2[1]);
yy_1_1_2_2_2_2_2_1_1_2_2 = ((yy)yy_1_1_2_2_2_2_2_1_1_2[2]);
yy_1_1_2_2_2_2_2_1_1_2_3 = ((yy)yy_1_1_2_2_2_2_2_1_1_2[3]);
yyv_Arg = yy_1_1_2_2_2_2_2_1_1_2_1;
yyv_ArgPos = yy_1_1_2_2_2_2_2_1_1_2_2;
{
yy yysb = yyb;
yyb = yyh;
yyh += 6; if (yyh > yyhx) yyExtend();
yy_1_1_2_2_2_2_2_1_2_1_1_1 = yyv_Arg;
yy_1_1_2_2_2_2_2_1_2_1_1_2 = yy_1_1_2_2_2_2_2_1_2_1_1_1;
if (yy_1_1_2_2_2_2_2_1_2_1_1_2[0] != 5) goto yyfl_435_1_1_1_2_2_2_2_2_1_2_1;
yy_1_1_2_2_2_2_2_1_2_1_1_2_1 = ((yy)yy_1_1_2_2_2_2_2_1_2_1_1_2[1]);
yy_1_1_2_2_2_2_2_1_2_1_1_2_2 = ((yy)yy_1_1_2_2_2_2_2_1_2_1_1_2[2]);
yy_1_1_2_2_2_2_2_1_2_1_1_2_3 = ((yy)yy_1_1_2_2_2_2_2_1_2_1_1_2[3]);
yyv_Tmp = yy_1_1_2_2_2_2_2_1_2_1_1_2_1;
yyv_Name = yy_1_1_2_2_2_2_2_1_2_1_1_2_2;
yyv_VarPos = yy_1_1_2_2_2_2_2_1_2_1_1_2_3;
{
yy yysb = yyb;
yy_1_1_2_2_2_2_2_1_2_1_2_1_1_1 = yyv_Name;
if (! GetLocalMeaning(yy_1_1_2_2_2_2_2_1_2_1_2_1_1_1, &yy_1_1_2_2_2_2_2_1_2_1_2_1_1_2)) goto yyfl_435_1_1_1_2_2_2_2_2_1_2_1_2_1;
yyv_Type = yy_1_1_2_2_2_2_2_1_2_1_2_1_1_2;
goto yysl_435_1_1_1_2_2_2_2_2_1_2_1_2;
yyfl_435_1_1_1_2_2_2_2_2_1_2_1_2_1 : ;
yy_1_1_2_2_2_2_2_1_2_1_2_2_1_1 = ((yy)"'");
yy_1_1_2_2_2_2_2_1_2_1_2_2_1_2 = yyv_Name;
yy_1_1_2_2_2_2_2_1_2_1_2_2_1_3 = ((yy)"' is not (component of) primary CHOICE argument");
yy_1_1_2_2_2_2_2_1_2_1_2_2_1_4 = yyv_VarPos;
MESSAGE1(yy_1_1_2_2_2_2_2_1_2_1_2_2_1_1, yy_1_1_2_2_2_2_2_1_2_1_2_2_1_2, yy_1_1_2_2_2_2_2_1_2_1_2_2_1_3, yy_1_1_2_2_2_2_2_1_2_1_2_2_1_4);
yy_1_1_2_2_2_2_2_1_2_1_2_2_2 = yyglov_errorId;
if (yy_1_1_2_2_2_2_2_1_2_1_2_2_2 == (yy) yyu) yyErr(1,4755);
yyv_Type = yy_1_1_2_2_2_2_2_1_2_1_2_2_2;
goto yysl_435_1_1_1_2_2_2_2_2_1_2_1_2;
yysl_435_1_1_1_2_2_2_2_2_1_2_1_2 : ;
yyb = yysb;
}
yy_1_1_2_2_2_2_2_1_2_1_3_1 = yyv_Name;
yy_1_1_2_2_2_2_2_1_2_1_3_2 = yyv_Pathes;
yy_1_1_2_2_2_2_2_1_2_1_3_3 = yyv_VarPos;
LookupPath(yy_1_1_2_2_2_2_2_1_2_1_3_1, yy_1_1_2_2_2_2_2_1_2_1_3_2, yy_1_1_2_2_2_2_2_1_2_1_3_3, &yy_1_1_2_2_2_2_2_1_2_1_3_4);
yyv_Path = yy_1_1_2_2_2_2_2_1_2_1_3_4;
yy_1_1_2_2_2_2_2_1_2_1_4 = yyglov_choice_Types;
if (yy_1_1_2_2_2_2_2_1_2_1_4 == (yy) yyu) yyErr(1,4758);
yyv_TypeList = yy_1_1_2_2_2_2_2_1_2_1_4;
yy_1_1_2_2_2_2_2_1_2_1_5_1 = yyv_TypeList;
yy_1_1_2_2_2_2_2_1_2_1_5_2 = yyv_Type;
GetChoiceTypeIndex(yy_1_1_2_2_2_2_2_1_2_1_5_1, yy_1_1_2_2_2_2_2_1_2_1_5_2, &yy_1_1_2_2_2_2_2_1_2_1_5_3);
yyv_ChoiceTypeIndex = yy_1_1_2_2_2_2_2_1_2_1_5_3;
yy_1_1_2_2_2_2_2_1_2_1_6_1_1 = yyv_Predicate;
yy_1_1_2_2_2_2_2_1_2_1_6_1_2 = yyv_Name;
yy_1_1_2_2_2_2_2_1_2_1_6_1_3 = yyv_Path;
yy_1_1_2_2_2_2_2_1_2_1_6_1_4 = yyv_ChoiceTypeIndex;
yy_1_1_2_2_2_2_2_1_2_1_6_1_5 = yyv_OldList;
yy_1_1_2_2_2_2_2_1_2_1_6_1 = yyb + 0;
yy_1_1_2_2_2_2_2_1_2_1_6_1[0] = 1;
yy_1_1_2_2_2_2_2_1_2_1_6_1[1] = ((long)yy_1_1_2_2_2_2_2_1_2_1_6_1_1);
yy_1_1_2_2_2_2_2_1_2_1_6_1[2] = ((long)yy_1_1_2_2_2_2_2_1_2_1_6_1_2);
yy_1_1_2_2_2_2_2_1_2_1_6_1[3] = ((long)yy_1_1_2_2_2_2_2_1_2_1_6_1_3);
yy_1_1_2_2_2_2_2_1_2_1_6_1[4] = ((long)yy_1_1_2_2_2_2_2_1_2_1_6_1_4);
yy_1_1_2_2_2_2_2_1_2_1_6_1[5] = ((long)yy_1_1_2_2_2_2_2_1_2_1_6_1_5);
yy_1_1_2_2_2_2_2_1_2_1_6_2 = yy_1_1_2_2_2_2_2_1_2_1_6_1;
yyv_NewList = yy_1_1_2_2_2_2_2_1_2_1_6_2;
goto yysl_435_1_1_1_2_2_2_2_2_1_2;
yyfl_435_1_1_1_2_2_2_2_2_1_2_1 : ;
yy_1_1_2_2_2_2_2_1_2_2_1_1 = ((yy)"illegal input for CHOICE predicate");
yy_1_1_2_2_2_2_2_1_2_2_1_2 = yyv_ArgPos;
MESSAGE(yy_1_1_2_2_2_2_2_1_2_2_1_1, yy_1_1_2_2_2_2_2_1_2_2_1_2);
yy_1_1_2_2_2_2_2_1_2_2_2_1 = yyv_OldList;
yy_1_1_2_2_2_2_2_1_2_2_2_2 = yy_1_1_2_2_2_2_2_1_2_2_2_1;
yyv_NewList = yy_1_1_2_2_2_2_2_1_2_2_2_2;
goto yysl_435_1_1_1_2_2_2_2_2_1_2;
yysl_435_1_1_1_2_2_2_2_2_1_2 : ;
yyb = yysb;
}
goto yysl_435_1_1_1_2_2_2_2_2;
yyfl_435_1_1_1_2_2_2_2_2_1 : ;
yy_1_1_2_2_2_2_2_2_1_1 = yyv_InArgs;
yy_1_1_2_2_2_2_2_2_1_2 = yy_1_1_2_2_2_2_2_2_1_1;
if (yy_1_1_2_2_2_2_2_2_1_2[0] != 2) goto yyfl_435_1_1_1_2_2_2_2_2_2;
yy_1_1_2_2_2_2_2_2_1_2_1 = ((yy)yy_1_1_2_2_2_2_2_2_1_2[1]);
yyv_ArgPos = yy_1_1_2_2_2_2_2_2_1_2_1;
yy_1_1_2_2_2_2_2_2_2_1 = ((yy)"missing input argument");
yy_1_1_2_2_2_2_2_2_2_2 = yyv_ArgPos;
MESSAGE(yy_1_1_2_2_2_2_2_2_2_1, yy_1_1_2_2_2_2_2_2_2_2);
yy_1_1_2_2_2_2_2_2_3_1 = yyv_OldList;
yy_1_1_2_2_2_2_2_2_3_2 = yy_1_1_2_2_2_2_2_2_3_1;
yyv_NewList = yy_1_1_2_2_2_2_2_2_3_2;
goto yysl_435_1_1_1_2_2_2_2_2;
yyfl_435_1_1_1_2_2_2_2_2_2 : ;
goto yyfl_435_1_1_1_2_2_2_2;
yysl_435_1_1_1_2_2_2_2_2 : ;
yyb = yysb;
}
goto yysl_435_1_1_1_2_2_2;
yyfl_435_1_1_1_2_2_2_2 : ;
yy_1_1_2_2_2_3_1_1 = yyv_OldList;
yy_1_1_2_2_2_3_1_2 = yy_1_1_2_2_2_3_1_1;
yyv_NewList = yy_1_1_2_2_2_3_1_2;
goto yysl_435_1_1_1_2_2_2;
yysl_435_1_1_1_2_2_2 : ;
yyb = yysb;
}
goto yysl_435_1_1_1_2;
yyfl_435_1_1_1_2_2 : ;
yy_1_1_2_3_1_1 = yyv_OldList;
yy_1_1_2_3_1_2 = yy_1_1_2_3_1_1;
yyv_NewList = yy_1_1_2_3_1_2;
goto yysl_435_1_1_1_2;
yysl_435_1_1_1_2 : ;
yyb = yysb;
}
goto yysl_435_1_1;
yyfl_435_1_1_1 : ;
yy_1_2_1_1 = yyv_Head;
POS_MEMBER(yy_1_2_1_1, &yy_1_2_1_2);
yyv_Pos = yy_1_2_1_2;
yy_1_2_2_1 = ((yy)"Only predicate calls allowed in 'CHOICE' rules");
yy_1_2_2_2 = yyv_Pos;
MESSAGE(yy_1_2_2_1, yy_1_2_2_2);
yy_1_2_3_1 = yyv_OldList;
yy_1_2_3_2 = yy_1_2_3_1;
yyv_NewList = yy_1_2_3_2;
goto yysl_435_1_1;
yysl_435_1_1 : ;
yyb = yysb;
}
yy_2_1 = yyv_Tail;
yy_2_2 = yyv_Pathes;
yy_2_3 = yyv_NewList;
CodeTryConditions(yy_2_1, yy_2_2, yy_2_3, &yy_2_4);
yyv_List = yy_2_4;
yy_0_4 = yyv_List;
*yyout_1 = yy_0_4;
return;
yyfl_435_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Pathes;
yy yy_0_2;
yy yyv_L;
yy yy_0_3;
yy yy_0_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 2) goto yyfl_435_2;
yyv_Pathes = yy_0_2;
yyv_L = yy_0_3;
yy_0_4 = yyv_L;
*yyout_1 = yy_0_4;
return;
yyfl_435_2 : ;
}
yyErr(2,4667);
}
CodeTryConditionOutArgs(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_H;
yy yy_0_1_1;
yy yy_0_1_2;
yy yyv_T;
yy yy_0_1_3;
yy yyv_Sep;
yy yy_0_2;
yy yy_1_1;
yy yy_2_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_436_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_H = yy_0_1_1;
yyv_T = yy_0_1_3;
yyv_Sep = yy_0_2;
yy_1_1 = yyv_Sep;
s(yy_1_1);
yy_2_1 = ((yy)"&yyDummy");
s(yy_2_1);
return;
yyfl_436_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_Sep;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_436_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yyv_Sep = yy_0_2;
return;
yyfl_436_2 : ;
}
yyErr(2,4789);
}
int IsInChoiceTypeList(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yyv_Type;
yy yy_0_1;
yy yy_0_2;
yy yyv_Id;
yy yy_0_2_1;
yy yyv_Index;
yy yy_0_2_2;
yy yyv_Tail;
yy yy_0_2_3;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Type = yy_0_1;
if (yy_0_2[0] != 1) goto yyfl_437_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yy_0_2_3 = ((yy)yy_0_2[3]);
yyv_Id = yy_0_2_1;
yyv_Index = yy_0_2_2;
yyv_Tail = yy_0_2_3;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Type;
yy_1_1_1_2 = yyv_Id;
if (! yyeq_IDENT(yy_1_1_1_1, yy_1_1_1_2)) goto yyfl_437_1_1_1;
goto yysl_437_1_1;
yyfl_437_1_1_1 : ;
yy_1_2_1_1 = yyv_Type;
yy_1_2_1_2 = yyv_Tail;
if (! IsInChoiceTypeList(yy_1_2_1_1, yy_1_2_1_2)) goto yyfl_437_1_1_2;
goto yysl_437_1_1;
yyfl_437_1_1_2 : ;
goto yyfl_437_1;
yysl_437_1_1 : ;
yyb = yysb;
}
return 1;
yyfl_437_1 : ;
}
return 0;
}
GetChoiceTypeIndex(yyin_1, yyin_2, yyout_1)
yy yyin_1;
yy yyin_2;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Id;
yy yy_0_1_1;
yy yyv_Index;
yy yy_0_1_2;
yy yyv_Tail;
yy yy_0_1_3;
yy yyv_Type;
yy yy_0_2;
yy yy_0_3;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yy_1_1_2_1;
yy yyv_X;
yy yy_1_1_2_2;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_1_2_1_3;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_438_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Id = yy_0_1_1;
yyv_Index = yy_0_1_2;
yyv_Tail = yy_0_1_3;
yyv_Type = yy_0_2;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Type;
yy_1_1_1_2 = yyv_Id;
if (! yyeq_IDENT(yy_1_1_1_1, yy_1_1_1_2)) goto yyfl_438_1_1_1;
yy_1_1_2_1 = yyv_Index;
yy_1_1_2_2 = yy_1_1_2_1;
yyv_X = yy_1_1_2_2;
goto yysl_438_1_1;
yyfl_438_1_1_1 : ;
yy_1_2_1_1 = yyv_Tail;
yy_1_2_1_2 = yyv_Type;
GetChoiceTypeIndex(yy_1_2_1_1, yy_1_2_1_2, &yy_1_2_1_3);
yyv_X = yy_1_2_1_3;
goto yysl_438_1_1;
yysl_438_1_1 : ;
yyb = yysb;
}
yy_0_3 = yyv_X;
*yyout_1 = yy_0_3;
return;
yyfl_438_1 : ;
}
yyErr(2,4802);
}
CodeTryConditionArgs(yyin_1, yyin_2, yyin_3, yyout_1)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy *yyout_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Head;
yy yy_0_1_1;
yy yyv_Pos;
yy yy_0_1_2;
yy yyv_Tail;
yy yy_0_1_3;
yy yyv_Pathes;
yy yy_0_2;
yy yyv_Sep;
yy yy_0_3;
yy yy_0_4;
yy yy_1_1;
yy yy_2_1;
yy yy_2_2;
yy yy_2_3;
yy yy_3_1;
yy yy_3_2;
yy yy_3_3;
yy yyv_Sep2;
yy yy_3_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 1) goto yyfl_439_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Head = yy_0_1_1;
yyv_Pos = yy_0_1_2;
yyv_Tail = yy_0_1_3;
yyv_Pathes = yy_0_2;
yyv_Sep = yy_0_3;
yy_1_1 = yyv_Sep;
s(yy_1_1);
yy_2_1 = yyv_Head;
yy_2_2 = yyv_Pos;
yy_2_3 = yyv_Pathes;
CodeTryConditionArg(yy_2_1, yy_2_2, yy_2_3);
yy_3_1 = yyv_Tail;
yy_3_2 = yyv_Pathes;
yy_3_3 = ((yy)", ");
CodeTryConditionArgs(yy_3_1, yy_3_2, yy_3_3, &yy_3_4);
yyv_Sep2 = yy_3_4;
yy_0_4 = yyv_Sep2;
*yyout_1 = yy_0_4;
return;
yyfl_439_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yy_0_2;
yy yyv_Sep;
yy yy_0_3;
yy yy_0_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
if (yy_0_1[0] != 2) goto yyfl_439_2;
yy_0_1_1 = ((yy)yy_0_1[1]);
yyv_Sep = yy_0_3;
yy_0_4 = yyv_Sep;
*yyout_1 = yy_0_4;
return;
yyfl_439_2 : ;
}
yyErr(2,4810);
}
CodeTryConditionArg(yyin_1, yyin_2, yyin_3)
yy yyin_1;
yy yyin_2;
yy yyin_3;
{
{
yy yyb;
yy yyv_Head;
yy yy_0_1;
yy yyv_Pos;
yy yy_0_2;
yy yyv_Pathes;
yy yy_0_3;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yyv_Tmp;
yy yy_1_1_1_2_1;
yy yyv_Name;
yy yy_1_1_1_2_2;
yy yy_1_1_1_2_3;
yy yy_1_1_2_1_1_1;
yy yy_1_1_2_1_1_2;
yy yy_1_1_2_2_1_1;
yy yy_1_1_2_2_1_2;
yy yy_1_1_2_2_1_3;
yy yy_1_1_2_2_1_4;
yy yy_1_1_3_1;
yy yy_1_1_3_2;
yy yy_1_1_3_3;
yy yyv_Path;
yy yy_1_1_3_4;
yy yy_1_1_4_1;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_1_2_1_2_1;
yy yyv_N;
yy yy_1_2_1_2_2;
yy yy_1_2_1_2_3;
yy yy_1_2_2_1;
yy yy_1_3_1_1;
yy yy_1_3_1_2;
yy yy_1_3_1_2_1;
yy yyv_Str;
yy yy_1_3_1_2_2;
yy yy_1_3_1_2_3;
yy yy_1_3_2_1;
yy yy_1_4_1_1;
yy yy_1_4_1_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Head = yy_0_1;
yyv_Pos = yy_0_2;
yyv_Pathes = yy_0_3;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Head;
yy_1_1_1_2 = yy_1_1_1_1;
if (yy_1_1_1_2[0] != 5) goto yyfl_440_1_1_1;
yy_1_1_1_2_1 = ((yy)yy_1_1_1_2[1]);
yy_1_1_1_2_2 = ((yy)yy_1_1_1_2[2]);
yy_1_1_1_2_3 = ((yy)yy_1_1_1_2[3]);
yyv_Tmp = yy_1_1_1_2_1;
yyv_Name = yy_1_1_1_2_2;
{
yy yysb = yyb;
yy_1_1_2_1_1_1 = yyv_Name;
if (! GetLocalMeaning(yy_1_1_2_1_1_1, &yy_1_1_2_1_1_2)) goto yyfl_440_1_1_1_2_1;
goto yysl_440_1_1_1_2;
yyfl_440_1_1_1_2_1 : ;
yy_1_1_2_2_1_1 = ((yy)"'");
yy_1_1_2_2_1_2 = yyv_Name;
yy_1_1_2_2_1_3 = ((yy)"' is not (component of) primary CHOICE argument");
yy_1_1_2_2_1_4 = yyv_Pos;
MESSAGE1(yy_1_1_2_2_1_1, yy_1_1_2_2_1_2, yy_1_1_2_2_1_3, yy_1_1_2_2_1_4);
goto yysl_440_1_1_1_2;
yysl_440_1_1_1_2 : ;
yyb = yysb;
}
yy_1_1_3_1 = yyv_Name;
yy_1_1_3_2 = yyv_Pathes;
yy_1_1_3_3 = yyv_Pos;
LookupPath(yy_1_1_3_1, yy_1_1_3_2, yy_1_1_3_3, &yy_1_1_3_4);
yyv_Path = yy_1_1_3_4;
yy_1_1_4_1 = yyv_Path;
argpath(yy_1_1_4_1);
goto yysl_440_1_1;
yyfl_440_1_1_1 : ;
yy_1_2_1_1 = yyv_Head;
yy_1_2_1_2 = yy_1_2_1_1;
if (yy_1_2_1_2[0] != 6) goto yyfl_440_1_1_2;
yy_1_2_1_2_1 = ((yy)yy_1_2_1_2[1]);
yy_1_2_1_2_2 = ((yy)yy_1_2_1_2[2]);
yy_1_2_1_2_3 = ((yy)yy_1_2_1_2[3]);
yyv_N = yy_1_2_1_2_2;
yy_1_2_2_1 = yyv_N;
i(yy_1_2_2_1);
goto yysl_440_1_1;
yyfl_440_1_1_2 : ;
yy_1_3_1_1 = yyv_Head;
yy_1_3_1_2 = yy_1_3_1_1;
if (yy_1_3_1_2[0] != 7) goto yyfl_440_1_1_3;
yy_1_3_1_2_1 = ((yy)yy_1_3_1_2[1]);
yy_1_3_1_2_2 = ((yy)yy_1_3_1_2[2]);
yy_1_3_1_2_3 = ((yy)yy_1_3_1_2[3]);
yyv_Str = yy_1_3_1_2_2;
yy_1_3_2_1 = yyv_Str;
qu_s(yy_1_3_2_1);
goto yysl_440_1_1;
yyfl_440_1_1_3 : ;
yy_1_4_1_1 = ((yy)"Illegal input for condition in CHOICE rule");
yy_1_4_1_2 = yyv_Pos;
MESSAGE(yy_1_4_1_1, yy_1_4_1_2);
goto yysl_440_1_1;
yysl_440_1_1 : ;
yyb = yysb;
}
return;
}
}
CodeCostUpdate(yyin_1, yyin_2, yyin_3, yyin_4, yyin_5)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy yyin_4;
yy yyin_5;
{
{
yy yyb;
yy yyv_Predicate;
yy yy_0_1;
yy yyv_Cost;
yy yy_0_2;
yy yyv_Line;
yy yy_0_3;
yy yyv_List;
yy yy_0_4;
yy yyv_ChainRule;
yy yy_0_5;
yy yy_1_1;
yy yy_2_1;
yy yy_4_1;
yy yy_5_1;
yy yy_7_1;
yy yy_8_1;
yy yy_9_1;
yy yy_11_1;
yy yy_12_1;
yy yy_13_1;
yy yy_15_1;
yy yy_16_1;
yy yy_17_1;
yy yy_19_1;
yy yy_20_1;
yy yy_21_1;
yy yy_22_1;
yy yy_23_1;
yy yy_25_1_1_1;
yy yy_25_1_1_2;
yy yy_25_1_2_1;
yy yy_26_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yy_0_4 = yyin_4;
yy_0_5 = yyin_5;
yyv_Predicate = yy_0_1;
yyv_Cost = yy_0_2;
yyv_Line = yy_0_3;
yyv_List = yy_0_4;
yyv_ChainRule = yy_0_5;
yy_1_1 = ((yy)"yycost = ");
s(yy_1_1);
yy_2_1 = yyv_Cost;
i(yy_2_1);
nl();
yy_4_1 = yyv_List;
CodeCostComponents(yy_4_1);
yy_5_1 = ((yy)";");
s(yy_5_1);
nl();
yy_7_1 = ((yy)"if (yycost < yyCostFor_");
s(yy_7_1);
yy_8_1 = yyv_Predicate;
id(yy_8_1);
yy_9_1 = ((yy)") {");
s(yy_9_1);
nl();
yy_11_1 = ((yy)"int yyRule_");
s(yy_11_1);
yy_12_1 = yyv_Line;
i(yy_12_1);
yy_13_1 = ((yy)"();");
s(yy_13_1);
nl();
yy_15_1 = ((yy)"yyCostFor_");
s(yy_15_1);
yy_16_1 = yyv_Predicate;
id(yy_16_1);
yy_17_1 = ((yy)" = yycost;");
s(yy_17_1);
nl();
yy_19_1 = ((yy)"yyRuleFor_");
s(yy_19_1);
yy_20_1 = yyv_Predicate;
id(yy_20_1);
yy_21_1 = ((yy)" = (intptr_t) yyRule_");
s(yy_21_1);
yy_22_1 = yyv_Line;
i(yy_22_1);
yy_23_1 = ((yy)";");
s(yy_23_1);
nl();
{
yy yysb = yyb;
yy_25_1_1_1 = yyv_ChainRule;
yy_25_1_1_2 = yy_25_1_1_1;
if (yy_25_1_1_2[0] != 1) goto yyfl_441_1_25_1;
yy_25_1_2_1 = ((yy)"yyapplied = 1;");
s(yy_25_1_2_1);
nl();
goto yysl_441_1_25;
yyfl_441_1_25_1 : ;
goto yysl_441_1_25;
yysl_441_1_25 : ;
yyb = yysb;
}
yy_26_1 = ((yy)"}");
s(yy_26_1);
nl();
return;
}
}
CodeCostComponents(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Predicate;
yy yy_0_1_1;
yy yyv_Var;
yy yy_0_1_2;
yy yyv_Path;
yy yy_0_1_3;
yy yyv_ChoiceTypeIndex;
yy yy_0_1_4;
yy yyv_Tail;
yy yy_0_1_5;
yy yyv_PredicateList;
yy yy_1;
yy yy_2_1;
yy yy_2_2;
yy yy_2_3;
yy yyv_Offset;
yy yy_2_4;
yy yy_3_1_1_1;
yy yy_3_1_1_2;
yy yy_3_1_2_1;
yy yy_3_1_3_1;
yy yy_3_2_1_1;
yy yy_3_2_2_1;
yy yy_3_2_3_1;
yy yy_3_2_4_1;
yy yy_3_2_5_1;
yy yy_4_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_442_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yy_0_1_4 = ((yy)yy_0_1[4]);
yy_0_1_5 = ((yy)yy_0_1[5]);
yyv_Predicate = yy_0_1_1;
yyv_Var = yy_0_1_2;
yyv_Path = yy_0_1_3;
yyv_ChoiceTypeIndex = yy_0_1_4;
yyv_Tail = yy_0_1_5;
yy_1 = (yy) yyv_ChoiceTypeIndex[1];
if (yy_1 == (yy) yyu) yyErr(1,4855);
yyv_PredicateList = yy_1;
yy_2_1 = yyv_Predicate;
yy_2_2 = yyv_PredicateList;
yy_2_3 = ((yy)0);
GetCntlOffset(yy_2_1, yy_2_2, yy_2_3, &yy_2_4);
yyv_Offset = yy_2_4;
{
yy yysb = yyb;
yy_3_1_1_1 = yyv_Path;
yy_3_1_1_2 = yy_3_1_1_1;
if (yy_3_1_1_2[0] != 2) goto yyfl_442_1_3_1;
yy_3_1_2_1 = ((yy)"+ yyCostFor_");
s(yy_3_1_2_1);
yy_3_1_3_1 = yyv_Predicate;
id(yy_3_1_3_1);
goto yysl_442_1_3;
yyfl_442_1_3_1 : ;
yy_3_2_1_1 = ((yy)"+ ((yy)");
s(yy_3_2_1_1);
yy_3_2_2_1 = yyv_Path;
ulcntlpath(yy_3_2_2_1);
yy_3_2_3_1 = ((yy)")[");
s(yy_3_2_3_1);
yy_3_2_4_1 = yyv_Offset;
i(yy_3_2_4_1);
yy_3_2_5_1 = ((yy)"]");
s(yy_3_2_5_1);
nl();
goto yysl_442_1_3;
yysl_442_1_3 : ;
yyb = yysb;
}
yy_4_1 = yyv_Tail;
CodeCostComponents(yy_4_1);
return;
yyfl_442_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_442_2;
return;
yyfl_442_2 : ;
}
yyErr(2,4853);
}
GetCntlOffset(yyin_1, yyin_2, yyin_3, yyout_1)
yy yyin_1;
yy yyin_2;
yy yyin_3;
yy *yyout_1;
{
{
yy yyb;
yy yyv_Predicate;
yy yy_0_1;
yy yy_0_2;
yy yyv_Pred;
yy yy_0_2_1;
yy yyv_Tail;
yy yy_0_2_2;
yy yyv_N1;
yy yy_0_3;
yy yy_0_4;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yy_1_1_2_1;
yy yyv_N2;
yy yy_1_1_2_2;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_1_2_1_3;
yy yy_1_2_1_3_1;
yy yy_1_2_1_3_2;
yy yy_1_2_1_4;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yy_0_3 = yyin_3;
yyv_Predicate = yy_0_1;
if (yy_0_2[0] != 1) goto yyfl_443_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yyv_Pred = yy_0_2_1;
yyv_Tail = yy_0_2_2;
yyv_N1 = yy_0_3;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Predicate;
yy_1_1_1_2 = yyv_Pred;
if (! yyeq_IDENT(yy_1_1_1_1, yy_1_1_1_2)) goto yyfl_443_1_1_1;
yy_1_1_2_1 = yyv_N1;
yy_1_1_2_2 = yy_1_1_2_1;
yyv_N2 = yy_1_1_2_2;
goto yysl_443_1_1;
yyfl_443_1_1_1 : ;
yy_1_2_1_1 = yyv_Predicate;
yy_1_2_1_2 = yyv_Tail;
yy_1_2_1_3_1 = yyv_N1;
yy_1_2_1_3_2 = ((yy)2);
yy_1_2_1_3 = (yy)(((long)yy_1_2_1_3_1)+((long)yy_1_2_1_3_2));
GetCntlOffset(yy_1_2_1_1, yy_1_2_1_2, yy_1_2_1_3, &yy_1_2_1_4);
yyv_N2 = yy_1_2_1_4;
goto yysl_443_1_1;
yysl_443_1_1 : ;
yyb = yysb;
}
yy_0_4 = yyv_N2;
*yyout_1 = yy_0_4;
return;
yyfl_443_1 : ;
}
yyErr(2,4865);
}
CodeChoiceRuleProcs_Decls(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Decl;
yy yy_0_1_1;
yy yy_0_1_1_1;
yy yyv_Id;
yy yy_0_1_1_1_1;
yy yyv_P;
yy yy_0_1_1_1_2;
yy yy_0_1_1_1_3;
yy yyv_Name;
yy yy_0_1_1_1_3_1;
yy yy_0_1_1_1_3_2;
yy yy_0_1_1_1_3_3;
yy yyv_InArgs;
yy yy_0_1_1_1_3_3_1;
yy yyv_OutArgs;
yy yy_0_1_1_1_3_3_2;
yy yyv_Rules;
yy yy_0_1_1_1_3_4;
yy yyv_Tail;
yy yy_0_1_2;
yy yy_1_1;
yy yy_1_2;
yy yy_2_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_444_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_1_1 = yy_0_1_1;
yyv_Decl = yy_0_1_1;
if (yy_0_1_1_1[0] != 2) goto yyfl_444_1;
yy_0_1_1_1_1 = ((yy)yy_0_1_1_1[1]);
yy_0_1_1_1_2 = ((yy)yy_0_1_1_1[2]);
yy_0_1_1_1_3 = ((yy)yy_0_1_1_1[3]);
yyv_Id = yy_0_1_1_1_1;
yyv_P = yy_0_1_1_1_2;
if (yy_0_1_1_1_3[0] != 4) goto yyfl_444_1;
yy_0_1_1_1_3_1 = ((yy)yy_0_1_1_1_3[1]);
yy_0_1_1_1_3_2 = ((yy)yy_0_1_1_1_3[2]);
yy_0_1_1_1_3_3 = ((yy)yy_0_1_1_1_3[3]);
yy_0_1_1_1_3_4 = ((yy)yy_0_1_1_1_3[4]);
yyv_Name = yy_0_1_1_1_3_1;
if (yy_0_1_1_1_3_2[0] != 5) goto yyfl_444_1;
if (yy_0_1_1_1_3_3[0] != 1) goto yyfl_444_1;
yy_0_1_1_1_3_3_1 = ((yy)yy_0_1_1_1_3_3[1]);
yy_0_1_1_1_3_3_2 = ((yy)yy_0_1_1_1_3_3[2]);
yyv_InArgs = yy_0_1_1_1_3_3_1;
yyv_OutArgs = yy_0_1_1_1_3_3_2;
yyv_Rules = yy_0_1_1_1_3_4;
yyv_Tail = yy_0_1_2;
yy_1_1 = yyv_Decl;
yy_1_2 = yyv_Rules;
CodeChoiceRuleProcs_Rules(yy_1_1, yy_1_2);
yy_2_1 = yyv_Tail;
CodeChoiceRuleProcs_Decls(yy_2_1);
return;
yyfl_444_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_444_2;
return;
yyfl_444_2 : ;
}
yyErr(2,4872);
}
CodeChoiceRuleProcs_Rules(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yyv_Decl;
yy yy_0_1;
yy yy_0_2;
yy yyv_Head;
yy yy_0_2_1;
yy yyv_Tail;
yy yy_0_2_2;
yy yy_1_1;
yy yy_1_2;
yy yy_2_1;
yy yy_2_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Decl = yy_0_1;
if (yy_0_2[0] != 1) goto yyfl_445_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yyv_Head = yy_0_2_1;
yyv_Tail = yy_0_2_2;
yy_1_1 = yyv_Decl;
yy_1_2 = yyv_Head;
CodeChoiceRuleProc(yy_1_1, yy_1_2);
yy_2_1 = yyv_Decl;
yy_2_2 = yyv_Tail;
CodeChoiceRuleProcs_Rules(yy_2_1, yy_2_2);
return;
yyfl_445_1 : ;
}
{
yy yyb;
yy yyv_Decl;
yy yy_0_1;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Decl = yy_0_1;
if (yy_0_2[0] != 2) goto yyfl_445_2;
return;
yyfl_445_2 : ;
}
yyErr(2,4884);
}
CodeChoiceRuleProc(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yyv_Id;
yy yy_0_1_1;
yy yyv_P;
yy yy_0_1_2;
yy yy_0_1_3;
yy yyv_Name;
yy yy_0_1_3_1;
yy yy_0_1_3_2;
yy yy_0_1_3_3;
yy yyv_FInArgs;
yy yy_0_1_3_3_1;
yy yyv_FOutArgs;
yy yy_0_1_3_3_2;
yy yyv_Rules;
yy yy_0_1_3_4;
yy yyv_Rule;
yy yy_0_2;
yy yy_0_2_1;
yy yy_0_2_1_1;
yy yyv_Predicate;
yy yy_0_2_1_1_1;
yy yyv_Pos;
yy yy_0_2_1_1_2;
yy yyv_In;
yy yy_0_2_1_1_3;
yy yyv_Out;
yy yy_0_2_1_1_4;
yy yyv_Rhs;
yy yy_0_2_1_2;
yy yyv_Cost;
yy yy_0_2_1_3;
yy yy_1;
yy yy_2_1;
yy yyv_Line;
yy yy_2_2;
yy yy_3_1;
yy yy_4_1;
yy yy_5_1;
yy yy_6_1;
yy yy_6_2;
yy yy_6_3;
yy yy_6_4;
yy yyv_Sep;
yy yy_6_5;
yy yy_7_1;
yy yy_7_2;
yy yy_7_3;
yy yy_7_4;
yy yyv_Dummy;
yy yy_7_5;
yy yy_8_1;
yy yy_10_1;
yy yy_12_1;
yy yy_12_2;
yy yy_12_3;
yy yy_13_1;
yy yy_13_2;
yy yy_13_3;
yy yy_14_1;
yy yy_16;
yy yy_17;
yy yy_18;
yy yy_19;
yy yy_20_1;
yy yy_20_2;
yy yyv_Arg;
yy yy_20_2_1;
yy yy_20_2_2;
yy yy_20_2_3;
yy yy_21_1;
yy yy_21_2;
yy yy_21_3;
yy yyv_Pathes;
yy yy_21_4;
yy yy_22;
yy yy_23_1;
yy yy_23_2;
yy yy_23_3;
yy yy_23_4;
yy yy_23_5;
yy yy_23_6;
yy yyv_MayFailHead;
yy yy_23_7;
yy yy_24_1_1;
yy yy_24_1_2_1;
yy yy_24_1_3_1;
yy yy_24_1_4_1;
yy yy_25_1;
yy yy_27;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_447_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
yyv_Id = yy_0_1_1;
yyv_P = yy_0_1_2;
if (yy_0_1_3[0] != 4) goto yyfl_447_1;
yy_0_1_3_1 = ((yy)yy_0_1_3[1]);
yy_0_1_3_2 = ((yy)yy_0_1_3[2]);
yy_0_1_3_3 = ((yy)yy_0_1_3[3]);
yy_0_1_3_4 = ((yy)yy_0_1_3[4]);
yyv_Name = yy_0_1_3_1;
if (yy_0_1_3_2[0] != 5) goto yyfl_447_1;
if (yy_0_1_3_3[0] != 1) goto yyfl_447_1;
yy_0_1_3_3_1 = ((yy)yy_0_1_3_3[1]);
yy_0_1_3_3_2 = ((yy)yy_0_1_3_3[2]);
yyv_FInArgs = yy_0_1_3_3_1;
yyv_FOutArgs = yy_0_1_3_3_2;
yyv_Rules = yy_0_1_3_4;
yy_0_2_1 = yy_0_2;
yyv_Rule = yy_0_2;
if (yy_0_2_1[0] != 1) goto yyfl_447_1;
yy_0_2_1_1 = ((yy)yy_0_2_1[1]);
yy_0_2_1_2 = ((yy)yy_0_2_1[2]);
yy_0_2_1_3 = ((yy)yy_0_2_1[3]);
if (yy_0_2_1_1[0] != 1) goto yyfl_447_1;
yy_0_2_1_1_1 = ((yy)yy_0_2_1_1[1]);
yy_0_2_1_1_2 = ((yy)yy_0_2_1_1[2]);
yy_0_2_1_1_3 = ((yy)yy_0_2_1_1[3]);
yy_0_2_1_1_4 = ((yy)yy_0_2_1_1[4]);
yyv_Predicate = yy_0_2_1_1_1;
yyv_Pos = yy_0_2_1_1_2;
yyv_In = yy_0_2_1_1_3;
yyv_Out = yy_0_2_1_1_4;
yyv_Rhs = yy_0_2_1_2;
yyv_Cost = yy_0_2_1_3;
yyb = yyh;
yyh += 7; if (yyh > yyhx) yyExtend();
yy_1 = yyb + 0;
yy_1[0] = 1;
yyglov_insideChoiceRule = yy_1;
yy_2_1 = yyv_Pos;
PosToLineNumber(yy_2_1, &yy_2_2);
yyv_Line = yy_2_2;
yy_3_1 = ((yy)"yyRule_");
s(yy_3_1);
yy_4_1 = yyv_Line;
i(yy_4_1);
yy_5_1 = ((yy)"(yyCntl, ");
s(yy_5_1);
yy_6_1 = yyv_FInArgs;
yy_6_2 = ((yy)1);
yy_6_3 = ((yy)"yyin_");
yy_6_4 = ((yy)"");
ListFormalParams(yy_6_1, yy_6_2, yy_6_3, yy_6_4, &yy_6_5);
yyv_Sep = yy_6_5;
yy_7_1 = yyv_FOutArgs;
yy_7_2 = ((yy)1);
yy_7_3 = ((yy)"yyout_");
yy_7_4 = yyv_Sep;
ListFormalParams(yy_7_1, yy_7_2, yy_7_3, yy_7_4, &yy_7_5);
yyv_Dummy = yy_7_5;
yy_8_1 = ((yy)")");
s(yy_8_1);
nl();
yy_10_1 = ((yy)"yy yyCntl;");
s(yy_10_1);
nl();
yy_12_1 = yyv_FInArgs;
yy_12_2 = ((yy)1);
yy_12_3 = ((yy)"yyin_");
DeclareFormalParams(yy_12_1, yy_12_2, yy_12_3);
yy_13_1 = yyv_FOutArgs;
yy_13_2 = ((yy)1);
yy_13_3 = ((yy)"*yyout_");
DeclareFormalParams(yy_13_1, yy_13_2, yy_13_3);
yy_14_1 = ((yy)"{");
s(yy_14_1);
nl();
yy_16 = ((yy)0);
yyglov_currentProcNumber = yy_16;
yy_17 = ((yy)0);
yyglov_currentRuleNumber = yy_17;
yy_18 = yyb + 1;
yy_18[0] = 2;
yyglov_currentFailLabel = yy_18;
yy_19 = yyb + 2;
yy_19[0] = 2;
yyglov_currentFailLabelUsed = yy_19;
yy_20_1 = yyv_In;
yy_20_2 = yy_20_1;
if (yy_20_2[0] != 1) goto yyfl_447_1;
yy_20_2_1 = ((yy)yy_20_2[1]);
yy_20_2_2 = ((yy)yy_20_2[2]);
yy_20_2_3 = ((yy)yy_20_2[3]);
yyv_Arg = yy_20_2_1;
yy_21_1 = yyv_Arg;
yy_21_2 = yyb + 3;
yy_21_2[0] = 2;
yy_21_3 = yyb + 4;
yy_21_3[0] = 2;
MakePathes(yy_21_1, yy_21_2, yy_21_3, &yy_21_4);
yyv_Pathes = yy_21_4;
yy_22 = yyv_Pathes;
yyglov_pathes = yy_22;
yy_23_1 = yyb + 5;
yy_23_1[0] = 5;
yy_23_2 = yyv_Id;
yy_23_3 = yyv_FInArgs;
yy_23_4 = yyv_FOutArgs;
yy_23_5 = yyv_Rule;
yy_23_6 = ((yy)0);
Code_Rule(yy_23_1, yy_23_2, yy_23_3, yy_23_4, yy_23_5, yy_23_6, &yy_23_7);
yyv_MayFailHead = yy_23_7;
{
yy yysb = yyb;
yy_24_1_1 = yyglov_currentFailLabelUsed;
if (yy_24_1_1 == (yy) yyu) yyErr(1,4922);
if (yy_24_1_1[0] != 1) goto yyfl_447_1_24_1;
yy_24_1_2_1 = ((yy)"yyErr(4,");
s(yy_24_1_2_1);
yy_24_1_3_1 = yyv_Line;
i(yy_24_1_3_1);
yy_24_1_4_1 = ((yy)");");
s(yy_24_1_4_1);
nl();
goto yysl_447_1_24;
yyfl_447_1_24_1 : ;
goto yysl_447_1_24;
yysl_447_1_24 : ;
yyb = yysb;
}
yy_25_1 = ((yy)"}");
s(yy_25_1);
nl();
yy_27 = yyb + 6;
yy_27[0] = 2;
yyglov_insideChoiceRule = yy_27;
return;
yyfl_447_1 : ;
}
yyErr(2,4892);
}
SF_Assoc(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yy_0_2;
yy yy_0_2_1;
yy yyv_Functors;
yy yy_0_2_2;
yy yy_1_1;
yy yy_2_1;
yy yy_4_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Id = yy_0_1;
if (yy_0_2[0] != 1) goto yyfl_478_1;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yyv_Functors = yy_0_2_2;
yy_1_1 = ((yy)"'TYPE' ");
s(yy_1_1);
yy_2_1 = yyv_Id;
id(yy_2_1);
nl();
yy_4_1 = yyv_Functors;
SF_FunctorSpecList(yy_4_1);
return;
yyfl_478_1 : ;
}
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yy_0_2;
yy yyv_Type;
yy yy_0_2_1;
yy yyv_Pos;
yy yy_0_2_2;
yy yy_1_1;
yy yy_2_1;
yy yy_3_1;
yy yy_4_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Id = yy_0_1;
if (yy_0_2[0] != 2) goto yyfl_478_2;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yyv_Type = yy_0_2_1;
yyv_Pos = yy_0_2_2;
yy_1_1 = ((yy)"'VAR' ");
s(yy_1_1);
yy_2_1 = yyv_Id;
id(yy_2_1);
yy_3_1 = ((yy)": ");
s(yy_3_1);
yy_4_1 = yyv_Type;
id(yy_4_1);
nl();
return;
yyfl_478_2 : ;
}
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yy_0_2;
yy yyv_Fields;
yy yy_0_2_1;
yy yy_1_1;
yy yy_2_1;
yy yy_3_1;
yy yy_4_1;
yy yy_5_1;
yy yy_5_2;
yy yy_6_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Id = yy_0_1;
if (yy_0_2[0] != 3) goto yyfl_478_3;
yy_0_2_1 = ((yy)yy_0_2[1]);
yyv_Fields = yy_0_2_1;
yy_1_1 = ((yy)"'TABLE' ");
s(yy_1_1);
yy_2_1 = yyv_Id;
id(yy_2_1);
yy_3_1 = ((yy)" ");
s(yy_3_1);
yy_4_1 = ((yy)"(");
s(yy_4_1);
yy_5_1 = yyv_Fields;
yy_5_2 = ((yy)"");
SF_TableFieldList(yy_5_1, yy_5_2);
yy_6_1 = ((yy)")");
s(yy_6_1);
nl();
return;
yyfl_478_3 : ;
}
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yy_0_2;
yy yyv_Name;
yy yy_0_2_1;
yy yyv_Class;
yy yy_0_2_2;
yy yyv_Formals;
yy yy_0_2_3;
yy yyv_Rules;
yy yy_0_2_4;
yy yy_1_1_1_1;
yy yy_1_1_1_2;
yy yy_1_1_2_1;
yy yy_1_2_1_1;
yy yy_1_2_1_2;
yy yy_1_2_2_1;
yy yy_1_3_1_1;
yy yy_1_3_1_2;
yy yy_1_3_2_1;
yy yy_1_4_1_1;
yy yy_1_4_1_2;
yy yy_1_4_2_1;
yy yy_1_5_1_1;
yy yy_1_5_1_2;
yy yy_1_5_2_1;
yy yy_1_6_1_1;
yy yy_1_6_1_2;
yy yy_1_6_2_1;
yy yy_2_1;
yy yy_3_1;
yy yy_4_1;
yy yy_5_1;
yy yy_5_2;
yy yyv_InArgs;
yy yy_5_2_1;
yy yyv_OutArgs;
yy yy_5_2_2;
yy yy_6_1;
yy yy_6_2;
yy yy_7_1_1_1;
yy yy_7_1_1_2;
yy yy_7_1_2_1;
yy yy_7_1_2_2;
yy yy_7_2_1_1;
yy yy_7_2_1_2;
yy yy_8_1;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Id = yy_0_1;
if (yy_0_2[0] != 4) goto yyfl_478_4;
yy_0_2_1 = ((yy)yy_0_2[1]);
yy_0_2_2 = ((yy)yy_0_2[2]);
yy_0_2_3 = ((yy)yy_0_2[3]);
yy_0_2_4 = ((yy)yy_0_2[4]);
yyv_Name = yy_0_2_1;
yyv_Class = yy_0_2_2;
yyv_Formals = yy_0_2_3;
yyv_Rules = yy_0_2_4;
{
yy yysb = yyb;
yy_1_1_1_1 = yyv_Class;
yy_1_1_1_2 = yy_1_1_1_1;
if (yy_1_1_1_2[0] != 1) goto yyfl_478_4_1_1;
yy_1_1_2_1 = ((yy)"'ACTION'");
s(yy_1_1_2_1);
goto yysl_478_4_1;
yyfl_478_4_1_1 : ;
yy_1_2_1_1 = yyv_Class;
yy_1_2_1_2 = yy_1_2_1_1;
if (yy_1_2_1_2[0] != 2) goto yyfl_478_4_1_2;
yy_1_2_2_1 = ((yy)"'CONDITION'");
s(yy_1_2_2_1);
goto yysl_478_4_1;
yyfl_478_4_1_2 : ;
yy_1_3_1_1 = yyv_Class;
yy_1_3_1_2 = yy_1_3_1_1;
if (yy_1_3_1_2[0] != 6) goto yyfl_478_4_1_3;
yy_1_3_2_1 = ((yy)"'SWEEP'");
s(yy_1_3_2_1);
goto yysl_478_4_1;
yyfl_478_4_1_3 : ;
yy_1_4_1_1 = yyv_Class;
yy_1_4_1_2 = yy_1_4_1_1;
if (yy_1_4_1_2[0] != 3) goto yyfl_478_4_1_4;
yy_1_4_2_1 = ((yy)"'NONTERM'");
s(yy_1_4_2_1);
goto yysl_478_4_1;
yyfl_478_4_1_4 : ;
yy_1_5_1_1 = yyv_Class;
yy_1_5_1_2 = yy_1_5_1_1;
if (yy_1_5_1_2[0] != 4) goto yyfl_478_4_1_5;
yy_1_5_2_1 = ((yy)"'TOKEN'");
s(yy_1_5_2_1);
goto yysl_478_4_1;
yyfl_478_4_1_5 : ;
yy_1_6_1_1 = yyv_Class;
yy_1_6_1_2 = yy_1_6_1_1;
if (yy_1_6_1_2[0] != 5) goto yyfl_478_4_1_6;
yy_1_6_2_1 = ((yy)"'CHOICE'");
s(yy_1_6_2_1);
goto yysl_478_4_1;
yyfl_478_4_1_6 : ;
goto yyfl_478_4;
yysl_478_4_1 : ;
yyb = yysb;
}
yy_2_1 = ((yy)" ");
s(yy_2_1);
yy_3_1 = yyv_Id;
id(yy_3_1);
yy_4_1 = ((yy)" (");
s(yy_4_1);
yy_5_1 = yyv_Formals;
yy_5_2 = yy_5_1;
if (yy_5_2[0] != 1) goto yyfl_478_4;
yy_5_2_1 = ((yy)yy_5_2[1]);
yy_5_2_2 = ((yy)yy_5_2[2]);
yyv_InArgs = yy_5_2_1;
yyv_OutArgs = yy_5_2_2;
yy_6_1 = yyv_InArgs;
yy_6_2 = ((yy)"");
SF_ArgSpecList(yy_6_1, yy_6_2);
{
yy yysb = yyb;
yy_7_1_1_1 = yyv_InArgs;
yy_7_1_1_2 = yy_7_1_1_1;
if (yy_7_1_1_2[0] != 2) goto yyfl_478_4_7_1;
yy_7_1_2_1 = yyv_OutArgs;
yy_7_1_2_2 = ((yy)"-> ");
SF_ArgSpecList(yy_7_1_2_1, yy_7_1_2_2);
goto yysl_478_4_7;
yyfl_478_4_7_1 : ;
yy_7_2_1_1 = yyv_OutArgs;
yy_7_2_1_2 = ((yy)" -> ");
SF_ArgSpecList(yy_7_2_1_1, yy_7_2_1_2);
goto yysl_478_4_7;
yysl_478_4_7 : ;
yyb = yysb;
}
yy_8_1 = ((yy)")");
s(yy_8_1);
nl();
return;
yyfl_478_4 : ;
}
{
yy yyb;
yy yyv_Id;
yy yy_0_1;
yy yyv_Head;
yy yy_0_2;
yy yy_1_1;
yy yy_1_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
yyv_Id = yy_0_1;
yyv_Head = yy_0_2;
yy_1_1 = yyv_Id;
yy_1_2 = yyv_Head;
SF_Assoc(yy_1_1, yy_1_2);
return;
yyfl_478_5 : ;
}
yyErr(2,4991);
}
SF_FunctorSpecList(yyin_1)
yy yyin_1;
{
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_Functor;
yy yy_0_1_1_1;
yy yy_0_1_1_2;
yy yyv_Args;
yy yy_0_1_1_3;
yy yyv_Tail;
yy yy_0_1_2;
yy yy_1_1;
yy yy_2_1;
yy yy_3_1_1_1;
yy yy_3_1_1_2;
yy yy_3_2_1_1;
yy yy_3_2_2_1;
yy yy_3_2_2_2;
yy yy_3_2_3_1;
yy yy_5_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 1) goto yyfl_479_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
if (yy_0_1_1[0] != 1) goto yyfl_479_1;
yy_0_1_1_1 = ((yy)yy_0_1_1[1]);
yy_0_1_1_2 = ((yy)yy_0_1_1[2]);
yy_0_1_1_3 = ((yy)yy_0_1_1[3]);
yyv_Functor = yy_0_1_1_1;
yyv_Args = yy_0_1_1_3;
yyv_Tail = yy_0_1_2;
yy_1_1 = ((yy)"   ");
s(yy_1_1);
yy_2_1 = yyv_Functor;
id(yy_2_1);
{
yy yysb = yyb;
yy_3_1_1_1 = yyv_Args;
yy_3_1_1_2 = yy_3_1_1_1;
if (yy_3_1_1_2[0] != 2) goto yyfl_479_1_3_1;
goto yysl_479_1_3;
yyfl_479_1_3_1 : ;
yy_3_2_1_1 = ((yy)"(");
s(yy_3_2_1_1);
yy_3_2_2_1 = yyv_Args;
yy_3_2_2_2 = ((yy)"");
SF_ArgSpecList(yy_3_2_2_1, yy_3_2_2_2);
yy_3_2_3_1 = ((yy)")");
s(yy_3_2_3_1);
goto yysl_479_1_3;
yysl_479_1_3 : ;
yyb = yysb;
}
nl();
yy_5_1 = yyv_Tail;
SF_FunctorSpecList(yy_5_1);
return;
yyfl_479_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy_0_1 = yyin_1;
if (yy_0_1[0] != 2) goto yyfl_479_2;
return;
yyfl_479_2 : ;
}
yyErr(2,5027);
}
SF_TableFieldList(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_Name;
yy yy_0_1_1_1;
yy yyv_Type;
yy yy_0_1_1_2;
yy yyv_Pos;
yy yy_0_1_1_3;
yy yyv_Tail;
yy yy_0_1_2;
yy yyv_Sep;
yy yy_0_2;
yy yy_1_1;
yy yy_2_1;
yy yy_3_1;
yy yy_4_1;
yy yy_5_1;
yy yy_5_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_480_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
if (yy_0_1_1[0] != 1) goto yyfl_480_1;
yy_0_1_1_1 = ((yy)yy_0_1_1[1]);
yy_0_1_1_2 = ((yy)yy_0_1_1[2]);
yy_0_1_1_3 = ((yy)yy_0_1_1[3]);
yyv_Name = yy_0_1_1_1;
yyv_Type = yy_0_1_1_2;
yyv_Pos = yy_0_1_1_3;
yyv_Tail = yy_0_1_2;
yyv_Sep = yy_0_2;
yy_1_1 = yyv_Sep;
s(yy_1_1);
yy_2_1 = yyv_Name;
id(yy_2_1);
yy_3_1 = ((yy)": ");
s(yy_3_1);
yy_4_1 = yyv_Type;
id(yy_4_1);
yy_5_1 = yyv_Tail;
yy_5_2 = ((yy)", ");
SF_TableFieldList(yy_5_1, yy_5_2);
return;
yyfl_480_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Sep;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_480_2;
yyv_Sep = yy_0_2;
return;
yyfl_480_2 : ;
}
yyErr(2,5041);
}
SF_ArgSpecList(yyin_1, yyin_2)
yy yyin_1;
yy yyin_2;
{
{
yy yyb;
yy yy_0_1;
yy yy_0_1_1;
yy yyv_FormalName;
yy yy_0_1_1_1;
yy yyv_Type;
yy yy_0_1_1_2;
yy yy_0_1_1_3;
yy yyv_Pos;
yy yy_0_1_2;
yy yyv_Tail;
yy yy_0_1_3;
yy yyv_Sep;
yy yy_0_2;
yy yy_1_1;
yy yy_2_1;
yy yy_3_1;
yy yy_3_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 1) goto yyfl_481_1;
yy_0_1_1 = ((yy)yy_0_1[1]);
yy_0_1_2 = ((yy)yy_0_1[2]);
yy_0_1_3 = ((yy)yy_0_1[3]);
if (yy_0_1_1[0] != 1) goto yyfl_481_1;
yy_0_1_1_1 = ((yy)yy_0_1_1[1]);
yy_0_1_1_2 = ((yy)yy_0_1_1[2]);
yy_0_1_1_3 = ((yy)yy_0_1_1[3]);
yyv_FormalName = yy_0_1_1_1;
yyv_Type = yy_0_1_1_2;
yyv_Pos = yy_0_1_2;
yyv_Tail = yy_0_1_3;
yyv_Sep = yy_0_2;
yy_1_1 = yyv_Sep;
s(yy_1_1);
yy_2_1 = yyv_Type;
id(yy_2_1);
yy_3_1 = yyv_Tail;
yy_3_2 = ((yy)", ");
SF_ArgSpecList(yy_3_1, yy_3_2);
return;
yyfl_481_1 : ;
}
{
yy yyb;
yy yy_0_1;
yy yyv_Sep;
yy yy_0_2;
yy_0_1 = yyin_1;
yy_0_2 = yyin_2;
if (yy_0_1[0] != 2) goto yyfl_481_2;
yyv_Sep = yy_0_2;
return;
yyfl_481_2 : ;
}
yyErr(2,5052);
}
