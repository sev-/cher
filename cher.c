#include <stdio.h>
#include <signal.h>

#define XSIZE 		57
#define YSIZE 		43
#define LEVELSIZE	((XSIZE*YSIZE)/8+1)

#define TMPFILE		"cher$$$.tmp"

main()
{
  FILE *in, *in1, *out1;
  char level[LEVELSIZE];
  char twostrings[2][XSIZE];
  int i, j;
  int number;
  int m, inbits;
  void *clean();

  signal(SIGINT, clean);

  if((in1 = fopen("cher.stg", "r")) == (FILE *)NULL)
  {
    printf("Нет файла cher.stg !!\n");
    exit(0);
  }
  out1 = fopen(TMPFILE, "w");

  fgetc(in1);
  fgetc(in1);

  melt(in1, out1);

  fclose(in1);
  fclose(out1);

  if((in = fopen(TMPFILE, "r")) == (FILE *)NULL)
  {
    printf("Нет файла cher.stg !!\n");
    exit(0);
  }

  do
  {
    printf("Level: ");
    scanf("%d", &number);

    fseek(in, (long)(number*LEVELSIZE), SEEK_SET);

    for(i = 0; i < LEVELSIZE; i++)
      level[i] = fgetc(in);

    m = 0;
    inbits = 0;

    for(i = 0; i < YSIZE; i+=2)
    {
      for(j = 0; j < XSIZE; j++)
      {
	twostrings[0][j] = (level[m] & 0x80);	/* 1000 0000 */
	level[m] <<= 1;
	inbits++;
	if(inbits == 8)
	{
	  m++;
	  inbits = 0;
	}
      }
      if(i != YSIZE -1)
	for(j = 0; j < XSIZE; j++)
	{
	  twostrings[1][j] = (level[m] & 0x80);	/* 0111 1111 */
	  level[m] <<= 1;
	  inbits++;
	  if(inbits == 8)
	  {
	    m++;
	    inbits = 0;
	  }
	}
      else
	for(i = 0; i < XSIZE; i++)
	  twostrings[1][i] = 0;

      for(j = 0; j < XSIZE; j++)
	if(!twostrings[0][j])
	  if(!twostrings[1][j])
	    putc(' ', stdout);
	  else
	    putc('▄', stdout);
	else
	  if(!twostrings[1][j])
	    putc('▀', stdout);
	  else
	    putc('█', stdout);
      putc('\n', stdout);
    }
  } while(number <= 29);

  fclose(in);
  remove(TMPFILE);
  return 0;
}

void *clean()
{
  remove(TMPFILE);

  printf("Interrupt\n");
  exit(1);
}

