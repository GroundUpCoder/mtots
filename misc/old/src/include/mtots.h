#ifndef mtots_h
#define mtots_h

#include <stddef.h>

#define MTOTS_MAX_VALUE_STACK_SIZE 1024

#define MTOTS_KIND_NIL 0
#define MTOTS_KIND_BOOL 1
#define MTOTS_KIND_NUMBER 2
#define MTOTS_KIND_CFUNCTION 3
#define MTOTS_KIND_USERDATA 4
#define MTOTS_KIND_STRING 5
#define MTOTS_KIND_LIST 6
#define MTOTS_KIND_TABLE 7
#define MTOTS_KIND_FUNCTION 8

#define MTOTS_OP_ADD 0
#define MTOTS_OP_SUB 1
#define MTOTS_OP_MUL 2
#define MTOTS_OP_DIV 3
#define MTOTS_OP_IDIV 4  /* floor division */
#define MTOTS_OP_MOD 5
#define MTOTS_OP_POW 6
#define MTOTS_OP_UNM 7   /* unary minus */

#define MTOTS_OP_EQ 0
#define MTOTS_OP_LT 1
#define MTOTS_OP_LE 2

typedef struct mtots_State mtots_State;

typedef int (*mtots_CFunction)(mtots_State *L);

mtots_State *mtots_newstate();

void mtots_panic(mtots_State *L, const char *msg);
void mtots_panicf(mtots_State *L, const char *fmt, ...);

/* Returns the number of elements in the current stack */
int mtots_gettop(mtots_State *L);

void mtots_pop(mtots_State *L, int n);
void mtots_pushnil(mtots_State *L);
void mtots_pushbool(mtots_State *L, int b);
void mtots_pushnumber(mtots_State *L, double n);
void mtots_pushcfunction(mtots_State *L, mtots_CFunction f);
void mtots_pushstring(mtots_State *L, const char *s);

void mtots_pushfstring(mtots_State *L, const char *fmt, ...);
void mtots_pushlstring(mtots_State *L, const char *fmt, size_t len);

/* Pops two (or one when op is UPM) operands from the top of the stack,
 * performs the indicated operation and pushes the result onto the stack.
 */
void mtots_arith(mtots_State *L, int op);

/* Compares two mtots values and returns 1 if the operation is satisfied,
 * or 0 if not.
 */
int mtots_compare(mtots_State *L, int index1, int index2, int op);

/* Copies the element at index fromidx into the valid index toidx,
 * replacing the value at that position.
 * Values at other positions are not affected.
 */
void mtots_copy(mtots_State *L, int fromidx, int toidx);

/* Calls a function.
 * To call a function in mtots, first push the function onto the stack.
 * Then push nargs arguments onto the stack in order (first argument
 * should be pushed first).
 * Then finally call `mtots_call` with 'nargs' indicating the number of
 * arguments you pushed onto the stack.
 * When the function returns, all arguments and the function are popped
 * from the stack, and all results are pushed onto the stack.
 * If more results are expected than the function provides, the stack
 * is padded with 'nil's. If more results are provided than expected,
 * those extra result values are dropped.
 */
void mtots_call(mtots_State *L, int nargs, int nresults);

#endif/*mtots_h*/
