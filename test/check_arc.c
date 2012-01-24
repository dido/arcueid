/* 
  Copyright (C) 2012 Rafael R. Sevilla

  This file is part of Arcueid

  Arcueid is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <check.h>
#include <math.h>
#include <unistd.h>
#include <stdarg.h>
#include "../src/arcueid.h"
#include "../src/builtin.h"
#include "../src/vmengine.h"
#include "../src/symbols.h"
#include "../config.h"

arc *c, cc;

static void error_handler(struct arc *c, value exc)
{
  /* always fail the test if this ever gets called */
  fail("exception received");
}

#define TEST(testexpr) {				\
  value str, sexpr, fp, cctx, code;			\
  str = arc_mkstringc(c, testexpr);			\
  fp = arc_instring(c, str, CNIL);			\
  sexpr = arc_read(c, fp, CNIL);			\
  cctx = arc_mkcctx(c, INT2FIX(1), 0);			\
  arc_compile(c, sexpr, cctx, CNIL, CTRUE);		\
  code = arc_cctx2code(c, cctx);			\
  ret = arc_macapply(c, code, CNIL); }

START_TEST(test_do)
{
  value ret;

  TEST("(do)");
  fail_unless(ret == CNIL);

  TEST("(do 1)");
  fail_unless(ret == INT2FIX(1));

  TEST("(do 1 2)");
  fail_unless(ret == INT2FIX(2));

  TEST("(do 1 2 3)");
  fail_unless(ret == INT2FIX(3));
}
END_TEST

START_TEST(test_safeset)
{
  value ret;

  TEST("(safeset foo 2)");
  fail_unless(ret == INT2FIX(2));
  fail_unless(arc_hash_lookup(c, c->genv, arc_intern_cstr(c, "foo")) == INT2FIX(2));

  TEST("(safeset foo 3)");
  fail_unless(ret == INT2FIX(3));
  fail_unless(arc_hash_lookup(c, c->genv, arc_intern_cstr(c, "foo")) == INT2FIX(3));
}
END_TEST

START_TEST(test_apply)
{
  value ret;
  
  TEST("(apply (fn a a) '(1 2) '(3 4))");
  fail_unless(TYPE(ret) == T_CONS);
  fail_unless(TYPE(car(ret)) == T_CONS);
  fail_unless(car(car(ret)) == INT2FIX(1));
  fail_unless(car(cdr(car(ret))) == INT2FIX(2));
  fail_unless(TYPE(cdr(ret)) == T_CONS);
  fail_unless(car(cdr(ret)) == INT2FIX(3));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(4));
}
END_TEST

START_TEST(test_def)
{
  value ret;

  TEST("(def bar (a b) (+ a b))");
  fail_unless(TYPE(ret) == T_CLOS);

  TEST("(bar 42 75)");
  fail_unless(ret == INT2FIX(117));
}
END_TEST

START_TEST(test_caar)
{
  value ret;

  TEST("(caar '((1 2) 3 4))");
  fail_unless(ret == INT2FIX(1));
}
END_TEST

START_TEST(test_cadr)
{
  value ret;

  TEST("(cadr '((1 2) 3 4))");
  fail_unless(ret == INT2FIX(3));
}
END_TEST

START_TEST(test_cddr)
{
  value ret;

  TEST("(cddr '((1 2) 3 4))");
  fail_unless(TYPE(ret) == T_CONS);
  fail_unless(car(ret) == INT2FIX(4));
}
END_TEST

START_TEST(test_no)
{
  value ret;

  TEST("(no nil)");
  fail_unless(ret == CTRUE);

  TEST("(no 1)");
  fail_unless(ret == CNIL);
}
END_TEST

START_TEST(test_acons)
{
  value ret;

  TEST("(acons nil)");
  fail_unless(ret == CNIL);

  TEST("(acons 1)");
  fail_unless(ret == CNIL);

  TEST("(acons '(1 2))");
  fail_unless(ret == CTRUE);
}
END_TEST

START_TEST(test_copylist)
{
  value ret;

  TEST("(copylist '(1 2 3))");
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(3));
}
END_TEST

START_TEST(test_list)
{
  value ret;

  TEST("(list 1 2 3)");
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(3));
}
END_TEST

START_TEST(test_idfn)
{
  value ret;

  TEST("(idfn '(1 2 3))");
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(3));

  TEST("(idfn 31337)");
  fail_unless(ret == INT2FIX(31337));
}
END_TEST

START_TEST(test_map1)
{
  value ret;

  TEST("(map1 [+ _ 1] '(1 2 3))");
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == INT2FIX(2));
  fail_unless(car(cdr(ret)) == INT2FIX(3));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(4));
}
END_TEST

START_TEST(test_pair)
{
  value ret;

  TEST("(pair '(1 2 3))");
  fail_unless(CONS_P(ret));
  fail_unless(CONS_P(car(ret)));
  fail_unless(car(car(ret)) == INT2FIX(1));
  fail_unless(car(cdr(car(ret))) == INT2FIX(2));
  fail_unless(CONS_P(car(cdr(ret))));
  fail_unless(car(car(cdr(ret))) == INT2FIX(3));
}
END_TEST

START_TEST(test_mac)
{
  value ret;

  TEST("(mac foofoo () (list '+ 1 2))");
  fail_unless(TYPE(ret) == T_TAGGED);
  fail_unless(arc_type(c, ret) == ARC_BUILTIN(c, S_MAC));

  /* Try to use the macro */
  TEST("(+ 10 (foofoo))");
  fail_unless(FIX2INT(ret) == 13);
}
END_TEST

START_TEST(test_and)
{
  value ret;

  TEST("(and 1 2 3)");
  fail_unless(ret == INT2FIX(3));

  TEST("(and 1 nil 3)");
  fail_unless(ret == CNIL);

  TEST("(and nil 3 4)");
  fail_unless(ret == CNIL);
}
END_TEST

START_TEST(test_assoc)
{
  value ret;

  TEST("(assoc 'a '((a 1) (b 2)))");
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == arc_intern_cstr(c, "a"));
  fail_unless(car(cdr(ret)) == INT2FIX(1));

  TEST("(assoc 'b '((a 1) (b 2)))");
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == arc_intern_cstr(c, "b"));
  fail_unless(car(cdr(ret)) == INT2FIX(2));

  TEST("(assoc 'c '((a 1) (b 2)))");
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_alref)
{
  value ret;

  TEST("(alref '((a 1) (b 2)) 'a)");
  fail_unless(ret == INT2FIX(1));

  TEST("(alref '((a 1) (b 2)) 'b)");
  fail_unless(ret == INT2FIX(2));

  TEST("(alref '((a 1) (b 2)) 'c)");
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_with)
{
  value ret;

  TEST("(with (a 1 b 2) a)");
  fail_unless(ret == INT2FIX(1));

  TEST("(with (a 1 b 2) b)");
  fail_unless(ret == INT2FIX(2));

  TEST("(with (a 1 b 2))");
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_let)
{
  value ret;

  TEST("(let x 1 x)");
  fail_unless(ret == INT2FIX(1));

  TEST("(let x 1)");
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_withs)
{
  value ret;

  /* uses upper binding a is 3 */
  TEST("(let a 3 (with (a 1 b (+ a 1)) (+ a b)))");
  fail_unless(ret == INT2FIX(5));

  /* uses simultaneous binding, so a is 1 */
  TEST("(let a 3 (withs (a 1 b (+ a 1)) (+ a b)))");
  fail_unless(ret == INT2FIX(3));
}
END_TEST

START_TEST(test_join)
{
  value ret;

  /* uses upper binding a is 3 */
  TEST("(join '(1 2 3) '(4 5 6))");
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(3));
  fail_unless(car(cdr(cdr(cdr(ret)))) == INT2FIX(4));
  fail_unless(car(cdr(cdr(cdr(cdr(ret))))) == INT2FIX(5));
  fail_unless(car(cdr(cdr(cdr(cdr(cdr(ret)))))) == INT2FIX(6));
}
END_TEST

START_TEST(test_rfn)
{
  value ret;

  TEST("((rfn fact (x) (if (is x 0) 1 (* x (fact (- x 1))))) 7)");
  fail_unless(ret == INT2FIX(5040));
}
END_TEST

START_TEST(test_afn)
{
  value ret;

  TEST("((afn (x) (if (is x 0) 1 (* x (self (- x 1))))) 7)");
  fail_unless(ret == INT2FIX(5040));
}
END_TEST

START_TEST(test_compose)
{
  value ret;

  TEST("(let div2 (fn (x) (/ x 2)) ((compose div2 len) \"abcd\"))");
  fail_unless(ret == INT2FIX(2));

  TEST("(let div2 (fn (x) (/ x 2)) (div2:len \"abcd\"))");
  fail_unless(ret == INT2FIX(2));
}
END_TEST

START_TEST(test_complement)
{
  value ret;

  TEST("((complement atom) 'foo)");
  fail_unless(NIL_P(ret));

  TEST("((complement atom) '(1 2 3))");
  fail_unless(ret == CTRUE);

  TEST("(~atom 'foo)");
  fail_unless(NIL_P(ret));

  TEST("(~atom '(1 2 3))");
  fail_unless(ret == CTRUE);
}
END_TEST

START_TEST(test_rev)
{
  value ret;

  TEST("(rev '(1 2 3))");
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == INT2FIX(3));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(1));
}
END_TEST

START_TEST(test_isnt)
{
  value ret;

  TEST("(isnt 1 1)");
  fail_unless(NIL_P(ret));

  TEST("(isnt 1 2)");
  fail_unless(ret == CTRUE);
}
END_TEST

START_TEST(test_or)
{
  value ret;

  TEST("(or 1 2 3)");
  fail_unless(ret == INT2FIX(1));

  TEST("(or nil 2 3)");
  fail_unless(ret == INT2FIX(2));

  TEST("(or nil nil 3)");
  fail_unless(ret == INT2FIX(3));

  TEST("(or nil nil 3)");
  fail_unless(ret == INT2FIX(3));

  TEST("(or nil nil nil)");
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_in)
{
  value ret;

  TEST("(in 1 0 2 3)");
  fail_unless(NIL_P(ret));

  TEST("(in 1 1 2 3)");
  fail_unless(ret == CTRUE);
}
END_TEST

START_TEST(test_when)
{
  value ret;

  TEST("(when t (assign whentest 123))");
  fail_unless(ret == INT2FIX(123));
  fail_unless(arc_hash_lookup(c, c->genv, arc_intern_cstr(c, "whentest")) == INT2FIX(123));

  TEST("(when nil (assign whentest 456))");
  fail_unless(NIL_P(ret));
  fail_unless(arc_hash_lookup(c, c->genv, arc_intern_cstr(c, "whentest")) == INT2FIX(123));
}
END_TEST

START_TEST(test_unless)
{
  value ret;

  TEST("(unless nil (assign whentest 123))");
  fail_unless(ret == INT2FIX(123));
  fail_unless(arc_hash_lookup(c, c->genv, arc_intern_cstr(c, "whentest")) == INT2FIX(123));

  TEST("(unless t (assign whentest 456))");
  fail_unless(NIL_P(ret));
  fail_unless(arc_hash_lookup(c, c->genv, arc_intern_cstr(c, "whentest")) == INT2FIX(123));
}
END_TEST

START_TEST(test_while)
{
  value ret;

  TEST("(with (x 0) (do (while (isnt x 10) (= x (+ x 1))) x))");
  fail_unless(ret == INT2FIX(10));

  TEST("(with (xs '(1 2 3 4 5 6 7) acc 1) (while (= xs (cdr xs)) (assign acc (* acc (car xs)))) acc)");
  fail_unless(ret == INT2FIX(5040));
}
END_TEST

START_TEST(test_empty)
{
  value ret;

  TEST("(empty nil)");
  fail_unless(ret == CTRUE);

  TEST("(empty \"\")");
  fail_unless(ret == CTRUE);

  TEST("(empty (table))");
  fail_unless(ret == CTRUE);

  TEST("(empty '(1 2 3))");
  fail_unless(NIL_P(ret));

  TEST("(empty \"abc\")");
  fail_unless(NIL_P(ret));

  TEST("(let x (table) (do (= (x 'foo) 1) (empty x)))");
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_reclist)
{
  value ret;

  TEST("(reclist no:car '(1 2 nil 3))");
  fail_unless(ret == CTRUE);

  TEST("(reclist no:car '(1 2 3))");
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_recstring)
{
  value ret;

  TEST("(recstring [is _ 1] \"abc\")");
  fail_unless(ret == CTRUE);

  TEST("(recstring [is _ 3] \"abc\")");
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_testify)
{
  value ret;

  TEST("((testify [isa _ 'sym]) 'foo)");
  fail_unless(ret == CTRUE);

  TEST("((testify [isa _ 'sym]) 1)");
  fail_unless(NIL_P(ret));

  TEST("((testify 'foo) 'foo)");
  fail_unless(ret == CTRUE);

  TEST("((testify 'foo) 'bar)");
  fail_unless(NIL_P(ret));

  TEST("((testify 'foo) 1)");
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_some)
{
  value ret;

  TEST("(some 1 '(1 2 3)))");
  fail_unless(ret == CTRUE);

  TEST("(some #\\a \"abc\")");
  fail_unless(ret == CTRUE);

  TEST("(some 9 '(1 2 3)))");
  fail_unless(NIL_P(ret));

  TEST("(some #\\z \"abc\")");
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_all)
{
  value ret;

  TEST("(all 1 '(1 1 1)))");
  fail_unless(ret == CTRUE);

  TEST("(all #\\a \"aaa\")");
  fail_unless(ret == CTRUE);

  TEST("(all 1 '(1 2 3)))");
  fail_unless(NIL_P(ret));

  TEST("(all #\\a \"abc\")");
  fail_unless(NIL_P(ret));

  TEST("(all 9 '(1 2 3)))");
  fail_unless(NIL_P(ret));

  TEST("(all #\\z \"abc\")");
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_find)
{
  value ret;

  TEST("(find 2 '(1 2 3)))");
  fail_unless(ret == INT2FIX(2));

  TEST("(find #\\c \"abc\")");
  fail_unless(TYPE(ret) == T_CHAR);
  fail_unless(REP(ret)._char = 'c');

  TEST("(find 9 '(1 2 3)))");
  fail_unless(NIL_P(ret));

  TEST("(find #\\z \"abc\")");
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_isa)
{
  value ret;

  TEST("(isa 1 'int)");
  fail_unless(ret == CTRUE);

  TEST("(isa 1.1 'int)");
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_map)
{
  value ret;

  TEST("(map (fn (x y) `(,x ,y)) '(1 2) '(3 4))");
  fail_unless(TYPE(ret) == T_CONS);
  fail_unless(TYPE(car(ret)) == T_CONS);
  fail_unless(car(car(ret)) == INT2FIX(1));
  fail_unless(car(cdr(car(ret))) == INT2FIX(3));
  fail_unless(TYPE(cdr(ret)) == T_CONS);
  fail_unless(car(car(cdr(ret))) == INT2FIX(2));
  fail_unless(car(cdr(car(cdr(ret)))) == INT2FIX(4));
}
END_TEST

START_TEST(test_mappend)
{
  value ret;

  TEST("(mappend (fn (x y) `(,x ,y)) '(1 2) '(3 4))");
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(car(cdr(ret)) == INT2FIX(3));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(2));
  fail_unless(car(cdr(cdr(cdr(ret)))) == INT2FIX(4));
}
END_TEST

START_TEST(test_firstn)
{
  value ret;

  TEST("(firstn 3 '(1 2 3 4 5 6 7 8 9 10))");
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(3));
}
END_TEST

START_TEST(test_nthcdr)
{
  value ret;

  TEST("(nthcdr 7 '(1 2 3 4 5 6 7 8 9 10))");
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == INT2FIX(8));
  fail_unless(car(cdr(ret)) == INT2FIX(9));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(10));
}
END_TEST

START_TEST(test_tuples)
{
  value ret;

  TEST("(tuples '(1 2 3 4 5 6 7 8 9 10) 3)");
  fail_unless(CONS_P(ret));
  fail_unless(CONS_P(car(ret)));
  fail_unless(car(car(ret)) == INT2FIX(1));
  fail_unless(car(cdr(car(ret))) == INT2FIX(2));
  fail_unless(car(cdr(cdr(car(ret)))) == INT2FIX(3));
  fail_unless(CONS_P(car(cdr(ret))));
  fail_unless(car(car(cdr(ret))) == INT2FIX(4));
  fail_unless(car(cdr(car(cdr(ret)))) == INT2FIX(5));
  fail_unless(car(cdr(cdr(car(cdr(ret))))) == INT2FIX(6));
}
END_TEST

START_TEST(test_defs)
{
  value ret;

  TEST("(defs hoge (x) (+ x 1) page (x) (+ x 2) piyo (x) (+ x 3))");
  fail_unless(TYPE(ret) == T_CLOS);

  TEST("(hoge 42)");
  fail_unless(ret == INT2FIX(43));

  TEST("(page 42)");
  fail_unless(ret == INT2FIX(44));

  TEST("(piyo 42)");
  fail_unless(ret == INT2FIX(45));
}
END_TEST

START_TEST(test_caris)
{
  value ret;

  TEST("(caris '(1 2 3) 1)");
  fail_unless(ret == CTRUE);

  TEST("(caris '(1 2 3) 2)");
  fail_unless(NIL_P(ret));
}
END_TEST

/* This test should cover all the machinery behind places and setforms. */
START_TEST(test_places)
{
  value ret;

  TEST("(let foo nil (= foo '(1 2 3)) (car foo))");
  fail_unless(ret == INT2FIX(1));

  TEST("(let foo '(1 2 3) (= (car foo) 4) (car foo))");
  fail_unless(ret == INT2FIX(4));

  TEST("(let foo '(1 2 3) (= (car:cdr foo) 5) (car (cdr foo)))");
  fail_unless(ret == INT2FIX(5));

  TEST("(let foo (table) (= (foo 'bar) 6) (foo 'bar)))");
  fail_unless(ret == INT2FIX(6));

  TEST("(let x \"abc\" (= (x 1) #\\z) x)");
  fail_unless(FIX2INT(arc_strcmp(c, ret, arc_mkstringc(c, "azc"))) == 0);
}
END_TEST

START_TEST(test_loop)
{
  value ret;

  TEST("(with (x nil total 0) (loop (= x 0) (<= x 100) (= x (+ x 1)) (= total (+ total x))) total)");
  fail_unless(ret == INT2FIX(5050));
}
END_TEST

START_TEST(test_for)
{
  value ret;

  TEST("(let result 0 (for i 0 100 (= result (+ result i))) result)");
  fail_unless(ret == INT2FIX(5050));
}
END_TEST

START_TEST(test_down)
{
  value ret;

  TEST("(let lst nil (down i 5 1 (= lst (cons i lst))) lst)");
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(3));
  fail_unless(car(cdr(cdr(cdr(ret)))) == INT2FIX(4));
  fail_unless(car(cdr(cdr(cdr(cdr(ret))))) == INT2FIX(5));
}
END_TEST

START_TEST(test_repeat)
{
  value ret;

  TEST("(let x 1 (repeat 16 (= x (* x 2))) x)");
  fail_unless(ret == INT2FIX(65536));
}
END_TEST

START_TEST(test_walk)
{
  value ret;

  TEST("(let x 1 (walk '(1 2 3 4 5 6 7) [= x (* x _)]) x)");
  fail_unless(ret == INT2FIX(5040));

  TEST("(with (tbl (table) x 0 y 0) (= (tbl 1) 2) (= (tbl 2) 3) (= (tbl 3) 4) (walk tbl (fn ((xt yt)) (= x (+ x xt)) (= y (+ y yt)))) (list x y))");
  fail_unless(car(ret) == INT2FIX(6));
  fail_unless(cadr(ret) == INT2FIX(9));
}
END_TEST

START_TEST(test_each)
{
  value ret;

  TEST("(let x 1 (each y '(1 2 3 4 5 6 7) (= x (* x y))) x)");
  fail_unless(ret == INT2FIX(5040));

  TEST("(with (tbl (table) x 0 y 0) (= (tbl 1) 2) (= (tbl 2) 3) (= (tbl 3) 4) (each z tbl (with (xt (car z) yt (cadr z)) (= x (+ x xt)) (= y (+ y yt)))) (list x y))");
  fail_unless(car(ret) == INT2FIX(6));
  fail_unless(cadr(ret) == INT2FIX(9));
}
END_TEST

START_TEST(test_cut)
{
  value ret;

  TEST("(cut '(1 2 3 4 5 6) 1 4)");
  fail_unless(car(ret) == INT2FIX(2));
  fail_unless(car(cdr(ret)) == INT2FIX(3));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(4));

  TEST("(cut '(1 2 3 4 5 6) 2 -1)");
  fail_unless(car(ret) == INT2FIX(3));
  fail_unless(car(cdr(ret)) == INT2FIX(4));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(5));

  TEST("(cut \"abcdefgh\" 2 5)");
  fail_unless(FIX2INT(arc_strcmp(c, ret, arc_mkstringc(c, "cde"))) == 0);

  TEST("(cut \"abcdefgh\" 3 -1)");
  fail_unless(FIX2INT(arc_strcmp(c, ret, arc_mkstringc(c, "defg"))) == 0);
}
END_TEST

START_TEST(test_whilet)
{
  value ret;

  TEST("(let x 1 (whilet tst (<= x 2000) (= x (* x 2))) x)");
  fail_unless(ret == INT2FIX(2048));
}
END_TEST

START_TEST(test_last)
{
  value ret;

  TEST("(last '(1 2 4 8 16 32 64 128 256 512 1024 2048))");
  fail_unless(ret == INT2FIX(2048));
}
END_TEST

START_TEST(test_rem)
{
  value ret;

  TEST("(rem [is (mod _ 2) 0] '(1 2 3 4 5 6))");
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(car(cdr(ret)) == INT2FIX(3));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(5));

  TEST("(rem #\\z \"xyzzy\")");
  fail_unless(FIX2INT(arc_strcmp(c, ret, arc_mkstringc(c, "xyy"))) == 0);
}
END_TEST

START_TEST(test_keep)
{
  value ret;

  TEST("(keep [is (mod _ 2) 0] '(1 2 3 4 5 6))");
  fail_unless(car(ret) == INT2FIX(2));
  fail_unless(car(cdr(ret)) == INT2FIX(4));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(6));

  TEST("(keep #\\z \"xyzzy\")");
  fail_unless(FIX2INT(arc_strcmp(c, ret, arc_mkstringc(c, "zz"))) == 0);
}
END_TEST

START_TEST(test_trues)
{
  value ret;

  TEST("(trues [if (is (mod _ 2) 0) nil _] '(1 2 3 4 5 6))");
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(car(cdr(ret)) == INT2FIX(3));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(5));
}
END_TEST

START_TEST(test_do1)
{
  value ret;

  TEST("(do1)");
  fail_unless(ret == CNIL);

  TEST("(do1 1)");
  fail_unless(ret == INT2FIX(1));

  TEST("(do1 1 2)");
  fail_unless(ret == INT2FIX(1));

  TEST("(do1 1 2 3)");
  fail_unless(ret == INT2FIX(1));
}
END_TEST

START_TEST(test_caselet)
{
  value ret;

  TEST("(caselet x 1 1 'a 2 'b 3 'c 'd)");
  fail_unless(ret == arc_intern_cstr(c, "a"));

  TEST("(caselet x 21 1 'a 2 'b 3 'c 'd)");
  fail_unless(ret == arc_intern_cstr(c, "d"));

  TEST("(caselet x 21 1 'a 2 'b 3 'c)");
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_case)
{
  value ret;

  TEST("(case 1 1 'a 2 'b 3 'c 'd)");
  fail_unless(ret == arc_intern_cstr(c, "a"));

  TEST("(case 2 1 'a 2 'b 3 'c 'd)");
  fail_unless(ret == arc_intern_cstr(c, "b"));

  TEST("(case 3 1 'a 2 'b 3 'c 'd)");
  fail_unless(ret == arc_intern_cstr(c, "c"));

  TEST("(case 4 1 'a 2 'b 3 'c 'd)");
  fail_unless(ret == arc_intern_cstr(c, "d"));

  TEST("(case 21 1 'a 2 'b 3 'c 'd)");
  fail_unless(ret == arc_intern_cstr(c, "d"));

  TEST("(case 21 1 'a 2 'b 3 'c)");
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_pushpop)
{
  value ret, stack;

  TEST("(do (= stack nil) (push 1 stack) (push 2 stack) (push 3 stack))");
  fail_unless(CONS_P(ret));
  stack = arc_hash_lookup(c, c->genv, arc_intern_cstr(c, "stack"));
  fail_unless(CONS_P(stack));
  fail_unless(car(stack) == INT2FIX(3));
  fail_unless(car(cdr(stack)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(stack))) == INT2FIX(1));

  TEST("(pop stack)");
  fail_unless(ret == INT2FIX(3));
  stack = arc_hash_lookup(c, c->genv, arc_intern_cstr(c, "stack"));
  fail_unless(CONS_P(stack));
  fail_unless(car(stack) == INT2FIX(2));
  fail_unless(car(cdr(stack)) == INT2FIX(1));

  TEST("(pop stack)");
  fail_unless(ret == INT2FIX(2));
  stack = arc_hash_lookup(c, c->genv, arc_intern_cstr(c, "stack"));
  fail_unless(CONS_P(stack));
  fail_unless(car(stack) == INT2FIX(1));

  TEST("(pop stack)");
  fail_unless(ret == INT2FIX(1));
  stack = arc_hash_lookup(c, c->genv, arc_intern_cstr(c, "stack"));
  fail_unless(NIL_P(stack));

}
END_TEST

START_TEST(test_swap)
{
  value ret;

  TEST("(with (x 42 y 24) (swap x y) (list x y))");
  fail_unless(car(ret) == INT2FIX(24));
  fail_unless(car(cdr(ret)) == INT2FIX(42));
}
END_TEST


START_TEST(test_rotate)
{
  value ret;

  TEST("(let x '(1 2 3 4 5) (rotate (x 0) (x 2) (x 4)) x)");
  fail_unless(car(ret) == INT2FIX(3));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(5));
  fail_unless(car(cdr(cdr(cdr(ret)))) == INT2FIX(4));
  fail_unless(car(cdr(cdr(cdr(cdr(ret))))) == INT2FIX(1));
}
END_TEST

START_TEST(test_adjoin)
{
  value ret;

  TEST("(adjoin 0 '(1 2 0 3))");
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(0));
  fail_unless(car(cdr(cdr(cdr(ret)))) == INT2FIX(3));

  TEST("(adjoin 0 '(1 2 0 3) (fn (x y) t))");
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(0));
  fail_unless(car(cdr(cdr(cdr(ret)))) == INT2FIX(3));

  TEST("(adjoin 0 '(1 2 0 3) (fn (x y) nil))");
  fail_unless(car(ret) == INT2FIX(0));
  fail_unless(car(cdr(ret)) == INT2FIX(1));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(2));
  fail_unless(car(cdr(cdr(cdr(ret)))) == INT2FIX(0));
  fail_unless(car(cdr(cdr(cdr(cdr(ret))))) == INT2FIX(3));
}
END_TEST

START_TEST(test_pull)
{
  value ret;

  TEST("(let x '(1 2 3 4 5 6) (pull even x) x)");
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(car(cdr(ret)) == INT2FIX(3));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(5));
}
END_TEST

START_TEST(test_pushnew)
{
  value ret, stack;

  TEST("(do (= pnstack '(1 2 0 3)) (pushnew 0 pnstack))");
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(0));
  fail_unless(car(cdr(cdr(cdr(ret)))) == INT2FIX(3));
  stack = arc_hash_lookup(c, c->genv, arc_intern_cstr(c, "pnstack"));
  fail_unless(CONS_P(stack));
  fail_unless(car(stack) == INT2FIX(1));
  fail_unless(car(cdr(stack)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(stack))) == INT2FIX(0));
  fail_unless(car(cdr(cdr(cdr(stack)))) == INT2FIX(3));

  TEST("(do (= pnstack '(1 2 0 3)) (pushnew 0 pnstack (fn (x y) t))))");
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(0));
  fail_unless(car(cdr(cdr(cdr(ret)))) == INT2FIX(3));
  stack = arc_hash_lookup(c, c->genv, arc_intern_cstr(c, "pnstack"));
  fail_unless(CONS_P(stack));
  fail_unless(car(stack) == INT2FIX(1));
  fail_unless(car(cdr(stack)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(stack))) == INT2FIX(0));
  fail_unless(car(cdr(cdr(cdr(stack)))) == INT2FIX(3));

  TEST("(do (= pnstack '(1 2 0 3)) (pushnew 4 pnstack))");
  fail_unless(car(ret) == INT2FIX(4));
  fail_unless(car(cdr(ret)) == INT2FIX(1));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(2));
  fail_unless(car(cdr(cdr(cdr(ret)))) == INT2FIX(0));
  fail_unless(car(cdr(cdr(cdr(cdr(ret))))) == INT2FIX(3));
  stack = arc_hash_lookup(c, c->genv, arc_intern_cstr(c, "pnstack"));
  fail_unless(CONS_P(stack));
  fail_unless(car(stack) == INT2FIX(4));
  fail_unless(car(cdr(stack)) == INT2FIX(1));
  fail_unless(car(cdr(cdr(stack))) == INT2FIX(2));
  fail_unless(car(cdr(cdr(cdr(stack)))) == INT2FIX(0));
  fail_unless(car(cdr(cdr(cdr(cdr(stack))))) == INT2FIX(3));

  TEST("(do (= pnstack '(1 2 0 3)) (pushnew 0 pnstack (fn (x y) nil)))");
  fail_unless(car(ret) == INT2FIX(0));
  fail_unless(car(cdr(ret)) == INT2FIX(1));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(2));
  fail_unless(car(cdr(cdr(cdr(ret)))) == INT2FIX(0));
  fail_unless(car(cdr(cdr(cdr(cdr(ret))))) == INT2FIX(3));
  stack = arc_hash_lookup(c, c->genv, arc_intern_cstr(c, "pnstack"));
  fail_unless(CONS_P(stack));
  fail_unless(car(stack) == INT2FIX(0));
  fail_unless(car(cdr(stack)) == INT2FIX(1));
  fail_unless(car(cdr(cdr(stack))) == INT2FIX(2));
  fail_unless(car(cdr(cdr(cdr(stack)))) == INT2FIX(0));
  fail_unless(car(cdr(cdr(cdr(cdr(stack))))) == INT2FIX(3));
}
END_TEST

START_TEST(test_togglemem)
{
  value ret;

  TEST("(let xs '(1 2 3) (togglemem 2 xs) xs)");
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(car(cdr(ret)) == INT2FIX(3));

  TEST("(let xs '(1 2 3) (togglemem 2 xs) (togglemem 2 xs) xs)");
  fail_unless(car(ret) == INT2FIX(2));
  fail_unless(car(cdr(ret)) == INT2FIX(1));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(3));
}
END_TEST

START_TEST(test_plusplus)
{
  value ret;

  TEST("(let v 0 (++ v) v)");
  fail_unless(ret == INT2FIX(1));

  TEST("(let v 0 (++ v 2) v)");
  fail_unless(ret == INT2FIX(2));
}
END_TEST

START_TEST(test_minusminus)
{
  value ret;

  TEST("(let v 2 (-- v) v)");
  fail_unless(ret == INT2FIX(1));

  TEST("(let v 2 (-- v 2) v)");
  fail_unless(ret == INT2FIX(0));
}
END_TEST

START_TEST(test_zap)
{
  value ret;

  TEST("(let s \"abc\" (zap upcase (s 0)) s)");
  fail_unless(FIX2INT(arc_strcmp(c, ret, arc_mkstringc(c, "Abc"))) == 0);

  TEST("(let x '(10 10) (zap mod (car x) 3) x)");
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(car(cdr(ret)) == INT2FIX(10));
}
END_TEST

START_TEST(test_wipe)
{
  value ret;

  TEST("(with (a 1 b 2 c 3) (wipe a b c) (list a b c))");
  fail_unless(NIL_P(car(ret)));
  fail_unless(NIL_P(car(cdr(ret))));
  fail_unless(NIL_P(car(cdr(cdr(ret)))));
}
END_TEST

START_TEST(test_set)
{
  value ret;

  TEST("(with (a 1 b 2 c 3) (set a b c) (list a b c))");
  fail_unless(car(ret) == CTRUE);
  fail_unless(car(cdr(ret)) == CTRUE);
  fail_unless(car(cdr(cdr(ret))) == CTRUE);
}
END_TEST

START_TEST(test_iflet)
{
  value ret;

  TEST("(iflet x t x 1)");
  fail_unless(ret == CTRUE);

  TEST("(iflet x nil x 1)");
  fail_unless(ret == INT2FIX(1));
}
END_TEST

START_TEST(test_whenlet)
{
  value ret;

  TEST("(let tst 1 (whenlet x t (= tst x)) tst)");
  fail_unless(ret == CTRUE);

  TEST("(let tst 1 (whenlet x nil (= tst x)) tst)");
  fail_unless(ret == INT2FIX(1));
}
END_TEST

START_TEST(test_aif)
{
  value ret;

  TEST("(aif t it 1)");
  fail_unless(ret == CTRUE);

  TEST("(aif nil it 1)");
  fail_unless(ret == INT2FIX(1));
}
END_TEST

START_TEST(test_awhen)
{
  value ret;

  TEST("(let tst 1 (awhen t (= tst it)) tst)");
  fail_unless(ret == CTRUE);

  TEST("(let tst 1 (awhen nil (= tst it)) tst)");
  fail_unless(ret == INT2FIX(1));
}
END_TEST

START_TEST(test_aand)
{
  value ret;

  TEST("(aand 1 2 3 (odd it) 4)");
  fail_unless(ret == INT2FIX(4));

  TEST("(aand 1 2 3 (odd it))");
  fail_unless(ret == CTRUE);

  TEST("(aand 1 2 3 (even it) 4)");
  fail_unless(NIL_P(ret));

  TEST("(aand 1 2 3 (even it))");
  fail_unless(NIL_P(ret));
}
END_TEST

START_TEST(test_accum)
{
  value ret;

  TEST("(let test [+ _ 1] (accum test (test 1) (test 2) (test 3)))");
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(3));
}
END_TEST

START_TEST(test_drain)
{
  value ret;

  TEST("(let test '(1 2 3) (drain (do1 (car test) (= test (cdr test)))))");
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(3));
}
END_TEST

START_TEST(test_whiler)
{
  value ret;

  TEST("(with (xs '(1 2 3 4 5 6 7 8) acc 1) (whiler i (cadr xs) nil (= acc (* acc (car xs))) (= xs (cdr xs))) acc)");
  fail_unless(ret == INT2FIX(5040));
}
END_TEST

START_TEST(test_consif)
{
  value ret;

  TEST("(consif nil 2)");
  fail_unless(ret == INT2FIX(2));

  TEST("(consif 1 2)");
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(cdr(ret) == INT2FIX(2));

}
END_TEST

START_TEST(test_string)
{
  value ret;

  TEST("(string '(1 2 3) '(a b c))");
  fail_unless(FIX2INT(arc_strcmp(c, ret, arc_mkstringc(c, "123abc"))) == 0);
}
END_TEST

START_TEST(test_flat)
{
  value ret;

  TEST("(flat '(1 (2 3 (4 5) 6 (7 8) 9) 10))");
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(3));
  fail_unless(car(cdr(cdr(cdr(ret)))) == INT2FIX(4));
  fail_unless(car(cdr(cdr(cdr(cdr(ret))))) == INT2FIX(5));
  fail_unless(car(cdr(cdr(cdr(cdr(cdr(ret)))))) == INT2FIX(6));
  fail_unless(car(cdr(cdr(cdr(cdr(cdr(cdr(ret))))))) == INT2FIX(7));
  fail_unless(car(cdr(cdr(cdr(cdr(cdr(cdr(cdr(ret)))))))) == INT2FIX(8));
  fail_unless(car(cdr(cdr(cdr(cdr(cdr(cdr(cdr(cdr(ret))))))))) == INT2FIX(9));
  fail_unless(car(cdr(cdr(cdr(cdr(cdr(cdr(cdr(cdr(cdr(ret)))))))))) == INT2FIX(10));
}
END_TEST

START_TEST(test_check)
{
  value ret;

  TEST("(check 1 [is _ 0])");
  fail_unless(NIL_P(ret));

  TEST("(check 1 [is _ 0] -1)");
  fail_unless(ret == INT2FIX(-1));

  TEST("(check 1 [is _ 1])");
  fail_unless(ret == INT2FIX(1));

  TEST("(check 1 [is _ 1] -1)");
  fail_unless(ret == INT2FIX(1));
}
END_TEST

START_TEST(test_pos)
{
  value ret;

  TEST("(pos 1 '(2 3 4))");
  fail_unless(NIL_P(ret));

  TEST("(pos 1 '(2 3 1 4))");
  fail_unless(ret == INT2FIX(2));

  TEST("(pos 1 '(1 2 3 4))");
  fail_unless(ret == INT2FIX(0));

  TEST("(pos 1 '(1 2 3 4) 1)");
  fail_unless(NIL_P(ret));

  TEST("(pos 1 '(1 3 1 4) 1)");
  fail_unless(ret == INT2FIX(2));

}
END_TEST

START_TEST(test_even)
{
  value ret;

  TEST("(even 7)");
  fail_unless(NIL_P(ret));

  TEST("(even 8)");
  fail_unless(ret == CTRUE);
}
END_TEST

START_TEST(test_odd)
{
  value ret;

  TEST("(odd 8)");
  fail_unless(NIL_P(ret));

  TEST("(odd 7)");
  fail_unless(ret == CTRUE);
}
END_TEST

/* compare with the tests for protect in check_error.c */
START_TEST(test_after)
{
  value ret;

  TEST("(with (x nil y nil) (list (+ 1 (on-err (fn (ex) (= exc ex) (+ x y)) (fn () (after (after (err \"test\") (= y 1)) (= x (+ y 2)))))) x y exc))");
  fail_unless(car(ret) == INT2FIX(5));
  fail_unless(car(cdr(ret)) == INT2FIX(3));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(1));
  fail_unless(TYPE(car(cdr(cdr(cdr(ret))))) == T_EXCEPTION);
}
END_TEST

START_TEST(test_int)
{
  value ret;

  TEST("(int \"100\")");
  fail_unless(ret == INT2FIX(100));

  TEST("(int \"100\" 16)");
  fail_unless(ret == INT2FIX(256));
}
END_TEST


START_TEST(test_rand_choice)
{
  value ret;
  int i;

  /* do it several times to make sure the choices appear reasonably often */
  for (i=0; i<100; i++) {
    TEST("(rand-choice 1 2 3 4)");
    fail_unless(ret == INT2FIX(1) || ret == INT2FIX(2) || ret == INT2FIX(3) || ret == INT2FIX(4));
  }
}
END_TEST

START_TEST(test_n_of)
{
  value ret;

  TEST("(let x 0 (n-of 5 (++ x)))");
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(3));
  fail_unless(car(cdr(cdr(cdr(ret)))) == INT2FIX(4));
  fail_unless(car(cdr(cdr(cdr(cdr(ret))))) == INT2FIX(5));
}
END_TEST


START_TEST(test_forlen)
{
  value ret;

  TEST("(with (x 1 y '(1 2 3 4 5 6 7)) (forlen j y (= x (* x (y j)))) x)");
  fail_unless(ret == INT2FIX(5040));
}
END_TEST

START_TEST(test_on)
{
  value ret;

  TEST("(with (x 0 y 1 seq '(1 2 3 4 5 6 7)) (on var seq (= x (+ x index)) (= y (* y var))) (list x y))");
  fail_unless(car(ret) == INT2FIX(21));
  fail_unless(car(cdr(ret)) == INT2FIX(5040));
}
END_TEST

START_TEST(test_best)
{
  value ret;

  TEST("(best > '(3 1 2 4 7 3 6))");
  fail_unless(ret == INT2FIX(7));

  TEST("(best < '(3 1 2 4 7 3 6))");
  fail_unless(ret == INT2FIX(1));
}
END_TEST

START_TEST(test_max)
{
  value ret;

  TEST("(max 3 1 2 4 7 3 6)");
  fail_unless(ret == INT2FIX(7));
}
END_TEST

START_TEST(test_min)
{
  value ret;

  TEST("(min 3 1 2 4 7 3 6)");
  fail_unless(ret == INT2FIX(1));

}
END_TEST

START_TEST(test_most)
{
  value ret;

  /* compute the stopping time of a hailstone sequence */
  TEST("(def hailstone (n) (let f (fn (n) (if (even n) (/ n 2) (+ (* 3 n) 1))) ((afn (a i) (if (is a 1) i (self (f a) (+ i 1)))) n 0)))");

  TEST("(let nums nil (for i 1 100 (= nums (cons i nums))) (most hailstone nums))");
  fail_unless(ret == INT2FIX(97));
}
END_TEST

START_TEST(test_insert_sorted)
{
  value ret;

  TEST("(insert-sorted < 4 '(1 2 3 5))");
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(3));
  fail_unless(car(cdr(cdr(cdr(ret)))) == INT2FIX(4));
  fail_unless(car(cdr(cdr(cdr(cdr(ret))))) == INT2FIX(5));
}
END_TEST

START_TEST(test_insort)
{
  value ret, ret2;

  TEST("(let xs '(0 (1 2 3 5) 6 7) (insort < 4 (cadr xs)) xs)");
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) = INT2FIX(0));
  ret2 = car(cdr(ret));
  fail_unless(car(ret2) == INT2FIX(1));
  fail_unless(car(cdr(ret2)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret2))) == INT2FIX(3));
  fail_unless(car(cdr(cdr(cdr(ret2)))) == INT2FIX(4));
  fail_unless(car(cdr(cdr(cdr(cdr(ret2))))) == INT2FIX(5));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(6));
  fail_unless(car(cdr(cdr(cdr(ret)))) == INT2FIX(7));
}
END_TEST

START_TEST(test_reinsert_sorted)
{
  value ret;

  TEST("(reinsert-sorted < 3 '(1 2 3 4))");
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) == INT2FIX(1));
  fail_unless(car(cdr(ret)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(3));
  fail_unless(car(cdr(cdr(cdr(ret)))) == INT2FIX(4));
}
END_TEST

START_TEST(test_insortnew)
{
  value ret, ret2;

  TEST("(let xs '(0 (1 2 3 4) 6 7) (insortnew < 3 (cadr xs)) xs)");
  fail_unless(CONS_P(ret));
  fail_unless(car(ret) = INT2FIX(0));
  ret2 = car(cdr(ret));
  fail_unless(car(ret2) == INT2FIX(1));
  fail_unless(car(cdr(ret2)) == INT2FIX(2));
  fail_unless(car(cdr(cdr(ret2))) == INT2FIX(3));
  fail_unless(car(cdr(cdr(cdr(ret2)))) == INT2FIX(4));
  fail_unless(car(cdr(cdr(ret))) == INT2FIX(6));
  fail_unless(car(cdr(cdr(cdr(ret)))) == INT2FIX(7));
}
END_TEST

START_TEST(test_defmemo)
{
  value ret;

  /* the memoizing functionality of defmemo ensures that memoval is
     incremented only once for every value of the argument to tdm. */
  TEST("(do (= memoval 0) (defmemo tdm (x) (++ memoval)) (with (x (tdm 0) y (tdm 0) z (tdm 1) w (tdm 1)) (and (is x y) (is z w) (isnt x z) (isnt x w) (isnt y z) (isnt y w))))");
  fail_unless(ret == CTRUE);
}
END_TEST

START_TEST(test_lte)
{
  value ret;

  TEST("(<= 1 2 2 3)");
  fail_unless(ret == CTRUE);

  TEST("(< 1 2 2 3)");
  fail_unless(NIL_P(ret));

  TEST("(<= 1 2 3)");
  fail_unless(ret == CTRUE);

  TEST("(<= 1 2 4 3)");
  fail_unless(ret == CNIL);
}
END_TEST

START_TEST(test_gte)
{
  value ret;

  TEST("(>= 3 2 2 1)");
  fail_unless(ret == CTRUE);

  TEST("(> 3 2 2 1)");
  fail_unless(NIL_P(ret));

  TEST("(>= 3 2 1)");
  fail_unless(ret == CTRUE);

  TEST("(>= 3 4 2 1)");
  fail_unless(ret == CNIL);
}
END_TEST

START_TEST(test_whitec)
{
  value ret;

  TEST("(and (whitec #\\space) (whitec #\\newline) (whitec #\\tab) (whitec #\\return) (no (whitec #\\a)))");
  fail_unless(ret == CTRUE);
}
END_TEST

START_TEST(test_nonwhite)
{
  value ret;

  TEST("(and (no (nonwhite #\\space)) (no (nonwhite #\\newline)) (no (nonwhite #\\tab)) (no (nonwhite #\\return)) (nonwhite #\\a))");
  fail_unless(ret == CTRUE);
}
END_TEST

START_TEST(test_letter)
{
  value ret;

  TEST("(and (letter #\\a) (letter #\\A) (letter #\\z) (letter #\\Z) (no (letter #\\0)) (no (letter #\\space)))");
  fail_unless(ret == CTRUE);
}
END_TEST

START_TEST(test_digit)
{
  value ret;

  TEST("(and (digit #\\0) (digit #\\5) (digit #\\6) (digit #\\9) (no (digit #\\a)) (no (digit #\\space)))");
  fail_unless(ret == CTRUE);
}
END_TEST

START_TEST(test_alphadig)
{
  value ret;

  TEST("(and (alphadig #\\a) (alphadig #\\A) (alphadig #\\0) (alphadig #\\9) (no (alphadig #\\$)) (no (alphadig #\\space)))");
  fail_unless(ret == CTRUE);
}
END_TEST

START_TEST(test_punc)
{
  value ret;

  TEST("(and (punc #\\.) (punc #\\,) (punc #\\;) (punc #\\?) (no (punc #\\$)) (no (punc #\\space)) (no (punc #\\a)))");
  fail_unless(ret == CTRUE);
}
END_TEST

START_TEST(test_summing)
{
  value ret;

  TEST("(summing idfn (+ (idfn 1) (+ (idfn 2) 3)))");
  fail_unless(ret == INT2FIX(2));
}
END_TEST

START_TEST(test_sum)
{
  value ret;

  TEST("(sum [+ _ 1] '(1 2 3 4))");
  fail_unless(ret == INT2FIX(14));
}
END_TEST

START_TEST(test_treewise)
{
  value ret;

  /* XXX - do something about 'f' as well as 'base'? */
  TEST("(iso (let x nil (treewise (fn (x y)) [= x (cons _ x)] '(f (b a d c . e) g nil i h)) x) '(nil h i nil g e c d a b f))");
  fail_unless(ret == CTRUE);
}
END_TEST

START_TEST(test_carif)
{
  value ret;

  TEST("(is (carif 1) 1)");
  fail_unless(ret == CTRUE);

  TEST("(is (carif '(3 4 5)) 3)");
  fail_unless(ret == CTRUE);
}
END_TEST

START_TEST(test_tree_subst)
{
  value ret;

  /* XXX - do something about 'f' as well as 'base'? */
  TEST("(iso (tree-subst 'g '(g h i) '(f (b a d c . e) g nil i h)) '(f (b a d c . e) (g h i) nil i h))");
  fail_unless(ret == CTRUE);
}
END_TEST

START_TEST(test_ontree)
{
  value ret;

  /* XXX - do something about 'f' as well as 'base'? */
  TEST("(iso (let x nil (ontree [when (atom _) (= x (cons _ x))] '(f (b a d c . e) g nil i h)) x) '(nil h i nil g e c d a b f))");
  fail_unless(ret == CTRUE);
}
END_TEST

START_TEST(test_dotted)
{
  value ret;

  TEST("(dotted '(b . c))");
  fail_unless(ret == CTRUE);

  TEST("(dotted '(a b c d e f . g))");
  fail_unless(ret == CTRUE);

  TEST("(dotted '(a b c d e f g))");
  fail_unless(ret == CNIL);

  TEST("(dotted 'a)");
  fail_unless(ret == CNIL);
}
END_TEST

START_TEST(test_fill_table)
{
  value ret;

  TEST("(let tbl (table) (fill-table tbl '(a 1 b 2 c 3)) (and (is (tbl 'a) 1) (is (tbl 'b) 2) (is (tbl 'c) 3) (no (tbl 'd))))");
  fail_unless(ret == CTRUE);
}
END_TEST

START_TEST(test_keys)
{
  value ret;

  TEST("(iso (let tbl (table) (fill-table tbl '(a 1 b 2 c 3)) (sort (fn (a b) (< (coerce a 'string) (coerce b 'string)))  (keys tbl))) '(a b c))");
  fail_unless(ret == CTRUE);
}
END_TEST

START_TEST(test_vals)
{
  value ret;

  TEST("(iso (let tbl (table) (fill-table tbl '(a 1 b 2 c 3)) (sort (fn (a b) (< a b))  (vals tbl))) '(1 2 3))");
  fail_unless(ret == CTRUE);
}
END_TEST

START_TEST(test_tablist)
{
  value ret;

  TEST("(iso (let tbl (table) (fill-table tbl '(a 1 b 2 c 3)) (sort (fn (a b) (< (coerce (car a) 'string) (coerce (car b) 'string))) (tablist tbl))) '((a 1) (b 2) (c 3)))");
  fail_unless(ret == CTRUE);
}
END_TEST

START_TEST(test_listtab)
{
  value ret;

  TEST("(let tbl (listtab '((a 1) (b 2) (c 3))) (and (is (tbl 'a) 1) (is (tbl 'b) 2) (is (tbl 'c) 3) (no (tbl 'd))))");
  fail_unless(ret == CTRUE);
}
END_TEST

START_TEST(test_obj)
{
  value ret;

  TEST("(let tbl (obj a 1 b 2 c 3) (and (is (tbl 'a) 1) (is (tbl 'b) 2) (is (tbl 'c) 3) (no (tbl 'd))))");
  fail_unless(ret == CTRUE);
}
END_TEST

START_TEST(test_copy)
{
  value ret;

  TEST("(withs (x '(a b c) cp (copy x)) (and (no (is x cp)) (iso x cp)))");
  fail_unless(ret == CTRUE);

  TEST("(withs (x (obj a 1 b 2 c 3) cp (copy x)) (and (no (is x cp)) (iso x cp)))");
 fail_unless(ret == CTRUE);

}
END_TEST

START_TEST(test_abs)
{
  value ret;

  TEST("(is (abs 1) 1)");
  fail_unless(ret == CTRUE);

  TEST("(is (abs -1) 1)");
  fail_unless(ret == CTRUE);

  TEST("(is (abs -1.0) 1.0)");
  fail_unless(ret == CTRUE);

  TEST("(is (abs 1.0) 1.0)");
  fail_unless(ret == CTRUE);

#ifdef HAVE_GMP_H
  TEST("(is (abs 1000000000000000000000000000000000000000000000000000000000000) 1000000000000000000000000000000000000000000000000000000000000)");
  fail_unless(ret == CTRUE);

  TEST("(is (abs -1000000000000000000000000000000000000000000000000000000000000) 1000000000000000000000000000000000000000000000000000000000000)");
  fail_unless(ret == CTRUE);

  TEST("(is (abs 3/2) 3/2)")
  fail_unless(ret == CTRUE);
  TEST("(is (abs -3/2) 3/2)")
  fail_unless(ret == CTRUE);
#endif
}
END_TEST

START_TEST(test_round)
{
  value ret;

  TEST("(is (round 1.1) 1)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 1.2) 1)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 1.3) 1)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 1.4) 1)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 1.5) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 1.6) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 1.7) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 1.8) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 1.9) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 2.1) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 2.2) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 2.3) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 2.4) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 2.5) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 2.6) 3)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 2.7) 3)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 2.8) 3)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 2.9) 3)");
  fail_unless(ret == CTRUE);

#ifdef HAVE_GMP_H
  TEST("(is (round 11/10) 1)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 6/5) 1)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 13/10) 1)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 7/5) 1)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 3/2) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 8/5) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 17/10) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 9/5) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 19/10) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 21/10) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 11/5) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 23/10) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 12/5) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 5/2) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 13/5) 3)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 27/10) 3)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 14/5) 3)");
  fail_unless(ret == CTRUE);

  TEST("(is (round 29/10) 3)");
  fail_unless(ret == CTRUE);
#endif
}
END_TEST

START_TEST(test_roundup)
{
  value ret;

  TEST("(is (roundup 1.1) 1)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 1.2) 1)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 1.3) 1)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 1.4) 1)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 1.5) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 1.6) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 1.7) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 1.8) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 1.9) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 2.1) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 2.2) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 2.3) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 2.4) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 2.5) 3)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 2.6) 3)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 2.7) 3)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 2.8) 3)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 2.9) 3)");
  fail_unless(ret == CTRUE);

#ifdef HAVE_GMP_H
  TEST("(is (roundup 11/10) 1)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 6/5) 1)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 13/10) 1)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 7/5) 1)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 3/2) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 8/5) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 17/10) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 9/5) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 19/10) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 21/10) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 11/5) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 23/10) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 12/5) 2)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 5/2) 3)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 13/5) 3)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 27/10) 3)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 14/5) 3)");
  fail_unless(ret == CTRUE);

  TEST("(is (roundup 29/10) 3)");
  fail_unless(ret == CTRUE);
#endif
}
END_TEST

int main(void)
{
  int number_failed;
  Suite *s = suite_create("arc.arc");
  TCase *tc_arc = tcase_create("arc.arc");
  SRunner *sr;
  char *loadstr = "arc.arc";
  value initload, sexpr, cctx, code;

  c = &cc;
  arc_init(c);

  initload = arc_infile(c, arc_mkstringc(c, loadstr));
  arc_bindsym(c, arc_intern_cstr(c, "initload"), initload);
  c->signal_error = NULL;
  while ((sexpr = arc_read(c, initload, CNIL)) != CNIL) {
    cctx = arc_mkcctx(c, INT2FIX(1), 0);
    arc_compile(c, sexpr, cctx, CNIL, CTRUE);
    code = arc_cctx2code(c, cctx);
    arc_macapply(c, code, CNIL);
    c->rungc(c);
  }
  arc_close(c, initload);

  c->signal_error = error_handler;
  tcase_add_test(tc_arc, test_do);
  tcase_add_test(tc_arc, test_safeset);
  tcase_add_test(tc_arc, test_apply);
  tcase_add_test(tc_arc, test_def);
  tcase_add_test(tc_arc, test_caar);
  tcase_add_test(tc_arc, test_cadr);
  tcase_add_test(tc_arc, test_cddr);
  tcase_add_test(tc_arc, test_no);
  tcase_add_test(tc_arc, test_acons);
  tcase_add_test(tc_arc, test_copylist);
  tcase_add_test(tc_arc, test_list);
  tcase_add_test(tc_arc, test_idfn);
  tcase_add_test(tc_arc, test_map1);
  tcase_add_test(tc_arc, test_pair);
  tcase_add_test(tc_arc, test_mac);
  tcase_add_test(tc_arc, test_and);
  tcase_add_test(tc_arc, test_assoc);
  tcase_add_test(tc_arc, test_alref);
  tcase_add_test(tc_arc, test_with);
  tcase_add_test(tc_arc, test_let);
  tcase_add_test(tc_arc, test_withs);
  tcase_add_test(tc_arc, test_join);
  tcase_add_test(tc_arc, test_rfn);
  tcase_add_test(tc_arc, test_afn);
  tcase_add_test(tc_arc, test_compose);
  tcase_add_test(tc_arc, test_complement);
  tcase_add_test(tc_arc, test_rev);
  tcase_add_test(tc_arc, test_isnt);
  /* w/uniq is used in many other macros so any problems with it should
     manifest when we test the other functions. */
  tcase_add_test(tc_arc, test_or);
  tcase_add_test(tc_arc, test_in);
  tcase_add_test(tc_arc, test_when);
  tcase_add_test(tc_arc, test_unless);
  tcase_add_test(tc_arc, test_while);
  tcase_add_test(tc_arc, test_empty);
  tcase_add_test(tc_arc, test_reclist);
  tcase_add_test(tc_arc, test_recstring);
  tcase_add_test(tc_arc, test_testify);
  tcase_add_test(tc_arc, test_some);
  tcase_add_test(tc_arc, test_all);
  tcase_add_test(tc_arc, test_find);
  tcase_add_test(tc_arc, test_isa);
  tcase_add_test(tc_arc, test_map);
  tcase_add_test(tc_arc, test_mappend);
  tcase_add_test(tc_arc, test_firstn);
  tcase_add_test(tc_arc, test_nthcdr);
  tcase_add_test(tc_arc, test_tuples);
  tcase_add_test(tc_arc, test_defs);
  tcase_add_test(tc_arc, test_caris);
  /* atomic, atlet, atwith, and atwiths are meaningless until we
     get threading, so we won't bother writing tests for them. */
  tcase_add_test(tc_arc, test_places);
  tcase_add_test(tc_arc, test_loop);
  tcase_add_test(tc_arc, test_for);
  tcase_add_test(tc_arc, test_down);
  tcase_add_test(tc_arc, test_repeat);
  tcase_add_test(tc_arc, test_walk);
  tcase_add_test(tc_arc, test_each);
  tcase_add_test(tc_arc, test_cut);
  tcase_add_test(tc_arc, test_whilet);
  tcase_add_test(tc_arc, test_last);
  tcase_add_test(tc_arc, test_rem);
  tcase_add_test(tc_arc, test_keep);
  tcase_add_test(tc_arc, test_trues);
  tcase_add_test(tc_arc, test_do1);
  tcase_add_test(tc_arc, test_caselet);
  tcase_add_test(tc_arc, test_case);
  tcase_add_test(tc_arc, test_pushpop);
  tcase_add_test(tc_arc, test_swap);
  tcase_add_test(tc_arc, test_rotate);
  tcase_add_test(tc_arc, test_adjoin);
  tcase_add_test(tc_arc, test_pushnew);
  tcase_add_test(tc_arc, test_pull);
  tcase_add_test(tc_arc, test_togglemem);
  tcase_add_test(tc_arc, test_plusplus);
  tcase_add_test(tc_arc, test_minusminus);
  tcase_add_test(tc_arc, test_zap);
  /* XXX - no tests for pr, prt and prn yet.  We need to fix I/O
     more. */
  tcase_add_test(tc_arc, test_wipe);
  tcase_add_test(tc_arc, test_set);
  tcase_add_test(tc_arc, test_iflet);
  tcase_add_test(tc_arc, test_whenlet);
  tcase_add_test(tc_arc, test_aif);
  tcase_add_test(tc_arc, test_awhen);
  tcase_add_test(tc_arc, test_aand);
  tcase_add_test(tc_arc, test_accum);
  tcase_add_test(tc_arc, test_drain);
  tcase_add_test(tc_arc, test_whiler);
  tcase_add_test(tc_arc, test_consif);
  tcase_add_test(tc_arc, test_string);
  tcase_add_test(tc_arc, test_flat);
  tcase_add_test(tc_arc, test_check);
  tcase_add_test(tc_arc, test_pos);
  tcase_add_test(tc_arc, test_even);
  tcase_add_test(tc_arc, test_odd);
  tcase_add_test(tc_arc, test_after);
  /* no tests for I/O functions and macros (yet?):
     w/infile
     w/outfile
     w/instring
     w/socket
     w/outstring
     w/appendfile
     w/stdout
     w/stdin
     tostring
     fromstring
     readstring1
     read
     readfile
     readfile1
     readall
     allchars
     filechars
     writefile
   */
  tcase_add_test(tc_arc, test_int);
  tcase_add_test(tc_arc, test_rand_choice);
  tcase_add_test(tc_arc, test_n_of);
  /* no test for rand-string (yet?) */
  tcase_add_test(tc_arc, test_forlen);
  tcase_add_test(tc_arc, test_on);
  tcase_add_test(tc_arc, test_best);
  tcase_add_test(tc_arc, test_max);
  tcase_add_test(tc_arc, test_min);
  tcase_add_test(tc_arc, test_most);
  tcase_add_test(tc_arc, test_insert_sorted);
  tcase_add_test(tc_arc, test_insort);
  tcase_add_test(tc_arc, test_reinsert_sorted);
  tcase_add_test(tc_arc, test_insortnew);
  /* memo is tested by defmemo */
  tcase_add_test(tc_arc, test_defmemo);
  tcase_add_test(tc_arc, test_lte);
  tcase_add_test(tc_arc, test_gte);
  tcase_add_test(tc_arc, test_whitec);
  tcase_add_test(tc_arc, test_nonwhite);
  tcase_add_test(tc_arc, test_letter);
  tcase_add_test(tc_arc, test_digit);
  tcase_add_test(tc_arc, test_alphadig);
  tcase_add_test(tc_arc, test_punc);
  /* no test for readline */
  tcase_add_test(tc_arc, test_summing);
  tcase_add_test(tc_arc, test_sum);
  tcase_add_test(tc_arc, test_treewise);
  tcase_add_test(tc_arc, test_carif);
  /* no test for prall or prs */
  tcase_add_test(tc_arc, test_tree_subst);
  tcase_add_test(tc_arc, test_ontree);
  tcase_add_test(tc_arc, test_dotted);
  tcase_add_test(tc_arc, test_fill_table);
  tcase_add_test(tc_arc, test_keys);
  tcase_add_test(tc_arc, test_vals);
  tcase_add_test(tc_arc, test_tablist);
  tcase_add_test(tc_arc, test_listtab);
  tcase_add_test(tc_arc, test_obj);
  /* no tests for load-table, read-table, load-tables, save-table, or
     write-table yet */
  tcase_add_test(tc_arc, test_copy);
  tcase_add_test(tc_arc, test_abs);
  tcase_add_test(tc_arc, test_round);
  tcase_add_test(tc_arc, test_roundup);

  suite_add_tcase(s, tc_arc);
  sr = srunner_create(s);
  /* srunner_set_fork_status(sr, CK_NOFORK); */
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return((number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

