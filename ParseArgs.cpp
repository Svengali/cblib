#include "ParseArgs.h"
#include "tweakvar.h"
#include "log.h"
#include "strutil.h"
#include "ctype.h"

START_CB

void ParseArgs_ShowHelp()
{
	lprintf("--lN : set log name to N\n");
	lprintf("--tV=X : set tweak var V to X\n");
	lprintf("--q : quiet (--qq very quiet)\n");
	lprintf("--v : verbose (--vv very verbose)\n");
}

bool ParseArgs(int argc,const char **argv,
		ParsedArgs * pResult)
{
	for (int argi=1;argi<argc;argi++)
	{
		if ( argv[argi][0] == '-' || argv[argi][0] == '/' )
		{
			if ( argv[argi][1] == '-' || argv[argi][1] == '/' )
			{
				// --
				char option = (char) tolower(argv[argi][2]);
				if ( option == '?' || option == 'h' )
					return true;
				
				const char * optData = &argv[argi][3];
				if ( *optData == '=' ) optData++;
				if ( strend(argv[argi])[-1] == '=' )
				{
					argi++;
					if ( argi >= argc ) return true;
					optData = argv[argi];
				}
				
				switch(option)
				{
				case 'l':
					LogOpen(optData,NULL,true);
					lprintf("got arg : log name : %s\n",optData);
					break;
				
				case 'q':
					LogSetVerboseLevel(0);
					if ( tolower(*optData) == 'q' )
					{
						LogSetState(0);
						lprintf("got arg : very quiet\n");
					}
					else
					{
						LogSetState(CB_LOG_TO_FILE|CB_LOG_TO_DEBUGGER);
						lprintf("got arg : quiet\n");
					}
					break;
					
				case 'v':
					LogSetVerboseLevel(2);
					lprintf("got arg : verbose\n");
					break;				
				
				case 't': // set tweak var					
					if ( strisame(optData,"?") || strisame(optData,"h") )
					{
						lprintf("got option : tweakvar log\n");
						TweakVar_LogAll();
					}
					else
					{
						const char * pVar = optData;
						const char * pVal = NULL;
						const char * pEq = strchr(optData,'=');
						if ( pEq )
						{
							*(const_cast<char *>(pEq)) = 0;
							pVal = pEq+1;
						}
						else
						{
							argi++;
							if ( argi >= argc ) return true;
							pVal = argv[argi];
						}
						lprintf("got option : set '%s' = '%s' : ",pVar,pVal);
						if ( TweakVar_SetFromText(pVar,pVal) )
							lprintf("ok.\n");
						else
							lprintf("not found.\n");
					}
					break;
				
				default:
					lprintf("unknown arg : %s\n",argv[argi]);
					break;
				}
			}
			else
			{
				// -
				char option = (char) tolower(argv[argi][1]);
				if ( option == '?' || option == 'h' )
					return true;
					
				const char * optData = &argv[argi][2];
				if ( *optData == '=' ) optData++;
				if ( strend(argv[argi])[-1] == '=' )
				{
					argi++;
					if ( argi >= argc ) return true;
					optData = argv[argi];
				}
				
				pResult->options.push_back(option);
				pResult->optionData.push_back(optData);
			}
		}
		else
		{
			pResult->arguments.push_back(argv[argi]);
		}
	}
	
	return false;
}

END_CB
