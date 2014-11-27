/* machine.c */

#define CODESIZE  401024
#define STACKSIZE 1024

enum opcode { lit, opr, lod, sto, cal, int_, jmp, jpc };

typedef struct { enum opcode f; int l; int a; } instruction;

instruction code[CODESIZE];

int pc = -1;

append (f, l, a)
   enum opcode f; int l; int a;
{
   pc++;
   if (pc >= CODESIZE) {
      printf("machine: code overflow (CODESIZE = %d)\n", CODESIZE);
      exit(1);
   }
   code[pc].f = f; code[pc].l = l; code[pc].a = a;
}


LIT(N) { append (lit, 0, N); }

OPR(N) { append (opr, 0, N); }

LOD(L, A) { append (lod, L, A); }

STO(L, A) { append (sto, L, A); }

CAL(L, A) { append (cal, L, A); }

INT_(N) { append (int_, 0, N); }

JMP(N) { append (jmp, 0, N); }

JPC(N) { append (jpc, 0, N); }


PC(REF_N)
   long *REF_N;
{
   *REF_N = pc+1;
}

PATCH(i)
{
   code[i].a = pc+1;
}

printcode ()
{
   int i;
   for (i = 1; i <= pc; i++) {
      printf("%d	", i);
      switch (code[i].f) {
      case lit: printf("LIT"); break;
      case opr: printf("OPR"); break;
      case lod: printf("LOD"); break;
      case sto: printf("STO"); break;
      case cal: printf("CAL"); break;
      case int_: printf("INT"); break;
      case jmp: printf("JMP"); break;
      case jpc: printf("JPC"); break;
      }
      printf(" %d,%d\n", code[i].l, code[i].a);
   }
}

int p, b, t;
instruction i;
int s[STACKSIZE];

check_t()
{
   if (t+3 >= STACKSIZE) {
      printf("machine: stack overflow (STACKSIZE = %d)\n", STACKSIZE);
      exit(1);
   }
}

int base(l)
   int l;
{
   int b1;
   b1 = b;
   while (l > 0) {
      b1 = s[b1]; l--;
   }
   return b1;
}

execute()
{

   t = 0; b = 1; p = 0;
   s[1] = 0; s[2] = 0; s[3] = 0;

   do {
      i = code[p];
      p++;
      switch (i.f) {
      case lit:
	 t++; check_t(); s[t] = i.a;
	 break;
      case opr:
	 switch (i.a) {
	 case 0:
	    t = b-1; p = s[t+3]; b = s[t+2];
	    break;
	 case 1:
	    s[t] = -s[t];
	    break;
	 case 2:
	    t--; s[t] = s[t] + s[t+1];
	    break;
	 case 3:
	    t--; s[t] = s[t] - s[t+1];
	    break;
	 case 4:
	    t--; s[t] = s[t] * s[t+1];
	    break;
	 case 5:
	    t--; s[t] = s[t] / s[t+1];
	    break;
	 case 6:
	    s[t] = s[t] % 2;
	    break;
	 case 8:
	    t--; s[t] = s[t] == s[t+1];
	    break;
	 case 9:
	    t--; s[t] = s[t] != s[t+1];
	    break;
	 case 10:
	    t--; s[t] = s[t] < s[t+1];
	    break;
	 case 11:
	    t--; s[t] = s[t] >= s[t+1];
	    break;
	 case 12:
	    t--; s[t] = s[t] > s[t+1];
	    break;
	 case 13:
	    t--; s[t] = s[t] <= s[t+1];
	    break;
	 case 14:
	    printf("%d\n", s[t]); t--;
	    break;
	 }
	 break;
      case lod:
	 t++; check_t(); s[t] = s[base(i.l)+i.a];
	 break;
      case sto:
	 s[base(i.l)+i.a] = s[t]; t--;
	 break;
      case cal:
	 s[t+1] = base(i.l); s[t+2] = b; s[t+3] = p;
	 b = t+1; p = i.a;
	 break;
      case int_:
	 t += i.a; check_t();
	 break;
      case jmp:
	 p = i.a;
	 break;
      case jpc:
	 if (s[t] == 0) p = i.a; t--;
	 break;
      }
   } while (p != 0);
}
