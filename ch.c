#include <stdio.h>
#include <termio.h>
#include <signal.h>
#include <sys/select.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#define	LEFTKEY		keyboard[0]
#define	RIGHTKEY	keyboard[1]
#define	UPKEY		keyboard[2]
#define	DOWNKEY		keyboard[3]
#define	PAUSEKEY	keyboard[4]
#define	QUITKEY		keyboard[5]

#define XSIZE 		57
#define YSIZE 		43
#define LEVELSIZE	((XSIZE*YSIZE)/8+1)

#define MAXWORM		1024

#define TMPFILE		"cher$$$.stg"

int c, score, xdirect, ydirect;
int x, y;
int area[YSIZE][XSIZE];
int worm[MAXWORM][2];
int head, tail;
int apple[2];
int tailrest;
int appleslost;
int length;

struct timeval tt;

char obuf[1024];

getchr()
{
  unsigned char xc[1];

  if( read(0,xc,1) != 1 )
	return -1;
  return xc[0];
}

char *howprint(x1, y1)
int x1, y1;
{
  if(y1 % 2)
    y1--;

  if(y1 == YSIZE-1)
    return "▀";

  if(!area[y1][x1])
    if(!area[y1+1][x1])
      return " ";
    else
      return "▄";
  else
    if(!area[y1+1][x1])
      return "▀";
    else
      return "█";
}

redraw(x1, y1)
int x1, y1;
{
  sprintf(obuf, "\033[%02d;%02dH%s", y1/2+3, x1+1, howprint(x1, y1));
  write(1, obuf, strlen(obuf));
}

int Move()
{
  int i;

  x += xdirect;
  y += ydirect;

  if(area[y][x] && area[y][x] != 10)
    return 0;

  if(!y)
  {
    for(i = tail; i != head; i++)
    {
      area[worm[i][1]][worm[i][0]] = 0;
      redraw(worm[i][0], worm[i][1]);
      if(i == MAXWORM-1)
        i = 0;
    }
    head = tail = 0;
    return 1;
  }

  if(area[y][x] == 10)
  {
    tailrest += 12;
    appleslost --;
    score += 100;
    if(appleslost)
    {
      while(area[(apple[1] = rand()%YSIZE)][(apple[0] = rand()%XSIZE)]);
      area[apple[1]][apple[0]] = 10;
    }
    else
      for(i = 0; i < 5; i++)
      {
        area[0][3+i] = 0;
        redraw(3+i, 0);
      }
  }

  area[y][x] = 2;

  if(tailrest)
  {
    tailrest --;
    length ++;
  }

  if(!tailrest)
  {
    area[worm[tail][1]][worm[tail][0]] = 0;
    redraw(worm[tail][0], worm[tail][1]);
    tail++;
    if(tail == MAXWORM)
      tail = 0;
  }

  if(appleslost)
    blink_apple();

  head++;
  if(head == MAXWORM)
    head = 0;

  worm[head][0] = x;
  worm[head][1] = y;

  redraw(x, y);

  sprintf(obuf, "\033[%02d;%02dH", y/2+3, x+1);
  write(1, obuf, strlen(obuf));

  return 1;
}

blink_apple()
{
  static int mode;

  if(mode)
    area[apple[1]][apple[0]] = 0;
  else
    write(1, "\033[1m", 4);

  redraw(apple[0], apple[1]);

  if(mode)
    area[apple[1]][apple[0]] = 10;
  else
    write(1, "\033[m", 3);

  mode = !mode;
}
  
show(number)
int number;
{
  FILE *in;
  char level[LEVELSIZE];
  int m = 0, inbits = 0;
  int i, j;

  in = fopen(TMPFILE, "r");

  fseek(in, (long)(number*LEVELSIZE), SEEK_SET);

  write(1, "\n", 1);

  for(i = 0; i < LEVELSIZE; i++)
    level[i] = fgetc(in);

  for(i = 0; i < YSIZE; i += 2)
  {
    for(j = 0; j < XSIZE; j++)
    {
      area[i][j] = (level[m] & 0x80) ? 1 : 0;	/* 1000 0000 */
      level[m] <<= 1;
      inbits++;
      if(inbits == 8)
      {
	m++;
	inbits = 0;
      }
    }
    if(i != YSIZE - 1)
      for(j = 0; j < XSIZE; j++)
      {
        area[i+1][j] = (level[m] & 0x80) ? 1 : 0;	/* 1000 0000 */
        level[m] <<= 1;
        inbits++;
        if(inbits == 8)
	{
	  m++;
	  inbits = 0;
	}
      }
    obuf[0] = 0;
    strcat(obuf, "\r\n");
    for(j = 0; j < XSIZE; j++)
      strcat(obuf, howprint(j, i));

    write(1, obuf, strlen(obuf));
  }

  fclose(in);
}

main(argc, argv)
int argc;
char **argv;
{
  struct termio termstruct, termold;
  time_t date;
  char *keyboard;
  FILE *pipe, *in1, *out1;
  unsigned int level = 0;
  unsigned int speed;
  struct stat tmpstat;
  int lives;

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

  stat(TMPFILE, &tmpstat);
  printf("I have a %d levels\n\n", (tmpstat.st_size)/LEVELSIZE);

/*
  printf("Level: ");
  scanf("%u", &level);
*/
  level = 0;

  printf("Speed: ");
  scanf("%u", &speed);

  ioctl(0, TCGETA, &termold);
  termstruct = termold;
  termstruct.c_lflag &= 0;
  termstruct.c_iflag &= ~(ICRNL | IGNCR | INLCR);
  termstruct.c_oflag &= 0;
  termstruct.c_cc[VMIN] = 0;
  termstruct.c_cc[VTIME] = 0;
  ioctl(0, TCSETA, &termstruct);

  score = 0;

  lives = 8;

loop:
  tt.tv_usec = 800000-(speed*10000);

  keyboard = argc > 2 ? argv[2] : "4685pq";

  tailrest = xdirect = tail = 0;
  ydirect = -1;
  head = 2;
  length = 3;
  x = 5;
  y = YSIZE - 5;
  appleslost = 10;
  lives += 2;

  setbuf(stdout, NULL);
  write(1, "\033[H\033[J", 6);	/* cls */

  show(level);

  worm[0][0] =  x;
  worm[0][1] =  y - 2;
  worm[1][0] =  x;
  worm[1][1] =  y - 1;
  worm[2][0] =  x;
  worm[2][1] =  y;

  while(area[(apple[1] = rand()%YSIZE)][(apple[0] = rand()%XSIZE)]);
  area[apple[1]][apple[0]] = 10;

  while((c = getchr()) == -1);

  while(1)
  {
    sprintf(obuf, "\033[2;4HScore: %5d\tLevel: %d\tLength: %d\tApples: %2d\tLives %d",
				score, level, length, appleslost, lives);
    write(1, obuf, strlen(obuf));

    if(!Move(x, y))
    {
      lives --;
      if(lives)
      {
        lives -= 2;
        goto loop;
      }
      c = QUITKEY;
    }
    else
    {
      if(!head && !tail)	/* to next level */
      {
        level ++;
        goto loop;
      }

      select(1, 1, 0, 0, &tt);
      c = getchr();
      score++;
    }

	if( c == LEFTKEY )	/* left */
	{
		xdirect = -1;
		ydirect = 0;
	}

	if( c == RIGHTKEY )  /* right */
	{
		xdirect = 1;
		ydirect = 0;
	}

	if( c == UPKEY )  /* up */
	{
		xdirect = 0;
		ydirect = -1;
	}

	if( c == DOWNKEY )  /* down */
	{
		xdirect = 0;
		ydirect = 1;
	}

	if( c == PAUSEKEY )	/* pause */
	  while( getchr() - PAUSEKEY );

	if ( c == QUITKEY )	/* exit */
	{
	  sprintf(obuf,"\033[23;30HAnother game ? (Y/N)");
	  write(1, obuf, strlen(obuf));
	  while((c = getchr()) <= 0 );

	  pipe = fopen("HI_CH", "a");

	  date = time(0);
	  fprintf(pipe,"%4d from level %2d length %5d by %s at   %s",
		score, level, length, getlogin(), asctime(gmtime(&date)));
	  fclose(pipe);
	  if( c != 'n' && c != 'N' && c != 'q' && c != 'Q' )
	  {
	    c = 0;
	    lives = 8;
	    score = 0;
	    goto loop;
	  }
	  break;
	}
  }

  ioctl(0, TCSETA, &termold);
  pipe = popen("clear;sort -nr -o HI_CH  HI_CH; head HI_CH", "w");
  pclose(pipe);

  remove(TMPFILE);
}

void *clean()
{
  remove(TMPFILE);

  printf("\n\n\rInterrupted\n");
  exit(1);
}
