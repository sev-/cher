#include <stdio.h>
#include <signal.h>
#include <termio.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>

#define	LEFTKEY		keyboard[0]
#define	RIGHTKEY	keyboard[1]
#define	UPKEY		keyboard[2]
#define	DOWNKEY		keyboard[3]
#define	QUITKEY		keyboard[4]

#define EMPTY		0
#define WALL		1
#define CONTAINER	0x14
#define PLACE		3
#define PLACE_CONT	(PLACE+CONTAINER)

#define XSIZE 		19
#define YSIZE 		16
#define LEVELSIZE	(XSIZE*YSIZE+2)
#define TMPFILE		"sok$$$.stg"

long find_begin_trace();

int level[YSIZE][XSIZE];
int x, y;
char obuf[1024];
unsigned short move;
unsigned int containers_left;	/* Число контейнеров не на месте */
char commands[2048];	/* tasha blit */

getchr()
{
  unsigned char xc[1];

  if( read(0,xc,1) != 1 )
	return -1;
  return xc[0];
}

add_command(xdirect, ydirect)
int xdirect, ydirect;
{
  int numbyte = move / 4;
  int numtw = 3 - move % 4;
  char direct;

  if(xdirect == -1)
    direct = 0;
  if(xdirect == 1)
    direct = 1;
  if(ydirect == -1)
    direct = 2;
  if(ydirect == 1)
    direct = 3;

  commands[numbyte] |= direct << (numtw << 1);
}

get_command()
{
  int numbyte = move / 4;
  int numtw = 3 - move % 4;
  struct timeval tt;
  tt.tv_usec = 500000;

  select(1, 1, 0, 0, &tt);

  return (commands[numbyte] & (3 << (numtw+numtw))) >> (numtw+numtw);
}

char *howprint(x1, y1)
int x1, y1;
{
  if(x1 == x && y1 == y)
    return "┌┐";
  switch(level[y1][x1])
  {
    case EMPTY:	/* пусто */
	return "  ";
    case WALL: /* стена */
	return "██";
    case PLACE: /* место */
	return "<>";
    case CONTAINER: /* контейнер */
	return "[]";
    case PLACE_CONT: /* стоит */
	return "░░";
  }
  return "??";
}

redraw(x1, y1)
int x1, y1;
{
  sprintf(obuf, "\033[%02d;%02dH%s", y1+2, x1*2+1, howprint(x1, y1));

  write(1, obuf, strlen(obuf));
}  

Move(xdirect, ydirect)
int xdirect, ydirect;
{
  int move1 = move;

  switch(level[y+ydirect][x+xdirect])
  {
    case EMPTY:	/* пусто */
    case PLACE: /* место */
	y += ydirect;
	x += xdirect;
	move ++;
	break;
    case WALL: /* стена */
	break;
    case CONTAINER: /* контейнер */
    case PLACE_CONT: /* стоит */
	switch(level[y+ydirect+ydirect][x+xdirect+xdirect])
	{
	  case EMPTY:  /* пусто */
	  	if(level[y+ydirect][x+xdirect] == PLACE_CONT)
			containers_left ++;
	  case PLACE: /* место */
		if(level[y+ydirect][x+xdirect] == CONTAINER &&
			level[y+ydirect+ydirect][x+xdirect+xdirect] != EMPTY)
		  containers_left --;
		level[y+ydirect+ydirect][x+xdirect+xdirect] += CONTAINER;
		level[y+ydirect][x+xdirect] -= CONTAINER;
		y += ydirect;
		x += xdirect;
		move ++;
		redraw(x+xdirect, y+ydirect);
		break;
	}
	break;
  }
  if(move1 != move)
  {
    redraw(x, y);
    redraw(x-xdirect, y-ydirect);
    move --;
    add_command(xdirect, ydirect);
    move ++;
  }
}

show(number)
int number;
{
  FILE *in;
  int i, j;
  int doker;

  in = fopen(TMPFILE, "r");

  fseek(in, number*LEVELSIZE, SEEK_SET);

  doker = fgetc(in);
  doker |= fgetc(in) << 8;
  containers_left = 0;

  for(i = 0; i < YSIZE; i++)
  {
    obuf[0] = 0;
    strcat(obuf, "\r\n");
    for(j = 0; j < XSIZE; j++, doker--)
    {
      level[i][j] = fgetc(in);
      if(!doker)
      {
        x = j;
        y = i;
      }
      strcat(obuf, howprint(j, i));
      if(level[i][j] == CONTAINER)
        containers_left ++;
    }
    write(1, obuf, strlen(obuf));
  }
  fclose(in);
}

main(argc, argv)
int argc;
char *argv[];
{
  struct termio termstruct, termold;
  char *keyboard;
  FILE *in1, *out1;
  int curr = 0;
  struct stat tmpstat;
  int c;
  int i;

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

  printf("Level: ");
  scanf("%d", &curr);

  ioctl(0, TCGETA, &termold);
  termstruct = termold;
  termstruct.c_lflag &= 0;
  termstruct.c_iflag &= ~(ICRNL | IGNCR | INLCR);
  termstruct.c_oflag &= 0;
  termstruct.c_cc[VMIN] = 1;
  termstruct.c_cc[VTIME] = 0;
  ioctl(0, TCSETA, &termstruct);

  keyboard = argc > 2 ? argv[2] : "4685q";

  for(i = 0; i < sizeof(commands); i ++)
    commands[i] = 0;

  while(1)
  {
    move = 0;

    setbuf(stdout, NULL);
    write(1, "\033[H\033[J", 6);	/* cls */

    show(curr);

    while(containers_left)
    {
      sprintf(obuf, "\033[2;4HMove: %5d\tLevel: %d", move, curr);
      write(1, obuf, strlen(obuf));

      c = getchr();

      if( c == LEFTKEY )	/* left */
	Move(-1, 0);

      if( c == RIGHTKEY )  /* right */
	Move(1, 0);

      if( c == UPKEY )  /* up */
	Move(0, -1);

      if( c == DOWNKEY )  /* down */
	Move(0, 1);

      if( c == 'p' )
      {
        play(curr, &termstruct);
      }

      if ( c == QUITKEY )	/* exit */
      {
	ioctl(0, TCSETA, &termold);
	remove(TMPFILE);
	return 0;
      }
    } /* while containers */
/*    write_trace(curr);
*/    curr++;
  } /* while 1 */
}

void *clean()
{
  remove(TMPFILE);

  printf("\n\n\rInterrupted\n");
  exit(1);
}

play(curr, termstruct1)
int curr;
struct termio *termstruct1;
{
  long tmpseek;
  FILE *in;
  int move1;

  tmpseek = find_begin_trace(curr);


  if(tmpseek == -1)
    return;
  in = fopen("track", "r");

  fseek(in, tmpseek, SEEK_SET);

  fread(commands, 1, tmpseek - sizeof(short), in);
  fclose(in);

  termstruct1->c_cc[VMIN] = 0;
  ioctl(0, TCSETA, termstruct1);

  do
  {
move1 = move;
    switch(get_command())
    {
      case 0:
	Move(-1, 0);
	break;
      case 1:
	Move(1, 0);
	break;
      case 2:
	Move(0, -1);
	break;
      case 3:
	Move(0, 1);
	break;
    }
if(move1==move) move++;
  } while(getchr() == -1);
  termstruct1->c_cc[VMIN] = 1;
  ioctl(0, TCSETA, termstruct1);
}

write_trace(curr)
int curr;
{
/*        out1 = fopen("track", "r+");
      tmpseek = 0;
      for(i = 0; i < curr - 1; i ++)
      {
        fread(&tmpseek1, sizeof(short), 1, out1);
        tmpseek += tmpseek1;
        fseek(out1, tmpseek, SEEK_SET);
      }
      if(feof(out1) || tmpseek1 - sizeof(short) > (move/4 + 1))
      {
        tmpseek1 = move/4 +1 + sizeof(short);
        fwrite(&tmpseek1, sizeof(short), 1, out1);
        fwrite(commands, 1, move / 4 + 1, out1);
        fclose(out1);
      }
    }
*/
}

long find_begin_trace(curr)
int curr;
{
  FILE *in;
  long tmpseek;
  unsigned short tmpseek1;
  int i;
  
  if((in = fopen("track", "r")) == (FILE *)NULL)
    return -1l;

  tmpseek = 0;
  for(i = 0; !feof(in) && i < curr - 1; i ++)
  {
    fread(&tmpseek1, sizeof(short), 1, in);
    tmpseek += tmpseek1;
    fseek(in, tmpseek, SEEK_SET);
  }

  fread(&tmpseek1, sizeof(short), 1, in);
  if(feof(in))
    tmpseek = -1;
  else
    tmpseek += sizeof(short);
  fclose(in);

  return tmpseek;
}
