#include <stdio.h>

main()
{
  FILE *in;
  int i, j;
  int curr;
  int tmpseek;
  unsigned short tmpseek1;
  char commands[2048];

  if((in = fopen("track", "r")) == (FILE *)NULL)
  {
    printf("Нет файла track !!\n");
    exit(0);
  }

  do
  {
    printf("Level: ");
    scanf("%d", &curr);

    fread(&tmpseek1, sizeof(short), 1, in);
    tmpseek = 0;
    for(i = 0; i < curr; i ++)
    {
      tmpseek += tmpseek1;
      fseek(in, tmpseek, SEEK_SET);
      fread(&tmpseek1, sizeof(short), 1, in);
    }
    tmpseek1 -= sizeof(short);
    fread(commands, 1, tmpseek1, in);

    printf("Moves: %d\n", tmpseek1*4);

    for(j = 0; j < tmpseek1; j++)
    {
      for(i = 0; i < 4; i++)
      {
        switch((commands[j] & 0xc0) >> 6)
        {
          case 0:
		printf("Left ");
		break;
          case 1:
		printf("Right ");
		break;
          case 2:
		printf("Up ");
		break;
          case 3:
		printf("Down ");
		break;
        }
        commands[j] <<= 2;
      }
    }
    printf("\n");
  } while(curr < 50);
}
