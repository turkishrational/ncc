/******************************************************************************
 * @file            lib.c
 *****************************************************************************/
#include    <ctype.h>
#include    <errno.h>
#include    <limits.h>
#include    <stddef.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>

#include    "ld.h"
#include    "lib.h"
#include    "report.h"

struct option {

    const char *name;
    int index, flags;

};

#define     OPTION_NO_ARG               0x0001
#define     OPTION_HAS_ARG              0x0002

enum options {

    OPTION_IGNORED = 0,
    OPTION_EMULATION,
    OPTION_ENTRY,
    OPTION_FORMAT,
    OPTION_HELP,
    OPTION_IMPURE,
    OPTION_INC_BSS,
    OPTION_MAP,
    OPTION_MAPFILE,
    OPTION_OUTFILE,
    OPTION_STACK,
    OPTION_STRIP,
    OPTION_TEXT

};

static struct option opts[] = {

    { "M",              OPTION_MAP,         OPTION_NO_ARG   },
    { "Map",            OPTION_MAPFILE,     OPTION_HAS_ARG  },
    { "N",              OPTION_IMPURE,      OPTION_NO_ARG   },
    { "Ttext",          OPTION_TEXT,        OPTION_HAS_ARG  },
    
    { "e",              OPTION_ENTRY,       OPTION_HAS_ARG  },
    { "m",              OPTION_EMULATION,   OPTION_HAS_ARG  },
    { "o",              OPTION_OUTFILE,     OPTION_HAS_ARG  },
    { "s",              OPTION_STRIP,       OPTION_NO_ARG   },
    
    { "-include-bss",   OPTION_INC_BSS,     OPTION_NO_ARG   },
    { "-oformat",       OPTION_FORMAT,      OPTION_HAS_ARG  },
    { "-stacksize",     OPTION_STACK,       OPTION_HAS_ARG  },
    { "-help",          OPTION_HELP,        OPTION_NO_ARG   },
    
    { 0,                0,                  0               }

};

static void print_help (void) {

    if (!program_name) {
        goto _exit;
    }
    
    fprintf (stderr, "Usage: %s [options] file...\n\n", program_name);
    fprintf (stderr, "Options:\n\n");
    
    fprintf (stderr, "    -M                    Print map file on standard out\n");
    fprintf (stderr, "    -Map FILE             Write a map file\n");
    fprintf (stderr, "    -N                    Do not page align data\n");
    fprintf (stderr, "    -Ttext OFFSET         Offset code by the specified offset\n");
    
    fprintf (stderr, "    -e ADDRESS            Set start address\n");
    fprintf (stderr, "    -m EMULATION          Set emulation\n");
    fprintf (stderr, "    -o FILE               Set output file name (default a.out)\n");
    fprintf (stderr, "    -s                    Strip all (currently does nothing)\n");
    
    fprintf (stderr, "    --include-bss         Include bss data in a flat binary\n");
    fprintf (stderr, "    --oformat FORMAT      Specify the format of output file (default msdos)\n");
    fprintf (stderr, "                              Supported formats are:\n");
    /*fprintf (stderr, "                                  a.out-i386, coff-i386, msdos-i386, pe-i386\n");*/
    fprintf (stderr, "                                  a.out-i386, binary, msdos, msdos-mz\n");
    fprintf (stderr, "    --stacksize SIZE      Specifies the size of the stack (default 4096)\n");
    fprintf (stderr, "    --help                Print this help information\n");
    
_exit:
    
    exit (EXIT_SUCCESS);

}

char *xstrdup (const char *str) {

    char *ptr = xmalloc (strlen (str) + 1);
    strcpy (ptr, str);
    
    return ptr;

}

int strstart (const char *val, const char **str) {

    const char *p = val;
    const char *q = *str;
    
    while (*p != '\0') {
    
        if (*p != *q) {
            return 0;
        }
        
        ++p;
        ++q;
    
    }
    
    *str = q;
    return 1;

}

int xstrcasecmp (const char *s1, const char *s2) {

    const unsigned char *p1;
    const unsigned char *p2;
    
    p1 = (const unsigned char *) s1;
    p2 = (const unsigned char *) s2;
    
    while (*p1 != '\0') {
    
        if (toupper (*p1) < toupper (*p2)) {
            return (-1);
        } else if (toupper (*p1) > toupper (*p2)) {
            return (1);
        }
        
        p1++;
        p2++;
    
    }
    
    if (*p2 == '\0') {
        return (0);
    } else {
        return (-1);
    }

}

void *xmalloc (unsigned long size) {

    void *ptr = malloc (size);
    
    if (ptr == NULL && size) {
    
        report_at (program_name, 0, REPORT_ERROR, "memory full (malloc)");
        exit (EXIT_FAILURE);
    
    }
    
    memset (ptr, 0, size);
    return ptr;

}

void *xrealloc (void *ptr, unsigned long size) {

    void *new_ptr = realloc (ptr, size);
    
    if (new_ptr == NULL && size) {
    
        report_at (program_name, 0, REPORT_ERROR, "memory full (realloc)");
        exit (EXIT_FAILURE);
    
    }
    
    return new_ptr;

}

void dynarray_add (void *ptab, long *nb_ptr, void *data) {

    int32_t nb, nb_alloc;
    void **pp;
    
    nb = *nb_ptr;
    pp = *(void ***) ptab;
    
    if ((nb & (nb - 1)) == 0) {
    
        if (!nb) {
            nb_alloc = 1;
        } else {
            nb_alloc = nb * 2;
        }
        
        pp = xrealloc (pp, nb_alloc * sizeof (void *));
        *(void ***) ptab = pp;
    
    }
    
    pp[nb++] = data;
    *nb_ptr = nb;

}

void parse_args (int *pargc, char ***pargv, int optind) {

    char **argv = *pargv;
    int argc = *pargc;
    
    struct option *popt;
    const char *optarg, *r;
    
    if (argc == optind) {
        print_help ();
    }
    
    while (optind < argc) {
    
        r = argv[optind++];
        
        if (r[0] != '-' || r[1] == '\0') {
        
            dynarray_add (&state->files, &state->nb_files, xstrdup (r));
            continue;
        
        }
        
        for (popt = opts; popt; ++popt) {
        
            const char *p1 = popt->name;
            const char *r1 = (r + 1);
            
            if (!p1) {
            
                report_at (program_name, 0, REPORT_ERROR, "invalid option -- '%s'", r);
                exit (EXIT_FAILURE);
            
            }
            
            if (!strstart (p1, &r1)) {
                continue;
            }
            
            optarg = r1;
            
            if (popt->flags & OPTION_HAS_ARG) {
            
                if (*optarg == '\0') {
                
                    if (optind >= argc) {
                    
                        report_at (program_name, 0, REPORT_ERROR, "argument to '%s' is missing", r);
                        exit (EXIT_FAILURE);
                    
                    }
                    
                    optarg = argv[optind++];
                
                }
            
            } else if (*optarg != '\0') {
                continue;
            }
            
            break;
        
        }
        
        switch (popt->index) {
        
            case OPTION_EMULATION: {
            
                if (xstrcasecmp (optarg, "a.out_i386") == 0) {
                
                    state->flat_bin = 1;
                    break;
                
                }
                
                report_at (program_name, 0, REPORT_ERROR, "'%s' is not a valid emulation option", optarg);
                exit (EXIT_FAILURE);
            
            }
            
            case OPTION_ENTRY: {
            
                state->entry = xstrdup (optarg);
                break;
            
            }
            
            case OPTION_FORMAT: {
            
                if (xstrcasecmp (optarg, "a.out-i386") == 0) {
                
                    state->format = LD_FORMAT_I386_AOUT;
                    break;
                
                }/* else if (xstrcasecmp (optarg, "coff-i386") == 0) {
                
                    state->format = LD_FORMAT_I386_COFF;
                    break;
                
                } else if (xstrcasecmp (optarg, "pe-i386") == 0) {
                
                    state->format = LD_FORMAT_I386_PE;
                    break;
                
                }*/ else if (xstrcasecmp (optarg, "binary") == 0) {
                
                    state->format = LD_FORMAT_BINARY;
                    break;
                
                } else if (xstrcasecmp (optarg, "msdos") == 0) {
                
                    state->format = LD_FORMAT_MSDOS;
                    break;
                
                } else if (xstrcasecmp (optarg, "msdos-mz") == 0) {
                
                    state->format = LD_FORMAT_MSDOS_MZ;
                    break;
                
                } else {
                
                    report_at (program_name, 0, REPORT_ERROR, "unsupported format '%s' specified", optarg);
                    exit (EXIT_FAILURE);
                
                }
            
            }
            
            case OPTION_HELP: {
            
                print_help ();
                break;
            
            }
            
            case OPTION_IMPURE: {
            
                state->impure = 1;
                break;
            
            }
            
            case OPTION_INC_BSS: {
            
                state->include_bss = 1;
                break;
            
            }
            
            case OPTION_MAP: {
            
                if (state->mapfile) {
                
                    report_at (program_name, 0, REPORT_ERROR, "multiple map files provided");
                    exit (EXIT_FAILURE);
                
                }
                
                state->mapfile = "";
                break;
            
            }
            
            case OPTION_MAPFILE: {
            
                if (state->mapfile) {
                
                    report_at (program_name, 0, REPORT_ERROR, "multiple map files provided");
                    exit (EXIT_FAILURE);
                
                }
                
                state->mapfile = xstrdup (optarg);
                break;
            
            }
            
            case OPTION_OUTFILE: {
            
                if (state->outfile) {
                
                    report_at (program_name, 0, REPORT_ERROR, "multiple output files provided");
                    exit (EXIT_FAILURE);
                
                }
                
                state->outfile = xstrdup (optarg);
                break;
            
            }
            
            case OPTION_STACK: {
            
                long conversion;
                char *temp;
                
                errno = 0;
                conversion = strtol (optarg, &temp, 0);
                
                if (!*optarg || isspace ((int) *optarg) || errno || *temp) {
                
                    report_at (program_name, 0, REPORT_ERROR, "bad number of stack size");
                    exit (EXIT_FAILURE);
                
                }
                
                if (conversion < 0 || conversion > USHRT_MAX) {
                
                    report_at (program_name, 0, REPORT_ERROR, "stack size must be between 0 and %u", USHRT_MAX);
                    exit (EXIT_FAILURE);
                
                }
                
                state->stack_size = (unsigned long ) conversion;
                break;
            
            }
            
            case OPTION_STRIP: {
                break;
            }
            
            case OPTION_TEXT: {
            
                long conversion;
                char *temp;
                
                errno = 0;
                conversion = strtol (optarg, &temp, 0);
                
                if (!*optarg || isspace ((int) *optarg) || errno || *temp) {
                
                    report_at (program_name, 0, REPORT_ERROR, "bad number for text start");
                    exit (EXIT_FAILURE);
                
                }
                
                if (conversion < 0 || conversion > LONG_MAX) {
                
                    report_at (program_name, 0, REPORT_ERROR, "text start must be between 0 and %u", ULONG_MAX);
                    exit (EXIT_FAILURE);
                
                }
                
                state->code_offset = (unsigned long) conversion;
                break;
            
            }
            
            default: {
            
                report_at (program_name, 0, REPORT_ERROR, "unsupported option '%s'", r);
                exit (EXIT_FAILURE);
            
            }
        
        }
    
    }
    
#ifdef      __MSDOS__
    if (!state->format) { state->format = LD_FORMAT_MSDOS_MZ; }
#else
    if (!state->format) { state->format = LD_FORMAT_I386_AOUT; }
#endif
    
    if (!state->outfile) { state->outfile = "a.out"; }
    if (!state->stack_size) { state->stack_size = 4096; }

}
