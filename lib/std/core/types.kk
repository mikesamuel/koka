public module std/core/types

infixr 30  (&&)
infixr 20  (||)

extern include {
  js file "types-inline.js"
}

// ----------------------------------------------------------------------------
// Core types
// ----------------------------------------------------------------------------

// An arbitrary precision signed integer.
type int

// A string is a sequence of unicode character points (`char`).
// The encoding of a string is internal and there
// is no constant-time indexing of characters in a string.
// Use the `:sslice` type for efficient matching and retrieving
// sub-strings from string.\
// See also the [``std/text/string``](std_text_string.html) module.
type string

// A 64-bit IEEE 754 floating point value.
// See also `std/num/double` for more operations on `:double`s.
type value double

// A 32-bit signed integer (represented in two's complement).
// See the [``std/int32``](std_int32.html) module for operations on 32-bit integers.
type value int32

// A signed integer equal to `size_t` in C. 
// Provides no operations but is used mostly for interaction with external code.
// The Javascript backend uses `int32` for this.
type value size_t

// A 8-bit unsigned integer (represented in two's complement).
// Provides no operations but is used mostly for interaction with external code.
type value uint8

// A 8-bit signed integer (represented in two's complement).
// Provides no operations but is used mostly for interaction with external code.
type value int8

// A 16-bit signed integer (represented in two's complement).
// Provides no operations but is used mostly for interaction with external code.
type value int16

// A 32-bit IEEE 754 floating point value.
// Not supported by the Javascript backend
// Provides no operations but is used mostly for interaction with external code.
type value float32

// An any type. Used for extern calls
type value any

// Internal type used for in-place update of unique pattern matches
type reuse

extern no-reuse() : reuse {
  c inline "NULL"
  js inline "null"
}

// A unicode character.
// Characters are unicode _codepoint_\/s.
// This is different from a unicode _grapheme_ which represents a single displayed
// symbol and can consists of multiple codepoints due to combining characters and marks.
// (see also the [``std/text/unicode``](std_text_unicode.html) module.)
type value char

// The _total_ effect represents the absence of any effect.
type <> :: E

// The effect constructor extends an effect with another effect.
type <|> :: (X,E) -> E

// The type of immutable arrays is called `:vector`.
type vector<a>


// The divergence effect: a divergent function may not terminate.
type div :: X

// The non-determism effect.
type ndet :: X

// The alloc effect signifies a function may allocate in a heap `:h`
type alloc :: H -> X

// The read effect: signifies that a function may read from from heap `:h`.
type read :: H -> X

// The write effect: signifies that a function may write to heap `:h`.
type write :: H -> X

// Stateful funs can manipulate heap `:h` using allocations, reads and writes.
alias st<h> = <read<h>, write<h>, alloc<h>>

// The `:global` heap is a special heap constant to denote the global shared heap
type global :: H


// ----------------------------------------------------------------------------
// Internal types
// ----------------------------------------------------------------------------

// Optional is used internally by the compiler to pass optional arguments.
// It is usually displayed as `:?a` for some type `:a`.
type value optional<a> {
  // The `Optional` constructor is used when an optional argument is given.
  con Optional(value:a)
  // `None` is used when an optional argument is not provided.
  con None
}

// Handled effects are lifted to an atomic effect using the `:handled` type constructor
type handled    :: HX -> X;

// Linear effects are lifted to an atomic effect using the `:handled1` type constructor
type handled1   :: HX1 -> X;

// The `:inst` effect is the default umbrella effect for effect instances
type inst :: HX


// ----------------------------------------------------------------------------
// Standard Data types
// ----------------------------------------------------------------------------

// The `:void` type is empty and has no constructors.
// See also the `:()` unit type and the `:bool` type.
type void

// The type of booleans has two inhabitants: `True` and `False`.
type bool {
  con False
  con True
}

// The unit type `:()` is inhabited by just a single value, namely `()`.
// See also the `:void` type and the `:bool` type.
struct ()

// A pair of values `:a` and `:b`.
struct (,)<a,b>(fst:a,snd:b)

// A triple of values.
struct (,,)<a,b,c>(fst:a,snd:b,thd:c)

// A quadruple of values.
struct (,,,)<a,b,c,d>(fst:a,snd:b,thd:c,field4:d)

// A quintuple of values.
struct (,,,,)<a,b,c,d,e>(fst:a,snd:b,thd:c,field4:d,field5:e)

// The `:maybe` type is used to represent either a value (`Just(x)`) or `Nothing`.
// This type is often used to represent values that can be _null_.
type maybe<a> {
  con Nothing
  con Just( value : a )
}

// The choice type represents one of two possible types `:a` or `:b`.
type either<a,b> {
  con Left( left : a )
  con Right( right : b )
}

// An enumeration to represent order
type order {
  Lt
  Eq
  Gt
}

// ----------------------------------------------------------------------------
// Standard functions
// ----------------------------------------------------------------------------

fun id(x : a) : a {
  x
}

// Logical conjuction
fun (&&)( x : bool, y : bool) : bool {  // inlined in the compiler for short-circuit evaluation
  if (x) then y else False
}

// Logical disjunction
fun (||)( x : bool, y : bool) : bool { // inlined in the compiler for short-circuit evaluation
  if (x) then True else y
}

// extern inline (&&) : (bool,bool) -> bool  { inline "(#1 && #2)" }
// extern inline (||) : (bool,bool) -> bool  { inline "(#1 || #2)" }

// for efficiency we use extern here (for now)
// fun (!)( b : bool ) : bool = if (x) then false else true
// fun not( b : bool ) : bool = if (x) then false else true

extern inline not  : (bool) -> bool  { inline "!(#1)" }
extern inline (!)  : (bool) -> bool  { inline "!(#1)" }

// needed for markers in `std/core/hnd`.
extern inline zero32() : int32 { inline "0" }

// ----------------------------------------------------------------------------
// References
// ----------------------------------------------------------------------------

// The predicate `:hdiv<h,a,e>` signifies that if the type `:a` contains a reference to `:h`,
// then the effect `:e` must contain the divergent effect (`:div`). This constraint is generated
// when reading from the heap (see `(!)`) and is necessary to catch cases where code can diverge
// by storing self referential functions in the heap.
type hdiv :: (H,V,E) -> P

// The predicate `:ediv<x,a,e>` signifies that if the type `:a` contains a reference to effect constant `:x",
// then the effect `:e` must contain the divergent effect (`:div`). This constraint is generated
// for operations on first-class effects where code can diverge through operations.
type ediv :: (X,V,E) -> P


// A reference `:ref<h,a>` points to a value of type `:a` in heap `:h`.
type ref :: (H,V) -> V

// Allocate a fresh reference with an initial value.
extern inline ref : forall<h,a> (value:a) -> alloc<h> ref<h,a> {
  c inline "kk_ref_alloc(#1,kk_context())"
  cs inline "new Ref<##1,##2>(#1)"
  js inline "{ value: #1 }"
}

// Assign a new value to a reference.
extern inline set  : forall<h,a> (ref: ref<h,a>, assigned: a) -> <write<h>> () {
  c inline "kk_ref_set(#1,#2,kk_context())"
  cs inline "#1.Set(#2)";
  js inline "((#1).value = #2)"
}

// Read the value of reference.
extern inline (!) : forall<h,a,e> (ref: ref<h,a>) -> <read<h>|e> a with(hdiv<h,a,e>)  {
  c inline "kk_ref_get(#1,kk_context())"
  cs inline "#1.Value";
  js inline "((#1).value)";
}

// If a heap effect is unobservable, the heap effect can be erased by using the `run` fun.
// See also: _State in Haskell, by Simon Peyton Jones and John Launchbury_.
extern inline run : forall<e,a> ( action : forall<h> () -> <alloc<h>,read<h>,write<h> | e> a ) -> e a  {
  c  "(kk_function_call(kk_box_t,(kk_function_t,kk_context_t*),#1,(#1,kk_context())))"
  cs inline "Primitive.Run<##2>(#1)";
  js inline "((#1)())";
}

// ----------------------------------------------------------------------------
// Local Variables
// ----------------------------------------------------------------------------

// A local variable `:var<s,a>` points to a value of type `:a` in local scope `:s`
type local :: (H) -> X
type local-var :: (H,V) -> V

// Allocate a fresh local with an initial value.
extern inline local-new : forall<s,a> (value:a) -> local<s> local-var<s,a> {
  c inline "kk_ref_alloc(#1,kk_context())"
  cs inline "new Ref<##1,##2>(#1)"
  js inline "{ value: #1 }"
}

// Assign a new value to a local variable
extern inline local-set  : forall<s,a,l> (v: local-var<s,a>, assigned: a) -> <local<s>> () {
  c inline "(kk_ref_set(#1,#2,kk_context()))"
  cs inline "#1.Set(#2)";
  js inline "((#1).value = #2)"
}

// Read the value of a local variable.
extern inline local-get : forall<s,a,e> (v: local-var<s,a>) -> <local<s>|e> a with(hdiv<s,a,e>)  {
  c inline "(kk_ref_get(#1,kk_context()))"
  cs inline "#1.Value";
  js inline "((#1).value)";
}

// If local mutation is unobservable, the `:local` effect can be erased by using the `local-scope` function.
// See also: _State in Haskell, by Simon Peyton Jones and John Launchbury_.
extern inline local-scope : forall<e,a> ( action : forall<h> () -> <local<h> | e> a) -> e a  {
  c  "(kk_function_call(kk_box_t,(kk_function_t,kk_context_t*),#1,(#1,kk_context())))"
  cs inline "Primitive.Run<##2>(#1)";
  js inline "((#1)())";
}

extern inline byref(loc : a) : a { inline "#1" }

// ----------------------------------------------------------------------------
// Unsafe: todo: move to separate module
// ----------------------------------------------------------------------------

// _Unsafe_. Mark a function parameter as decreasing to suppress the non-termination effect (`:div`).
extern inline unsafe-decreasing( x : a ) : a { inline "#1" }

private extern inline unsafe-total-cast : forall<e,a> ( action : () -> e a ) -> (() -> a) { inline "#1" }

// _Unsafe_. This function calls a function and pretends it did not have any effect at all.
fun unsafe-total( action : () -> e a ) : a {
  unsafe-total-cast( action )()
}

// _Unsafe_. This function pretends the give action was deterministic
fun unsafe-no-ndet( action : () -> <ndet|e> a ) : e a {
  unsafe-total-cast( action )()
}

// _Unsafe_. This function pretends the give action is terminating
fun unsafe-no-div( action : () -> <div|e> a ) : e a {
  unsafe-total-cast( action )()
}
