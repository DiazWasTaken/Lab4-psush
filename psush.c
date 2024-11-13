//Brian Cabrera Diaz

/*

11/12 - No real goals for today, just get makefile done and copy over the example and work for a bit on it at the code party

*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/param.h>

#include "cmd_parse.h"

// Chaney says this is a global used for good
/*
im assuming the reason this is a global is due to the fact that verbose is 
done everywhere in a program and having to manage that everywhere in the
program would be actual asinine 
*/
extern unsigned short is_verbose;

int main( int argc, char *argv[] )
{
    int ret = 0;
    simple_argv(argc, argv);
    ret = process_user_input_simple();

    return ret;
}