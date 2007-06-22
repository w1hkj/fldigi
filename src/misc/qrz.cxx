#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "qrzlib.h"

QRZ *qCall;

void DumpRecord()
{
  printf("%s\n", qCall->CSV_Record() );
}

void ShowCall()
{
  printf("%s\n", qCall->Fmt_Record() );
}

void Usage()
{
  printf("Usage: qrz call   [args]\n" );
  printf("       qrz name   n[args]\n");
  printf("       qrz ST     s[args]\n");
  printf("       qrz STcity s[args]\n");
  printf("       qrz ZIP    z[args]<n>\n");
  printf("  where args may be:\n" );
  printf("     c   - count\n" );
  printf("     cd  - count & dump all matching records\n" );
  printf("     NNN - dump NNN number of records\n" );
  exit(0);
}

int main( int argc, char *argv[] )
{
  char unknown[64];
  int  matched = 0;
  int  extras = 0, docount = 0, dodump  = 0;
  long count = 0;

  memset(unknown, 0, 64 );

  if( argc < 2 ) 
    Usage();
  
  if( argc == 4 )
    sscanf( argv[3], "%d", &extras );
    
  if ( argc == 2)
    qCall = new QRZ( "CALLBKC");
  else {
    switch (argv[2][0]) {
      case 'c' :
        qCall = new QRZ( "CALLBKC" );
        break;
      case 'n' :
        qCall = new QRZ( "CALLBKN" );
        break;
      case 's' :
        qCall = new QRZ( "CALLBKS" );
        break;
      case 'z' :
        qCall = new QRZ( "CALLBKZ" );
        break;
      default :
        Usage();
    }
    if( strlen(argv[2]) > 1 ) {
      if( argv[2][1] == 'C' || argv[2][1] == 'c' )
        // user wants count of all matches
        docount = 1;
      if( argv[2][2] == 'd' || argv[2][2] == 'D' )
        // user wants all found dumped to stdio (comma delimited)
        dodump = 1;
    }
  }  
  

  if( qCall->getQRZvalid() == 0 ) {
    printf("could not open %s\n", "QRZ database" );
    exit(0);
  }
  
  strcpy( unknown, argv[1] );

  for( unsigned int i = 0; i < strlen(unknown); i++ )
    unknown[i] = toupper( unknown[i] );

  if( docount ) {
    if( qCall->FindRecord( unknown ) != 1 )
      exit(0);
    matched = 0;
    while (matched == 0) {
      if( dodump )
        DumpRecord();
      count++;
      qCall->NextRecord();
      switch (argv[2][0]) {
        case 'c' :
          matched = qCall->CallComp( Composite(unknown), qCall->GetCall() );
          break;
        case 'n' :
          matched = strcasecmp( unknown, qCall->GetLname() );
          break;
        case 'z' :
          matched = strncmp( unknown, qCall->GetZIP(), 5 );
          break;
        case 's' :
          matched = qCall->CompState( unknown, 
                                      qCall->GetState(),
                                      qCall->GetCity() );
          break;
        default  :
          Usage();
      }
    }
    printf("Count of records matching %s, = %ld\n", unknown, count );
    exit(0);
  }
 
  if( (matched = qCall->FindRecord( unknown )) == 1 ) {
    if( extras )
      DumpRecord();
    else
      ShowCall();
  }
  else
    if( matched == -1 ) {
      printf("%s => %s\n", unknown, qCall->GetCall() );
      strcpy( unknown, qCall->GetCall() );
      qCall->FindRecord( unknown );
      ShowCall();
    }
    else
      printf("NOT FOUND\n");

  if( extras ) {
    for( int i = 1; i < extras; i++) {
      if( qCall->NextRecord() )
        DumpRecord();
    }
    printf("\n");
  }
  return(0);
}

