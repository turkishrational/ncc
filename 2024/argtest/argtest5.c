// ARGTEST5.C (Windows Console program)
// Command Line Arguments Test 5
// Erdogan Tan - 10/11/2024
// (for TCC ..) 
// -getting cmd arguments without 'getopt' function-.
// Modification: 11/11/2024

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
     int m;
     int p = 0;
     int x;

     char c;
     char * text;

     if (argc<2) exit(0);

     output_file = malloc(128);
     list_file = malloc(128);

     x = 16;
     if (argc<16) x = argc-1;    
      
     for (i=0; i<x; i++)
         input_files[i] = malloc(128);
     
  for (i=1; i<argc; i++)
  {
    if (c = argv[i][0] == '-')  // option
      {
	c = argv[i][1]; 
        if (c == 'o' & k == 0) {
           p = 1;
           text = argv[i];
           text += 2;
           if ((c = *text) > 32) {
              if (m = strlen(text) > 2)
               { 
                 strncpy(output_file, text, 127);
		 printf("Output file: %s\n", output_file);
                 k++;
                 if (l) break;
                 p = 0;
               }
            }
         } else if (c == 'l' && l == 0) {
                p = 2;
                text = argv[i];
                text += 2;
                if ((c = *text) > 32) {
                   if (m = strlen(text) > 2)
                    { 
                      strncpy(list_file, text, 127);
		      printf("List file: %s\n", list_file);
                      l++;
                      if (k) break;
                      p = 0;
                    }
                 }
           }
       } else if (p == 0 && k == 0 && l == 0 && j < x) { // 11/11/2024
               if (m = strlen(argv[i]) > 2) {
		 text = &input_files[j];
		 strcpy(text, argv[i]);
                 printf("Input file %d: %s\n", j+1, text);
                 j++;
               }
            } else if (p == 1) {
                   if (m = strlen(argv[i]) > 2)
                    { 
                      strncpy(output_file, argv[i], 127);
                      printf("Output file: %s\n", output_file);
                      k++;
                      if (l) break;
                      p = 0;
                    }
                } else if (p == 2) {
                       if (m = strlen(argv[i]) > 2)
                        { 
                          strncpy(list_file, argv[i], 127);
                          printf("List file: %s\n", list_file);
                          l++;
                          if (k) break;
                          p = 0;
                        }
                  }
  }
exit(0);
}