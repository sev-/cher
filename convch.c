#include <stdio.h>
#include <signal.h>

#define XSIZE 		57
#define YSIZE 		43
#define LEVELSIZE	(XSIZE*YSIZE)+1

main()
{
  FILE *in;
  char level[LEVELSIZE];
  int i, j;
  long number;
  char outch;
  int bitsouted = 0;
  int how = 0;

  if((in = fopen("cher.stg", "r")) == (FILE *)NULL)
  {
    printf("Нет файла cher.stg !!\n");
    exit(0);
  }

  for(number = 0; number < 25; number++)
  {
    fgets(level, LEVELSIZE, in);

    outch = 0;
    bitsouted = 0;

    for(i = 0; i < YSIZE; i++)
    {
      for(j = 0; j < XSIZE; j++)
      {
	outch = outch * 2 + ((level[i*XSIZE + j] == '`') ? 0 : 1);
	bitsouted++;
	if(bitsouted == 8)
	{
	  putc(outch, stdout);
	  outch = 0;
	  how ++;
	  bitsouted = 0;
	}
      }
    }
    putc(outch << 5, stdout);	/* after each level */
    fprintf(stderr,"%d - %d bytes\n", number, how+1);
    how = 0;
  }
  return 0;
}
