#pragma once

#define CSV_ERR_LONGLINE 0
#define CSV_ERR_NO_MEMORY 1

char **parse_csv( const char *line );
void free_csv_line( char **parsed );
