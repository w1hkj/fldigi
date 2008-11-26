//=============================================================================
// cabrillo.cpp
//
// cabrillo file writing function for the qso database
//

int  write_cabrillo(void)
{
	extern char backgrnd_str[];
	extern char logfile[];
	extern char call[];
	extern int cqww;
	extern int arrldx_usa;
	extern int other_flg;
	extern int wysiwyg_multi;
	extern int wysiwyg_once;
	extern int serial_grid4_mult;
	extern char mycqzone[];
	extern char exchange[];
	extern int cqwwm2;
	extern int arrlss;
	extern int wpx;
	extern char whichcontest[];

    char standardexchange[70] = "";
	char buf[181];
	char buffer[4000]= "";
	
	FILE *fp1,  *fp2;

	getsummary();
	
	if (strlen(exchange) > 0) strcpy (standardexchange, exchange);

	if  ( (fp1 = fopen(logfile,"r"))  == NULL){
			fprintf(stdout,  "Opening logfile not possible.\n");
			return(1);
		}
	if  ( (fp2 = fopen("./cabrillo","w"))  == NULL){
			fprintf(stdout,  "Opening cbr  file not possible.\n");
			return(2);
		}
	if (strlen(standardexchange) == 0) {
			nicebox (14,  0, 1, 78 , "Exchange used:");
			mvprintw(15, 1,  "                                                       ");
			mvprintw(15, 1,  "");
			attron(COLOR_PAIR(7) | A_STANDOUT);
			echo();
			if (arrlss == 1)
				getnstr(standardexchange,  6);
			else
				getnstr(standardexchange,  10);

			noecho();
	}

while ( !feof(fp1))
	{
		buf[0] = '\0';
		buffer[0]='\0';

		fgets (buf,  180,  fp1);

		if (buf[0] != ';' && strlen(buf) > 60)
		{

			buffer[0] = '\0';

			strcat(buffer, "QSO: ");
/*------------------------------------------------------------------
frequency
-------------------------------------------------------------------*/
			if  (buf[1]  == '6')
				strcat(buffer, " 1800");
			else  if (buf[1] ==  '8')
				strcat(buffer," 3500");
			else  if (buf[1] ==  '4')
				strcat(buffer," 7000");
			else  if (buf[1] ==  '2')
				strcat(buffer,"14000");
			else  if (buf[1] ==  '1' &&  buf[2] ==  '5')
				strcat(buffer,"21000");
			else  if (buf[1] ==  '1' &&  buf[2] ==  '0')
				strcat(buffer, "28000");

/*------------------------------------------------------------------
mode
-------------------------------------------------------------------*/

			if (buf[3] == 'C')
				strcat(buffer, " CW 20");
			else if (buf[3] == 'S')
				strcat(buffer, " PH 20");
			else
				strcat(buffer, " RY 20");

/*------------------------------------------------------------------
date
-------------------------------------------------------------------*/

			strncat(buffer, buf+14, 2); /* year */

			if (buf[10] == 'J' && buf[11] == 'a')
				strcat(buffer, "-01-");
			if (buf[10] == 'F')
				strcat(buffer, "-02-");
			if (buf[10] == 'M' && buf[12] == 'r')
				strcat(buffer, "-03-");
			if (buf[10] == 'A' && buf[12] == 'r')
				strcat(buffer, "-04-");
			if (buf[10] == 'M' && buf[12] == 'y')
				strcat(buffer, "-05-");
			if (buf[10] == 'J' && buf[11] == 'u' &&buf[12] == 'n')
				strcat(buffer, "-06-");
			if (buf[10] == 'J' && buf[12] == 'l')
				strcat(buffer, "-07-");
			if (buf[10] == 'A' && buf[12] == 'g')
				strcat(buffer, "-08-");
			if (buf[10] == 'S')
				strcat(buffer, "-09-");
			if (buf[10] == 'O')
				strcat(buffer, "-10-");
			if (buf[10] == 'N')
				strcat(buffer, "-11-");
			if (buf[10] == 'D')
				strcat(buffer, "-12-");
/*------------------------------------------------------------------
day
-------------------------------------------------------------------*/

			strncat(buffer, buf+7, 2);
/*------------------------------------------------------------------
time
-------------------------------------------------------------------*/

			strncat(buffer, buf+16, 3);
			strncat(buffer, buf+20, 3);
/*------------------------------------------------------------------
mycall
-------------------------------------------------------------------*/

			strncat(buffer, call, strlen(call) -1);         /* strip the \n */
			strncat(buffer, backgrnd_str, 15 - strlen(call));
/*------------------------------------------------------------------
exchange given
-------------------------------------------------------------------*/


			if (arrlss ==1)
			{
/*------------------------------------------------------------------
report given
-------------------------------------------------------------------*/

				sprintf(buffer + 41 , "%4d", atoi(buf+22));
				strcat(buffer, "                    ");
/*------------------------------------------------------------------
exchange given
-------------------------------------------------------------------*/
				sprintf(buffer + 46, "%c" , standardexchange[0]);
				strcat(buffer, "                    ");
				sprintf(buffer + 48, "%2d", atoi(standardexchange + 1));
				strcat(buffer, "                    ");
				sprintf(buffer + 51, "%s", standardexchange + 3);
				strcat(buffer, "                    ");
				sprintf(buffer + 55, "%s", buf + 29);
				strcat(buffer, "                    ");
/*------------------------------------------------------------------
exchange received
-------------------------------------------------------------------*/

				sprintf(buffer + 66, "%4d", atoi(buf+54));
				strcat(buffer, "                    ");
				sprintf(buffer + 71, "%c", buf[59]);
				strcat(buffer, "                    ");
				sprintf(buffer + 72, "%s", buf + 60);
				strcat(buffer, "                    ");
				sprintf(buffer + 75, "%s", buf + 63);
				strcat(buffer, "                    ");
				buffer[79] = '\0';
				strcat(buffer, "\n");

			}   else	// not arllss
			{
/*------------------------------------------------------------------
report given
-------------------------------------------------------------------*/


				if (buf[3] == 'S')
					strcat(buffer, "59  ");
				else
					strcat(buffer, "599 ");
/*------------------------------------------------------------------
exchange given
-------------------------------------------------------------------*/


				if (other_flg == 1|| wysiwyg_multi == 1 || wysiwyg_once == 1)
				{
					strcat(buffer, standardexchange);
					strncat(buffer, "            ", 7 - strlen(standardexchange));
				} /* end other (wysiwyg) */

				else if ((wpx == 1) || ((standardexchange[0] == '#')
					&& (strcmp (whichcontest, "ssa_mt") != 0)))
				{
						strncat(buffer, buf+23,4);
						strncat(buffer, standardexchange+1, 7);
						strcat (buffer, " ");
				}

				else if (cqww == 1){
					strcat(buffer, mycqzone);
					strcat(buffer, "     ");
				}

				else if (arrldx_usa == 1)
				{
					strncat(buffer, exchange, 2);
					strcat (buffer, "     ");

				}else if (serial_grid4_mult == 1) {
					strcat (buffer, "  ");
					sprintf(buffer + 49, "%s", buf+24);
					sprintf(buffer + 52, "%s", standardexchange + 1);

					strcat (buffer, "                ");
					sprintf(buffer + 60, "%s          ", buf+29);
					buffer[74] = '\0';
				}
				else
				{

					strncat(buffer, standardexchange, 10);
					strncat (buffer, "     ", strlen(buffer)-8);
				}

/*------------------------------------------------------------------
his call
-------------------------------------------------------------------*/

				if (strcmp (whichcontest, "ssa_mt") != 0) strncat(buffer, buf+29, 14);


/*------------------------------------------------------------------
rprt given
-------------------------------------------------------------------*/

				if (buf[3] == 'S')
					strcat(buffer, "59  ");
				else
					strcat(buffer, "599 ");

				if (serial_grid4_mult == 1) {
					char ssa_mt_exchange[30];
					int i=0, j=0, k = 0;
//					strncat(buffer, buf+54, 9);  // tbf for all contests? RC
					strcat(buffer, "                      ");

					sprintf(buffer + 79, "%03d   ", atoi(buf+54));

					for (i=0; i < 12; i++) {
						if (isalpha(buf[54+i])){
							for (j=0; j < (13 - i); j++) {
								if (isalnum(buf[54 + i + j])) {
									ssa_mt_exchange[k] = buf[54 + i + j];
									k++;
								}
								else {
									if (j > 0 && isspace(buf[54 + i + j - 1])){
										ssa_mt_exchange[k] = '\0';
										break;
									}
								}
							}
							if (j > 0) break;
						}
					}

					sprintf(buffer+83, "%s        ", ssa_mt_exchange);
					sprintf(buffer+90, "%s", "0");
				}
				else
					strncat(buffer, buf+54, 6);

				strcat(buffer, "  ");



				if ((cqww == 1) && (cqwwm2 == 1))
				{               // cqww M2 mode
					if (buf[79] == '*')
					{
						strcat(buffer, " 1\n");
					}
					else
						strcat(buffer, " 0\n");
				}
				else
				{
					if (strcmp(whichcontest, "ssa_mt") == 1)
						strcat(buffer, " 0\n");
					else
						strcat(buffer, "\n");
				}
			}       // end else arrlss

			if (strlen(buffer) > 11) fputs(buffer,fp2);

		}


	} // end while !eof

	fclose(fp1);
	fclose(fp2);


	fp2 = fopen("cabrillo","a");
	fputs("END-OF-LOG:\n", fp2);
	fclose(fp2);

	system("cat cabrillo >> header");
	system("cp header cabrillo");
	system ("mv header summary.txt");


	return(0);
}

