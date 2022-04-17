/************************************************************
	Project#1:	CLP & DDL
 ************************************************************/

#include "db.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(_WIN32) || defined(_WIN64)
#define strcasecmp _stricmp
#endif

/* global variable declaration */
int total_rows;

int main(int argc, char** argv)
{
	int rc = 0;
  total_rows = 0;

	token_list *tok_list=NULL, *tok_ptr=NULL, *tmp_tok_ptr=NULL;

	if ((argc != 2) || (strlen(argv[1]) == 0))
	{
		printf("Usage: db \"command statement\"\n");
		return 1;
	}

	rc = initialize_tpd_list();

  if (rc)
  {
		printf("\nError in initialize_tpd_list().\nrc = %d\n", rc);
  }
	else
	{
    rc = get_token(argv[1], &tok_list);

		/* Test code */
		tok_ptr = tok_list;
		while (tok_ptr != NULL)
		{
			printf("%16s \t%d \t %d\n",tok_ptr->tok_string, tok_ptr->tok_class,
				      tok_ptr->tok_value);
			tok_ptr = tok_ptr->next;
		}
    
		if (!rc)
		{
			rc = do_semantic(tok_list);
		}

		if (rc)
		{
			tok_ptr = tok_list;
			while (tok_ptr != NULL)
			{
				if ((tok_ptr->tok_class == error) ||
					  (tok_ptr->tok_value == INVALID))
				{
					printf("\nError in the string: %s\n", tok_ptr->tok_string);
					printf("rc=%d\n", rc);
					break;
				}
				tok_ptr = tok_ptr->next;
			}
		}

    /* Whether the token list is valid or not, we need to free the memory */
		tok_ptr = tok_list;
		while (tok_ptr != NULL)
		{
      tmp_tok_ptr = tok_ptr->next;
      free(tok_ptr);
      tok_ptr=tmp_tok_ptr;
		}
	}

	return rc;
}

/************************************************************* 
	This is a lexical analyzer for simple SQL statements
 *************************************************************/
int get_token(char* command, token_list** tok_list)
{
	int rc=0,i,j;
	char *start, *cur, temp_string[MAX_TOK_LEN];
	bool done = false;
	
	start = cur = command;
	while (!done)
	{
		bool found_keyword = false;

		/* This is the TOP Level for each token */
	  memset ((void*)temp_string, '\0', MAX_TOK_LEN);
		i = 0;

		/* Get rid of all the leading blanks */
		while (*cur == ' ')
			cur++;

		if (cur && isalpha(*cur))
		{
			// find valid identifier
			int t_class;
			do 
			{
				temp_string[i++] = *cur++;
			}
			while ((isalnum(*cur)) || (*cur == '_'));

			if (!(strchr(STRING_BREAK, *cur)))
			{
				/* If the next char following the keyword or identifier
				   is not a blank, (, ), or a comma, then append this
					 character to temp_string, and flag this as an error */
				temp_string[i++] = *cur++;
				add_to_list(tok_list, temp_string, error, INVALID);
				rc = INVALID;
				done = true;
			}
			else
			{

				// We have an identifier with at least 1 character
				// Now check if this ident is a keyword
				for (j = 0, found_keyword = false; j < TOTAL_KEYWORDS_PLUS_TYPE_NAMES; j++)
				{
					if ((strcasecmp(keyword_table[j], temp_string) == 0))
					{
						found_keyword = true;
						break;
					}
				}

				if (found_keyword)
				{
				  if (KEYWORD_OFFSET+j < K_CREATE)
						t_class = type_name;
					else if (KEYWORD_OFFSET+j >= F_SUM)
            t_class = function_name;
          else
					  t_class = keyword;

					add_to_list(tok_list, temp_string, t_class, KEYWORD_OFFSET+j);
				}
				else
				{
					if (strlen(temp_string) <= MAX_IDENT_LEN)
					  add_to_list(tok_list, temp_string, identifier, IDENT);
					else
					{
						add_to_list(tok_list, temp_string, error, INVALID);
						rc = INVALID;
						done = true;
					}
				}

				if (!*cur)
				{
					add_to_list(tok_list, "", terminator, EOC);
					done = true;
				}
			}
		}
		else if (isdigit(*cur))
		{
			// find valid number
			do 
			{
				temp_string[i++] = *cur++;
			}
			while (isdigit(*cur));

			if (!(strchr(NUMBER_BREAK, *cur)))
			{
				/* If the next char following the keyword or identifier
				   is not a blank or a ), then append this
					 character to temp_string, and flag this as an error */
				temp_string[i++] = *cur++;
				add_to_list(tok_list, temp_string, error, INVALID);
				rc = INVALID;
				done = true;
			}
			else
			{
				add_to_list(tok_list, temp_string, constant, INT_LITERAL);

				if (!*cur)
				{
					add_to_list(tok_list, "", terminator, EOC);
					done = true;
				}
			}
		}
		else if ((*cur == '(') || (*cur == ')') || (*cur == ',') || (*cur == '*')
		         || (*cur == '=') || (*cur == '<') || (*cur == '>'))
		{
			/* Catch all the symbols here. Note: no look ahead here. */
			int t_value;
			switch (*cur)
			{
				case '(' : t_value = S_LEFT_PAREN; break;
				case ')' : t_value = S_RIGHT_PAREN; break;
				case ',' : t_value = S_COMMA; break;
				case '*' : t_value = S_STAR; break;
				case '=' : t_value = S_EQUAL; break;
				case '<' : t_value = S_LESS; break;
				case '>' : t_value = S_GREATER; break;
			}

			temp_string[i++] = *cur++;

			add_to_list(tok_list, temp_string, symbol, t_value);

			if (!*cur)
			{
				add_to_list(tok_list, "", terminator, EOC);
				done = true;
			}
		}
    else if (*cur == '\'')
    {
      /* Find STRING_LITERRAL */
			int t_class;
      cur++;
			do 
			{
				temp_string[i++] = *cur++;
			}
			while ((*cur) && (*cur != '\''));

      temp_string[i] = '\0';

			if (!*cur)
			{
				/* If we reach the end of line */
				add_to_list(tok_list, temp_string, error, INVALID);
				rc = INVALID;
				done = true;
			}
      else /* must be a ' */
      {
        add_to_list(tok_list, temp_string, constant, STRING_LITERAL);
        cur++;
				if (!*cur)
				{
					add_to_list(tok_list, "", terminator, EOC);
					done = true;
        }
      }
    }
		else
		{
			if (!*cur)
			{
				add_to_list(tok_list, "", terminator, EOC);
				done = true;
			}
			else
			{
				/* not a ident, number, or valid symbol */
				temp_string[i++] = *cur++;
				add_to_list(tok_list, temp_string, error, INVALID);
				rc = INVALID;
				done = true;
			}
		}
	}
			
  return rc;
}

void add_to_list(token_list **tok_list, char *tmp, int t_class, int t_value)
{
	token_list *cur = *tok_list;
	token_list *ptr = NULL;

	// printf("%16s \t%d \t %d\n",tmp, t_class, t_value);

	ptr = (token_list*)calloc(1, sizeof(token_list));
	strcpy(ptr->tok_string, tmp);
	ptr->tok_class = t_class;
	ptr->tok_value = t_value;
	ptr->next = NULL;

  if (cur == NULL)
		*tok_list = ptr;
	else
	{
		while (cur->next != NULL)
			cur = cur->next;

		cur->next = ptr;
	}
	return;
}

int do_semantic(token_list *tok_list)
{
	int rc = 0, cur_cmd = INVALID_STATEMENT;
	bool unique = false;
  token_list *cur = tok_list;

	if ((cur->tok_value == K_CREATE) &&
			((cur->next != NULL) && (cur->next->tok_value == K_TABLE)))
	{
		printf("CREATE TABLE statement\n");
		cur_cmd = CREATE_TABLE;
		cur = cur->next->next;
	}
	else if ((cur->tok_value == K_DROP) &&
					((cur->next != NULL) && (cur->next->tok_value == K_TABLE)))
	{
		printf("DROP TABLE statement\n");
		cur_cmd = DROP_TABLE;
		cur = cur->next->next;
	}
	else if ((cur->tok_value == K_LIST) &&
					((cur->next != NULL) && (cur->next->tok_value == K_TABLE)))
	{
		printf("LIST TABLE statement\n");
		cur_cmd = LIST_TABLE;
		cur = cur->next->next;
	}
	else if ((cur->tok_value == K_LIST) &&
					((cur->next != NULL) && (cur->next->tok_value == K_SCHEMA)))
	{
		printf("LIST SCHEMA statement\n");
		cur_cmd = LIST_SCHEMA;
		cur = cur->next->next;
	}
  else if ((cur->tok_value == K_INSERT) &&
					((cur->next != NULL) && (cur->next->tok_value == K_INTO)))
	{
		printf("INSERT INTO statement\n");
		cur_cmd = INSERT;
		cur = cur->next->next;
	}
  else if ((cur->tok_value == K_SELECT) &&
					((cur->next != NULL) && (cur->next->tok_value == S_STAR)))
	{
		printf("SELECT * statement\n");
		cur_cmd = SELECT;
		cur = cur->next->next;
	}
	else
  {
		printf("Invalid statement\n");
		rc = cur_cmd;
	}

	if (cur_cmd != INVALID_STATEMENT)
	{
		switch(cur_cmd)
		{
			case CREATE_TABLE:
						rc = sem_create_table(cur);
						break;
			case DROP_TABLE:
						rc = sem_drop_table(cur);
						break;
			case LIST_TABLE:
						rc = sem_list_tables();
						break;
			case LIST_SCHEMA:
						rc = sem_list_schema(cur);
						break;
			case INSERT:
						rc = sem_insert_into(cur);
						break;
			case SELECT:
						rc = sem_select_star(cur);
						break;
			default:
					; /* no action */
		}
	}
	
	return rc;
}

int sem_create_table(token_list *t_list)
{
	int rc = 0;
	token_list *cur;
	tpd_entry tab_entry;
	tpd_entry *new_entry = NULL;
	bool column_done = false;
	int cur_id = 0;
	cd_entry	col_entry[MAX_NUM_COL];
  int record_size = 0;


	memset(&tab_entry, '\0', sizeof(tpd_entry));
	cur = t_list;
	if ((cur->tok_class != keyword) &&
		  (cur->tok_class != identifier) &&
			(cur->tok_class != type_name))
	{
		// Error
		rc = INVALID_TABLE_NAME;
		cur->tok_value = INVALID;
	}
	else
	{
		if ((new_entry = get_tpd_from_list(cur->tok_string)) != NULL)
		{
			rc = DUPLICATE_TABLE_NAME;
			cur->tok_value = INVALID;
		}
		else
		{
			strcpy(tab_entry.table_name, cur->tok_string);
			cur = cur->next;
			if (cur->tok_value != S_LEFT_PAREN)
			{
				//Error
				rc = INVALID_TABLE_DEFINITION;
				cur->tok_value = INVALID;
			}
			else
			{
				memset(&col_entry, '\0', (MAX_NUM_COL * sizeof(cd_entry)));

				/* Now build a set of column entries */
				cur = cur->next;
				do
				{
					if ((cur->tok_class != keyword) &&
							(cur->tok_class != identifier) &&
							(cur->tok_class != type_name))
					{
						// Error
						rc = INVALID_COLUMN_NAME;
						cur->tok_value = INVALID;
					}
					else
					{
						int i;
						for(i = 0; i < cur_id; i++)
						{
              /* make column name case sensitive */
							if (strcmp(col_entry[i].col_name, cur->tok_string)==0)
							{
								rc = DUPLICATE_COLUMN_NAME;
								cur->tok_value = INVALID;
								break;
							}
						}

						if (!rc)
						{
							strcpy(col_entry[cur_id].col_name, cur->tok_string);
							col_entry[cur_id].col_id = cur_id;
							col_entry[cur_id].not_null = false;    /* set default */

							cur = cur->next;
							if (cur->tok_class != type_name)
							{
								// Error
								rc = INVALID_TYPE_NAME;
								cur->tok_value = INVALID;
							}
							else
							{
                /* Set the column type here, int or char */
								col_entry[cur_id].col_type = cur->tok_value;
								cur = cur->next;
		
								if (col_entry[cur_id].col_type == T_INT)
								{
									if ((cur->tok_value != S_COMMA) &&
										  (cur->tok_value != K_NOT) &&
										  (cur->tok_value != S_RIGHT_PAREN))
									{
										rc = INVALID_COLUMN_DEFINITION;
										cur->tok_value = INVALID;
									}
								  else
									{
										col_entry[cur_id].col_len = sizeof(int);
										
										if ((cur->tok_value == K_NOT) &&
											  (cur->next->tok_value != K_NULL))
										{
											rc = INVALID_COLUMN_DEFINITION;
											cur->tok_value = INVALID;
										}	
										else if ((cur->tok_value == K_NOT) &&
											    (cur->next->tok_value == K_NULL))
										{					
											col_entry[cur_id].not_null = true;
											cur = cur->next->next;
										}
	
										if (!rc)
										{
											/* I must have either a comma or right paren */
											if ((cur->tok_value != S_RIGHT_PAREN) &&
												  (cur->tok_value != S_COMMA))
											{
												rc = INVALID_COLUMN_DEFINITION;
												cur->tok_value = INVALID;
											}
											else
		                  {
												if (cur->tok_value == S_RIGHT_PAREN)
												{
 													column_done = true;
												}
                        record_size += sizeof(int) + 1;
												cur = cur->next;
											}
										}
									}
								}   // end of T_INT processing
								else
								{
									// It must be char() or varchar() 
									if (cur->tok_value != S_LEFT_PAREN)
									{
										rc = INVALID_COLUMN_DEFINITION;
										cur->tok_value = INVALID;
									}
									else
									{
										/* Enter char(n) processing */
										cur = cur->next;
		
										if (cur->tok_value != INT_LITERAL)
										{
											rc = INVALID_COLUMN_LENGTH;
											cur->tok_value = INVALID;
										}
										else
										{
											/* Got a valid integer - convert */
											col_entry[cur_id].col_len = atoi(cur->tok_string);
											cur = cur->next;
											
											if (cur->tok_value != S_RIGHT_PAREN)
											{
												rc = INVALID_COLUMN_DEFINITION;
												cur->tok_value = INVALID;
											}
											else
											{
												cur = cur->next;
						
												if ((cur->tok_value != S_COMMA) &&
														(cur->tok_value != K_NOT) &&
														(cur->tok_value != S_RIGHT_PAREN))
												{
													rc = INVALID_COLUMN_DEFINITION;
													cur->tok_value = INVALID;
												}
												else
												{
													if ((cur->tok_value == K_NOT) &&
														  (cur->next->tok_value != K_NULL))
													{
														rc = INVALID_COLUMN_DEFINITION;
														cur->tok_value = INVALID;
													}
													else if ((cur->tok_value == K_NOT) &&
																	 (cur->next->tok_value == K_NULL))
													{					
														col_entry[cur_id].not_null = true;
														cur = cur->next->next;
													}
		
													if (!rc)
													{
														/* I must have either a comma or right paren */
														if ((cur->tok_value != S_RIGHT_PAREN) && (cur->tok_value != S_COMMA))
														{
															rc = INVALID_COLUMN_DEFINITION;
															cur->tok_value = INVALID;
														}
														else
													  {
															if (cur->tok_value == S_RIGHT_PAREN)
															{
																column_done = true;
															}
                              record_size += col_entry[cur_id].col_len + 1;
															cur = cur->next;
														}
													}
												}
											}
										}	/* end char(n) processing */
									}
								} /* end char processing */
							}
						}  // duplicate column name
					} // invalid column name

					/* If rc=0, then get ready for the next column */
					if (!rc)
					{
						cur_id++;
					}

				} while ((rc == 0) && (!column_done));
	
				if ((column_done) && (cur->tok_value != EOC))
				{
					rc = INVALID_TABLE_DEFINITION;
					cur->tok_value = INVALID;
				}

				if (!rc)
				{
					/* Now finished building tpd and add it to the tpd list */
					tab_entry.num_columns = cur_id;
					tab_entry.tpd_size = sizeof(tpd_entry) + sizeof(cd_entry) *	tab_entry.num_columns;
				  tab_entry.cd_offset = sizeof(tpd_entry);
					new_entry = (tpd_entry*)calloc(1, tab_entry.tpd_size);

					if (new_entry == NULL)
					{
						rc = MEMORY_ERROR;
					}
					else
					{
            memcpy((void*)new_entry, (void*)&tab_entry, sizeof(tpd_entry));
            
            memcpy((void*)((char*)new_entry + sizeof(tpd_entry)),
              (void*)col_entry, sizeof(cd_entry) * tab_entry.num_columns);
            
            table_file_header *tableHeaderPtr;
            tableHeaderPtr = (table_file_header*)calloc(1, sizeof(table_file_header));
            
            printf("sizeof table file header: %lu\n\n", sizeof(table_file_header));

            tableHeaderPtr->file_size = sizeof(table_file_header);
            tableHeaderPtr->record_size = round_up(record_size);
            tableHeaderPtr->num_records = 0;
            tableHeaderPtr->record_offset = sizeof(table_file_header);
            tableHeaderPtr->file_header_flag = 0;
            tableHeaderPtr->tpd_ptr = new_entry;

            char filename[MAX_IDENT_LEN + 4];
            strcpy(filename, strcat(tab_entry.table_name, ".tab"));

            FILE *fp = NULL;
            if ((fp = fopen(filename, "wbc")) == NULL) {
              rc = FILE_OPEN_ERROR;
            } else {
              fwrite(&(tableHeaderPtr->file_size), sizeof(int), 1, fp);
              fwrite(&(tableHeaderPtr->record_size), sizeof(int), 1, fp);
              fwrite(&(tableHeaderPtr->num_records), sizeof(int), 1, fp);
              fwrite(&(tableHeaderPtr->record_offset), sizeof(int), 1, fp);
              fwrite(&(tableHeaderPtr->file_header_flag), sizeof(int), 1, fp);
              fwrite((tableHeaderPtr->tpd_ptr), sizeof(tpd_entry*), 1, fp);
              fclose(fp);
            }            
	
						rc = add_tpd_to_list(new_entry);

						free(new_entry);
            
					}
				}
			}
		}
	}
  return rc;
}

int round_up(int num) {
  int mod = num % 4;
  if (mod == 0) {
    return num;
  }
  return num + 4 - mod;
}

int sem_drop_table(token_list *t_list)
{
	int rc = 0;
	token_list *cur;
	tpd_entry *tab_entry = NULL;

	cur = t_list;
	if ((cur->tok_class != keyword) &&
		  (cur->tok_class != identifier) &&
			(cur->tok_class != type_name))
	{
		// Error
		rc = INVALID_TABLE_NAME;
		cur->tok_value = INVALID;
	}
	else
	{
		if (cur->next->tok_value != EOC)
		{
			rc = INVALID_STATEMENT;
			cur->next->tok_value = INVALID;
		}
		else
		{
			if ((tab_entry = get_tpd_from_list(cur->tok_string)) == NULL)
			{
				rc = TABLE_NOT_EXIST;
				cur->tok_value = INVALID;
			}
			else
			{
				/* Found a valid tpd, drop it from tpd list */
        char filename[MAX_IDENT_LEN + 4];
        strcpy(filename, strcat(tab_entry->table_name, ".tab"));

        // remove file with given file name
        if (remove(filename) == 0) {
          printf("Successfully deleted file.\n");
        } else {
          printf("Unable to delete file.\n");
        }

				rc = drop_tpd_from_list(cur->tok_string);
			}
		}
	}

  return rc;
}

int sem_list_tables()
{
	int rc = 0;
	int num_tables = g_tpd_list->num_tables;
	tpd_entry *cur = &(g_tpd_list->tpd_start);

	if (num_tables == 0)
	{
		printf("\nThere are currently no tables defined\n");
	}
	else
	{
		printf("\nTable List\n");
		printf("*****************\n");
		while (num_tables-- > 0)
		{
			printf("%s\n", cur->table_name);
			if (num_tables > 0)
			{
				cur = (tpd_entry*)((char*)cur + cur->tpd_size);
			}
		}
		printf("****** End ******\n");
	}

  return rc;
}

int sem_list_schema(token_list *t_list)
{
	int rc = 0;
	token_list *cur;
	tpd_entry *tab_entry = NULL;
	cd_entry  *col_entry = NULL;
	char tab_name[MAX_IDENT_LEN+1];
	char filename[MAX_IDENT_LEN+1];
	bool report = false;
	FILE *fhandle = NULL;
	int i = 0;

	cur = t_list;

	if (cur->tok_value != K_FOR)
  {
		rc = INVALID_STATEMENT;
		cur->tok_value = INVALID;
	}
	else
	{
		cur = cur->next;

		if ((cur->tok_class != keyword) &&
			  (cur->tok_class != identifier) &&
				(cur->tok_class != type_name))
		{
			// Error
			rc = INVALID_TABLE_NAME;
			cur->tok_value = INVALID;
		}
		else
		{
			memset(filename, '\0', MAX_IDENT_LEN+1);
			strcpy(tab_name, cur->tok_string);
			cur = cur->next;

			if (cur->tok_value != EOC)
			{
				if (cur->tok_value == K_TO)
				{
					cur = cur->next;
					
					if ((cur->tok_class != keyword) &&
						  (cur->tok_class != identifier) &&
							(cur->tok_class != type_name))
					{
						// Error
						rc = INVALID_REPORT_FILE_NAME;
						cur->tok_value = INVALID;
					}
					else
					{
						if (cur->next->tok_value != EOC)
						{
							rc = INVALID_STATEMENT;
							cur->next->tok_value = INVALID;
						}
						else
						{
							/* We have a valid file name */
							strcpy(filename, cur->tok_string);
							report = true;
						}
					}
				}
				else
				{ 
					/* Missing the TO keyword */
					rc = INVALID_STATEMENT;
					cur->tok_value = INVALID;
				}
			}

			if (!rc)
			{
				if ((tab_entry = get_tpd_from_list(tab_name)) == NULL)
				{
					rc = TABLE_NOT_EXIST;
					cur->tok_value = INVALID;
				}
				else
				{
					if (report)
					{
						if((fhandle = fopen(filename, "a+tc")) == NULL)
						{
							rc = FILE_OPEN_ERROR;
						}
					}

					if (!rc)
					{
						/* Find correct tpd, need to parse column and index information */

						/* First, write the tpd_entry information */
						printf("Table PD size            (tpd_size)    = %d\n", tab_entry->tpd_size);
						printf("Table Name               (table_name)  = %s\n", tab_entry->table_name);
						printf("Number of Columns        (num_columns) = %d\n", tab_entry->num_columns);
						printf("Column Descriptor Offset (cd_offset)   = %d\n", tab_entry->cd_offset);
            printf("Table PD Flags           (tpd_flags)   = %d\n\n", tab_entry->tpd_flags); 

						if (report)
						{
							fprintf(fhandle, "Table PD size            (tpd_size)    = %d\n", tab_entry->tpd_size);
							fprintf(fhandle, "Table Name               (table_name)  = %s\n", tab_entry->table_name);
							fprintf(fhandle, "Number of Columns        (num_columns) = %d\n", tab_entry->num_columns);
							fprintf(fhandle, "Column Descriptor Offset (cd_offset)   = %d\n", tab_entry->cd_offset);
              fprintf(fhandle, "Table PD Flags           (tpd_flags)   = %d\n\n", tab_entry->tpd_flags); 
						}

						/* Next, write the cd_entry information */
						for(i = 0, col_entry = (cd_entry*)((char*)tab_entry + tab_entry->cd_offset);
								i < tab_entry->num_columns; i++, col_entry++)
						{
							printf("Column Name   (col_name) = %s\n", col_entry->col_name);
							printf("Column Id     (col_id)   = %d\n", col_entry->col_id);
							printf("Column Type   (col_type) = %d\n", col_entry->col_type);
							printf("Column Length (col_len)  = %d\n", col_entry->col_len);
							printf("Not Null flag (not_null) = %d\n\n", col_entry->not_null);

							if (report)
							{
								fprintf(fhandle, "Column Name   (col_name) = %s\n", col_entry->col_name);
								fprintf(fhandle, "Column Id     (col_id)   = %d\n", col_entry->col_id);
								fprintf(fhandle, "Column Type   (col_type) = %d\n", col_entry->col_type);
								fprintf(fhandle, "Column Length (col_len)  = %d\n", col_entry->col_len);
								fprintf(fhandle, "Not Null Flag (not_null) = %d\n\n", col_entry->not_null);
							}
						}
	
						if (report)
						{
							fflush(fhandle);
							fclose(fhandle);
						}
					} // File open error							
				} // Table not exist
			} // no semantic errors
		} // Invalid table name
	} // Invalid statement

  return rc;
}

int sem_insert_into(token_list *t_list) 
{
  int rc = 0;
  token_list *cur;
  tpd_entry *tab_entry = NULL;
  cd_entry  *col_entry = NULL;

  FILE *fp = NULL;
  FILE *rp = NULL;

  int record_size = 0;
  int i = 0;

  cur = t_list;

  if ((cur->tok_class != keyword) &&
		  (cur->tok_class != identifier) &&
			(cur->tok_class != type_name))
	{
		rc = INVALID_TABLE_NAME;
		cur->tok_value = INVALID;
	}
	else
	{
    // Check if table exists
    if ((tab_entry = get_tpd_from_list(cur->tok_string)) == NULL) {
      rc = TABLE_NOT_EXIST;
      cur->tok_value = INVALID;
      printf("Table %s not found\n", cur->tok_string);
    } else { // Found a valid tpd
      // Open table file for reading
      char filename[MAX_IDENT_LEN + 4] = {0};
      strcpy(filename, strcat(tab_entry->table_name, ".tab"));
      if ((fp = fopen(filename, "abc")) == NULL) {
        printf("Error while opening %s file\n", filename);
        rc = FILE_OPEN_ERROR;
        cur->tok_value = INVALID;
      } else {
        cur = cur->next;
      
        if (cur->tok_value != K_VALUES) {
          rc = INVALID_TYPE_NAME;
          cur->tok_value = INVALID;
        } 
        else {
          cur = cur->next;
          // if current token is not (
          if (cur->tok_value != S_LEFT_PAREN) {
            rc = INVALID_TYPE_NAME;
            cur->tok_value = INVALID;
          } else {
            cur = cur->next;
            // Now we are pointing at the first value to be inserted
            col_entry = (cd_entry*)((char*)tab_entry + tab_entry->cd_offset);
            for (i = 0; i < tab_entry->num_columns; i++, col_entry++) 
            {
              if (cur->tok_value == EOC) {
                rc = MAX_COLUMN_EXCEEDED;
                cur->tok_value = INVALID;
                break;
              }

              if (cur->tok_value == S_COMMA) {
                if (cur->next->tok_value != INT_LITERAL 
                    && cur->next->tok_value != STRING_LITERAL
                    && cur->next->tok_value != K_NULL) 
                {
                    rc = INVALID_TYPE_NAME;
                    cur->tok_value = INVALID;
                    break;
                } else {
                  cur = cur->next;
                  i--;
                  col_entry--;
                }
              }

              // Invalid type. Expected int, string, or null
              else if (cur->tok_value != INT_LITERAL 
                    && cur->tok_value != STRING_LITERAL
                    && cur->tok_value != K_NULL) 
              {
                rc = INVALID_TYPE;
                cur->tok_value = INVALID;
                printf("Error: Expected int, string, or null\n");
                break;
              }

              // Expectd string, got int
              else if (cur->tok_value == INT_LITERAL && col_entry->col_type == T_CHAR) {
                rc = INVALID_TYPE;
                cur->tok_value = INVALID;
                printf("Error: Expected string, got int\n");
                break;
              }

              // Expected int, got string
              else if (cur->tok_value == STRING_LITERAL && col_entry->col_type == T_INT) {
                rc = INVALID_TYPE;
                cur->tok_value = INVALID;
                printf("Error: Expected int, got string\n");
                break; 
              }

              // Null constraint failed
              else if (cur->tok_value == K_NULL && col_entry->not_null) {
                rc = NULL_CONSTRAINT;
                cur->tok_value = INVALID;
                printf("Error: Null constraint violated\n");
                break; 
              }

              else {
                // Check string length
                if (cur->tok_value == STRING_LITERAL && col_entry->col_type == T_CHAR 
                    && strlen(cur->tok_string) > col_entry->col_len) {
                    rc = INVALID_COLUMN_LENGTH;
                    cur->tok_value = INVALID;
                    printf("Error: String exceeds varchar limit\n");
                    break; 
                } else {
                  cur = cur->next;
                }
              }
            } // end for loop. finished checking int/char values
            
            // closing parenthesis, EOC
            if (!rc && cur->tok_value != S_RIGHT_PAREN) {
              if (cur->tok_value == EOC) {
                printf("Missing closing paren\n");
              } else {
                printf("Too many arguments\n");
              }
              rc = INVALID_STATEMENT;
		          cur->tok_value = INVALID;
            }

            if (!rc && cur->next->tok_value != EOC) {
              printf("Unexpected characters after closing paren\n");
              rc = INVALID_STATEMENT;
		          cur->tok_value = INVALID;
            }

            if (total_rows > MAX_ROWS) {
              printf("Cannot complete insert. Maximum rows in memory reached\n");
              return rc;
            }

            if (!rc) { // Finished validating values
              // Reset cur to first value
              cur = t_list->next->next->next; // skip '<table_name> values ('
              int num_columns = tab_entry->num_columns;
              cd_entry* col_entry = (cd_entry*)((char*)tab_entry + tab_entry->cd_offset);

              for (int i = 0; i < num_columns; i++, col_entry++) {
                if (cur->tok_value == INT_LITERAL) {
                  int int_size = sizeof(int);
                  int int_value = atoi(cur->tok_string);
                  fwrite(&int_size, 1, 1, fp);
                  fwrite(&int_value, sizeof(int), 1, fp);
                  cur = cur->next;
                }
                
                else if (cur->tok_value == STRING_LITERAL) {
                  int str_length = strlen(cur->tok_string);
                  int max_length = col_entry->col_len;
                  fwrite(&str_length, 1, 1, fp);
                  fwrite(cur->tok_string, str_length, 1, fp);

                  int padding = max_length - str_length;
                  int zero = 0;
                  if (padding > 0) {
                    fwrite(&zero, padding, 1, fp);
                  }
                  cur = cur->next;
                }

                else if (cur->tok_value == K_NULL) {
                  int zero = 0;
                  fwrite(&zero, 1, 1, fp); // size byte is 0
                  // if int, write 0
                  if (col_entry->col_type == T_INT) {
                    fwrite(&zero, sizeof(int), 1, fp); 
                  }
                  // if char, write 0 in entire buffer
                  if (col_entry->col_type == T_CHAR) {
                    int col_length = col_entry->col_len;
                    fwrite(&zero, col_length, 1, fp);
                  }
                 
                  cur = cur->next;
                }
                else { // comma
                  col_entry--;
                  i--;
                  cur = cur->next;
                }
              }

              total_rows++;
              fflush(fp);
              fclose(fp);

              // Update table header: file size and num records
              if ((rp = fopen(filename, "r+b")) == NULL) {
                printf("Error while opening %s file\n", filename);
                rc = FILE_OPEN_ERROR;
                cur->tok_value = INVALID;
              } else {
                int num_records, file_size;

                // Read record size
                if ((fseek(rp, 4, SEEK_SET)) == 0) {
                  fread(&record_size, sizeof(int), 1, rp);
                }
                
                // Update number of records
                if((fseek(rp, 8, SEEK_SET)) == 0)
                {
                  fread(&num_records, sizeof(int), 1, rp);
                  num_records += 1;
                  
                  if((fseek(rp, 8, SEEK_SET)) == 0)
                  {
                    fwrite(&num_records, 1, 1, rp);
                  }
                }

                // Update file size
                if((fseek(rp, 0, SEEK_SET)) == 0)
                {
                  fread(&file_size, sizeof(int), 1, rp);
                  file_size += record_size;
                  printf("%s size: %d. \n", filename, file_size);
                  printf("Number of records: %d\n", num_records);
                  if((fseek(rp, 0, SEEK_SET)) == 0)
                  {
                    fwrite(&file_size, sizeof(int), 1, rp);
                  }
                }

                printf("Row inserted successfully\n\n");
              }
            }
          }
        }
      }
    }
  }
  return rc; 
}

int sem_select_star(token_list *t_list) {
  int rc = 0;
  token_list *cur;
  tpd_entry *tab_entry1 = NULL;
  tpd_entry *tab_entry2 = NULL;

  cd_entry  *col_entry1 = NULL;

  FILE *fp = NULL;

  int record_size = 0;
  int i = 0, j = 0;

  cur = t_list;
  bool natural_join = false;

  if ((cur->tok_class != keyword) &&
		  (cur->tok_class != identifier) &&
			(cur->tok_class != type_name))
	{
		rc = INVALID_TABLE_NAME;
		cur->tok_value = INVALID;
	}
	else
	{
    if (cur->tok_value != K_FROM) {
      rc = INVALID_STATEMENT;
      cur->tok_value = INVALID;
    } 
    else {
      cur = cur->next;
      // Check if table exists
      
      if ((tab_entry1 = get_tpd_from_list(cur->tok_string)) == NULL) {
        rc = TABLE_NOT_EXIST;
        cur->tok_value = INVALID;
        printf("Table %s not found\n", cur->tok_string);
      } else { // Found a valid tpd
        
        if (cur->next->tok_value != EOC && cur->next->tok_value == K_NATURAL && cur->next->next->tok_value == K_JOIN) {
          // natural join keyword
          // check table 2
          cur = cur->next->next->next;
          printf("current token: %s\n", cur->tok_string);
          if ((cur->tok_class != keyword) &&
              (cur->tok_class != identifier) &&
              (cur->tok_class != type_name)) {
            // Error
            rc = INVALID_TABLE_NAME;
            cur->tok_value = INVALID;
          } else {
            if (cur->next->tok_value != EOC) {
              rc = INVALID_STATEMENT;
              cur->next->tok_value = INVALID;
            } else {
              if ((tab_entry2 = get_tpd_from_list(cur->tok_string)) == NULL) {
                rc = TABLE_NOT_EXIST;
                cur->tok_value = INVALID;
                printf("Table %s not found\n", cur->tok_string);
              } else {
                // save table2 name
                natural_join = true;
              }
            }
          }
        } else if (cur->next->tok_value != EOC) {
          rc = INVALID_STATEMENT;
          cur->next->tok_value = INVALID;
        }
      }
    }
  }

  if (!rc) {
    // Open table1 file for reading
    int num_rows1 = 0, record_size1 = 0, h_offset1 = 0;

    char filename1[MAX_IDENT_LEN + 4] = {0};
    strcpy(filename1, strcat(tab_entry1->table_name, ".tab"));

    if ((fp = fopen(filename1, "rbc")) == NULL) {
      printf("Error while opening %s file\n", filename1);
      rc = FILE_OPEN_ERROR;
      cur->tok_value = INVALID;
    } 

    // Get record size for table1
		if((fseek(fp, 4, SEEK_SET)) == 0)
			{
				fread(&record_size1, sizeof(int), 1, fp);
				printf("Record size: %d\n", record_size1);
			}

		// Get number of records in table1
		if((fseek(fp, 8, SEEK_SET)) == 0)
		{
			fread(&num_rows1, sizeof(int), 1, fp);
			printf("Rows in table: %d\n", num_rows1);
		}

    // Get header offset for table1
		if((fseek(fp, 12, SEEK_SET)) == 0)
		{
			fread(&h_offset1, sizeof(int), 1, fp);
			printf("Header offset: %d\n", h_offset1);
		}

    if (!rc) {
      char *header = "-----------------";

      int num_columns = tab_entry1->num_columns;
      // char *column_names = new char[num_columns];
      char **column_names = (char**)malloc(sizeof(char) * MAX_IDENT_LEN * num_columns);

      int *column_lengths = (int*)malloc(sizeof(int) * num_columns);
      int *column_types = (int*)malloc(sizeof(int) * num_columns);

      if (!natural_join) { // Print all columns for one table
        // Print top border
        for(i = 0; i < num_columns; i++) {
          printf("%s", header);
        }
        printf("\n");

        // Print column names
        for(i = 0, col_entry1 = (cd_entry*)((char*)tab_entry1 + tab_entry1->cd_offset);
								i < num_columns; i++, col_entry1++) 
        {
          column_lengths[i] = col_entry1->col_len;
          column_types[i] = col_entry1->col_type;
            printf("%-16s| ", col_entry1->col_name);
        }
        printf("\n");

        // Print bottom border
        for(i = 0; i < num_columns; i++) {
          if(i == (num_columns-1)) {
            printf("%s\n", header);
          } else {
            printf("%s", header);
          }
        }
        
        bool notEOF = true;
        fseek(fp, h_offset1, SEEK_SET); // move file pointer to start of record

        // char *record = (char*)malloc(record_size1);        
        // while (fread(record, record_size1, 1, fp) == 1) {
          // col_entry1 = (cd_entry*)((char*)tab_entry1 + tab_entry1->cd_offset);
        //   printf("\n");

        // }

        while (notEOF) {
          col_entry1 = (cd_entry*)((char*)tab_entry1 + tab_entry1->cd_offset);
          for (i = 0; i < num_columns; i++) {
            char c;
            c = fgetc(fp);
            printf("first char: %d\n", c);
            if (c == '\0') {
              //print null
            } else {
              ungetc(c, fp);
              if (col_entry1->col_type == T_INT) {
                int val = 0;
                fread(&val, sizeof(int), 1, fp);
                printf("%16d |", val);
                char ch;
                ch = fgetc(fp);
              }

              col_entry1++;
            }
          } // end for

          int eof;
          if (fscanf(fp, "%d", &eof) == EOF) {
            notEOF = false;
          }
        }

        // print one row at a time
        // for (i = 0; i < num_rows1; i++) {
        //   int bytes_read = 0;
        //   // Read one record
        //   char *record_buffer = (char*)malloc(record_size1);
        //   bytes_read = fread(record_buffer, record_size1, 1, fp);
        //   printf("buffer length: %d", strlen(record_buffer));
        //   fwrite(record_buffer, 1, record_size1, stdout);
        //   printf("\n");

        //   for (j = 0; j < num_columns; j++) {
            
            
        //   }

        //   free(record_buffer);

        //   fseek(fp, record_size1, SEEK_CUR);
        // }
        // get the row of data

        // iterate through the columns
      }

      

      else {
        // natural join

      }
    }
  
  }



  return rc;
}

// void dump_buffer(void *buffer, int buffer_size)
// {
//   int i;

//   for(i = 0;i < buffer_size;++i)
//      printf("%02x", buffer[i]);
    
//   printf("\n");
// }

int initialize_tpd_list()
{
	int rc = 0;
	FILE *fhandle = NULL;
//	struct _stat file_stat;
	struct stat file_stat;

  /* Open for read */
  if((fhandle = fopen("dbfile.bin", "rbc")) == NULL)
	{
		if((fhandle = fopen("dbfile.bin", "wbc")) == NULL)
		{
			rc = FILE_OPEN_ERROR;
		}
    else
		{
			g_tpd_list = NULL;
			g_tpd_list = (tpd_list*)calloc(1, sizeof(tpd_list));
			
			if (!g_tpd_list)
			{
				rc = MEMORY_ERROR;
			}
			else
			{
				g_tpd_list->list_size = sizeof(tpd_list);
				fwrite(g_tpd_list, sizeof(tpd_list), 1, fhandle);
				fflush(fhandle);
				fclose(fhandle);
			}
		}
	}
	else
	{
		/* There is a valid dbfile.bin file - get file size */
//		_fstat(_fileno(fhandle), &file_stat);
		fstat(fileno(fhandle), &file_stat);
		printf("dbfile.bin size = %d\n", file_stat.st_size);

		g_tpd_list = (tpd_list*)calloc(1, file_stat.st_size);

		if (!g_tpd_list)
		{
			rc = MEMORY_ERROR;
		}
		else
		{
			fread(g_tpd_list, file_stat.st_size, 1, fhandle);
			fflush(fhandle);
			fclose(fhandle);

			if (g_tpd_list->list_size != file_stat.st_size)
			{
				rc = DBFILE_CORRUPTION;
			}

		}
	}
    
	return rc;
}
	
int add_tpd_to_list(tpd_entry *tpd)
{
	int rc = 0;
	int old_size = 0;
	FILE *fhandle = NULL;

	if((fhandle = fopen("dbfile.bin", "wbc")) == NULL)
	{
		rc = FILE_OPEN_ERROR;
	}
  else
	{
		old_size = g_tpd_list->list_size;

		if (g_tpd_list->num_tables == 0)
		{
			/* If this is an empty list, overlap the dummy header */
			g_tpd_list->num_tables++;
		 	g_tpd_list->list_size += (tpd->tpd_size - sizeof(tpd_entry));
			fwrite(g_tpd_list, old_size - sizeof(tpd_entry), 1, fhandle);
		}
		else
		{
			/* There is at least 1, just append at the end */
			g_tpd_list->num_tables++;
		 	g_tpd_list->list_size += tpd->tpd_size;
			fwrite(g_tpd_list, old_size, 1, fhandle);
		}

		fwrite(tpd, tpd->tpd_size, 1, fhandle);
		fflush(fhandle);
		fclose(fhandle);
	}

	return rc;
}

int drop_tpd_from_list(char *tabname)
{
	int rc = 0;
	tpd_entry *cur = &(g_tpd_list->tpd_start);
	int num_tables = g_tpd_list->num_tables;
	bool found = false;
	int count = 0;

	if (num_tables > 0)
	{
		while ((!found) && (num_tables-- > 0))
		{
			if (strcasecmp(cur->table_name, tabname) == 0)
			{
				/* found it */
				found = true;
				int old_size = 0;
				FILE *fhandle = NULL;

				if((fhandle = fopen("dbfile.bin", "wbc")) == NULL)
				{
					rc = FILE_OPEN_ERROR;
				}
			  else
				{
					old_size = g_tpd_list->list_size;

					if (count == 0)
					{
						/* If this is the first entry */
						g_tpd_list->num_tables--;

						if (g_tpd_list->num_tables == 0)
						{
							/* This is the last table, null out dummy header */
							memset((void*)g_tpd_list, '\0', sizeof(tpd_list));
							g_tpd_list->list_size = sizeof(tpd_list);
							fwrite(g_tpd_list, sizeof(tpd_list), 1, fhandle);
						}
						else
						{
							/* First in list, but not the last one */
							g_tpd_list->list_size -= cur->tpd_size;

							/* First, write the 8 byte header */
							fwrite(g_tpd_list, sizeof(tpd_list) - sizeof(tpd_entry),
								     1, fhandle);

							/* Now write everything starting after the cur entry */
							fwrite((char*)cur + cur->tpd_size,
								     old_size - cur->tpd_size -
										 (sizeof(tpd_list) - sizeof(tpd_entry)),
								     1, fhandle);
						}
					}
					else
					{
						/* This is NOT the first entry - count > 0 */
						g_tpd_list->num_tables--;
					 	g_tpd_list->list_size -= cur->tpd_size;

						/* First, write everything from beginning to cur */
						fwrite(g_tpd_list, ((char*)cur - (char*)g_tpd_list),
									 1, fhandle);

						/* Check if cur is the last entry. Note that g_tdp_list->list_size
						   has already subtracted the cur->tpd_size, therefore it will
						   point to the start of cur if cur was the last entry */
						if ((char*)g_tpd_list + g_tpd_list->list_size == (char*)cur)
						{
							/* If true, nothing else to write */
						}
						else
						{
							/* NOT the last entry, copy everything from the beginning of the
							   next entry which is (cur + cur->tpd_size) and the remaining size */
							fwrite((char*)cur + cur->tpd_size,
										 old_size - cur->tpd_size -
										 ((char*)cur - (char*)g_tpd_list),							     
								     1, fhandle);
						}
					}

					fflush(fhandle);
					fclose(fhandle);
				}

				
			}
			else
			{
				if (num_tables > 0)
				{
					cur = (tpd_entry*)((char*)cur + cur->tpd_size);
					count++;
				}
			}
		}
	}
	
	if (!found)
	{
		rc = INVALID_TABLE_NAME;
	}

	return rc;
}

tpd_entry* get_tpd_from_list(char *tabname)
{
	tpd_entry *tpd = NULL;
	tpd_entry *cur = &(g_tpd_list->tpd_start);
	int num_tables = g_tpd_list->num_tables;
	bool found = false;

	if (num_tables > 0)
	{
		while ((!found) && (num_tables-- > 0))
		{
			if (strcasecmp(cur->table_name, tabname) == 0)
			{
				/* found it */
				found = true;
				tpd = cur;
			}
			else
			{
				if (num_tables > 0)
				{
					cur = (tpd_entry*)((char*)cur + cur->tpd_size);
				}
			}
		}
	}

	return tpd;
}