// ARGTEST6.C (Windows Console program)
// Command Line Arguments Test 6
// Erdogan Tan - 11/11/2024
// (for TCC ..) 
// -getting cmd arguments without 'getopt' function-.
// Modification: 12/11/2024

#include <stdio.h>

const char * output_file;
const char * list_file;

const int input_files[16];

main(int argc, char *argv[])
{
     int i;
     int j = 0;
     int k = 0;
     int l = 0;
     int x;
     int m;

     char c;
     char opt = 'i'; // input
     char * optarg;

     if (argc<2) exit(1);

     output_file = malloc(128);
     list_file = malloc(128);

     x = 16;
     if (argc<16) x = argc-1;    
      
     for (i=0; i<x; i++)
         input_files[i] = malloc(128);
     
  for (i=1; i<argc; i++)
  {
    if (opt == '-') opt = 'i';
    if (c = argv[i][0] == '-') // option
      {
	opt = '-';
	c = argv[i][1]; 
        if (c == 'o' && k == 0) {
           opt = 'o';
           optarg = argv[i];
           optarg += 2;
           if ((c = *optarg) > 32) {
              strncpy(output_file, optarg, 127);
	      printf("Output file: %s\n", output_file);
              k++;
              opt = 'i';
            }  // 12/11/2024
          } else if (c == 'l' && l == 0) {
                   optarg = argv[i];
                   optarg += 2;
		   if ((c = *optarg) > 32) {
                      strncpy(list_file, optarg, 127);
                      printf("List file: %s\n", list_file);
                      l = i;
		      if (j > 0 && l > j) m = 1;
                      opt = 'i';
                   }
                   else
                      opt = 'l';
             }

       } else if (opt == 'i' && k > 0 && j == 0) {
               j = i;
            } else if (opt == 'o') {
                    strncpy(output_file, argv[i], 127);
                    printf("Output file: %s\n", output_file);
                    k++;
                    opt = 'i';
                } else if (opt == 'l') {
                     l = i;
		     if (j > 0 && l > j) m = 2;
                     opt = 'i';
                     strncpy(list_file, argv[l], 127);
                     printf("List file: %s\n", list_file);
                   }
  }
   if (opt == '-') argc = argc-1;

  if (j) {
      if (l>j) {  // xchg list file ptr with the 1st input file ptr 
          i = argv[l];    
          for (x=l; x>=j+m; x--) argv[x] = argv[x-m];
          l = j;	   // -llistfile	
	  if (m == 2) ++l; // -l listfile (not necessary)	 
          argv [l] = i;
          j = j+m;    	         
     
         }
     for (i=j; i<argc; i++)
     {
      optarg = &input_files[j];
      strcpy(optarg, argv[i]);
      printf("Input file %d: %s\n", i-j+1, optarg);
     }
   }

exit(0);
}
