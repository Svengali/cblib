#pragma once

#include "vector.h"
#include <String.h>

/**

ParseArgs : does normal arg extraction

- options are for your app

 -cN
 -c=N
 -c= N
 
 all handled
 
 (-c N is not handled because I can't tell if N
  goes with c or is another argument)

-- options are for the system and are handled here

--lN : set log name to N
--tV=X : set tweak var V to X
--q : quiet (--qq very quiet)
--v : verbose (--vv very verbose)

**/

START_CB

struct ParsedArgs
{
	cb::vector<const char *>	arguments;
	cb::vector<char>			options;	// lower case
	cb::vector<const char *>	optionData;
};

void ParseArgs_ShowHelp();

// returns true if help was requested (with -?)
//	you should also check if arguments are empty, eg :
//	if ( ParseArgs(argc,argv,&args) || args.arguments.empty() )
//	{
//		lprintf("StringExtractor [-opts] <from1> [from2] ..\n");
bool ParseArgs(int argc,const char **argv,
		ParsedArgs * pResult);
		
END_CB
