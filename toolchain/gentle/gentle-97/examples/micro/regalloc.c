/* regalloc.c */

int DPoolIndex = 7;
int APoolIndex = 5;

int DPool[8] = {7, 6, 5, 4, 3, 2, 1, 0};
int APool[6] = {5, 4, 3, 2, 1, 0};

/*--------------------------------------------------------------------*/

FetchDReg (R)
   long *R;
{
   if (DPoolIndex < 0) {
      printf("running out of D registers\n");
      exit(1);
   }
   *R = DPool[DPoolIndex--];
}

/*--------------------------------------------------------------------*/

ReleaseDReg (R)
   long R;
{
   DPool[++DPoolIndex] = R;
}

/*--------------------------------------------------------------------*/

FetchAReg (R)
   long *R;
{
   if (APoolIndex < 0) {
      printf("running out of A registers\n");
      exit(1);
   }
   *R = APool[APoolIndex--];
}

/*--------------------------------------------------------------------*/

ReleaseAReg (R)
   long R;
{
   if (R >= 6) return; /* dont release dedicated registers */
   APool[++APoolIndex] = R;
}
