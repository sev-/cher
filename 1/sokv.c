#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#define XSIZE 		19
#define YSIZE 		16
#define LEVELSIZE	(XSIZE*YSIZE+2)
#define TMPFILE		"sok$$$.stg"

main()
{
  int level[YSIZE][XSIZE], i, j;
  FILE *in, *in1, *out1;
  struct stat tmpstat;
  int curr;
  int doker;
  int doker1;
  int dokerx;
  int dokery;

  void *clean();

  signal(SIGINT, clean);

  if((in1 = fopen("sok.stg", "r")) == (FILE *)NULL)
  {
    printf("Нет файла sok.stg !!\n");
    exit(0);
  }
  out1 = fopen(TMPFILE, "w");

  fgetc(in1);
  fgetc(in1);

  melt(in1, out1);

  fclose(in1);
  fclose(out1);

  stat(TMPFILE, &tmpstat);
  printf("I have a %d levels\n\n", (tmpstat.st_size)/LEVELSIZE);

  in = fopen(TMPFILE, "r");

  do
  {
    printf("Level: ");
    scanf("%d", &curr);
  
    fseek(in, curr*LEVELSIZE, SEEK_SET);

    doker = fgetc(in);
    doker |= fgetc(in) << 8;
    printf("Doker: %d\n", doker);

  
    for(i = 0; i < YSIZE; i++)
      for(j = 0; j < XSIZE; j++)
        level[i][j] = fgetc(in);
  
    for(i = 0; i < YSIZE; i++)
    {
      for(j = 0; j < XSIZE; j++, doker--)
        if(!doker)
        {
          if(level[i][j])
            fputs(stderr, "Error !!!");
          fputc('┌', stdout);
          fputc('┐', stdout);
        }
        else
          switch(level[i][j])
          {
            case 0:	/* пусто */
  		fputc(' ', stdout);
  		fputc(' ', stdout);
  		break;
            case 01: /* стена */
  		fputc('█', stdout);
  		fputc('█', stdout);
  		break;
            case 03: /* место */
  		fputc('<', stdout);
  		fputc('>', stdout);
  		break;
            case 0x14: /* контейнер */
  		fputc('[', stdout);
  		fputc(']', stdout);
  		break;
            case 0x17: /* стоит */
  		fputc('░', stdout);
  		fputc('░', stdout);
  		break;
  	    default:
  		fputc('?', stdout);
          }
      fputc('\n', stdout);
    }
  } while(curr < 50);
  remove(TMPFILE);
}

void *clean()
{
  remove(TMPFILE);

  printf("\n\n\rInterrupted\n");
  exit(1);
}

