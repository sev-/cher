#include <stdio.h>

main()
{
  char level[30][40];
  int i=1, j;
  FILE *in;

  in=fopen("screen.cut", "r");
  fgets(level[1], 28, in);
  level[1][strlen(level[i])-1] = 0;
  printf("%-29s\n", level[1]);

  while(i = 0 || level[i-1][0] != 'E')
  {
    do
    {
      fgets(level[i], 35, in);
      level[i][strlen(level[i])-1] = 0;
    }
    while(level[i][0] != 'L' && level[i++][0] != 'E');

    for(j = 0; j < i; j++)
      printf("%-29s\n", level[j]);
    for(;j < 21; j++)
      printf("%-29s\n", "   ");
    fputs(level[i], stdout);
  }
  return 1;
}

