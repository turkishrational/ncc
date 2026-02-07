// ARGTEST2.C (Windows Console program)
// Command Line Arguments Test 2
// Erdogan Tan - 09/11/2024
// (for TCC ..) 
// -getting cmd arguments without 'getopt' function-.

#include <stdio.h>

char * first_argument;
char * second_argument;

main(int argc, char *argv[])
{
     int i;
     int k = 0;
     int l = 0;
     int m;
 
  for (i=1; i<argc; i++)
  {
    if (argv[i] == 0) break;
    if (k == 0) {
       m = strlen(argv[i])+1;
       first_argument = malloc(m);
       strcpy(first_argument, argv[i]); 
       printf("first_argument's malloc address: %d, len: %d\n", first_argument, m);
       k = 1;
       if (l == 1) break;
    } else {
        if (l == 0) {
           m = strlen(argv[i])+1;
           second_argument = malloc(m);
           strcpy(second_argument, argv[i]);
	   printf("second_argument's malloc address: %d, len: %d\n", second_argument, m);
           l = 1;
           if (k == 1) break;
        }
      }
  }
  printf("\n");
  if (k = 1)
     printf("first_argument: '%s'\n", first_argument);  
  if (l = 1)
     printf("second_argument: '%s'\n", second_argument); 
exit(0);
}