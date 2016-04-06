// This file contains an implementation of perfect-hash generation.
// It is an bastardized version of code originally due to Bob Jenkins.
// See this website for discussion:
//   <http://www.burtleburtle.net/bob/hash/perfect.html>
//
// This code creates a command-line utility that should be placed
//

///////////////////////////////////////////////////////////////////////////////
// BEGIN STANDARD.H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

typedef uint64_t        ub8;
#define UB8MAXVAL 0xffffffffffffffffLL
#define UB8BITS 64
typedef int64_t         sb8;
#define SB8MAXVAL 0x7fffffffffffffffLL
typedef uint32_t        ub4;   /* unsigned 4-byte quantities */
#define UB4MAXVAL 0xffffffff
typedef int32_t         sb4;
#define UB4BITS 32
#define SB4MAXVAL 0x7fffffff
typedef uint16_t        ub2;
#define UB2MAXVAL 0xffff
#define UB2BITS 16
typedef int16_t         sb2;
#define SB2MAXVAL 0x7fff
typedef unsigned char   ub1;
#define UB1MAXVAL 0xff
#define UB1BITS 8
typedef signed char     sb1;   /* signed 1-byte quantities */
#define SB1MAXVAL 0x7f
typedef int32_t         word;  /* fastest type available */

#define bis(target,mask)  ((target) |=  (mask))
#define bic(target,mask)  ((target) &= ~(mask))
#define bit(target,mask)  ((target) &   (mask))
#ifndef min
# define min(a,b) (((a)<(b)) ? (a) : (b))
#endif /* min */
#ifndef max
# define max(a,b) (((a)<(b)) ? (b) : (a))
#endif /* max */
#ifndef align
# define align(a) (((ub4)a+(sizeof(void *)-1))&(~(sizeof(void *)-1)))
#endif /* align */
#ifndef abs
# define abs(a)   (((a)>0) ? (a) : -(a))
#endif
#define TRUE  1
#define FALSE 0
#define SUCCESS 0  /* 1 on VAX */

// END STANDARD.H
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// BEGIN RECYCLE.H

#define RESTART    0
#define REMAX      32000

struct recycle
{
   struct recycle *next;
};
typedef  struct recycle  recycle;

struct reroot
{
   struct recycle *list;     /* list of malloced blocks */
   struct recycle *trash;    /* list of deleted items */
   size_t          size;     /* size of an item */
   size_t          logsize;  /* log_2 of number of items in a block */
   word            numleft;  /* number of bytes left in this block */
};
typedef  struct reroot  reroot;

/* make a new recycling root */
reroot  *remkroot(/*_ size_t mysize _*/);

/* free a recycling root and all the items it has made */
void     refree(/*_ struct reroot *r _*/);

/* get a new (cleared) item from the root */
#define renew(r) ((r)->numleft ? \
   (((char *)((r)->list+1))+((r)->numleft-=(r)->size)) : renewx(r))

char    *renewx(/*_ struct reroot *r _*/);

/* delete an item; let the root recycle it */
/* void     redel(/o_ struct reroot *r, struct recycle *item _o/); */
#define redel(root,item) { \
   ((recycle *)item)->next=(root)->trash; \
   (root)->trash=(recycle *)(item); \
}

/* malloc, but complain to stderr and exit program if no joy */
/* use plain free() to free memory allocated by remalloc() */
char    *remalloc(/*_ size_t len, char *purpose _*/);

// END RECYCLE.H
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// BEGIN PERFECT.H

#define MAXKEYLEN 30                              /* maximum length of a key */
#define USE_SCRAMBLE  4096           /* use scramble if blen >= USE_SCRAMBLE */
#define SCRAMBLE_LEN ((ub4)1<<16)                    /* length of *scramble* */
#define RETRY_INITKEY 2048  /* number of times to try to find distinct (a,b) */
#define RETRY_PERFECT 1     /* number of times to try to make a perfect hash */
#define RETRY_HEX     200               /* RETRY_PERFECT when hex keys given */

/* the generated code for the final hash, assumes initial hash is done */
struct gencode
{
  char **line;                       /* array of text lines, 80 bytes apiece */
  /*
   * The code placed here must declare "ub4 rsl" 
   * and assign it the value of the perfect hash using the function inputs.
   * Later code will be tacked on which returns rsl or manipulates it according
   * to the user directives.
   *
   * This code is at the top of the routine; it may and must declare any
   * local variables it needs.
   *
   * Each way of filling in **line should be given a comment that is a unique
   * tag.  A testcase named with that tag should also be found which tests
   * the generated code.
   */
  ub4    len;                    /* number of lines available for final hash */
  ub4    used;                         /* number of lines used by final hash */

  ub4    lowbit;                          /* for HEX, lowest interesting bit */
  ub4    highbit;                        /* for HEX, highest interesting bit */
  ub4    diffbits;                         /* bits which differ for some key */
  ub4    i,j,k,l,m,n,o;                      /* state machine used in hexn() */
};
typedef  struct gencode  gencode;

/* user directives: perfect hash? minimal perfect hash? input is an int? */
struct hashform
{
  enum {
    NORMAL_HM,                                            /* key is a string */
    INLINE_HM,    /* user will do initial hash, we must choose salt for them */
    HEX_HM,              /* key to be hashed is a hexidecimal 4-byte integer */
    DECIMAL_HM,              /* key to be hashed is a decimal 4-byte integer */
    AB_HM,      /* key to be hashed is "A B", where A and B are (A,B) in hex */
    ABDEC_HM                                   /* like AB_HM, but in decimal */
  } mode;
  enum {
    STRING_HT,                                            /* key is a string */
    INT_HT,                                             /* key is an integer */
    AB_HT             /* dunno what key is, but input is distinct (A,B) pair */
  } hashtype;
  enum {
    NORMAL_HP,                                   /* just find a perfect hash */
    MINIMAL_HP                                /* find a minimal perfect hash */
  } perfect;
  enum {
    FAST_HS,                                                    /* fast mode */
    SLOW_HS                                                     /* slow mode */
  } speed;
};
typedef  struct hashform  hashform;

/* representation of a key */
struct key
{
  ub1        *name_k;                                      /* the actual key */
  ub4         len_k;                         /* the length of the actual key */
  ub4         hash_k;                 /* the initial hash value for this key */
  struct key *next_k;                                            /* next key */
/* beyond this point is mapping-dependent */
  ub4         a_k;                            /* a, of the key maps to (a,b) */
  ub4         b_k;                            /* b, of the key maps to (a,b) */
  struct key *nextb_k;                               /* next key with this b */
  ub4 final_hash_k;
};
typedef  struct key  key;

/* things indexed by b of original (a,b) pair */
struct bstuff
{
  ub2  val_b;                                        /* hash=a^tabb[b].val_b */
  key *list_b;                   /* tabb[i].list_b is list of keys with b==i */
  ub4  listlen_b;                                        /* length of list_b */
  ub4  water_b;           /* high watermark of who has visited this map node */
};
typedef  struct bstuff  bstuff;

/* things indexed by final hash value */
struct hstuff
{
  key *key_h;                   /* tabh[i].key_h is the key with a hash of i */
};
typedef  struct hstuff hstuff;

/* things indexed by queue position */
struct qstuff
{
  bstuff *b_q;                        /* b that currently occupies this hash */
  ub4     parent_q;     /* queue position of parent that could use this hash */
  ub2     newval_q;      /* what to change parent tab[b] to to use this hash */
  ub2     oldval_q;                              /* original value of tab[b] */
};
typedef  struct qstuff  qstuff;

/* return ceiling(log based 2 of x) */
ub4 mylog2(/*_ ub4 x _*/);

/* Given the keys, scramble[], and hash mode, find the perfect hash */
void findhash(/*_ bstuff **tabb, ub4 *alen, ub4 *blen, ub4 *salt,
		gencode *final, ub4 *scramble, ub4 smax, key *keys, ub4 nkeys, 
		hashform *form _*/);

/* private, but in a different file because it's excessively verbose */
int inithex(/*_ key *keys, ub4 *alen, ub4 *blen, ub4 smax, ub4 nkeys, 
	      ub4 salt, gencode *final, gencode *form _*/);

// END PERFECT.H
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// BEGIN LOOKUPA.H

#define CHECKSTATE 8
#define hashsize(n) ((ub4)1<<(n))
#define hashmask(n) (hashsize(n)-1)

ub4  lookup(/*_ ub1 *k, ub4 length, ub4 level _*/);
void checksum(/*_ ub1 *k, ub4 length, ub4 *state _*/);

// END LOOKUPA.H
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

reroot *remkroot(size)
size_t  size;
{
   reroot *r = (reroot *)remalloc(sizeof(reroot), "recycle.c, root");
   r->list = (recycle *)0;
   r->trash = (recycle *)0;
   r->size = align(size);
   r->logsize = RESTART;
   r->numleft = 0;
   return r;
}

void  refree(r)
struct reroot *r;
{
   recycle *temp;
   if (temp = r->list) while (r->list)
   {
      temp = r->list->next;
      free((char *)r->list);
      r->list = temp;
   }
   free((char *)r);
   return;
}

/* to be called from the macro renew only */
char  *renewx(r)
struct reroot *r;
{
   recycle *temp;
   if (r->trash)
   {  /* pull a node off the trash heap */
      temp = r->trash;
      r->trash = temp->next;
      (void)memset((void *)temp, 0, r->size);
   }
   else
   {  /* allocate a new block of nodes */
      r->numleft = r->size*((ub4)1<<r->logsize);
      if (r->numleft < REMAX) ++r->logsize;
      temp = (recycle *)remalloc(sizeof(recycle) + r->numleft, 
				 "recycle.c, data");
      temp->next = r->list;
      r->list = temp;
      r->numleft-=r->size;
      temp = (recycle *)((char *)(r->list+1)+r->numleft);
   }
   return (char *)temp;
}

char   *remalloc(len, purpose)
size_t  len;
char   *purpose;
{
  char *x = (char *)malloc(len);
  if (!x)
  {
    fprintf(stderr, "malloc of %d failed for %s\n", 
	    len, purpose);
    exit(1);
  }
  return x;
}

///////////////////////////////////////////////////////////////////////////////

/*
--------------------------------------------------------------------
mix -- mix 3 32-bit values reversibly.
For every delta with one or two bit set, and the deltas of all three
  high bits or all three low bits, whether the original value of a,b,c
  is almost all zero or is uniformly distributed,
* If mix() is run forward or backward, at least 32 bits in a,b,c
  have at least 1/4 probability of changing.
* If mix() is run forward, every bit of c will change between 1/3 and
  2/3 of the time.  (Well, 22/100 and 78/100 for some 2-bit deltas.)
mix() was built out of 36 single-cycle latency instructions in a 
  structure that could supported 2x parallelism, like so:
      a -= b; 
      a -= c; x = (c>>13);
      b -= c; a ^= x;
      b -= a; x = (a<<8);
      c -= a; b ^= x;
      c -= b; x = (b>>13);
      ...
  Unfortunately, superscalar Pentiums and Sparcs can't take advantage 
  of that parallelism.  They've also turned some of those single-cycle
  latency instructions into multi-cycle latency instructions.  Still,
  this is the fastest good hash I could find.  There were about 2^^68
  to choose from.  I only looked at a billion or so.
--------------------------------------------------------------------
*/
#define mix(a,b,c) \
{ \
  a -= b; a -= c; a ^= (c>>13); \
  b -= c; b -= a; b ^= (a<<8); \
  c -= a; c -= b; c ^= (b>>13); \
  a -= b; a -= c; a ^= (c>>12);  \
  b -= c; b -= a; b ^= (a<<16); \
  c -= a; c -= b; c ^= (b>>5); \
  a -= b; a -= c; a ^= (c>>3);  \
  b -= c; b -= a; b ^= (a<<10); \
  c -= a; c -= b; c ^= (b>>15); \
}

/*
--------------------------------------------------------------------
lookup() -- hash a variable-length key into a 32-bit value
  k     : the key (the unaligned variable-length array of bytes)
  len   : the length of the key, counting by bytes
  level : can be any 4-byte value
Returns a 32-bit value.  Every bit of the key affects every bit of
the return value.  Every 1-bit and 2-bit delta achieves avalanche.
About 6len+35 instructions.

The best hash table sizes are powers of 2.  There is no need to do
mod a prime (mod is sooo slow!).  If you need less than 32 bits,
use a bitmask.  For example, if you need only 10 bits, do
  h = (h & hashmask(10));
In which case, the hash table should have hashsize(10) elements.

If you are hashing n strings (ub1 **)k, do it like this:
  for (i=0, h=0; i<n; ++i) h = lookup( k[i], len[i], h);

By Bob Jenkins, 1996.  bob_jenkins@burtleburtle.net.  You may use this
code any way you wish, private, educational, or commercial.

See http://burtleburtle.net/bob/hash/evahash.html
Use for hash table lookup, or anything where one collision in 2^32 is
acceptable.  Do NOT use for cryptographic purposes.
--------------------------------------------------------------------
*/

ub4 lookup( k, length, level)
register ub1 *k;        /* the key */
register ub4  length;   /* the length of the key */
register ub4  level;    /* the previous hash, or an arbitrary value */
{
   register ub4 a,b,c,len;

   /* Set up the internal state */
   len = length;
   a = b = 0x9e3779b9;  /* the golden ratio; an arbitrary value */
   c = level;           /* the previous hash value */

   /*---------------------------------------- handle most of the key */
   while (len >= 12)
   {
      a += (k[0] +((ub4)k[1]<<8) +((ub4)k[2]<<16) +((ub4)k[3]<<24));
      b += (k[4] +((ub4)k[5]<<8) +((ub4)k[6]<<16) +((ub4)k[7]<<24));
      c += (k[8] +((ub4)k[9]<<8) +((ub4)k[10]<<16)+((ub4)k[11]<<24));
      mix(a,b,c);
      k += 12; len -= 12;
   }

   /*------------------------------------- handle the last 11 bytes */
   c += length;
   switch(len)              /* all the case statements fall through */
   {
   case 11: c+=((ub4)k[10]<<24);
   case 10: c+=((ub4)k[9]<<16);
   case 9 : c+=((ub4)k[8]<<8);
      /* the first byte of c is reserved for the length */
   case 8 : b+=((ub4)k[7]<<24);
   case 7 : b+=((ub4)k[6]<<16);
   case 6 : b+=((ub4)k[5]<<8);
   case 5 : b+=k[4];
   case 4 : a+=((ub4)k[3]<<24);
   case 3 : a+=((ub4)k[2]<<16);
   case 2 : a+=((ub4)k[1]<<8);
   case 1 : a+=k[0];
     /* case 0: nothing left to add */
   }
   mix(a,b,c);
   /*-------------------------------------------- report the result */
   return c;
}


/*
--------------------------------------------------------------------
mixc -- mixc 8 4-bit values as quickly and thoroughly as possible.
Repeating mix() three times achieves avalanche.
Repeating mix() four times eliminates all funnels and all
  characteristics stronger than 2^{-11}.
--------------------------------------------------------------------
*/
#define mixc(a,b,c,d,e,f,g,h) \
{ \
   a^=b<<11; d+=a; b+=c; \
   b^=c>>2;  e+=b; c+=d; \
   c^=d<<8;  f+=c; d+=e; \
   d^=e>>16; g+=d; e+=f; \
   e^=f<<10; h+=e; f+=g; \
   f^=g>>4;  a+=f; g+=h; \
   g^=h<<8;  b+=g; h+=a; \
   h^=a>>9;  c+=h; a+=b; \
}

/*
--------------------------------------------------------------------
checksum() -- hash a variable-length key into a 256-bit value
  k     : the key (the unaligned variable-length array of bytes)
  len   : the length of the key, counting by bytes
  state : an array of CHECKSTATE 4-byte values (256 bits)
The state is the checksum.  Every bit of the key affects every bit of
the state.  There are no funnels.  About 112+6.875len instructions.

If you are hashing n strings (ub1 **)k, do it like this:
  for (i=0; i<8; ++i) state[i] = 0x9e3779b9;
  for (i=0, h=0; i<n; ++i) checksum( k[i], len[i], state);

See http://burtleburtle.net/bob/hash/evahash.html
Use to detect changes between revisions of documents, assuming nobody
is trying to cause collisions.  Do NOT use for cryptography.
--------------------------------------------------------------------
*/
void  checksum( k, len, state)
register ub1 *k;
register ub4  len;
register ub4 *state;
{
   register ub4 a,b,c,d,e,f,g,h,length;

   /* Use the length and level; add in the golden ratio. */
   length = len;
   a=state[0]; b=state[1]; c=state[2]; d=state[3];
   e=state[4]; f=state[5]; g=state[6]; h=state[7];

   /*---------------------------------------- handle most of the key */
   while (len >= 32)
   {
      a += (k[0] +(k[1]<<8) +(k[2]<<16) +(k[3]<<24));
      b += (k[4] +(k[5]<<8) +(k[6]<<16) +(k[7]<<24));
      c += (k[8] +(k[9]<<8) +(k[10]<<16)+(k[11]<<24));
      d += (k[12]+(k[13]<<8)+(k[14]<<16)+(k[15]<<24));
      e += (k[16]+(k[17]<<8)+(k[18]<<16)+(k[19]<<24));
      f += (k[20]+(k[21]<<8)+(k[22]<<16)+(k[23]<<24));
      g += (k[24]+(k[25]<<8)+(k[26]<<16)+(k[27]<<24));
      h += (k[28]+(k[29]<<8)+(k[30]<<16)+(k[31]<<24));
      mixc(a,b,c,d,e,f,g,h);
      mixc(a,b,c,d,e,f,g,h);
      mixc(a,b,c,d,e,f,g,h);
      mixc(a,b,c,d,e,f,g,h);
      k += 32; len -= 32;
   }

   /*------------------------------------- handle the last 31 bytes */
   h += length;
   switch(len)
   {
   case 31: h+=(k[30]<<24);
   case 30: h+=(k[29]<<16);
   case 29: h+=(k[28]<<8);
   case 28: g+=(k[27]<<24);
   case 27: g+=(k[26]<<16);
   case 26: g+=(k[25]<<8);
   case 25: g+=k[24];
   case 24: f+=(k[23]<<24);
   case 23: f+=(k[22]<<16);
   case 22: f+=(k[21]<<8);
   case 21: f+=k[20];
   case 20: e+=(k[19]<<24);
   case 19: e+=(k[18]<<16);
   case 18: e+=(k[17]<<8);
   case 17: e+=k[16];
   case 16: d+=(k[15]<<24);
   case 15: d+=(k[14]<<16);
   case 14: d+=(k[13]<<8);
   case 13: d+=k[12];
   case 12: c+=(k[11]<<24);
   case 11: c+=(k[10]<<16);
   case 10: c+=(k[9]<<8);
   case 9 : c+=k[8];
   case 8 : b+=(k[7]<<24);
   case 7 : b+=(k[6]<<16);
   case 6 : b+=(k[5]<<8);
   case 5 : b+=k[4];
   case 4 : a+=(k[3]<<24);
   case 3 : a+=(k[2]<<16);
   case 2 : a+=(k[1]<<8);
   case 1 : a+=k[0];
   }
   mixc(a,b,c,d,e,f,g,h);
   mixc(a,b,c,d,e,f,g,h);
   mixc(a,b,c,d,e,f,g,h);
   mixc(a,b,c,d,e,f,g,h);

   /*-------------------------------------------- report the result */
   state[0]=a; state[1]=b; state[2]=c; state[3]=d;
   state[4]=e; state[5]=f; state[6]=g; state[7]=h;
}

///////////////////////////////////////////////////////////////////////////////

/*
------------------------------------------------------------------------------
Find the mapping that will produce a perfect hash
------------------------------------------------------------------------------
*/

/* return the ceiling of the log (base 2) of val */
ub4  mylog2(val)
ub4  val;
{
  ub4 i;
  for (i=0; ((ub4)1<<i) < val; ++i)
    ;
  return i;
}

/* compute p(x), where p is a permutation of 0..(1<<nbits)-1 */
/* permute(0)=0.  This is intended and useful. */
static ub4  permute(x, nbits)
ub4 x;                                       /* input, a value in some range */
ub4 nbits;                                 /* input, number of bits in range */
{
  int i;
  int mask   = ((ub4)1<<nbits)-1;                                /* all ones */
  int const2 = 1+nbits/2;
  int const3 = 1+nbits/3;
  int const4 = 1+nbits/4;
  int const5 = 1+nbits/5;
  for (i=0; i<20; ++i)
  {
    x = (x+(x<<const2)) & mask; 
    x = (x^(x>>const3));
    x = (x+(x<<const4)) & mask;
    x = (x^(x>>const5));
  }
  return x;
}

/* initialize scramble[] with distinct random values in 0..smax-1 */
static void scrambleinit(scramble, smax)
ub4      *scramble;                            /* hash is a^scramble[tab[b]] */
ub4       smax;                    /* scramble values should be in 0..smax-1 */
{
  ub4 i;

  /* fill scramble[] with distinct random integers in 0..smax-1 */
  for (i=0; i<SCRAMBLE_LEN; ++i)
  {
    scramble[i] = permute(i, mylog2(smax));
  }
}

/* 
 * Check if key1 and key2 are the same. 
 * We already checked (a,b) are the same.
 */
static void checkdup(key1, key2, form)
key      *key1;
key      *key2;
hashform *form;
{
  switch(form->hashtype)
  {
  case STRING_HT:
    if ((key1->len_k == key2->len_k) &&
	!memcmp(key1->name_k, key2->name_k, (size_t)key1->len_k))
    {
      fprintf(stderr, "perfect.c: Duplicate keys!  %.*s\n",
	      key1->len_k, key1->name_k);
      exit(1);
    }
    break;
  case INT_HT:
    if (key1->hash_k == key2->hash_k)
    {
      fprintf(stderr, "perfect.c: Duplicate keys!  %.8x\n", key1->hash_k);
      exit(2);
    }
    break;
  case AB_HT:
    fprintf(stderr, "perfect.c: Duplicate keys!  %.8x %.8x\n",
	    key1->a_k, key1->b_k);
    exit(3);
    break;
  default:
    fprintf(stderr, "perfect.c: Illegal hash type %u\n", (ub4)form->hashtype);
    exit(4);
    break;
  }
}


/* 
 * put keys in tabb according to key->b_k
 * check if the initial hash might work 
 */
static int inittab(tabb, blen, keys, form, complete)
bstuff   *tabb;                     /* output, list of keys with b for (a,b) */
ub4       blen;                                            /* length of tabb */
key      *keys;                               /* list of keys already hashed */
hashform *form;                                           /* user directives */
int       complete;        /* TRUE means to complete init despite collisions */
{
  int  nocollision = TRUE;
  key *mykey;

  memset((void *)tabb, 0, (size_t)(sizeof(bstuff)*blen));

  /* Two keys with the same (a,b) guarantees a collision */
  for (mykey=keys; mykey; mykey=mykey->next_k)
  {
    key *otherkey;

    for (otherkey=tabb[mykey->b_k].list_b; 
	 otherkey; 
	 otherkey=otherkey->nextb_k)
    {
      if (mykey->a_k == otherkey->a_k)
      {
        nocollision = FALSE;
	checkdup(mykey, otherkey, form);
	if (!complete)
	  return FALSE;
      }
    }
    ++tabb[mykey->b_k].listlen_b;
    mykey->nextb_k = tabb[mykey->b_k].list_b;
    tabb[mykey->b_k].list_b = mykey;
  }

  /* no two keys have the same (a,b) pair */
  return nocollision;
}


/* Do the initial hash for normal mode (use lookup and checksum) */
static void initnorm(keys, alen, blen, smax, salt, final)
key      *keys;                                          /* list of all keys */
ub4       alen;                    /* (a,b) has a in 0..alen-1, a power of 2 */
ub4       blen;                    /* (a,b) has b in 0..blen-1, a power of 2 */
ub4       smax;                   /* maximum range of computable hash values */
ub4       salt;                     /* used to initialize the hash function */
gencode  *final;                          /* output, code for the final hash */
{
  key *mykey;
  if (mylog2(alen)+mylog2(blen) > UB4BITS)
  {
    ub4 initlev = salt*0x9e3779b9;  /* the golden ratio; an arbitrary value */

    for (mykey=keys; mykey; mykey=mykey->next_k)
    {
      ub4 i, state[CHECKSTATE];
      for (i=0; i<CHECKSTATE; ++i) state[i] = initlev;
      checksum( mykey->name_k, mykey->len_k, state);
      mykey->a_k = state[0]&(alen-1);
      mykey->b_k = state[1]&(blen-1);
    }
    final->used = 4;
    sprintf(final->line[0], 
	    "  uint4 i,state[CHECKSTATE],rsl;\n");
    sprintf(final->line[1], 
	    "  for (i=0; i<CHECKSTATE; ++i) state[i]=0x%x;\n",initlev);
    sprintf(final->line[2],
	    "  checksum(key, len, state);\n");
    sprintf(final->line[3], 
	    "  rsl = ((state[0]&0x%x)^s_script_keyword_scramble_table[s_script_keyword_hash_table[state[1]&0x%x]]);\n",
	    alen-1, blen-1);
  }
  else
  {
    ub4 loga = mylog2(alen);                            /* log based 2 of blen */
    ub4 initlev = salt*0x9e3779b9;  /* the golden ratio; an arbitrary value */

    for (mykey=keys; mykey; mykey=mykey->next_k)
    {
      ub4 hash = lookup(mykey->name_k, mykey->len_k, initlev);
      mykey->a_k = (loga > 0) ? hash>>(UB4BITS-loga) : 0;
      mykey->b_k = (blen > 1) ? hash&(blen-1) : 0;
    }
    final->used = 2;
    sprintf(final->line[0], 
	    "  uint4 rsl, val = lookup(key, len, 0x%x);\n", initlev);
    if (smax <= 1)
    {
      sprintf(final->line[1], "  rsl = 0;\n");
    }
    else if (blen < USE_SCRAMBLE)
    {
      sprintf(final->line[1], "  rsl = ((val>>%u)^s_script_keyword_hash_table[val&0x%x]);\n",
	      UB4BITS-mylog2(alen), blen-1);
    }
    else
    {
      sprintf(final->line[1], "  rsl = ((val>>%u)^s_script_keyword_scramble_table[s_script_keyword_hash_table[val&0x%x]]);\n",
	      UB4BITS-mylog2(alen), blen-1);
    }
  }
}



/* Do initial hash for inline mode */
static void initinl(keys, alen, blen, smax, salt, final)
key      *keys;                                          /* list of all keys */
ub4       alen;                    /* (a,b) has a in 0..alen-1, a power of 2 */
ub4       blen;                    /* (a,b) has b in 0..blen-1, a power of 2 */
ub4       smax;                           /* range of computable hash values */
ub4       salt;                     /* used to initialize the hash function */
gencode  *final;                            /* generated code for final hash */
{
  key *mykey;
  ub4  amask = alen-1;
  ub4  blog  = mylog2(blen);
  ub4  initval = salt*0x9e3779b9;    /* the golden ratio; an arbitrary value */

  /* It's more important to have b uniform than a, so b is the low bits */
  for (mykey = keys;  mykey != (key *)0;  mykey = mykey->next_k)
  {
    ub4   hash = initval;
    ub4   i;
    for (i=0; i<mykey->len_k; ++i)
    {
      hash = (mykey->name_k[i] ^ hash) + ((hash<<(UB4BITS-6))+(hash>>6));
    }
    mykey->hash_k = hash;
    mykey->a_k = (alen > 1) ? (hash & amask) : 0;
    mykey->b_k = (blen > 1) ? (hash >> (UB4BITS-blog)) : 0;
  }
  final->used = 1;
  if (smax <= 1)
  {
    sprintf(final->line[0], "  uint4 rsl = 0;\n");
  }
  else if (blen < USE_SCRAMBLE)
  {
    sprintf(final->line[0], "  uint4 rsl = ((val & 0x%x) ^ s_script_keyword_hash_table[val >> %u]);\n",
	    amask, UB4BITS-blog);
  }
  else
  {
    sprintf(final->line[0], "  uint4 rsl = ((val & 0x%x) ^ s_script_keyword_scramble_table[s_script_keyword_hash_table[val >> %u]]);\n",
	    amask, UB4BITS-blog);
  }
}


/* 
 * Run a hash function on the key to get a and b 
 * Returns:
 *   0: didn't find distinct (a,b) for all keys
 *   1: found distinct (a,b) for all keys, put keys in tabb[]
 *   2: found a perfect hash, no need to do any more work
 */
static ub4 initkey(keys, nkeys, tabb, alen, blen, smax, salt, form, final)
key      *keys;                                          /* list of all keys */
ub4       nkeys;                                     /* total number of keys */
bstuff   *tabb;                                        /* stuff indexed by b */
ub4       alen;                    /* (a,b) has a in 0..alen-1, a power of 2 */
ub4       blen;                    /* (a,b) has b in 0..blen-1, a power of 2 */
ub4       smax;                           /* range of computable hash values */
ub4       salt;                     /* used to initialize the hash function */
hashform *form;                                           /* user directives */
gencode  *final;                                      /* code for final hash */
{
  ub4 finished;

  /* Do the initial hash of the keys */
  switch(form->mode)
  {
  case NORMAL_HM:
    initnorm(keys, alen, blen, smax, salt, final);
    break;
  case INLINE_HM:
    initinl(keys, alen, blen, smax, salt, final);
    break;
  case HEX_HM:
  case DECIMAL_HM:
    finished = inithex(keys, nkeys, alen, blen, smax, salt, final, form); 
    if (finished) return 2;
    break;
  default:
    fprintf(stderr, "fatal error: illegal mode\n"); 
    exit(1);
  }

  if (nkeys <= 1)
  {
    final->used = 1;
    sprintf(final->line[0], "  uint4 rsl = 0;\n");
    return 2;
  }

  return inittab(tabb, blen, keys, form, FALSE);
}

/* Print an error message and exit if there are duplicates */
static void duplicates(tabb, blen, keys, form)
bstuff   *tabb;                    /* array of lists of keys with the same b */
ub4       blen;                              /* length of tabb, a power of 2 */
key      *keys;
hashform *form;                                           /* user directives */
{
  ub4  i;
  key *key1;
  key *key2;

  (void)inittab(tabb, blen, keys, form, TRUE);

  /* for each b, do nested loops through key list looking for duplicates */
  for (i=0; i<blen; ++i)
    for (key1=tabb[i].list_b; key1; key1=key1->nextb_k)
      for (key2=key1->nextb_k; key2; key2=key2->nextb_k)
	checkdup(key1, key2, form);
}


/* Try to apply an augmenting list */
static int apply(tabb, tabh, tabq, blen, scramble, tail, rollback)
bstuff *tabb;
hstuff *tabh;
qstuff *tabq;
ub4     blen;
ub4    *scramble;
ub4     tail;
int     rollback;          /* FALSE applies augmenting path, TRUE rolls back */
{
  ub4     hash;
  key    *mykey;
  bstuff *pb;
  ub4     child;
  ub4     parent;
  ub4     stabb;                                         /* scramble[tab[b]] */

  /* walk from child to parent */
  for (child=tail-1; child; child=parent)
  {
    parent = tabq[child].parent_q;                    /* find child's parent */
    pb     = tabq[parent].b_q;             /* find parent's list of siblings */

    /* erase old hash values */
    stabb = scramble[pb->val_b];
    for (mykey=pb->list_b; mykey; mykey=mykey->nextb_k)
    {
      hash = mykey->a_k^stabb;
      if (mykey == tabh[hash].key_h)
      {                            /* erase hash for all of child's siblings */
	tabh[hash].key_h = (key *)0;
      }
    }

    /* change pb->val_b, which will change the hashes of all parent siblings */
    pb->val_b = (rollback ? tabq[child].oldval_q : tabq[child].newval_q);

    /* set new hash values */
    stabb = scramble[pb->val_b];
    for (mykey=pb->list_b; mykey; mykey=mykey->nextb_k)
    {
      hash = mykey->a_k^stabb;
      if (rollback)
      {
	if (parent == 0) continue;                  /* root never had a hash */
      }
      else if (tabh[hash].key_h)
      {
	/* very rare: roll back any changes */
	(void *)apply(tabb, tabh, tabq, blen, scramble, tail, TRUE);
	return FALSE;                                  /* failure, collision */
      }
      tabh[hash].key_h = mykey;
    }
  }
  return TRUE;
}


/*
-------------------------------------------------------------------------------
augment(): Add item to the mapping.

Construct a spanning tree of *b*s with *item* as root, where each
parent can have all its hashes changed (by some new val_b) with 
at most one collision, and each child is the b of that collision.

I got this from Tarjan's "Data Structures and Network Algorithms".  The
path from *item* to a *b* that can be remapped with no collision is 
an "augmenting path".  Change values of tab[b] along the path so that 
the unmapped key gets mapped and the unused hash value gets used.

Assuming 1 key per b, if m out of n hash values are still unused, 
you should expect the transitive closure to cover n/m nodes before 
an unused node is found.  Sum(i=1..n)(n/i) is about nlogn, so expect
this approach to take about nlogn time to map all single-key b's.
-------------------------------------------------------------------------------
*/
static int augment(tabb, tabh, tabq, blen, scramble, smax, item, nkeys, 
		   highwater, form)
bstuff   *tabb;                                        /* stuff indexed by b */
hstuff   *tabh;  /* which key is associated with which hash, indexed by hash */
qstuff   *tabq;            /* queue of *b* values, this is the spanning tree */
ub4       blen;                                            /* length of tabb */
ub4      *scramble;                      /* final hash is a^scramble[tab[b]] */
ub4       smax;                                 /* highest value in scramble */
bstuff   *item;                           /* &tabb[b] for the b to be mapped */
ub4       nkeys;                         /* final hash must be in 0..nkeys-1 */
ub4       highwater;        /* a value higher than any now in tabb[].water_b */
hashform *form;               /* TRUE if we should do a minimal perfect hash */
{
  ub4  q;                      /* current position walking through the queue */
  ub4  tail;              /* tail of the queue.  0 is the head of the queue. */
  ub4  limit=((blen < USE_SCRAMBLE) ? smax : UB1MAXVAL+1);
  ub4  highhash = ((form->perfect == MINIMAL_HP) ? nkeys : smax);
  int  trans = (form->speed == SLOW_HS || form->perfect == MINIMAL_HP);

  /* initialize the root of the spanning tree */
  tabq[0].b_q = item;
  tail = 1;

  /* construct the spanning tree by walking the queue, add children to tail */
  for (q=0; q<tail; ++q)
  {
    bstuff *myb = tabq[q].b_q;                        /* the b for this node */
    ub4     i;                              /* possible value for myb->val_b */

    if (!trans && (q == 1)) 
      break;                                  /* don't do transitive closure */

    for (i=0; i<limit; ++i)
    {
      bstuff *childb = (bstuff *)0;             /* the b that this i maps to */
      key    *mykey;                       /* for walking through myb's keys */

      for (mykey = myb->list_b; mykey; mykey=mykey->nextb_k)
      {
	key    *childkey;
	ub4 hash = mykey->a_k^scramble[i];

	if (hash >= highhash) break;                        /* out of bounds */
	childkey = tabh[hash].key_h;

	if (childkey)
	{
	  bstuff *hitb = &tabb[childkey->b_k];

	  if (childb)
	  {
	    if (childb != hitb) break;            /* hit at most one child b */
	  }
	  else
	  {
	    childb = hitb;                        /* remember this as childb */
	    if (childb->water_b == highwater) break;     /* already explored */
	  }
	}
      }
      if (mykey) continue;             /* myb with i has multiple collisions */

      /* add childb to the queue of reachable things */
      if (childb) childb->water_b = highwater;
      tabq[tail].b_q      = childb;
      tabq[tail].newval_q = i;     /* how to make parent (myb) use this hash */
      tabq[tail].oldval_q = myb->val_b;            /* need this for rollback */
      tabq[tail].parent_q = q;
      ++tail;

      if (!childb)
      {                                  /* found an *i* with no collisions? */
	/* try to apply the augmenting path */
	if (apply(tabb, tabh, tabq, blen, scramble, tail, FALSE))
	  return TRUE;        /* success, item was added to the perfect hash */

	--tail;                    /* don't know how to handle such a child! */
      }
    }
  }
  return FALSE;
}


/* find a mapping that makes this a perfect hash */
static int perfect(tabb, tabh, tabq, blen, smax, scramble, nkeys, form)
bstuff   *tabb;
hstuff   *tabh;
qstuff   *tabq;
ub4       blen;
ub4       smax;
ub4      *scramble;
ub4       nkeys;
hashform *form;
{
  ub4 maxkeys;                           /* maximum number of keys for any b */
  ub4 i, j;

  /* clear any state from previous attempts */
  memset((void *)tabh, 0, 
	 (size_t)(sizeof(hstuff)*
		  ((form->perfect == MINIMAL_HP) ? nkeys : smax)));
  memset((void *)tabq, 0, (size_t)(sizeof(qstuff)*(blen+1)));

  for (maxkeys=0,i=0; i<blen; ++i) 
    if (tabb[i].listlen_b > maxkeys) 
      maxkeys = tabb[i].listlen_b;

  /* In descending order by number of keys, map all *b*s */
  for (j=maxkeys; j>0; --j)
    for (i=0; i<blen; ++i)
      if (tabb[i].listlen_b == j)
	if (!augment(tabb, tabh, tabq, blen, scramble, smax, &tabb[i], nkeys, 
		     i+1, form))
	{
	  printf("fail to map group of size %u for tab size %u\n", j, blen);
	  return FALSE;
	}

  /* Success!  We found a perfect hash of all keys into 0..nkeys-1. */
  return TRUE;
}


/*
 * Simple case: user gave (a,b).  No more mixing, no guessing alen or blen. 
 * This assumes a,b reside in (key->a_k, key->b_k), and final->form == AB_HK.
 */
static void hash_ab(tabb, alen, blen, salt, final, 
	     scramble, smax, keys, nkeys, form)
bstuff  **tabb;           /* output, tab[] of the perfect hash, length *blen */
ub4      *alen;                 /* output, 0..alen-1 is range for a of (a,b) */
ub4      *blen;                 /* output, 0..blen-1 is range for b of (a,b) */
ub4      *salt;                         /* output, initializes initial hash */
gencode  *final;                                      /* code for final hash */
ub4      *scramble;                      /* input, hash = a^scramble[tab[b]] */
ub4      *smax;                           /* input, scramble[i] in 0..smax-1 */
key      *keys;                                       /* input, keys to hash */
ub4       nkeys;                       /* input, number of keys being hashed */
hashform *form;                                           /* user directives */
{
  hstuff *tabh;
  qstuff *tabq;
  key    *mykey;
  ub4     i;
  int     used_tab;

  /* initially make smax the first power of two bigger than nkeys */
  *smax = ((ub4)1<<mylog2(nkeys));
  scrambleinit(scramble, *smax);

  /* set *alen and *blen based on max A and B from user */
  *alen = 1;
  *blen = 1;
  for (mykey = keys;  mykey != (key *)0;  mykey = mykey->next_k)
  {
    while (*alen <= mykey->a_k) *alen *= 2;
    while (*blen <= mykey->b_k) *blen *= 2;
  }
  if (*alen > 2**smax)
  {
    fprintf(stderr,
      "perfect.c: Can't deal with (A,B) having A bigger than twice \n");
    fprintf(stderr,
      "  the smallest power of two greater or equal to any legal hash.\n");
    exit(1);
  }

  /* allocate working memory */
  *tabb = (bstuff *)malloc((size_t)(sizeof(bstuff)*(*blen))); 
  tabq  = (qstuff *)remalloc(sizeof(qstuff)*(*blen+1), "perfect.c, tabq");
  tabh  = (hstuff *)remalloc(sizeof(hstuff)*(form->perfect == MINIMAL_HP ? 
					     nkeys : *smax),
			     "perfect.c, tabh");

  /* check that (a,b) are distinct and put them in tabb indexed by b */
  (void)inittab(*tabb, *blen, keys, form, FALSE);

  /* try with smax */
  if (!perfect(*tabb, tabh, tabq, *blen, *smax, scramble, nkeys, form))
  {
    if (form->perfect == MINIMAL_HP)
    {
      printf("fatal error: Cannot find perfect hash for user (A,B) pairs\n");
      exit(1);
    }
    else
    {
      /* try with 2*smax */
      free((void *)tabh);
      *smax = *smax * 2;
      scrambleinit(scramble, *smax);
      tabh = (hstuff *)remalloc(sizeof(hstuff)*(form->perfect == MINIMAL_HP ?
						nkeys : *smax),
				"perfect.c, tabh");
      if (!perfect(*tabb, tabh, tabq, *blen, *smax, scramble, nkeys, form))
      {
	printf("fatal error: Cannot find perfect hash for user (A,B) pairs\n");
	exit(1);
      }
    }
  }

  /* check if tab[] was really needed */
  for (i=0; i<*blen; ++i)
  {
    if ((*tabb)[i].val_b != 0) break;            /* assumes permute(0) == 0 */
  }
  used_tab = (i < *blen);

  /* write the code for the perfect hash */
  *salt = 1;
  final->used = 1;
  if (!used_tab)
  {
    sprintf(final->line[0], "  ub4 rsl = a;\n");
  }
  else if (*blen < USE_SCRAMBLE)
  {
    sprintf(final->line[0], "  ub4 rsl = (a ^ tab[b]);\n");
  }
  else
  {
    sprintf(final->line[0], "  ub4 rsl = (a ^ scramble[tab[b]]);\n");
  }

  printf("success, found a perfect hash\n");

  free((void *)tabq);
  free((void *)tabh);
}


/* guess initial values for alen and blen */
static void initalen(alen, blen, smax, nkeys, form)
ub4      *alen;                                      /* output, initial alen */
ub4      *blen;                                      /* output, initial blen */
ub4      *smax;    /* input, power of two greater or equal to max hash value */
ub4       nkeys;                              /* number of keys being hashed */
hashform *form;                                           /* user directives */
{
  /*
   * Find initial *alen, *blen
   * Initial alen and blen values were found empirically.  Some factors:
   *
   * If smax<256 there is no scramble, so tab[b] needs to cover 0..smax-1.
   *
   * alen and blen must be powers of 2 because the values in 0..alen-1 and
   * 0..blen-1 are produced by applying a bitmask to the initial hash function.
   *
   * alen must be less than smax, in fact less than nkeys, because otherwise
   * there would often be no i such that a^scramble[i] is in 0..nkeys-1 for
   * all the *a*s associated with a given *b*, so there would be no legal
   * value to assign to tab[b].  This only matters when we're doing a minimal
   * perfect hash.
   *
   * It takes around 800 trials to find distinct (a,b) with nkey=smax*(5/8)
   * and alen*blen = smax*smax/32.
   *
   * Values of blen less than smax/4 never work, and smax/2 always works.
   *
   * We want blen as small as possible because it is the number of bytes in
   * the huge array we must create for the perfect hash.
   *
   * When nkey <= smax*(5/8), blen=smax/4 works much more often with 
   * alen=smax/8 than with alen=smax/4.  Above smax*(5/8), blen=smax/4
   * doesn't seem to care whether alen=smax/8 or alen=smax/4.  I think it
   * has something to do with 5/8 = 1/8 * 5.  For example examine 80000, 
   * 85000, and 90000 keys with different values of alen.  This only matters
   * if we're doing a minimal perfect hash.
   *
   * When alen*blen <= 1<<UB4BITS, the initial hash must produce one integer.
   * Bigger than that it must produce two integers, which increases the
   * cost of the hash per character hashed.
   */
  if (form->perfect == NORMAL_HP)
  {
    if ((form->speed == FAST_HS) && (nkeys > *smax*0.8))
    {
      *smax = *smax * 2;
    }

    *alen = ((form->hashtype==INT_HT) && *smax>131072) ? 
      ((ub4)1<<(UB4BITS-mylog2(*blen))) :   /* distinct keys => distinct (A,B) */
      *smax;                         /* no reason to restrict alen to smax/2 */
    if ((form->hashtype == INT_HT) && *smax < 32)
      *blen = *smax;                      /* go for function speed not space */
    else if (*smax/4 <= (1<<14))
      *blen = ((nkeys <= *smax*0.56) ? *smax/32 :
	       (nkeys <= *smax*0.74) ? *smax/16 : *smax/8);
    else
      *blen = ((nkeys <= *smax*0.6) ? *smax/16 : 
	       (nkeys <= *smax*0.8) ? *smax/8 : *smax/4);

    if ((form->speed == FAST_HS) && (*blen < *smax/8))
      *blen = *smax/8;

    if (*alen < 1) *alen = 1;
    if (*blen < 1) *blen = 1;
  }
  else
  {
    switch(mylog2(*smax))
    {
    case 0:
      *alen = 1;
      *blen = 1;
    case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8:
      *alen = (form->perfect == NORMAL_HP) ? *smax : *smax/2;
      *blen = *smax/2;
      break;
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
      if (form->speed == FAST_HS)
      {
	*alen = *smax/2;
	*blen = *smax/4;
      }
      else if (*smax/4 < USE_SCRAMBLE)
      {
	*alen = ((nkeys <= *smax*0.52) ? *smax/8 : *smax/4);
	*blen = ((nkeys <= *smax*0.52) ? *smax/8 : *smax/4);
      }
      else
      {
	*alen = ((nkeys <= *smax*(5.0/8.0)) ? *smax/8 : 
		 (nkeys <= *smax*(3.0/4.0)) ? *smax/4 : *smax/2);
	*blen = *smax/4;                /* always give the small size a shot */
      }
      break;
    case 18:
      if (form->speed == FAST_HS)
      {
	*alen = *smax/2;
	*blen = *smax/2;
      }
      else
      {
	*alen = *smax/8;                 /* never require the multiword hash */
	*blen = (nkeys <= *smax*(5.0/8.0)) ? *smax/4 : *smax/2;
      }
      break;
    case 19:
    case 20:
      *alen = (nkeys <= *smax*(5.0/8.0)) ? *smax/8 : *smax/2;
      *blen = (nkeys <= *smax*(5.0/8.0)) ? *smax/4 : *smax/2;
      break;
    default:
      *alen = *smax/2;              /* just find a hash as quick as possible */
      *blen = *smax/2;     /* we'll be thrashing virtual memory at this size */
      break;
    }
  }
}

/* 
** Try to find a perfect hash function.  
** Return the successful initializer for the initial hash. 
** Return 0 if no perfect hash could be found.
*/
void findhash(tabb, alen, blen, salt, final, 
	      scramble, smax, keys, nkeys, form)
bstuff  **tabb;           /* output, tab[] of the perfect hash, length *blen */
ub4      *alen;                 /* output, 0..alen-1 is range for a of (a,b) */
ub4      *blen;                 /* output, 0..blen-1 is range for b of (a,b) */
ub4      *salt;                         /* output, initializes initial hash */
gencode  *final;                                      /* code for final hash */
ub4      *scramble;                      /* input, hash = a^scramble[tab[b]] */
ub4      *smax;                           /* input, scramble[i] in 0..smax-1 */
key      *keys;                                       /* input, keys to hash */
ub4       nkeys;                       /* input, number of keys being hashed */
hashform *form;                                           /* user directives */
{
	ub4 i;
  ub4 bad_initkey;                       /* how many times did initkey fail? */
  ub4 bad_perfect;                       /* how many times did perfect fail? */
  ub4 trysalt;                        /* trial initializer for initial hash */
  ub4 maxalen;
  hstuff *tabh;                       /* table of keys indexed by hash value */
  qstuff *tabq;    /* table of stuff indexed by queue value, used by augment */

  /* The case of (A,B) supplied by the user is a special case */
  if (form->hashtype == AB_HT)
  {
    hash_ab(tabb, alen, blen, salt, final, 
	    scramble, smax, keys, nkeys, form);
    return;
  }

  /* guess initial values for smax, alen and blen */
  *smax = ((ub4)1<<mylog2(nkeys));
  initalen(alen, blen, smax, nkeys, form);

  scrambleinit(scramble, *smax);

  maxalen = (form->perfect == MINIMAL_HP) ? *smax/2 : *smax;

  /* allocate working memory */
  *tabb = (bstuff *)remalloc((size_t)(sizeof(bstuff)*(*blen)), 
			     "perfect.c, tabb");
  tabq  = (qstuff *)remalloc(sizeof(qstuff)*(*blen+1), "perfect.c, tabq");
  tabh  = (hstuff *)remalloc(sizeof(hstuff)*(form->perfect == MINIMAL_HP ? 
					     nkeys : *smax),
			     "perfect.c, tabh");

  /* Actually find the perfect hash */
  *salt = 0;
  bad_initkey = 0;
  bad_perfect = 0;
  for (trysalt=1; ; ++trysalt)
  {
    ub4 rslinit;
    /* Try to find distinct (A,B) for all keys */
    
    rslinit = initkey(keys, nkeys, *tabb, *alen, *blen, *smax, trysalt,
		      form, final);

    if (rslinit == 2)
    {      /* initkey actually found a perfect hash, not just distinct (a,b) */
      *salt = 1;
      *blen = 0;
      break;
    }
    else if (rslinit == 0)
    {
      /* didn't find distinct (a,b) */
      if (++bad_initkey >= RETRY_INITKEY)
      {
	/* Try to put more bits in (A,B) to make distinct (A,B) more likely */
	if (*alen < maxalen)
	{
	  *alen *= 2;
	} 
	else if (*blen < *smax)
	{
	  *blen *= 2;
	  free(tabq);
	  free(*tabb);
	  *tabb  = (bstuff *)malloc((size_t)(sizeof(bstuff)*(*blen)));
	  tabq  = (qstuff *)malloc((size_t)(sizeof(qstuff)*(*blen+1)));
	}
	else
	{
	  duplicates(*tabb, *blen, keys, form);      /* check for duplicates */
	  printf("fatal error: Cannot perfect hash: cannot find distinct (A,B)\n");
	  exit(-1);
	}
	bad_initkey = 0;
	bad_perfect = 0;
      }
      continue;                             /* two keys have same (a,b) pair */
    }

    //printf("found distinct (A,B) on attempt %ld\n", trysalt);

    /* Given distinct (A,B) for all keys, build a perfect hash */
    if (!perfect(*tabb, tabh, tabq, *blen, *smax, scramble, nkeys, form))
    {
      if ((form->hashtype != INT_HT && ++bad_perfect >= RETRY_PERFECT) || 
	  (form->hashtype == INT_HT && ++bad_perfect >= RETRY_HEX))
      {
	if (*blen < *smax)
	{
	  *blen *= 2;
	  free(*tabb);
	  free(tabq);
	  *tabb  = (bstuff *)malloc((size_t)(sizeof(bstuff)*(*blen)));
	  tabq  = (qstuff *)malloc((size_t)(sizeof(qstuff)*(*blen+1)));
	  --trysalt;               /* we know this salt got distinct (A,B) */
	}
	else
	{
	  printf("fatal error: Cannot perfect hash: cannot build tab[]\n");
	  exit(-1);
	}
	bad_perfect = 0;
      }
      continue;
    }
    
    *salt = trysalt;
    break;
  }

  //printf("built perfect hash table of size %ld\n", *blen);

	for(i = 0; i < nkeys; ++i)
		tabh[i] . key_h -> final_hash_k = i;

  /* free working memory */
  free((void *)tabh);
  free((void *)tabq);
}

/*
------------------------------------------------------------------------------
Input/output type routines
------------------------------------------------------------------------------
*/

/* get the list of keys */
static void getkeys(keys, nkeys, textroot, keyroot, form)
key      **keys;                                         /* list of all keys */
ub4       *nkeys;                                          /* number of keys */
reroot    *textroot;                          /* get space to store key text */
reroot    *keyroot;                                    /* get space for keys */
hashform  *form;                                          /* user directives */
{
  key  *mykey;
  char *mytext;
  mytext = (char *)renew(textroot);
  *keys = 0;
  *nkeys = 0;
  while (fgets(mytext, MAXKEYLEN, stdin))
  {
    mykey = (key *)renew(keyroot);
    if (form->mode == AB_HM)
    {
      sscanf(mytext, "%x %x ", &mykey->a_k, &mykey->b_k);
    }
    else if (form->mode == ABDEC_HM)
    {
      sscanf(mytext, "%u %u ", &mykey->a_k, &mykey->b_k);
    }
    else if (form->mode == HEX_HM)
    {
      sscanf(mytext, "%x ", &mykey->hash_k);
    }
    else if (form->mode == DECIMAL_HM)
    {
      sscanf(mytext, "%u ", &mykey->hash_k);
    }
    else
    {
      mykey->name_k = (ub1 *)mytext;
      mytext = (char *)renew(textroot);
      mykey->len_k  = (ub4)(strlen((char *)mykey->name_k)-1);
    }
    mykey->next_k = *keys;
    *keys = mykey;
    ++*nkeys;
  }
  redel(textroot, mytext);
}

/* make the .h file */
static void make_h(blen, smax, nkeys, salt, keys)
ub4  blen;
ub4  smax;
ub4  nkeys;
ub4  salt;
key *keys;
{
 // FILE *f;
 // f = fopen("phash.h", "w");
 // fprintf(f, "/* Perfect hash definitions */\n");
 // fprintf(f, "#ifndef STANDARD\n");
 // fprintf(f, "#include \"standard.h\"\n");
 // fprintf(f, "#endif /* STANDARD */\n");
 // fprintf(f, "#ifndef PHASH\n");
 // fprintf(f, "#define PHASH\n");
 // fprintf(f, "\n");
 // if (blen > 0)
 // {
 //   if (smax <= UB1MAXVAL+1 || blen >= USE_SCRAMBLE)
 //     fprintf(f, "extern ub1 tab[];\n");
 //   else
 //   {
 //     fprintf(f, "extern ub2 tab[];\n");
 //     if (blen >= USE_SCRAMBLE)
 //     {
	//if (smax <= UB2MAXVAL+1)
	//  fprintf(f, "extern ub2 scramble[];\n");
	//else
	//  fprintf(f, "extern ub4 scramble[];\n");
 //     }
 //   }
 //   fprintf(f, "#define PHASHLEN 0x%lx  /* length of hash mapping table */\n",
	//    blen);
 // }
 // fprintf(f, "#define PHASHNKEYS %ld  /* How many keys were hashed */\n",
 //         nkeys);
 // fprintf(f, "#define PHASHRANGE %ld  /* Range any input might map to */\n",
 //         smax);
 // fprintf(f, "#define PHASHSALT 0x%.8lx /* internal, initialize normal hash */\n",
 //         salt*0x9e3779b9);
 // fprintf(f, "\n");
 // fprintf(f, "ub4 phash();\n");
 // fprintf(f, "\n");
 // fprintf(f, "#endif  /* PHASH */\n");
 // fprintf(f, "\n");
 // fclose(f);

	unsigned int len;
	key *k;

	printf("#define SCRIPT_KEYWORD_SALT 0x%.8x\n", salt*0x9e3779b9);
	printf("#define SCRIPT_KEYWORD_COUNT %u\n", nkeys);

	len = 0;
	for(k = keys; k != NULL; k = k -> next_k)
		if (k -> len_k > len)
			len = k -> len_k;

	printf("#define SCRIPT_KEYWORD_LARGEST %u\n\n", len);
}

/* make the .c file */
static void make_c(tab, smax, blen, scramble, final, form, keys, nkeys)
bstuff   *tab;                                         /* table indexed by b */
ub4       smax;                                       /* range of scramble[] */
ub4       blen;                                /* b in 0..blen-1, power of 2 */
ub4      *scramble;                                    /* used in final hash */
gencode  *final;                                  /* code for the final hash */
hashform *form;                                           /* user directives */
key *keys;
ub4 nkeys;
{
	key *k, **map;
  ub4   i;
  if (blen >= USE_SCRAMBLE)
  {
    if (smax > UB2MAXVAL+1)
    {
      printf("uint4 s_script_keyword_scramble_table[] = {\n");
      for (i=0; i<=UB1MAXVAL; i+=4)
        printf("0x%.8x, 0x%.8x, 0x%.8x, 0x%.8x,\n",
                scramble[i+0], scramble[i+1], scramble[i+2], scramble[i+3]);
    }
    else
    {
      printf("uint2 s_script_keyword_scramble_table[] = {\n");
      for (i=0; i<=UB1MAXVAL; i+=8)
        printf("0x%.4x, 0x%.4x, 0x%.4x, 0x%.4x, 0x%.4x, 0x%.4x, 0x%.4x, 0x%.4x,\n",
                scramble[i+0], scramble[i+1], scramble[i+2], scramble[i+3],
                scramble[i+4], scramble[i+5], scramble[i+6], scramble[i+7]);
    }
    printf("};\n");
    printf("\n");
  }
  if (blen > 0)
  {
    if (smax <= UB1MAXVAL+1 || blen >= USE_SCRAMBLE)
      printf("uint1 s_script_keyword_hash_table[] = {\n");
    else
      printf("uint2 s_script_keyword_hash_table[] = {\n");

    if (blen < 16)
    {
      for (i=0; i<blen; ++i) printf("%3d,", scramble[tab[i].val_b]);
    }
    else if (blen <= 1024)
    {
      for (i=0; i<blen; i+=16)
	printf("%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,\n",
		scramble[tab[i+0].val_b], scramble[tab[i+1].val_b], 
		scramble[tab[i+2].val_b], scramble[tab[i+3].val_b], 
		scramble[tab[i+4].val_b], scramble[tab[i+5].val_b], 
		scramble[tab[i+6].val_b], scramble[tab[i+7].val_b], 
		scramble[tab[i+8].val_b], scramble[tab[i+9].val_b], 
		scramble[tab[i+10].val_b], scramble[tab[i+11].val_b], 
		scramble[tab[i+12].val_b], scramble[tab[i+13].val_b], 
		scramble[tab[i+14].val_b], scramble[tab[i+15].val_b]); 
    }
    else if (blen < USE_SCRAMBLE)
    {
      for (i=0; i<blen; i+=8)
	printf("%u,%u,%u,%u,%u,%u,%u,%u,\n",
		scramble[tab[i+0].val_b], scramble[tab[i+1].val_b], 
		scramble[tab[i+2].val_b], scramble[tab[i+3].val_b], 
		scramble[tab[i+4].val_b], scramble[tab[i+5].val_b], 
		scramble[tab[i+6].val_b], scramble[tab[i+7].val_b]); 
    }
    else 
    {
      for (i=0; i<blen; i+=16)
	printf("%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,\n",
		tab[i+0].val_b, tab[i+1].val_b, 
		tab[i+2].val_b, tab[i+3].val_b, 
		tab[i+4].val_b, tab[i+5].val_b, 
		tab[i+6].val_b, tab[i+7].val_b, 
		tab[i+8].val_b, tab[i+9].val_b, 
		tab[i+10].val_b, tab[i+11].val_b, 
		tab[i+12].val_b, tab[i+13].val_b, 
		tab[i+14].val_b, tab[i+15].val_b); 
    }
    printf("};\n");
    printf("\n");
  }

  printf("inline uint4 script_keyword_hash(uint4 val)\n{\n");
  for (i=0; i<final->used; ++i)
    printf("%s", final->line[i]);
  printf("  return rsl;\n");
  printf("}\n");
  printf("\n");

  map = (key **)malloc(sizeof(key *) * nkeys);
  printf("static const char *s_script_keywords[] =\n{\n");
  for(k = keys; k != NULL; k = k -> next_k)
	  map[k -> final_hash_k] = k;

  for(i = 0; i < nkeys; ++i)
	  printf("  \"%.*s\",\n", map[i] -> len_k, map[i] -> name_k);

  printf("};\n");

  free(map);
}

/*
------------------------------------------------------------------------------
Read in the keys, find the hash, and write the .c and .h files
------------------------------------------------------------------------------
*/
static void driver(form)
hashform *form;                                           /* user directives */
{
  ub4       nkeys;                                         /* number of keys */
  key      *keys;                                    /* head of list of keys */
  bstuff   *tab;                                       /* table indexed by b */
  ub4       smax;            /* scramble[] values in 0..smax-1, a power of 2 */
  ub4       alen;                            /* a in 0..alen-1, a power of 2 */
  ub4       blen;                            /* b in 0..blen-1, a power of 2 */
  ub4       salt;                       /* a parameter to the hash function */
  reroot   *textroot;                      /* MAXKEYLEN-character text lines */
  reroot   *keyroot;                                       /* source of keys */
  gencode   final;                                    /* code for final hash */
  ub4       i;
  ub4       scramble[SCRAMBLE_LEN];           /* used in final hash function */
  char      buf[10][80];                        /* buffer for generated code */
  char     *buf2[10];                             /* also for generated code */

  /* set up memory sources */
  textroot = remkroot((size_t)MAXKEYLEN);
  keyroot  = remkroot(sizeof(key));

  /* set up code for final hash */
  final.line = buf2;
  final.used = 0;
  final.len  = 10;
  for (i=0; i<10; ++i) final.line[i] = buf[i];

  /* read in the list of keywords */
  getkeys(&keys, &nkeys, textroot, keyroot, form);
  //printf("Read in %ld keys\n",nkeys);

  /* find the hash */
  findhash(&tab, &alen, &blen, &salt, &final, 
	   scramble, &smax, keys, nkeys, form);

  /* generate the phash.h file */
  make_h(blen, smax, nkeys, salt, keys);
  //printf("Wrote phash.h\n");

  /* generate the phash.c file */
  make_c(tab, smax, blen, scramble, &final, form, keys, nkeys);
  //printf("Wrote phash.c\n");

  /* clean up memory sources */
  refree(textroot);
  refree(keyroot);
  free((void *)tab);
  //printf("Cleaned up\n");
}


/* Describe how to use this utility */
static void usage_error()
{
  printf("Usage: perfect [-{NnIiHhDdAaBb}{MmPp}{FfSs}] < key.txt \n");
  printf("The input is a list of keys, one key per line.\n");
  printf("Only one of NnIiHhDdAa and one of MmPp may be specified.\n");
  printf("  N,n: normal mode, key is any string string (default).\n");
  printf("  I,i: initial hash for ASCII char strings.\n");
  printf("The initial hash must be\n");
  printf("  hash = PHASHSALT;\n");
  printf("  for (i=0; i<keylength; ++i) {\n");
  printf("    hash = (hash ^ key[i]) + ((hash<<26)+(hash>>6));\n");
  printf("  }\n");
  printf("Note that this can be inlined in any user loop that walks\n");
  printf("through the key anyways, eliminating the loop overhead.\n");
  printf("  H,h: Keys are 4-byte integers in hex in this format:\n");
  printf("ffffffff\n");
  printf("This is good for optimizing switch statement compilation.\n");
  printf("  D,d: Same as H,h, except in decimal not hexidecimal\n");
  printf("  A,a: An (A,B) pair is supplied in hex in this format:\n");
  printf("aaa bbb\n");
  printf("  B,b: Same as A,a, except in decimal not hexidecimal\n");
  printf("This mode does nothing but find the values of tab[].\n");
  printf("*A* must be less than the total number of keys.\n");
  printf("  M,m: Minimal perfect hash.  Hash will be in 0..nkeys-1 (default)\n");
  printf("  P,p: Perfect hash.  Hash will be in 0..n-1, where n >= nkeys\n");
  printf("and n is a power of 2.  Will probably use a smaller tab[].");
  printf("  F,f: Fast mode.  Generate the perfect hash fast.\n");
  printf("  S,s: Slow mode.  Spend time finding a good perfect hash.\n");

  exit(SUCCESS);
}


/* Interpret arguments and call the driver */
/* See usage_error for the expected arguments */
int main(argc, argv)
int    argc;
char **argv;
{
  int      mode_given = FALSE;
  int      minimal_given = FALSE;
  int      speed_given = FALSE;
  hashform form;
  char    *c;

  /* default behavior */
  form.mode = INLINE_HM;
  form.hashtype = STRING_HT;
  form.perfect = MINIMAL_HP;
  form.speed = SLOW_HS;

  /* let the user override the default behavior */
  /*switch (argc)
  {
  case 1:
    break;
  case 2:
    if (argv[1][0] != '-')
    {
      usage_error();
      break;
    }
    for (c = &argv[1][1]; *c != '\0'; ++c) switch(*c)
    {
    case 'n': case 'N':
    case 'i': case 'I':
    case 'h': case 'H':
    case 'd': case 'D':
    case 'a': case 'A':
    case 'b': case 'B':
      if (mode_given == TRUE) 
	usage_error();
      switch(*c)
      {
      case 'n': case 'N':
	form.mode = NORMAL_HM;  form.hashtype = STRING_HT; break;
      case 'i': case 'I':
	form.mode = INLINE_HM;  form.hashtype = STRING_HT; break;
      case 'h': case 'H':
	form.mode = HEX_HM;     form.hashtype = INT_HT; break;
      case 'd': case 'D':
	form.mode = DECIMAL_HM; form.hashtype = INT_HT; break;
      case 'a': case 'A':
	form.mode = AB_HM;      form.hashtype = AB_HT; break;
      case 'b': case 'B':
	form.mode = ABDEC_HM;   form.hashtype = AB_HT; break;
      }
      mode_given = TRUE;
      break;
    case 'm': case 'M':
    case 'p': case 'P':
      if (minimal_given == TRUE)
	usage_error();
      switch(*c)
      {
      case 'p': case 'P':
	form.perfect = NORMAL_HP; break;
      case 'm': case 'M':
	form.perfect = MINIMAL_HP; break;
      }
      minimal_given = TRUE;
      break;
    case 'f': case 'F':
    case 's': case 'S':
      if (speed_given == TRUE)
	usage_error();
      switch(*c)
      {
      case 'f': case 'F':
	form.speed = FAST_HS; break;
      case 's': case 'S':
	form.speed = SLOW_HS; break;
      }
      speed_given = TRUE;
      break;
    default:
      usage_error();
    }
    break;
  default:
    usage_error();
  }*/

  /* Generate the [minimal] perfect hash */
  driver(&form);

  return SUCCESS;
}

/* 
 * Find a perfect hash when there is only one key.  Zero instructions.
 * Hint: the one key always hashes to 0
 */
static void hexone(keys, final)
key     *keys;
gencode *final;
{
  /* 1 key: the hash is always 0 */
  keys->a_k = 0;
  keys->b_k = 0;
  final->used = 1;
  sprintf(final->line[0], "  ub4 rsl = 0;\n");                    /* h1a: 37 */
}



/*
 * Find a perfect hash when there are only two keys.  Max 2 instructions.
 * There exists a bit that is different for the two keys.  Test it.
 * Note that a perfect hash of 2 keys is automatically minimal.
 */
static void hextwo(keys, final)
key     *keys;
gencode *final;
{
  ub4 a = keys->hash_k;
  ub4 b = keys->next_k->hash_k;
  ub4 i;
  
  if (a == b)
  {
    printf("fatal error: duplicate keys\n");
    exit(1);
  }

  final->used = 1;
  
  /* one instruction */
  if ((a&1) != (b&1))
  {
    sprintf(final->line[0], "  ub4 rsl = (val & 1);\n");         /* h2a: 3,4 */
    return;
  }

  /* two instructions */
  for (i=0; i<UB4BITS; ++i)
  {
    if ((a&((ub4)1<<i)) != (b&((ub4)1<<i))) break;
  }
  /* h2b: 4,6 */
  sprintf(final->line[0], "  ub4 rsl = ((val << %u) & 1);\n", i);
}



/*
 * find the value to xor to a and b and c to make none of them 3 
 * assert, (a,b,c) are three distinct values in (0,1,2,3).
 */
static ub4 find_adder(a,b,c)
ub4 a;
ub4 b;
ub4 c;
{
  return (a^b^c^3);
}



/*
 * Find a perfect hash when there are only three keys.  Max 6 instructions.
 *
 * keys a,b,c.  
 * There exists bit i such that a[i] != b[i].
 * Either c[i] != a[i] or c[i] != b[i], assume c[i] != a[i].
 * There exists bit j such that b[j] != c[j].  Note i != j.
 * Final hash should be no longer than val[i]^val[j].
 *
 * A minimal perfect hash needs to xor one of 0,1,2,3 afterwards to cause
 * the hole to land on 3.  find_adder() finds that constant
 */
static void hexthree(keys, final, form)
key      *keys;
gencode  *final;
hashform *form;
{
  ub4 a = keys->hash_k;
  ub4 b = keys->next_k->hash_k;
  ub4 c = keys->next_k->next_k->hash_k;
  ub4 i,j,x,y,z;
  
  final->used = 1;

  if (a == b || a == c || b == c)
  {
    printf("fatal error: duplicate keys\n");
    exit(1);
  }
  
  /* one instruction */
  x = a&3; 
  y = b&3;
  z = c&3;
  if (x != y && x != z && y != z)
  {
    if (form->perfect == NORMAL_HP || (x != 3 && y != 3 && z != 3))
    {
      /* h3a: 0,1,2 */
      sprintf(final->line[0], "  ub4 rsl = (val & 3);\n");
    }
    else
    {
      /* h3b: 0,3,2 */
      sprintf(final->line[0], "  ub4 rsl = ((val & 3) ^ %d);\n",
	      find_adder(x,y,z));
    }
    return;
  }

  x = a>>(UB4BITS-2); 
  y = b>>(UB4BITS-2); 
  z = c>>(UB4BITS-2); 
  if (x != y && x != z && y != z)
  {
    if (form->perfect == NORMAL_HP || (x != 3 && y != 3 && z != 3)) 
    {
      /* h3c: 3fffffff, 7fffffff, bfffffff */
      sprintf(final->line[0], "  ub4 rsl = (val >> %u);\n", (ub4)(UB4BITS-2));
    }
    else
    {
      /* h3d: 7fffffff, bfffffff, ffffffff */
      sprintf(final->line[0], "  ub4 rsl = ((val >> %u) ^ %u);\n",
	      (ub4)(UB4BITS-2), find_adder(x,y,z));
    }
    return;
  }

  /* two instructions */
  for (i=0; i<final->highbit; ++i)
  {
    x = (a>>i)&3;
    y = (b>>i)&3;
    z = (c>>i)&3;
    if (x != y && x != z && y != z)
    {
      if (form->perfect == NORMAL_HP || (x != 3 && y != 3 && z != 3))
      {
	/* h3e: ffff3fff, ffff7fff, ffffbfff */
	sprintf(final->line[0], "  ub4 rsl = ((val >> %u) & 3);\n", i);
      }
      else
      {
	/* h3f: ffff7fff, ffffbfff, ffffffff */
	sprintf(final->line[0], "  ub4 rsl = (((val >> %u) & 3) ^ %u);\n", i,
		find_adder(x,y,z));
      }
      return;
    }
  }

  /* three instructions */
  for (i=0; i<=final->highbit; ++i)
  {
    x = (a+(a>>i))&3;
    y = (b+(b>>i))&3;
    z = (c+(c>>i))&3;
    if (x != y && x != z && y != z)
    {
      if (form->perfect == NORMAL_HP || (x != 3 && y != 3 && z != 3))
      {
	/* h3g: 0x000, 0x001, 0x100 */
	sprintf(final->line[0], "  ub4 rsl = ((val+(val>>%u))&3);\n", i);
      }
      else
      {
	/* h3h: 0x001, 0x100, 0x101 */
	sprintf(final->line[0], "  ub4 rsl = (((val+(val>>%u))&3)^%u);\n", i,
		find_adder(x,y,z));
      }
      return;
    }
  }

  /*
   * Four instructions: I can prove this will always work.
   *
   * If the three values are distinct, there are two bits which 
   * distinguish them.  Choose the two such bits that are closest together.
   * If those bits are values 001 and 100 for those three values,
   * then there either aren't any bits in between
   * or the in-between bits aren't valued 001, 110, 100, 011, 010, or 101,
   * because that would violate the closest-together assumption.
   * So any in-between bits must be 000 or 111, and of 000 and 111 with
   * the distinguishing bits won't cause them to stop being distinguishing.
   */
  for (i=final->lowbit; i<=final->highbit; ++i)
  {
    for (j=i; j<=final->highbit; ++j)
    {
      x = ((a>>i)^(a>>j))&3;
      y = ((b>>i)^(b>>j))&3;
      z = ((c>>i)^(c>>j))&3;
      if (x != y && x != z && y != z)
      {
	if (form->perfect == NORMAL_HP || (x != 3 && y != 3 && z != 3))
	{
	  /* h3i: 0x00, 0x04, 0x10 */
	  sprintf(final->line[0], 
		  "  ub4 rsl = (((val>>%u) ^ (val>>%u)) & 3);\n", i, j);
	}
	else
	{
	  /* h3j: 0x04, 0x10, 0x14 */
	  sprintf(final->line[0], 
		  "  ub4 rsl = ((((val>>%u) ^ (val>>%u)) & 3) ^ %u);\n",
		  i, j, find_adder(x,y,z));
	}
	return;
      }
    }
  }

  printf("fatal error: hexthree\n");
  exit(1);
}



/*
 * Check that a,b,c,d are some permutation of 0,1,2,3
 * Assume that a,b,c,d are all have values less than 32.
 */
static int testfour(a,b,c,d)
ub4 a;
ub4 b;
ub4 c;
ub4 d;
{
  ub4 mask = (1<<a)^(1<<b)^(1<<c)^(1<<d);
  return (mask == 0xf);
}



/*
 * Find a perfect hash when there are only four keys.  Max 10 instructions.
 * Note that a perfect hash for 4 keys will automatically be minimal.
 */
static void hexfour(keys, final)
key     *keys;
gencode *final;
{
  ub4 a = keys->hash_k;
  ub4 b = keys->next_k->hash_k;
  ub4 c = keys->next_k->next_k->hash_k;
  ub4 d = keys->next_k->next_k->next_k->hash_k;
  ub4 w,x,y,z;
  ub4 i,j,k;

  if (a==b || a==c || a==d || b==c || b==d || c==d)
  {
    printf("fatal error: Duplicate keys\n");
    exit(1);
  }

  final->used = 1;

  /* one instruction */
  if ((final->diffbits & 3) == 3)
  {
    w = a&3;
    x = b&3;
    y = c&3;
    z = d&3;
    if (testfour(w,x,y,z))
    {
      sprintf(final->line[0], "  ub4 rsl = (val & 3);\n");   /* h4a: 0,1,2,3 */
      return;
    }
  }

  if (((final->diffbits >> (UB4BITS-2)) & 3) == 3)
  {
    w = a>>(UB4BITS-2);
    x = b>>(UB4BITS-2);
    y = c>>(UB4BITS-2);
    z = d>>(UB4BITS-2);
    if (testfour(w,x,y,z))
    {                         /* h4b: 0fffffff, 4fffffff, 8fffffff, cfffffff */
      sprintf(final->line[0], "  ub4 rsl = (val >> %u);\n", (ub4)(UB4BITS-2));
      return;
    }
  }

  /* two instructions */
  for (i=final->lowbit; i<final->highbit; ++i)
  {
    if (((final->diffbits >> i) & 3) == 3)
    {
      w = (a>>i)&3;
      x = (b>>i)&3;
      y = (c>>i)&3;
      z = (d>>i)&3;
      if (testfour(w,x,y,z))
      {                                                      /* h4c: 0,2,4,6 */
	sprintf(final->line[0], "  ub4 rsl = ((val >> %u) & 3);\n", i);
	return;
      }
    }
  }

  /* three instructions (linear with the number of diffbits) */
  if ((final->diffbits & 3) != 0)
  {
    for (i=final->lowbit; i<=final->highbit; ++i)
    {
      if (((final->diffbits >> i) & 3) != 0)
      {
	w = (a+(a>>i))&3;
	x = (b+(b>>i))&3;
	y = (c+(c>>i))&3;
	z = (d+(d>>i))&3;
	if (testfour(w,x,y,z))
	{                                                    /* h4d: 0,1,2,4 */
	  sprintf(final->line[0], 
		  "  ub4 rsl = ((val + (val >> %u)) & 3);\n", i);
	  return;
	}

	w = (a-(a>>i))&3;
	x = (b-(b>>i))&3;
	y = (c-(c>>i))&3;
	z = (d-(d>>i))&3;
	if (testfour(w,x,y,z))
	{                                                    /* h4e: 0,1,3,5 */
	  sprintf(final->line[0], 
		  "  ub4 rsl = ((val - (val >> %u)) & 3);\n", i);
	  return;
	}

	/* h4f: ((val>>k)-val)&3: redundant with h4e */

	w = (a^(a>>i))&3;
	x = (b^(b>>i))&3;
	y = (c^(c>>i))&3;
	z = (d^(d>>i))&3;
	if (testfour(w,x,y,z))
	{                                                    /* h4g: 3,4,5,8 */
	  sprintf(final->line[0], 
		  "  ub4 rsl = ((val ^ (val >> %u)) & 3);\n", i);
	  return;
	}
      }
    }
  }

  /* four instructions (linear with the number of diffbits) */
  if ((final->diffbits & 3) != 0)
  {
    for (i=final->lowbit; i<=final->highbit; ++i)
    {
      if ((((final->diffbits >> i) & 1) != 0) &&
	  ((final->diffbits & 2) != 0))
      {
	w = (a&3)^((a>>i)&1);
	x = (b&3)^((b>>i)&1);
	y = (c&3)^((c>>i)&1);
	z = (d&3)^((d>>i)&1);
	if (testfour(w,x,y,z))
	{                                                    /* h4h: 1,2,6,8 */
	  sprintf(final->line[0], 
		  "  ub4 rsl = ((val & 3) ^ ((val >> %u) & 1));\n", i);
	  return;
	}

	w = (a&2)^((a>>i)&1);
	x = (b&2)^((b>>i)&1);
	y = (c&2)^((c>>i)&1);
	z = (d&2)^((d>>i)&1);
	if (testfour(w,x,y,z))
	{                                                    /* h4i: 1,2,8,a */
	  sprintf(final->line[0], 
		  "  ub4 rsl = ((val & 2) ^ ((val >> %u) & 1));\n", i);
	  return;
	}
      }

      if ((((final->diffbits >> i) & 2) != 0) &&
	  ((final->diffbits & 1) != 0))
      {
	w = (a&3)^((a>>i)&2);
	x = (b&3)^((b>>i)&2);
	y = (c&3)^((c>>i)&2);
	z = (d&3)^((d>>i)&2);
	if (testfour(w,x,y,z))
	{                                                    /* h4j: 0,1,3,4 */
	  sprintf(final->line[0], 
		  "  ub4 rsl = ((val & 3) ^ ((val >> %u) & 2));\n", i);
	  return;
	}

	w = (a&1)^((a>>i)&2);
	x = (b&1)^((b>>i)&2);
	y = (c&1)^((c>>i)&2);
	z = (d&1)^((d>>i)&2);
	if (testfour(w,x,y,z))
	{                                                    /* h4k: 1,4,7,8 */
	  sprintf(final->line[0], 
		  "  ub4 rsl = ((val & 1) ^ ((val >> %u) & 2));\n", i);
	  return;
	}
      }
    }
  }

  /* four instructions (quadratic in the number of diffbits) */
  for (i=final->lowbit; i<=final->highbit; ++i)
  {
    if (((final->diffbits >> i) & 1) == 1)
    {
      for (j=final->lowbit; j<=final->highbit; ++j)
      {
	if (((final->diffbits >> j) & 3) != 0)
	{
	  /* test + */
	  w = ((a>>i)+(a>>j))&3;
	  x = ((b>>i)+(a>>j))&3;
	  y = ((c>>i)+(a>>j))&3;
	  z = ((d>>i)+(a>>j))&3;
	  if (testfour(w,x,y,z))
	  {                                                /* h4l: testcase? */
	    sprintf(final->line[0], 
		    "  ub4 rsl = (((val >> %u) + (val >> %u)) & 3);\n", 
		    i, j);
	    return;
	  }

	  /* test - */
	  w = ((a>>i)-(a>>j))&3;
	  x = ((b>>i)-(a>>j))&3;
	  y = ((c>>i)-(a>>j))&3;
	  z = ((d>>i)-(a>>j))&3;
	  if (testfour(w,x,y,z))
	  {                                                /* h4m: testcase? */
	    sprintf(final->line[0], 
		    "  ub4 rsl = (((val >> %u) - (val >> %u)) & 3);\n",
		    i, j);
	    return;
	  }

	  /* test ^ */
	  w = ((a>>i)^(a>>j))&3;
	  x = ((b>>i)^(a>>j))&3;
	  y = ((c>>i)^(a>>j))&3;
	  z = ((d>>i)^(a>>j))&3;
	  if (testfour(w,x,y,z))
	  {                                                /* h4n: testcase? */
	    sprintf(final->line[0], 
		    "  ub4 rsl = (((val >> %u) ^ (val >> %u)) & 3);\n",
		    i, j);
	    return;
	  }
	}
      }
    }
  }

  /* five instructions (quadratic in the number of diffbits) */
  for (i=final->lowbit; i<=final->highbit; ++i)
  {
    if (((final->diffbits >> i) & 1) != 0)
    {
      for (j=final->lowbit; j<=final->highbit; ++j)
      {
	if (((final->diffbits >> j) & 3) != 0)
	{
	  w = ((a>>j)&3)^((a>>i)&1);
	  x = ((b>>j)&3)^((b>>i)&1);
	  y = ((c>>j)&3)^((c>>i)&1);
	  z = ((d>>j)&3)^((d>>i)&1);
	  if (testfour(w,x,y,z))
	  {                                                  /* h4o: 0,4,8,a */
	    sprintf(final->line[0], 
		    "  ub4 rsl = (((val >> %u) & 3) ^ ((val >> %u) & 1));\n", 
		    j, i);
	    return;
	  }
	  
	  w = ((a>>j)&2)^((a>>i)&1);
	  x = ((b>>j)&2)^((b>>i)&1);
	  y = ((c>>j)&2)^((c>>i)&1);
	  z = ((d>>j)&2)^((d>>i)&1);
	  if (testfour(w,x,y,z))
	  {                                   /* h4p: 0x04, 0x08, 0x10, 0x14 */
	    sprintf(final->line[0], 
		    "  ub4 rsl = (((val >> %u) & 2) ^ ((val >> %u) & 1));\n", 
		    j, i);
	    return;
	  }
	}
	
	if (i==0)
	{
	  w = ((a>>j)^(a<<1))&3;
	  x = ((b>>j)^(b<<1))&3;
	  y = ((c>>j)^(c<<1))&3;
	  z = ((d>>j)^(d<<1))&3;
	}
	else
	{
	  w = ((a>>j)&3)^((a>>(i-1))&2);
	  x = ((b>>j)&3)^((b>>(i-1))&2);
	  y = ((c>>j)&3)^((c>>(i-1))&2);
	  z = ((d>>j)&3)^((d>>(i-1))&2);
	}
	if (testfour(w,x,y,z))
	{
	  if (i==0)                                          /* h4q: 0,4,5,8 */
	  {
	    sprintf(final->line[0], 
		    "  ub4 rsl = (((val >> %u) ^ (val << 1)) & 3);\n",
		    j);
	  }
	  else if (i==1)                         /* h4r: 0x01,0x09,0x0b,0x10 */
	  {
	    sprintf(final->line[0], 
		    "  ub4 rsl = (((val >> %u) & 3) ^ (val & 2));\n",
		    j);
	  }
	  else                                               /* h4s: 0,2,6,8 */
	  {
	    sprintf(final->line[0], 
		    "  ub4 rsl = (((val >> %u) & 3) ^ ((val >> %u) & 2));\n",
		    j, (i-1));
	  }
	  return;
	}
	  
	w = ((a>>j)&1)^((a>>i)&2);
	x = ((b>>j)&1)^((b>>i)&2);
	y = ((c>>j)&1)^((c>>i)&2);
	z = ((d>>j)&1)^((d>>i)&2);
	if (testfour(w,x,y,z))                   /* h4t: 0x20,0x14,0x10,0x06 */
	{                   
	  sprintf(final->line[0], 
		  "  ub4 rsl = (((val >> %u) & 1) ^ ((val >> %u) & 2));\n",
		  j, i);
	  return;
	}
      }
    }
  }

  /*
   * OK, bring out the big guns.
   * There exist three bits i,j,k which distinguish a,b,c,d.
   * i^(j<<1)^(k*q) is guaranteed to work for some q in {0,1,2,3},
   *   proven by exhaustive search of all (8 choose 4) cases.
   * Find three such bits and try the 4 cases.
   * Linear with the number of diffbits.
   * Some cases below may duplicate some cases above.  I did it that way
   *   so that what is below is guaranteed to work, no matter what was
   *   attempted above.
   * The generated hash is at most 10 instructions.
   */
  for (i=final->lowbit; i<UB4BITS; ++i)
  {
    y = (c>>i)&1;
    z = (d>>i)&1;
    if (y != z)
      break;
  }

  for (j=final->lowbit; j<UB4BITS; ++j)
  {
    x = ((b>>i)&1)^(((b>>j)&1)<<1);
    y = ((c>>i)&1)^(((c>>j)&1)<<1);
    z = ((d>>i)&1)^(((d>>j)&1)<<1);
    if (x != y && x != z && y != z)
      break;
  }

  for (k=final->lowbit; k<UB4BITS; ++k)
  {
    w = ((a>>i)&1)^(((a>>j)&1)<<1)^(((a>>k)&1)<<2);
    x = ((b>>i)&1)^(((b>>j)&1)<<1)^(((b>>k)&1)<<2);
    y = ((c>>i)&1)^(((c>>j)&1)<<1)^(((c>>k)&1)<<2);
    z = ((d>>i)&1)^(((d>>j)&1)<<1)^(((d>>k)&1)<<2);
    if (w != x && w != y && w != z && x != y && x != z && y != z)
      break;
  }

  /* Assert: bits i,j,k were found which distinguish a,b,c,d */
  if (i==UB4BITS || j==UB4BITS || k==UB4BITS)
  {
    printf("Fatal error: hexfour(), i %u j %u k %u\n", i,j,k);
    exit(1);
  }

  /* now try the four cases */
  {
    ub4 m,n,o,p;
    
    /* if any bit has two 1s and two 0s, make that bit o */
    if (((a>>i)&1)+((b>>i)&1)+((c>>i)&1)+((d>>i)&1) != 2)
      { m=j; n=k; o=i; }
    else if (((a>>j)&1)+((b>>j)&1)+((c>>j)&1)+((d>>j)&1) != 2)
      { m=i; n=k; o=j; }
    else
      { m=i; n=j; o=k; }
    if (m > n) {p=m; m=n; n=p; }                          /* guarantee m < n */

    /* printf("m %u n %u o %u  %u %u %u %u\n", m, n, o, w,x,y,z); */

    /* seven instructions, multiply bit o by 1 */
    w = (((a>>m)^(a>>o))&1)^((a>>(n-1))&2);
    x = (((b>>m)^(b>>o))&1)^((b>>(n-1))&2);
    y = (((c>>m)^(c>>o))&1)^((c>>(n-1))&2);
    z = (((d>>m)^(d>>o))&1)^((d>>(n-1))&2);
    if (testfour(w,x,y,z))
    {
      if (m>o) {p=m; m=o; o=p;}                 /* make sure m < o and m < n */

      if (m==0)                                                   /* 0,2,8,9 */
      {
	sprintf(final->line[0], 
		"  ub4 rsl = (((val^(val>>%u))&1)^((val>>%u)&2));\n", o, n-1);
      }
      else                                            /* 0x00,0x04,0x10,0x12 */
      {
	sprintf(final->line[0], 
		"  ub4 rsl = ((((val>>%u) ^ (val>>%u)) & 1) ^ ((val>>%u) & 2));\n",
		m, o, n-1);
      }
      return;
    }
    
    /* six to seven instructions, multiply bit o by 2 */
    w = ((a>>m)&1)^((((a>>n)^(a>>o))&1)<<1);
    x = ((b>>m)&1)^((((b>>n)^(b>>o))&1)<<1);
    y = ((c>>m)&1)^((((c>>n)^(c>>o))&1)<<1);
    z = ((d>>m)&1)^((((d>>n)^(d>>o))&1)<<1);
    if (testfour(w,x,y,z))
    {
      if (m==o-1) {p=n; n=o; o=p;}                /* make m==n-1 if possible */

      if (m==0)                                                   /* 0,1,5,8 */
      {
	sprintf(final->line[0], 
		"  ub4 rsl = ((val & 1) ^ (((val>>%u) ^ (val>>%u)) & 2));\n",
		n-1, o-1);
      }
      else if (o==0)                                  /* 0x00,0x04,0x05,0x10 */
      {
	sprintf(final->line[0], 
		"  ub4 rsl = (((val>>%u) & 2) ^ (((val>>%u) ^ val) & 1));\n",
		m-1, n);
      }
      else                                            /* 0x00,0x02,0x0a,0x10 */
      {
	sprintf(final->line[0], 
		"  ub4 rsl = (((val>>%u) & 1) ^ (((val>>%u) ^ (val>>%u)) & 2));\n",
		m, n-1, o-1);
      }
      return;
    }
    
    /* multiplying by 3 is a pain: seven or eight instructions */
    w = (((a>>m)&1)^((a>>(n-1))&2))^((a>>o)&1)^(((a>>o)&1)<<1);
    x = (((b>>m)&1)^((b>>(n-1))&2))^((b>>o)&1)^(((b>>o)&1)<<1);
    y = (((c>>m)&1)^((c>>(n-1))&2))^((c>>o)&1)^(((c>>o)&1)<<1);
    z = (((d>>m)&1)^((d>>(n-1))&2))^((d>>o)&1)^(((d>>o)&1)<<1);
    if (testfour(w,x,y,z))
    {
      final->used = 2;
      sprintf(final->line[0], "  ub4 b = (val >> %u) & 1;\n", o);
      if (m==o-1 && m==0)                             /* 0x02,0x10,0x11,0x18 */
      {
	sprintf(final->line[1], 
		"  ub4 rsl = ((val & 3) ^ ((val >> %u) & 2) ^ b);\n", n-1);
      }
      else if (m==o-1)                                            /* 0,4,6,c */
      {
	sprintf(final->line[1], 
		"  ub4 rsl = (((val >> %u) & 3) ^ ((val >> %u) & 2) ^ b);\n",
		m, n-1);
      }
      else if (m==n-1 && m==0)                                /* 02,0a,0b,18 */
      {
	sprintf(final->line[1], 
		"  ub4 rsl = ((val & 3) ^ b ^ (b << 1));\n");
      }
      else if (m==n-1)                                            /* 0,2,4,8 */
      {
	sprintf(final->line[1], 
		"  ub4 rsl = (((val >> %u) & 3) ^ b ^ (b << 1));\n", m);
      }
      else if (o==n-1 && m==0)                          /* h4am: not reached */
      {
	sprintf(final->line[1], 
		"  ub4 rsl = ((val & 1) ^ ((val >> %u) & 3) ^ (b <<1 ));\n",
		o);
      }
      else if (o==n-1)                                /* 0x00,0x02,0x08,0x10 */
      {
	sprintf(final->line[1], 
		"  ub4 rsl = (((val >> %u) & 1) ^ ((val >> %u) & 3) ^ (b << 1));\n",
		m, o);
      }
      else if ((m != o-1) && (m != n-1) && (o != m-1) && (o != n-1))
      {
	final->used = 3;
	sprintf(final->line[0], "  ub4 newval = val & 0x%x;\n", 
		(((ub4)1<<m)^((ub4)1<<n)^((ub4)1<<o)));
	if (o==0)                                     /* 0x00,0x01,0x04,0x10 */
	{
	  sprintf(final->line[1], "  ub4 b = -newval;\n");
	}
	else                                          /* 0x00,0x04,0x09,0x10 */
	{
	  sprintf(final->line[1], "  ub4 b = -(newval >> %u);\n", o);
	}
	if (m==0)                                     /* 0x00,0x04,0x09,0x10 */
	{
	  sprintf(final->line[2], 
		  "  ub4 rsl = ((newval ^ (newval>>%u) ^ b) & 3);\n", n-1);
	}
	else                                          /* 0x00,0x03,0x04,0x10 */
	{
	  sprintf(final->line[2], 
		  "  ub4 rsl = (((newval>>%u) ^ (newval>>%u) ^ b) & 3);\n",
		  m, n-1);
	}
      }
      else if (o == m-1)
      {
	if (o==0)                                     /* 0x02,0x03,0x0a,0x10 */
	{
	  sprintf(final->line[0], "  ub4 b = (val<<1) & 2;\n");
	}
	else if (o==1)                                /* 0x00,0x02,0x04,0x10 */
	{
	  sprintf(final->line[0], "  ub4 b = val & 2;\n");
	}
	else                                          /* 0x00,0x04,0x08,0x20 */
	{
	  sprintf(final->line[0], "  ub4 b = (val>>%u) & 2;\n", o-1);
	}

	if (o==0)                                     /* 0x02,0x03,0x0a,0x10 */
	{
	  sprintf(final->line[1],
		  "  ub4 rsl = ((val & 3) ^ ((val>>%u) & 1) ^ b);\n",
		  n);
	}
	else                                          /* 0x00,0x02,0x04,0x10 */
	{
	  sprintf(final->line[1],
		  "  ub4 rsl = (((val>>%u) & 3) ^ ((val>>%u) & 1) ^ b);\n",
		  o, n);
	}
      }
      else                         /* h4ax: 10 instructions, but not reached */
      {
	sprintf(final->line[1], 
		"  ub4 rsl = (((val>>%u) & 1) ^ ((val>>%u) & 2) ^ b ^ (b<<1));\n",
		m, n-1);
      }

      return;
    }

    /* five instructions, multiply bit o by 0, covered before the big guns */
    w = ((a>>m)&1)^(a>>(n-1)&2);
    x = ((b>>m)&1)^(b>>(n-1)&2);
    y = ((c>>m)&1)^(c>>(n-1)&2);
    z = ((d>>m)&1)^(d>>(n-1)&2);
    if (testfour(w,x,y,z))
    {                                                    /* h4v, not reached */
      sprintf(final->line[0], 
	      "  ub4 rsl = (((val>>%u) & 1) ^ ((val>>%u) & 2));\n", m, n-1);
      return;
    }
  }

  printf("fatal error: bug in hexfour!\n");
  exit(1);
  return;
}


/* test if a_k is distinct and in range for all keys */
static int testeight(keys, badmask)
key      *keys;                                         /* keys being hashed */
ub1       badmask;                       /* used for minimal perfect hashing */
{
  ub1  mask = badmask;
  key *mykey;

  for (mykey=keys; mykey; mykey=mykey->next_k)
  {
    if (bit(mask, 1<<mykey->a_k)) return FALSE;
    bis(mask, 1<<mykey->a_k);
  }
  return TRUE;
}



/*
 * Try to find a perfect hash when there are five to eight keys.
 *
 * We can't deterministically find a perfect hash, but there's a reasonable
 * chance we'll get lucky.  Give it a shot.  Return TRUE if we succeed.
 */
static int hexeight(keys, nkeys, final, form)
key      *keys;
ub4       nkeys;
gencode  *final;
hashform *form;
{
  key *mykey;                                       /* walk through the keys */
  ub4  i,j,k;
  ub1  badmask;

  printf("hexeight\n");

  /* what hash values should never be used? */
  badmask = 0;
  if (form->perfect == MINIMAL_HP)
  {
    for (i=nkeys; i<8; ++i)
      bis(badmask,(1<<i));
  }

  /* one instruction */
  for (mykey=keys; mykey; mykey=mykey->next_k)
    mykey->a_k = mykey->hash_k & 7;
  if (testeight(keys, badmask))
  {                                                                   /* h8a */
    final->used = 1;
    sprintf(final->line[0], "  ub4 rsl = (val & 7);\n");
    return TRUE;
  }

  /* two instructions */
  for (i=final->lowbit; i<=final->highbit-2; ++i)
  {
    for (mykey=keys; mykey; mykey=mykey->next_k)
      mykey->a_k = (mykey->hash_k >> i) & 7;
    if (testeight(keys, badmask))
    {                                                                 /* h8b */
      final->used = 1;
      sprintf(final->line[0], "  ub4 rsl = ((val >> %u) & 7);\n", i);
      return TRUE;
    }
  }

  /* four instructions */
  for (i=final->lowbit; i<=final->highbit; ++i)
  {
    for (j=i+1; j<=final->highbit; ++j)
    {
      for (mykey=keys; mykey; mykey=mykey->next_k)
	mykey->a_k = ((mykey->hash_k >> i)+(mykey->hash_k >> j)) & 7;
      if (testeight(keys, badmask))
      {
	final->used = 1;
	if (i == 0)                                                   /* h8c */
	  sprintf(final->line[0], 
		  "  ub4 rsl = ((val + (val >> %u)) & 7);\n", j);
	else                                                          /* h8d */
	  sprintf(final->line[0], 
		  "  ub4 rsl = (((val >> %u) + (val >> %u)) & 7);\n", i, j);
	return TRUE;
      }

      for (mykey=keys; mykey; mykey=mykey->next_k)
	mykey->a_k = ((mykey->hash_k >> i)^(mykey->hash_k >> j)) & 7;
      if (testeight(keys, badmask))
      {
	final->used = 1;
	if (i == 0)                                                   /* h8e */
	  sprintf(final->line[0], 
		  "  ub4 rsl = ((val ^ (val >> %u)) & 7);\n", j);
	else                                                          /* h8f */
	  sprintf(final->line[0], 
		  "  ub4 rsl = (((val >> %u) ^ (val >> %u)) & 7);\n", i, j);

	return TRUE;
      }

      for (mykey=keys; mykey; mykey=mykey->next_k)
	mykey->a_k = ((mykey->hash_k >> i)-(mykey->hash_k >> j)) & 7;
      if (testeight(keys, badmask))
      {
	final->used = 1;
	if (i == 0)                                                   /* h8g */
	  sprintf(final->line[0], 
		  "  ub4 rsl = ((val - (val >> %u)) & 7);\n", j);
	else                                                          /* h8h */
	  sprintf(final->line[0], 
		  "  ub4 rsl = (((val >> %u) - (val >> %u)) & 7);\n", i, j);

	return TRUE;
      }
    }
  }


  /* six instructions */
  for (i=final->lowbit; i<=final->highbit; ++i)
  {
    for (j=i+1; j<=final->highbit; ++j)
    {
      for (k=j+1; k<=final->highbit; ++k)
      {
	for (mykey=keys; mykey; mykey=mykey->next_k)
	  mykey->a_k  = ((mykey->hash_k >> i) +
			 (mykey->hash_k >> j) +
			 (mykey->hash_k >> k)) & 7;
	if (testeight(keys, badmask))
	{                                                             /* h8i */
	  final->used = 1;
	  sprintf(final->line[0], 
		  "  ub4 rsl = (((val >> %u) + (val >> %u) + (val >> %u)) & 7);\n", 
		  i, j, k);
	  return TRUE;
	}
      }
    }
  }


  return FALSE;
}



/*
 * Guns aren't enough.  Bring out the Bomb.  Use tab[].
 * This finds the initial (a,b) when we need to use tab[].
 *
 * We need to produce a different (a,b) every time this is called.  Try all
 * reasonable cases, fastest first.
 *
 * The initial mix (which this determines) can be filled into final starting
 * at line[1].  val is set and a,b are declared.  The final hash (at line[7])
 * is a^tab[b] or a^scramble[tab[b]].
 *
 * The code will probably look like this, minus some stuff:
 *     val += CONSTANT;
 *     val ^= (val<<16);
 *     val += (val>>8);
 *     val ^= (val<<4);
 *     b = (val >> l) & 7;
 *     a = (val + (val<<m)) >> 29;
 *     return a^scramble[tab[b]];
 * Note that *a* and tab[b] will be computed in parallel by most modern chips.
 *
 * final->i is the current state of the state machine.
 * final->j and final->k are counters in the loops the states simulate.
 */
static void hexn(keys, salt, alen, blen, final)
key     *keys;
ub4      salt;
ub4      alen;
ub4      blen;
gencode *final;
{
  key *mykey;
  ub4  highbit = final->highbit;
  ub4  lowbit = final->lowbit;
  ub4  alog = mylog2(alen);
  ub4  blog = mylog2(blen);

  for (;;)
  {
    switch(final->i)
    {
    case 1:
      /* a = val>>30; b=val&3 */
      for (mykey=keys; mykey; mykey=mykey->next_k)
      {
	mykey->a_k = (mykey->hash_k << (UB4BITS-(highbit+1)))>>(UB4BITS-alog);
	mykey->b_k = (mykey->hash_k >> lowbit) & (blen-1);
      }
      if (lowbit == 0)                                                /* hna */
	sprintf(final->line[5], "  b = (val & 0x%x);\n", 
		blen-1);
      else                                                            /* hnb */
	sprintf(final->line[5], "  b = ((val >> %u) & 0x%x);\n", 
		lowbit, blen-1);
      if (highbit+1 == UB4BITS)                                       /* hnc */
	sprintf(final->line[6], "  a = (val >> %u);\n",
		UB4BITS-alog);
      else                                                            /* hnd */
	sprintf(final->line[6], "  a = ((val << %u ) >> %u);\n",
		UB4BITS-(highbit+1), UB4BITS-alog);
  
      ++final->i;
      return;

    case 2:
      /* a = val&3; b=val>>30 */
      for (mykey=keys; mykey; mykey=mykey->next_k)
      {
	mykey->a_k = (mykey->hash_k >> lowbit) & (alen-1);
	mykey->b_k = (mykey->hash_k << (UB4BITS-(highbit+1)))>>(UB4BITS-blog);
      }
      if (highbit+1 == UB4BITS)                                       /* hne */
	sprintf(final->line[5], "  b = (val >> %u);\n",
		UB4BITS-blog);
      else                                                            /* hnf */
	sprintf(final->line[5], "  b = ((val << %u ) >> %u);\n",
		UB4BITS-(highbit+1), UB4BITS-blog);
      if (lowbit == 0)                                                /* hng */
	sprintf(final->line[6], "  a = (val & 0x%x);\n", 
		alen-1);
      else                                                            /* hnh */
	sprintf(final->line[6], "  a = ((val >> %u) & 0x%x);\n", 
		lowbit, alen-1);
  
      ++final->i;
      return;

    case 3:
      /*
       * cases 3,4,5:
       * for (k=lowbit; k<=highbit; ++k)
       *   for (j=lowbit; j<=highbit; ++j)
       *     b = (val>>j)&3;
       *     a = (val<<k)>>30;
       */
      final->k = lowbit;
      final->j = lowbit;
      ++final->i;
      break;

    case 4:
      if (!(final->j < highbit))
      {
	++final->i;
	break;
      }
      for (mykey=keys; mykey; mykey=mykey->next_k)
      {
	mykey->b_k = (mykey->hash_k >> (final->j)) & (blen-1);
	mykey->a_k = (mykey->hash_k << (UB4BITS-final->k-1)) >> (UB4BITS-alog);
      }
      if (final->j == 0)                                              /* hni */
	sprintf(final->line[5], "  b = val & 0x%x;\n",
		blen-1);
      else if (blog+final->j == UB4BITS)                             /* hnja */
	sprintf(final->line[5], "  b = val >> %u;\n",
		final->j);
      else
	sprintf(final->line[5], "  b = (val >> %u) & 0x%x;\n",      /* hnj */
		final->j, blen-1);
      if (UB4BITS-final->k-1 == 0)                                    /* hnk */
	sprintf(final->line[6], "  a = (val >> %u);\n",
		UB4BITS-alog);
      else                                                            /* hnl */
	sprintf(final->line[6], "  a = ((val << %u) >> %u);\n",
		UB4BITS-final->k-1, UB4BITS-alog);
      while (++final->j < highbit)
      {
	if (((final->diffbits>>(final->j)) & (blen-1)) > 2)
	  break;
      }
      return;

    case 5:
      while (++final->k < highbit)
      {
	if ((((final->diffbits<<(UB4BITS-final->k-1))>>alog) & (alen-1)) > 0)
	  break;
      }
      if (!(final->k < highbit))
      {
	++final->i;
	break;
      }
      final->j = lowbit;
      final->i = 4;
      break;


    case 6:
      /*
       * cases 6,7,8:
       * for (k=0; k<UB4BITS-alog; ++k)
       *   for (j=0; j<UB4BITS-blog; ++j)
       *     val = val+f(salt);
       *     val ^= (val >> 16);
       *     val += (val << 8);
       *     val ^= (val >> 4);
       *     b = (val >> j) & 3;
       *     a = (val + (val << k)) >> 30;
       */
      final->k = 0;
      final->j = 0;
      ++final->i;
      break;

    case 7:
      /* Just do something that will surely work */
      {
	ub4 addk = 0x9e3779b9*salt;

	if (!(final->j <= UB4BITS-blog))
	{
	  ++final->i;
	  break;
	}
	for (mykey=keys; mykey; mykey=mykey->next_k)
	{
	  ub4 val = mykey->hash_k + addk;
	  if (final->highbit+1 - final->lowbit > 16)
	    val ^= (val >> 16);
	  if (final->highbit+1 - final->lowbit > 8)
	    val += (val << 8);
	  val ^= (val >> 4);
	  mykey->b_k = (val >> final->j) & (blen-1);
	  if (final->k == 0)
	    mykey->a_k = val >> (UB4BITS-alog);
	  else
	    mykey->a_k = (val + (val << final->k)) >> (UB4BITS-alog);
	}
	sprintf(final->line[1], "  val += 0x%x;\n", addk);
	if (final->highbit+1 - final->lowbit > 16)                    /* hnm */
	  sprintf(final->line[2], "  val ^= (val >> 16);\n");
	if (final->highbit+1 - final->lowbit > 8)                     /* hnn */
	  sprintf(final->line[3], "  val += (val << 8);\n");
	sprintf(final->line[4], "  val ^= (val >> 4);\n");
	if (final->j == 0)              /* hno: don't know how to reach this */
	  sprintf(final->line[5], "  b = val & 0x%x;\n", blen-1);
	else                                                          /* hnp */
	  sprintf(final->line[5], "  b = (val >> %u) & 0x%x;\n",
		  final->j, blen-1);
	if (final->k == 0)                                            /* hnq */
	  sprintf(final->line[6], "  a = val >> %u;\n", UB4BITS-alog);
	else                                                          /* hnr */
	  sprintf(final->line[6], "  a = (val + (val << %u)) >> %u;\n",
		  final->k, UB4BITS-alog);

	++final->j;
	return;
      }

    case 8:
      ++final->k;
      if (!(final->k <= UB4BITS-alog))
      {
	++final->i;
	break;
      }
      final->j = 0;
      final->i = 7;
      break;

    case 9:
      final->i = 6;
      break;
    }
  }
}



/* find the highest and lowest bit where any key differs */
static void setlow(keys, final)
key     *keys;
gencode *final;
{
  ub4  lowbit;
  ub4  highbit;
  ub4  i;
  key *mykey;
  ub4  firstkey;

  /* mark the interesting bits in final->mask */
  final->diffbits = (ub4)0;
  if (keys) firstkey = keys->hash_k;
  for (mykey=keys;  mykey!=(key *)0;  mykey=mykey->next_k)
    final->diffbits |= (firstkey ^ mykey->hash_k);

  /* find the lowest interesting bit */
  for (i=0; i<UB4BITS; ++i)
    if (final->diffbits & (((ub4)1)<<i))
      break;
  final->lowbit = i;

  /* find the highest interesting bit */
  for (i=UB4BITS; --i; )
    if (final->diffbits & (((ub4)1)<<i))
      break;
  final->highbit = i;
}

/* 
 * Initialize (a,b) when keys are integers.
 *
 * Normally there's an initial hash which produces a number.  That hash takes
 * an initializer.  Changing the initializer causes the initial hash to 
 * produce a different (uniformly distributed) number without any extra work.
 *
 * Well, here we start with a number.  There's no initial hash.  Any mixing
 * costs extra work.  So we go through a lot of special cases to minimize the
 * mixing needed to get distinct (a,b).  For small sets of keys, it's often
 * fastest to skip the final hash and produce the perfect hash from the number
 * directly.
 *
 * The target user for this is switch statement optimization.  The common case
 * is 3 to 16 keys, and instruction counts matter.  The competition is a 
 * binary tree of branches.
 *
 * Return TRUE if we found a perfect hash and no more work is needed.
 * Return FALSE if we just did an initial hash and more work is needed.
 */
int inithex(keys, nkeys, alen, blen, smax, salt, final, form)
key      *keys;                                          /* list of all keys */
ub4       nkeys;                                   /* number of keys to hash */
ub4       alen;                    /* (a,b) has a in 0..alen-1, a power of 2 */
ub4       blen;                    /* (a,b) has b in 0..blen-1, a power of 2 */
ub4       smax;                   /* maximum range of computable hash values */
ub4       salt;                     /* used to initialize the hash function */
gencode  *final;                          /* output, code for the final hash */
hashform *form;                                           /* user directives */
{
  setlow(keys, final);

  switch (nkeys)
  {
  case 1:
    hexone(keys, final);
    return TRUE;
  case 2:
    hextwo(keys, final);
    return TRUE;
  case 3:
    hexthree(keys, final, form);
    return TRUE;
  case 4:
    hexfour(keys, final);
    return TRUE;
  case 5:  case 6:  case 7:  case 8:
    if (salt == 1 &&                                  /* first time through */
	hexeight(keys, nkeys, final, form)) /* get lucky, don't need tab[] ? */
      return TRUE;
    /* fall through */
  default:
    if (salt == 1)
    {
      final->used = 8;
      final->i = 1;
      final->j = final->k = final->l = final->m = final->n = final->o = 0;
      sprintf(final->line[0], "  ub4 a, b, rsl;\n");
      sprintf(final->line[1], "\n");
      sprintf(final->line[2], "\n");
      sprintf(final->line[3], "\n");
      sprintf(final->line[4], "\n");
      sprintf(final->line[5], "\n");
      sprintf(final->line[6], "\n");
      if (blen < USE_SCRAMBLE)
      {                                                               /* hns */
	sprintf(final->line[7], "  rsl = (a^tab[b]);\n");
      }
      else
      {                                                               /* hnt */
	sprintf(final->line[7], "  rsl = (a^scramble[tab[b]]);\n");
      }
    }
    hexn(keys, salt, alen, blen, final);
    return FALSE;
  }
}
