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
		// while (tok_ptr != NULL)
		// {
		// 	printf("%16s \t%d \t %d\n",tok_ptr->tok_string, tok_ptr->tok_class,
		// 		      tok_ptr->tok_value);
		// 	tok_ptr = tok_ptr->next;
		// }
    
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
  else if ((cur->tok_value == K_SELECT) && (cur->next != NULL) )
	{
		printf("SELECT statement\n");
		cur_cmd = SELECT;
		cur = cur->next;
	}
	else if ((cur->tok_value == K_DELETE) &&
					((cur->next != NULL) && (cur->next->tok_value == K_FROM)))
	{
		printf("DELETE FROM statement\n");
		cur_cmd = DELETE;
		cur = cur->next->next;
	}
	else if ((cur->tok_value == K_UPDATE) &&
					(cur->next != NULL)  && 
					(cur->next->next != NULL && (cur->next->next->tok_value == K_SET)))
	{
		printf("UPDATE statement\n");
		cur_cmd = UPDATE;
		cur = cur->next;
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
						rc = sem_select(cur);
						break;
			case DELETE:
						rc = sem_delete_from(cur);
						break;
			case UPDATE:
						rc = sem_update(cur);
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
      printf("Error: table already exists\n");
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
        printf("Error: Invalid table definition\n");
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
            printf("Error: Invalid column name\n");
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
								printf("Error: duplicate column name\n");
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
                printf("Error: Invalid type\n");
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
                    printf("Error: Invalid column definition\n");
										rc = INVALID_COLUMN_DEFINITION;
										cur->tok_value = INVALID;
									}
								  else
									{
										col_entry[cur_id].col_len = sizeof(int);
										
										if ((cur->tok_value == K_NOT) &&
											  (cur->next->tok_value != K_NULL))
										{
											printf("Error: Invalid column definition\n");
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
												printf("Error: Expected , or )\n");
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
            
            // printf("sizeof table file header: %lu\n\n", sizeof(table_file_header));
            // printf("sizeof tpd_ptr: %lu\n", sizeof(tpd_entry*));

            tableHeaderPtr->file_size = sizeof(table_file_header);
            tableHeaderPtr->record_size = round_up(record_size);
            tableHeaderPtr->num_records = 0;
            tableHeaderPtr->record_offset = sizeof(table_file_header);
            tableHeaderPtr->file_header_flag = 0;
            tableHeaderPtr->tpd_ptr = 0;

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
              fwrite(&(tableHeaderPtr->tpd_ptr), sizeof(tpd_entry*), 1, fp);

              // need to pad with 4 more bytes to reach record size of 32
              fwrite(&(tableHeaderPtr->file_header_flag), sizeof(int), 1, fp);
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
	int num_records = 0;
	int h_offset = 0;
	int file_size = 0;

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

			if ((rp = fopen(filename, "rb")) == NULL) {
				printf("Error while opening %s file\n", filename);
				rc = FILE_OPEN_ERROR;
				cur->tok_value = INVALID;
			} else {
				// Read file size
				if ((fseek(rp, 0, SEEK_SET)) == 0) {
					fread(&file_size, sizeof(int), 1, rp);
				}
				// Read record size
				if ((fseek(rp, 4, SEEK_SET)) == 0) {
					fread(&record_size, sizeof(int), 1, rp);
				}
				// Read num records
				if ((fseek(rp, 8, SEEK_SET)) == 0) {
					fread(&num_records, sizeof(int), 1, rp);
				}
				// Read header offset
				if ((fseek(rp, 12, SEEK_SET)) == 0) {
					fread(&h_offset, sizeof(int), 1, rp);
				}
				fclose(rp);
			}

      if ((fp = fopen(filename, "rb+")) == NULL) {
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

						int bytes_written = 0;

            if (!rc) { // Finished validating values
              // Reset cur to first value
              cur = t_list->next->next->next; // skip '<table_name> values ('
              int num_columns = tab_entry->num_columns;
              cd_entry* col_entry = (cd_entry*)((char*)tab_entry + tab_entry->cd_offset);

							// Move file pointer to next available record
							fseek(fp, h_offset + ((num_records) * record_size), SEEK_SET);
              for (int i = 0; i < num_columns; i++, col_entry++) {
                if (cur->tok_value == INT_LITERAL) {
                  int int_size = sizeof(int);
                  int int_value = atoi(cur->tok_string);
                  fwrite(&int_size, 1, 1, fp);
									bytes_written += 1;
                  fwrite(&int_value, sizeof(int), 1, fp);
									bytes_written += sizeof(int);
                  cur = cur->next;
                }
                
                else if (cur->tok_value == STRING_LITERAL) {
                  int str_length = strlen(cur->tok_string);
                  int max_length = col_entry->col_len;
                  fwrite(&str_length, 1, 1, fp);
									bytes_written += 1;
                  fwrite(cur->tok_string, str_length, 1, fp);

                  int padding = max_length - str_length;
                  int zero = 0;
                  if (padding > 0) {
                    fwrite(&zero, padding, 1, fp);
                  }
									bytes_written += max_length;
                  cur = cur->next;
                }

                else if (cur->tok_value == K_NULL) {
                  int zero = 0;
                  fwrite(&zero, 1, 1, fp); // size byte is 0
                  // if int, write 0
                  if (col_entry->col_type == T_INT) {
                    fwrite(&zero, sizeof(int), 1, fp); 
										bytes_written += sizeof(int);
                  }
                  // if char, write 0 in entire buffer
                  if (col_entry->col_type == T_CHAR) {
                    int col_length = col_entry->col_len;
                    fwrite(&zero, col_length, 1, fp);
										bytes_written += sizeof(col_length);
                  }
                 
                  cur = cur->next;
                }
                else { // comma
                  col_entry--;
                  i--;
                  cur = cur->next;
                }
              }

              // Pad the rest of the record with zeros. Round up to nearest byte
              int zero = 0;
              int padding = record_size - bytes_written;
              fwrite(&zero, 1, padding, fp);

              total_rows++;
              fflush(fp);
              
              // Update table header:
							// Update number of records
							num_records++;
							if((fseek(fp, 8, SEEK_SET)) == 0) {
								fwrite(&num_records, sizeof(int), 1, fp);
							}

							// Update file size
							file_size += record_size;
							if((fseek(fp, 0, SEEK_SET)) == 0) {
								fwrite(&file_size, sizeof(int), 1, fp);
							}
							printf("Row inserted successfully\n\n");
              
            }
          }
        }
				fclose(fp);
      }
    }
  }
  return rc; 
}

int sem_select(token_list *t_list) {
  int rc = 0;
  token_list *cur;
  tpd_entry *tab_entry1 = NULL;
  tpd_entry *tab_entry2 = NULL;
  cd_entry  *col_entry1 = NULL;
	cd_entry 	*col_entry2 = NULL;


  FILE *fp1 = NULL;
	FILE *fp2 = NULL;

  cur = t_list;
  bool natural_join = false;
	bool select_star = false;
	bool where_clause = false;
	int numSelectColumns = 0;

	int num_rows1 = 0, record_size1 = 0, h_offset1 = 0;
	int num_rows2 = 0, record_size2 = 0, h_offset2 = 0;

	// column info for WHERE
	int col1_type, col2_type; // column type
	int col1_ID, col2_ID; // column ID
	int col1_tab, col2_tab; // table 1 or 2
	int condition1_op, condition2_op; // operator =, <, >, IS-NULL, IS-NOT-NULL
	int condition1_val, condition2_val;
	char condition1_str[MAX_TOK_LEN], condition2_str [MAX_TOK_LEN];
	int and_or = -1;
	
	// select <column> info
	int *selectColID, *selectColTable;

	
  if ((cur->tok_class != keyword) &&
		  (cur->tok_class != identifier) &&
			(cur->tok_class != type_name) &&
			(cur->tok_value != S_STAR))
	{
		rc = INVALID_TABLE_NAME;
		cur->tok_value = INVALID;
		printf("Error: Expected keyword, identifier, or type name!\n");
		return rc;
	}
	else
	{
    if (cur->tok_value == S_STAR && cur->next->tok_value == K_FROM) {
			select_star = true;
      cur = cur->next;
    } else { // Did not find a *

			// Look ahead to find FROM keyword
			
			while (cur->tok_value != K_FROM) {

				if (cur->tok_value == EOC) {
					rc = INVALID_STATEMENT;
					cur->tok_value = INVALID;
					return rc;
				}

				else if (cur->tok_value == S_COMMA) {
					if ((cur->next->tok_class != keyword) 
					&& (cur->next->tok_class != identifier) 
					&& (cur->next->tok_class != type_name)) {
						rc = INVALID_COLUMN_NAME;
						cur->tok_value = INVALID;
						printf("Error: Invalid column name after comma\n");
						return rc;
					} else {
						cur = cur->next;
					}
				}

				else if ((cur->tok_class != keyword) 
					&& (cur->tok_class != identifier) 
					&& (cur->tok_class != type_name)) {
					rc = INVALID_COLUMN_NAME;
					cur->tok_value = INVALID;
					printf("Error: Invalid column name. Expected keyword, identifier, type name\n");
					return rc;
				}
				else {
					numSelectColumns++;
					cur = cur->next;
				}
			} // End while

			
			printf("num columns %d!\n", numSelectColumns);
			if (cur->tok_value != K_FROM) {
				rc = INVALID_STATEMENT;
				cur->tok_value = INVALID;
				printf("Error: Expected FROM clause to select from\n");
				return rc;
			} 
		}

		// Should be on FROM now. Check if table exists and check for natural join
		cur = cur->next;
		if ((tab_entry1 = get_tpd_from_list(cur->tok_string)) == NULL) {
			rc = TABLE_NOT_EXIST;
			cur->tok_value = INVALID;
			printf("Table %s not found\n", cur->tok_string);
			return rc;
		} else {
			// Check for 'natural join' keyword
			if (cur->next->tok_value != EOC && cur->next->tok_value == K_NATURAL && cur->next->next->tok_value == K_JOIN) {
				// check table 2
				cur = cur->next->next->next;
				if ((cur->tok_class != keyword) &&
						(cur->tok_class != identifier) &&
						(cur->tok_class != type_name)) {
					rc = INVALID_TABLE_NAME;
					cur->tok_value = INVALID;
					return rc;
				} else {
					// Check if table 2 exists
					if ((tab_entry2 = get_tpd_from_list(cur->tok_string)) == NULL) {
						rc = TABLE_NOT_EXIST;
						cur->tok_value = INVALID;
						printf("Table %s not found\n", cur->tok_string);
						return rc;
					} else {
						natural_join = true;
					}
				}
			} 
		}
		
		// After checking that tables exist, verify that columns are valid in those tables

		if (!rc && !select_star && numSelectColumns > 0) {
			selectColID = (int*)calloc(numSelectColumns, sizeof(int));
			selectColTable = (int*)calloc(numSelectColumns, sizeof(int));

			// Go back to verify columns
			token_list *temp = t_list; // Move back to beginnning
			
			for (int ci = 0; ci < numSelectColumns; ci++) {

				bool isValid = false;
				col_entry1 = (cd_entry*)((char*)tab_entry1 + tab_entry1->cd_offset);

				// Check table1's columns
				for (int i = 0; i < tab_entry1->num_columns; i++, col_entry1++) {
					if (strcmp(col_entry1->col_name, temp->tok_string) == 0) {
						selectColID[ci] = col_entry1->col_id;
						selectColTable[ci] = 1; // table 1
						isValid = true;
						break;
					}
				}

				// Check table2's columns
				if (natural_join && !isValid) {
					col_entry2 = (cd_entry*)((char*)tab_entry2 + tab_entry2->cd_offset);
				
					for (int i = 0; i < tab_entry2->num_columns; i++, col_entry2++) {
						if (strcmp(col_entry2->col_name, temp->tok_string) == 0) {
							isValid = true;
							selectColID[ci] = col_entry2->col_id;
							selectColTable[ci] = 2; // table 1
							break;
						}
					}
				}

				if (!isValid) {
					rc = INVALID_COLUMN_NAME;
					temp->tok_value = INVALID;
					printf("Error: Invalid column name\n");
					return rc;
				}

				if (ci == numSelectColumns - 1) {
					temp = temp->next;
				} else {
					temp = temp->next->next; // skip columns
				}
			} // End for
		} 
  }

	// Cur is currently on <table name> Check for WHERE keyword
	cur = cur->next;
	if (cur->tok_value == K_WHERE) {
		where_clause = true;
		cur = cur->next;
		
		if ((cur->tok_class != keyword) &&
				(cur->tok_class != identifier) &&
				(cur->tok_class != type_name)) 
		{
			printf("Error: Invalid column name\n");
			rc = INVALID_COLUMN_NAME;
			cur->tok_value = INVALID;
			return rc;
		}
		else 
		{
			// Check if valid column name in table 1
			bool foundColumn = false;

			col_entry1 = (cd_entry*)((char*)tab_entry1 + tab_entry1->cd_offset);	
			for (int ci = 0; ci < tab_entry1->num_columns; ci++) {
				if (strcmp(col_entry1->col_name, cur->tok_string) == 0) {
					foundColumn = true;
					col1_type = col_entry1->col_type;
					col1_ID = col_entry1->col_id;
					col1_tab = 1; 
					break;
				}
				col_entry1++;
			}

			// Check if valid column name in table 2
			if (!foundColumn && natural_join) {
				col_entry2 = (cd_entry*)((char*)tab_entry2 + tab_entry2->cd_offset);
				for (int ci = 0; ci < tab_entry2->num_columns; ci++) {
					if (strcmp(col_entry2->col_name, cur->tok_string) == 0) {
						foundColumn = true;
						col1_type = col_entry2->col_type;
						col1_ID = col_entry2->col_id;
						col1_tab = 2; 
						break;
					}
					col_entry2++;
				}
			}
		
			if (!foundColumn) {
				printf("Error: Invalid column name\n");
				rc = INVALID_COLUMN_NAME;
				cur->tok_value = INVALID;
				return rc;
			} else {
				// Check comparison operator
				cur = cur->next;
				// IS NULL
				if (cur->tok_value == K_IS && cur->next->tok_value == K_NULL) {
					condition1_op = S_ISNULL;
					cur = cur->next->next;
				}
				else if (cur->tok_value == K_IS && cur->next->tok_value == K_NOT && cur->next->next->tok_value == K_NULL) {
					condition1_op = S_ISNOTNULL;
					cur = cur->next->next->next;
				}
				else if (cur->tok_value == S_GREATER && col1_type == T_INT) {
					condition1_op = S_GREATER;
					cur = cur->next;
				}
				else if (cur->tok_value == S_LESS && col1_type == T_INT) {
					condition1_op = S_LESS;
					cur = cur->next;
				}
				else if (cur->tok_value == S_EQUAL) {
					condition1_op = S_EQUAL;
					cur = cur->next;
				} 
				else {
					printf("Error: Invalid comparison operator\n");
					rc = INVALID_TYPE;
					cur->tok_value = INVALID;
					return rc;
				}

				// Validate the value type
				if (condition1_op != S_ISNULL && condition1_op != S_ISNOTNULL) {
					if (cur->tok_value != INT_LITERAL && cur->tok_value != STRING_LITERAL && cur->tok_value != K_NULL) {
						rc = INVALID_TYPE;
						cur->tok_value = INVALID;
						printf("Error: Expected int, string, or null\n");
						return rc;
					}
					else if (col1_type == T_INT && cur->tok_value != INT_LITERAL) {
						rc = INVALID_TYPE;
						cur->tok_value = INVALID;
						printf("Error: Expected int literal\n");
						return rc;
					}
					else if (col1_type == T_CHAR && cur->tok_value != STRING_LITERAL) {
						rc = INVALID_TYPE;
						cur->tok_value = INVALID;
						printf("Error: Expected string literal\n");
						return rc;
					} else {
						// Save the value
						if (cur->tok_value == INT_LITERAL) {
							condition1_val = atoi(cur->tok_string);
						} else {
							strcpy(condition1_str, cur->tok_string);
						}
						cur = cur->next;
					}
				} // Finished validating value1
			}
		} // Finished checking first condition

		// Check AND/OR
		if (cur->tok_value == K_AND) {
			and_or = K_AND;
			cur = cur->next;
		} else if (cur->tok_value == K_OR) {
			and_or = K_OR;
			cur = cur->next;
		} 
		
		// Validate second condition
		if (and_or != -1) {
			
			if ((cur->tok_class != keyword) &&
				(cur->tok_class != identifier) &&
				(cur->tok_class != type_name)) 
			{
				printf("Error: Invalid column name\n");
				rc = INVALID_COLUMN_NAME;
				cur->tok_value = INVALID;
				return rc;
			}
			else 
			{
				// Check if valid column name in table 1
				bool foundColumn = false;

				col_entry1 = (cd_entry*)((char*)tab_entry1 + tab_entry1->cd_offset);	
				for (int ci = 0; ci < tab_entry1->num_columns; ci++) {
					if (strcmp(col_entry1->col_name, cur->tok_string) == 0) {
						foundColumn = true;
						col2_type = col_entry1->col_type;
						col2_ID = col_entry1->col_id;
						col2_tab = 1; 
						break;
					}
					col_entry1++;
				}

				// Check if valid column name in table 2
				if (!foundColumn && natural_join) {
					col_entry2 = (cd_entry*)((char*)tab_entry2 + tab_entry2->cd_offset);
					for (int ci = 0; ci < tab_entry2->num_columns; ci++) {
						if (strcmp(col_entry2->col_name, cur->tok_string) == 0) {
							foundColumn = true;
							col2_type = col_entry2->col_type;
							col2_ID = col_entry2->col_id;
							col2_tab = 2; 
							break;
						}
						col_entry2++;
					}
				}
				
				

				if (!foundColumn) {
					printf("Error: Invalid column name\n");
					rc = INVALID_COLUMN_NAME;
					cur->tok_value = INVALID;
					return rc;
				} else {
					// Check comparison operator
					cur = cur->next;
					// IS NULL
					if (cur->tok_value == K_IS && cur->next->tok_value == K_NULL) {
						condition2_op = S_ISNULL;
						cur = cur->next->next;
					}
					else if (cur->tok_value == K_IS && cur->next->tok_value == K_NOT && cur->next->next->tok_value == K_NULL) {
						condition2_op = S_ISNOTNULL;
						cur = cur->next->next->next;
					}
					else if (cur->tok_value == S_GREATER && col2_type == T_INT) {
						condition2_op = S_GREATER;
						cur = cur->next;
					}
					else if (cur->tok_value == S_LESS && col2_type == T_INT) {
						condition2_op = S_LESS;
						cur = cur->next;
					}
					else if (cur->tok_value == S_EQUAL) {
						condition2_op = S_EQUAL;
						cur = cur->next;
					} 
					else {
						printf("Error: Invalid comparison operator\n");
						rc = INVALID_TYPE;
						cur->tok_value = INVALID;
						return rc;
					}

					// Validate the value type
					if (condition2_op != S_ISNULL && condition2_op != S_ISNOTNULL) {
						if (cur->tok_value != INT_LITERAL && cur->tok_value != STRING_LITERAL && cur->tok_value != K_NULL) {
							rc = INVALID_TYPE;
							cur->tok_value = INVALID;
							printf("Error: Expected int, string, or null\n");
							return rc;
						}
						else if (col2_type == T_INT && cur->tok_value != INT_LITERAL) {
							rc = INVALID_TYPE;
							cur->tok_value = INVALID;
							printf("Error: Expected int literal\n");
							return rc;
						}
						else if (col2_type == T_CHAR && cur->tok_value != STRING_LITERAL) {
							rc = INVALID_TYPE;
							cur->tok_value = INVALID;
							printf("Error: Expected string literal\n");
							return rc;
						} else {
							// Save the value
							if (cur->tok_value == INT_LITERAL) {
								condition2_val = atoi(cur->tok_string);
							} else {
								strcpy(condition2_str, cur->tok_string);
								
							}
							cur = cur->next;
						}
					} // Finished validating value2
				}
			} // Finished checking second condition
		}
	} // End WHERE
	// else if () ORDER BY

	printf("select columns: ");
	for (int i = 0; i < numSelectColumns; i++) {
		printf("%d ", selectColID[i]);
	}
	printf("\n");

	if (natural_join) {
		printf("natural join ");
	} else {
		printf("not natural join ");
	}

	if (where_clause) {
		printf("where ");
		printf("column %d (type %d) from table %d ", col1_ID, col1_type, col1_tab);
		if (col1_type == T_INT) {
			printf("value %d ", condition1_val);
		} else {
			printf("string %s ", condition1_str);
		}

		if (and_or > 0) {
			printf("column %d (type %d) from table %d ", col2_ID, col2_type, col2_tab);
			if (col2_type == T_INT) {
				printf("value %d ", condition2_val);
			} else {
				printf("string %s ", condition2_str);
			}
		}
	} else {
		printf("no where ");
	}
	printf("\n\n");

	// FINISH PARSING

  if (!rc) {
    // Open table1 file for reading
    
    char filename1[MAX_IDENT_LEN + 4] = {0};
    strcpy(filename1, strcat(tab_entry1->table_name, ".tab"));

    if ((fp1 = fopen(filename1, "rbc")) == NULL) {
      printf("Error while opening %s file\n", filename1);
      rc = FILE_OPEN_ERROR;
      cur->tok_value = INVALID;
    } 

    // Get record size for table1
		if((fseek(fp1, 4, SEEK_SET)) == 0)
		{
			fread(&record_size1, sizeof(int), 1, fp1);
		}

		// Get number of records in table1
		if((fseek(fp1, 8, SEEK_SET)) == 0)
		{
			fread(&num_rows1, sizeof(int), 1, fp1);
		}

    // Get header offset for table1
		if((fseek(fp1, 12, SEEK_SET)) == 0)
		{
			fread(&h_offset1, sizeof(int), 1, fp1);
		}

    if (!rc) {
      char *header = "-----------------";
    
      if (!natural_join) { // TABLE 1 COLUMNS

				// PRINT HEADER
				if (select_star) { // Print all rows
					// Print top border
					for(int i = 0; i < tab_entry1->num_columns; i++) {
						printf("%s", header);
					}
					printf("\n");
					printf("|");

					// Print ALL column names
					int i;
					for(i = 0, col_entry1 = (cd_entry*)((char*)tab_entry1 + tab_entry1->cd_offset);
									i < tab_entry1->num_columns; i++, col_entry1++) 
					{
							printf("%-16s|", col_entry1->col_name);
					}
					printf("\n");

					// Print bottom border
					for(int i = 0; i < tab_entry1->num_columns; i++) {
						if(i == (tab_entry1->num_columns-1)) {
							printf("%s\n", header);
						} else {
							printf("%s", header);
						}
					}

				} else { // NOT SELECT *
					// Print top border
					for(int i = 0; i < numSelectColumns; i++) {
						printf("%s", header);
					}
					printf("\n");
					printf("|");

					// Print column names
					col_entry1 = (cd_entry*)((char*)tab_entry1 + tab_entry1->cd_offset);
					for (int i = 0; i < tab_entry1->num_columns; i++) {
						for (int j = 0; j < numSelectColumns; j++) {
							if (selectColID[j] == col_entry1->col_id) {
								printf("%-16s|", col_entry1->col_name);
							}
						}
						col_entry1++;
					}
					printf("\n");

					// Print bottom border
					for(int i = 0; i < numSelectColumns; i++) {
						if(i == (numSelectColumns - 1)) {
							printf("%s\n", header);
						} else {
							printf("%s", header);
						}
					}
				}

				// FINISH PRINTING HEADER

				int num_records_read = 0;

				// Print one row at a time
				while(num_records_read < num_rows1) {
					fseek(fp1, h_offset1 + (num_records_read * record_size1), SEEK_SET); // move file pointer to start of record
					
					col_entry1 = (cd_entry*)((char*)tab_entry1 + tab_entry1->cd_offset);
					bool allConditionsMet = false;

					// Check WHERE clause conditions for table 1
					if (where_clause) {
						
						// Move fp to the conditional column
						for (int ci = 0; ci < col1_ID; ci++) {
							fseek(fp1, 1 + col_entry1->col_len, SEEK_CUR);
							col_entry1++;
						}

						// Check if condition is met
						bool condition1Met = false;
						char size_byte = fgetc(fp1);
						
						if (condition1_op == S_ISNULL && size_byte == '\0') {
							condition1Met = true;
						} else if (condition1_op == S_ISNOTNULL && size_byte != '\0') {
							condition1Met= true;
						} else if (col1_type == T_INT) {
							int val = 0;
							fread(&val, sizeof(int), 1, fp1);
							if (condition1_op == S_EQUAL) {
								if (val == condition1_val) {
									condition1Met = true;
								}
							} else if (condition1_op == S_GREATER) {
								if (val > condition1_val) {
									condition1Met = true;
								}
							} else if (condition1_op == S_LESS) {
								if (val < condition1_val) {
									condition1Met = true;
								}
							}
						} else if (col1_type == T_CHAR) {
							char *str = (char*)calloc(col_entry1->col_len, 1);
							fread(str, col_entry1->col_len, 1, fp1);
							if (strcmp(str, condition1_str) == 0) {
								condition1Met = true;
							}
							free(str);
						}


						// Check for second condition
						if (and_or == -1 && condition1Met) {
							allConditionsMet = true;
						} else {
							// Need to check second condition. Reset col_entry1 to beginning of cd_entry
							fseek(fp1, h_offset1 + (num_records_read * record_size1), SEEK_SET); // move file pointer to start of record
							col_entry1 = (cd_entry*)((char*)tab_entry1 + tab_entry1->cd_offset);

							// Move fp2 to the conditional column
							for (int ci = 0; ci < col2_ID; ci++) {
								fseek(fp1, 1 + col_entry1->col_len, SEEK_CUR);
								col_entry1++;
							}

							// Check if condition is met
							bool condition2Met = false;
							char size_byte = fgetc(fp1);

							if (condition2_op == S_ISNULL && size_byte == '\0') {
								condition2Met = true;
							} else if (condition2_op == S_ISNOTNULL && size_byte != '\0') {
								condition2Met= true;
							} else if (col2_type == T_INT) {
								int val = 0;
								fread(&val, sizeof(int), 1, fp1);
								if (condition2_op == S_EQUAL) {
									if (val == condition2_val) {
										condition2Met = true;
									}
								} else if (condition2_op == S_GREATER) {
									if (val > condition2_val) {
										condition2Met = true;
									}
								} else if (condition2_op == S_LESS) {
									if (val < condition2_val) {
										condition2Met = true;
									}
								}
							} else if (col2_type == T_CHAR) {
								char *str = (char*)calloc(col_entry1->col_len, 1);
								fread(str, col_entry1->col_len, 1, fp1);
								if (strcmp(str, condition2_str) == 0) {
									condition2Met = true;
								}
								free(str);
							}


							// check for AND
							if (and_or == K_AND) {
								allConditionsMet = (condition1Met && condition2Met);
							}
							if (and_or == K_OR) {
								allConditionsMet = (condition1Met || condition2Met);
							}
						}
					} // Finish checking WHERE conditions

					// All conditions met. Print out
					if (!where_clause || allConditionsMet) {
						printf("|");
						// Reset file pointer to beginning of record
						fseek(fp1, h_offset1 + (num_records_read * record_size1), SEEK_SET); // move fp to next record
						col_entry1 = (cd_entry*)((char*)tab_entry1 + tab_entry1->cd_offset);

						for (int i = 0; i < tab_entry1->num_columns; i++) {

							bool selectedColumn = false;
							// Check if this column is a selected column
							if (!select_star && numSelectColumns > 0) {
								for (int j = 0; j < numSelectColumns; j++) {
									if (selectColID[j] == col_entry1->col_id) {
										selectedColumn = true;
										break;
									}
								}
							}

							if (select_star || selectedColumn) {
								char size_byte = fgetc(fp1);
								if (size_byte == '\0') { // Check if NULL
									if (col_entry1->col_type == T_INT) {
										printf("              - |");
									} else {
										printf(" -              |");
									}

									// move fp to next column
									char *str = (char*)calloc(col_entry1->col_len, 1);
									fread(str, col_entry1->col_len, 1, fp1);
									free(str);

								} else { // Not a NULL value
									if (col_entry1->col_type == T_INT) {
										int val = 0;
										fread(&val, sizeof(int), 1, fp1);
										printf("%15d |", val);
									} else {
										char *str = (char*)calloc(col_entry1->col_len, 1);
										fread(str, col_entry1->col_len, 1, fp1);
										printf("%-15s |", str);
										free(str);
									}
								}
							} 

							if (!select_star && !selectedColumn) {
								// Move fp to next column
								fseek(fp1, 1 + col_entry1->col_len, SEEK_CUR);
							}
								
							
							col_entry1++;
						}
						printf("\n");
					}

					
					num_records_read += 1;
				} // End while

				// Print bottom border
				if (select_star) {
					for(int i = 0; i < tab_entry1->num_columns; i++) {
						if(i == ((tab_entry1->num_columns) - 1)) {
							printf("%s\n", header);
						} else {
							printf("%s", header);
						}
					}
				} else {
					for(int i = 0; i < numSelectColumns; i++) {
						if(i == (numSelectColumns - 1)) {
							printf("%s\n", header);
						} else {
							printf("%s", header);
						}
					}
				}
			
				
      }
      else {  // NATURAL JOIN
				bool hasCommonAttribute = false;

				char filename2[MAX_IDENT_LEN + 4] = {0};
				strcpy(filename2, strcat(tab_entry2->table_name, ".tab"));

				if ((fp2 = fopen(filename2, "rbc")) == NULL) {
					printf("Error while opening %s file\n", filename2);
					rc = FILE_OPEN_ERROR;
					cur->tok_value = INVALID;
					return rc;
				} 

				// Get record size for table2
				if((fseek(fp2, 4, SEEK_SET)) == 0)
				{
					fread(&record_size2, sizeof(int), 1, fp2);
				}

				// Get number of records in table2
				if((fseek(fp2, 8, SEEK_SET)) == 0)
				{
					fread(&num_rows2, sizeof(int), 1, fp2);
				}

				// Get header offset for table2
				if((fseek(fp2, 12, SEEK_SET)) == 0)
				{
					fread(&h_offset2, sizeof(int), 1, fp2);
				}

				// 1. Find join attributes
				int num_columns1 = tab_entry1->num_columns;
				int num_columns2 = tab_entry2->num_columns;

				// change later
				int c_index = 0;
				int* commonAttributes1 = (int*)calloc(num_columns1 + num_columns2, sizeof(int));
				int* commonAttributes2 = (int*)calloc(num_columns1 + num_columns2, sizeof(int));


				int i, j;
				int count = 0;
				for(i = 0, col_entry1 = (cd_entry*)((char*)tab_entry1 + tab_entry1->cd_offset);
					i < num_columns1; i++, col_entry1++) 
        {
          for(j = 0, col_entry2 = (cd_entry*)((char*)tab_entry2 + tab_entry2->cd_offset);
						j < num_columns2; j++, col_entry2++) 
					{
						if (strcmp(col_entry1->col_name, col_entry2->col_name) == 0 
							&& col_entry1->col_type == col_entry2->col_type
							&& col_entry1->col_len == col_entry2->col_len) {
								commonAttributes1[c_index] = col_entry1->col_id; // Save the column ID from the first table
								commonAttributes2[c_index] = col_entry2->col_id; // Save the column ID from the second table
								c_index++;
								hasCommonAttribute = true;
						}
					}
        }


				if (c_index > 0) {
					// PRINT HEADER
					int num_joined_columns = 0;
					if (select_star) {
						// Print top border
						num_joined_columns = num_columns1 + num_columns2 - c_index;
						for(int i = 0; i < num_joined_columns; i++) {
							printf("%s", header);
						}
						printf("\n");
						printf("|");

						// Print ALL column names
						int i, j;
						// table 1
						for(i = 0, col_entry1 = (cd_entry*)((char*)tab_entry1 + tab_entry1->cd_offset);
									i < num_columns1; i++, col_entry1++) 
						{
							printf("%-16s|", col_entry1->col_name);
						}
						// table 2
						for(i = 0, col_entry2 = (cd_entry*)((char*)tab_entry2 + tab_entry2->cd_offset);
									i < num_columns2; i++, col_entry2++) 
						{
							bool foundColumn = false;
							for (j = 0; j < c_index; j++) {
								if (commonAttributes2[j] == col_entry2->col_id) {
									foundColumn = true;
								}
							}
							if (!foundColumn) {
								printf("%-16s|", col_entry2->col_name);
							}
						}
						printf("\n");

						// Print bottom border
						for(int i = 0; i < num_joined_columns; i++) {
							if(i == (num_joined_columns-1)) {
								printf("%s\n", header);
							} else {
								printf("%s", header);
							}
						}
        	} else { // NOT SELECT *
						// Print top border
						for(int i = 0; i < numSelectColumns; i++) {
							printf("%s", header);
						}
						printf("\n");
						printf("|");

						// Print column names
						col_entry1 = (cd_entry*)((char*)tab_entry1 + tab_entry1->cd_offset);
						col_entry2 = (cd_entry*)((char*)tab_entry2 + tab_entry2->cd_offset);
						for (int i = 0; i < numSelectColumns; i++) {
							int tabNum = selectColTable[i];
							int colId = selectColID[i];
							if (tabNum == 1) {
								printf("%-16s|", col_entry1[colId].col_name);
							} else {
								printf("%-16s|", col_entry2[colId].col_name);
							}
						}
						printf("\n");

						// Print bottom border
						for(int i = 0; i < numSelectColumns; i++) {
							if(i == (numSelectColumns - 1)) {
								printf("%s\n", header);
							} else {
								printf("%s", header);
							}
						}
					}

					// FINISH PRINTING HEADER

					// Print one row at a time

					// Read tuples of S
					for (int i = 0; i < num_rows1; i++) {
					
						for (int j = 0; j < num_rows2; j++) {

							// CHECKING VALUES
							
							bool same = checking_values(fp1, fp2, tab_entry1, tab_entry2, commonAttributes1, 
								commonAttributes2, c_index, record_size1, record_size2, i, j, h_offset1, h_offset2);

							if (same) {
								fseek(fp1, h_offset1 + (i * record_size1), SEEK_SET); // move file pointer to start of next record
								fseek(fp2, h_offset2 + (j * record_size2), SEEK_SET); // move file pointer to start of next record

								cd_entry *col_entry1 = (cd_entry*)((char*)tab_entry1 + tab_entry1->cd_offset);
								cd_entry *col_entry2 = (cd_entry*)((char*)tab_entry2 + tab_entry2->cd_offset);

								bool allConditionsMet = false;

								// Check WHERE clause conditions for table 1
								if (where_clause) {
									
									bool condition1Met = false;

									if (col1_tab == 1) { // Column in table 1
										// Move fp to the conditional column
										for (int ci = 0; ci < col1_ID; ci++) {
											fseek(fp1, 1 + col_entry1->col_len, SEEK_CUR);
											col_entry1++;
										}

										// Check if condition is met
										char size_byte = fgetc(fp1);

										if (condition1_op == S_ISNULL && size_byte == '\0') {
											condition1Met = true;
										} else if (condition1_op == S_ISNOTNULL && size_byte != '\0') {
											condition1Met= true;
										} else if (col1_type == T_INT) {
											int val = 0;
											fread(&val, sizeof(int), 1, fp1);
											// printf("%d and %d\n", val, condition1_val);
											if (condition1_op == S_EQUAL) {
												if (val == condition1_val) {
													condition1Met = true;
												}
											} else if (condition1_op == S_GREATER) {
												if (val > condition1_val) {
													condition1Met = true;
												}
											} else if (condition1_op == S_LESS) {
												if (val < condition1_val) {
													condition1Met = true;
												}
											}
										} else if (col1_type == T_CHAR) {
											char *str = (char*)calloc(col_entry1->col_len, 1);
											fread(str, col_entry1->col_len, 1, fp1);
											// printf("%s and %s\n", str, condition1_str);
											if (strcmp(str, condition1_str) == 0) {
												condition1Met = true;
											}
											free(str);
										}
									} else { // Column in table 2

										// Move fp to the conditional column
										for (int ci = 0; ci < col1_ID; ci++) {
											fseek(fp2, 1 + col_entry2->col_len, SEEK_CUR);
											col_entry2++;
										}

										// Check if condition is met
										char size_byte = fgetc(fp2);

										if (condition1_op == S_ISNULL && size_byte == '\0') {
											condition1Met = true;
										} else if (condition1_op == S_ISNOTNULL && size_byte != '\0') {
											condition1Met= true;
										} else if (col1_type == T_INT) {
											int val = 0;
											fread(&val, sizeof(int), 1, fp2);
											// printf("%d and %d\n", val, condition1_val);
											if (condition1_op == S_EQUAL) {
												if (val == condition1_val) {
													condition1Met = true;
												}
											} else if (condition1_op == S_GREATER) {
												if (val > condition1_val) {
													condition1Met = true;
												}
											} else if (condition1_op == S_LESS) {
												if (val < condition1_val) {
													condition1Met = true;
												}
											}
										} else if (col1_type == T_CHAR) {
											char *str = (char*)calloc(col_entry2->col_len, 1);
											fread(str, col_entry2->col_len, 1, fp2);
											// printf("%s and %s\n", str, condition1_str);
											if (strcmp(str, condition1_str) == 0) {
												condition1Met = true;
											}
											free(str);
										}
									}

									// Check for second condition
									if (and_or == -1 && condition1Met) {
										allConditionsMet = true;
									} else {
										// Need to check second condition. Reset col_entry1 to beginning of cd_entry
								
										bool condition2Met = false;

										if (col2_tab == 1) { // table 1

											fseek(fp1, h_offset1 + (i * record_size1), SEEK_SET); // move file pointer to start of next record
											col_entry1 = (cd_entry*)((char*)tab_entry1 + tab_entry1->cd_offset);
											
											// Move fp to the conditional column
											for (int ci = 0; ci < col2_ID; ci++) {
												fseek(fp1, 1 + col_entry1->col_len, SEEK_CUR);
												col_entry1++;
											}

											// Check if condition is met
											char size_byte = fgetc(fp1);

											if (condition2_op == S_ISNULL && size_byte == '\0') {
												condition2Met = true;
											} else if (condition2_op == S_ISNOTNULL && size_byte != '\0') {
												condition2Met= true;
											} else if (col1_type == T_INT) {
												int val = 0;
												fread(&val, sizeof(int), 1, fp1);
												// printf("%d and %d\n", val, condition2_val);
												if (condition2_op == S_EQUAL) {
													if (val == condition2_val) {
														condition2Met = true;
													}
												} else if (condition2_op == S_GREATER) {
													if (val > condition2_val) {
														condition2Met = true;
													}
												} else if (condition2_op == S_LESS) {
													if (val < condition2_val) {
														condition2Met = true;
													}
												}
											} else if (col2_type == T_CHAR) {
												char *str = (char*)calloc(col_entry1->col_len, 1);
												fread(str, col_entry1->col_len, 1, fp1);
												// printf("%s and %s\n", str, condition2_str);
												if (strcmp(str, condition2_str) == 0) {
													condition2Met = true;
												}
												free(str);
											}
										} else if (col2_tab == 2) { // TABLE 2

											fseek(fp2, h_offset2 + (j * record_size2), SEEK_SET); // move file pointer to start of next record
											col_entry2 = (cd_entry*)((char*)tab_entry2 + tab_entry2->cd_offset);
											
											// Move fp to the conditional column
											for (int ci = 0; ci < col2_ID; ci++) {
												fseek(fp2, 1 + col_entry2->col_len, SEEK_CUR);
												col_entry2++;
											}

											// Check if condition is met
											char size_byte = fgetc(fp2);

											if (condition2_op == S_ISNULL && size_byte == '\0') {
												condition2Met = true;
											} else if (condition2_op == S_ISNOTNULL && size_byte != '\0') {
												condition2Met= true;
											} else if (col2_type == T_INT) {
												int val = 0;
												fread(&val, sizeof(int), 1, fp2);
												// printf("%d and %d\n", val, condition2_val);
												if (condition2_op == S_EQUAL) {
													if (val == condition2_val) {
														condition2Met = true;
													}
												} else if (condition2_op == S_GREATER) {
													if (val > condition2_val) {
														condition2Met = true;
													}
												} else if (condition2_op == S_LESS) {
													if (val < condition2_val) {
														condition2Met = true;
													}
												}
											} else if (col2_type == T_CHAR) {
												char *str = (char*)calloc(col_entry2->col_len, 1);
												fread(str, col_entry2->col_len, 1, fp2);
												// printf("%s and %s\n", str, condition2_str);
												if (strcmp(str, condition2_str) == 0) {
													condition2Met = true;
												}
												free(str);
											}
										}

										// check for AND
										if (and_or == K_AND) {
											allConditionsMet = (condition1Met && condition2Met);
										}
										if (and_or == K_OR) {
											allConditionsMet = (condition1Met || condition2Met);
										}
									}
								} // Finished checking WHERE conditions

								// printf("all conditions met = %d\n", allConditionsMet);

								if (!where_clause || allConditionsMet) {
									printf("|");

									// Reset file pointer to beginning of record
									fseek(fp1, h_offset1 + (i * record_size1), SEEK_SET); // move file pointer to start of next record
									fseek(fp2, h_offset2 + (j * record_size2), SEEK_SET); // move file pointer to start of next record

									cd_entry *col_entry1 = (cd_entry*)((char*)tab_entry1 + tab_entry1->cd_offset);
									cd_entry *col_entry2 = (cd_entry*)((char*)tab_entry2 + tab_entry2->cd_offset);

									if (select_star) {
										// Read one record
										for (int ri = 0; ri < num_columns1; ri++) {   
											char size_byte = fgetc(fp1);
											if (size_byte == '\0') { // Check if NULL
												if (col_entry1->col_type == T_INT) {
													printf("              - |");
												} else {
													printf(" -              |");
												}

												// move fp to next column
												char *str = (char*)calloc(col_entry1->col_len, 1);
												fread(str, col_entry1->col_len, 1, fp1);
												free(str);

											} else { // Not a NULL value
												if (col_entry1->col_type == T_INT) {
													int val = 0;
													fread(&val, sizeof(int), 1, fp1);
													printf("%15d |", val);
												} else {
													char *str = (char*)calloc(col_entry1->col_len, 1);
													fread(str, col_entry1->col_len, 1, fp1);
													printf("%-15s |", str);
													free(str);
												}
											}
											col_entry1++;
										} 

										// PRINT TABLE 2
										for (int rj = 0; rj < num_columns2; rj++) {   
											bool foundCommon = false;
											for (int x = 0; x < c_index; x++) {
												
												if (commonAttributes2[x] == col_entry2->col_id) {
													foundCommon = true;
													break;
												}
											}

											if (!foundCommon) {
												char size_byte = fgetc(fp2);
												if (size_byte == '\0') { // Check if NULL
													if (col_entry2->col_type == T_INT) {
														printf("           NULL |");
													} else {
														printf("NULL            |");
													}

													// move fp to next column
													char *str = (char*)calloc(col_entry2->col_len, 1);
													fread(str, col_entry2->col_len, 1, fp2);
													free(str);

												} else { // Not a NULL value
													if (col_entry2->col_type == T_INT) {
														int val = 0;
														fread(&val, sizeof(int), 1, fp2);
														printf("%15d |", val);
													} else {
														char *str = (char*)calloc(col_entry2->col_len, 1);
														fread(str, col_entry2->col_len, 1, fp2);
														printf("%-15s |", str);
														free(str);
													}
												} 
											} else {
												// skip this value
												char size_byte = fgetc(fp2);
												char *str = (char*)calloc(col_entry2->col_len, 1);
												fread(str, col_entry2->col_len, 1, fp2);
												free(str);
											}
												
											col_entry2++;									
										} 
										printf("\n");
									} else { // NOT SELECT * 
										// Print column names
										
										for (int i = 0; i < numSelectColumns; i++) {
											fseek(fp1, h_offset1 + (i * record_size1), SEEK_SET); // move file pointer to start of next record
											fseek(fp2, h_offset2 + (j * record_size2), SEEK_SET); // move file pointer to start of next record
											col_entry1 = (cd_entry*)((char*)tab_entry1 + tab_entry1->cd_offset);
											col_entry2 = (cd_entry*)((char*)tab_entry2 + tab_entry2->cd_offset);
											
											int tabNum = selectColTable[i];
											int colId = selectColID[i];
											
											if (tabNum == 1) {
												// Move fp to that column
												for (int ci = 0; ci < colId; ci++) {
													fseek(fp1, 1 + col_entry1->col_len, SEEK_CUR);
													col_entry1++;
												}

												char size_byte = fgetc(fp1);
												if (size_byte == '\0') { // Check if NULL
													if (col_entry1->col_type == T_INT) {
														printf("              - |");
													} else {
														printf(" -              |");
													}
												} else { // not a NULL value
													if (col_entry1->col_type == T_INT) {
														int val = 0;
														fread(&val, sizeof(int), 1, fp1);
														printf("%15d |", val);
													} else {
														char *str = (char*)calloc(col_entry1->col_len, 1);
														fread(str, col_entry1->col_len, 1, fp1);
														printf("%-15s |", str);
														free(str);
													}
												}
											} else { // TABLE 2
												// Move fp to that column
												for (int ci = 0; ci < colId; ci++) {
													fseek(fp2, 1 + col_entry2->col_len, SEEK_CUR);
													col_entry2++;
												}

												char size_byte = fgetc(fp2);
												if (size_byte == '\0') { // Check if NULL
													if (col_entry2->col_type == T_INT) {
														printf("              - |");
													} else {
														printf(" -              |");
													}
												} else { // not a NULL value
													if (col_entry2->col_type == T_INT) {
														int val = 0;
														fread(&val, sizeof(int), 1, fp2);
														printf("%15d |", val);
													} else {
														char *str = (char*)calloc(col_entry2->col_len, 1);
														fread(str, col_entry2->col_len, 1, fp2);
														printf("%-15s |", str);
														free(str);
													}
												}
											} 
										} // End for - select Columns
										printf("\n");
									} // Finished printing selected columns
								}
							} 
						}	
					} // Finish iterating through t1 and t2

					// Print bottom border
					if (select_star) {
						for(int i = 0; i < num_joined_columns; i++) {
							if(i == (num_joined_columns-1)) {
								printf("%s\n", header);
							} else {
								printf("%s", header);
							}
						}
					} else {
						for(int i = 0; i < numSelectColumns; i++) {
							if(i == (numSelectColumns - 1)) {
								printf("%s\n", header);
							} else {
								printf("%s", header);
							}
						}
					}
					
				}
				free(commonAttributes1);
				free(commonAttributes2);
      } // End natural join

    }
		fclose(fp1);
		fclose(fp2);
  
  }

  return rc;
}

/*
* fp1 - points to a record in table1
* fp2 - points to a record in table2
* col_entry1 - column descriptors for table 1
* col_entry2 - column descriptors for table 2
* commonAttributes - index of the columns in common
*/
bool checking_values(
	FILE *fp1, 
	FILE *fp2, 
	tpd_entry *tab_entry1, 
	tpd_entry *tab_entry2,
	int *commonAttributes1,
	int *commonAttributes2,
	int c_index,
	int record_size1,
	int record_size2,
	int i,
	int j,
	int h_offset1,
	int h_offset2
) {
	bool matchingValues = false;

	for (int ci = 0; ci < c_index; ci++) {

		cd_entry *col_entry1 = (cd_entry*)((char*)tab_entry1 + tab_entry1->cd_offset);
		cd_entry *col_entry2 = (cd_entry*)((char*)tab_entry2 + tab_entry2->cd_offset);

		fseek(fp1, h_offset1 + (i * record_size1), SEEK_SET); // move file pointer to start of next record
		fseek(fp2, h_offset2 + (j * record_size2), SEEK_SET); // move file pointer to start of next record

		int col1 = commonAttributes1[ci];
		int col2 = commonAttributes2[ci];

		int int1, int2;
		char *str1, *str2;

		// TABLE 1
		// Move fp to column in record
		for (int k = 0; k < col1; k++) {
			fseek(fp1, 1 + col_entry1->col_len, SEEK_CUR);
			col_entry1++;
		}
		// Read the value
		char size_byte1 = fgetc(fp1);
		if (col_entry1->col_type  == T_INT) {
			fread(&int1, sizeof(int), 1, fp1);
		} else if (col_entry1->col_type  == T_CHAR) {
			str1 = (char*)calloc(col_entry1->col_len, 1);
			fread(str1, col_entry1->col_len, 1, fp1);
		}

		// TABLE 2
		// Move fp to column in record
		for (int l = 0; l < col2; l++) {
			fseek(fp2, 1 + col_entry2->col_len, SEEK_CUR);
			col_entry2++;
		}
		// Read the value
		char size_byte2 = fgetc(fp2);
		if (col_entry2->col_type  == T_INT) {
			fread(&int2, sizeof(int), 1, fp2);
		} else if (col_entry2->col_type  == T_CHAR) {
			str2 = (char*)calloc(col_entry2->col_len, 1);
			fread(str2, col_entry2->col_len, 1, fp2);
		}


		if (col_entry1->col_type == T_INT) {
			if (int1 != int2) {
				return false;
			}
		} else if (col_entry1->col_type == T_CHAR) {
			if (strcmp(str1, str2) != 0) {
				return false;
			}
			free(str1);
			free(str2);
		}

		fflush(fp1);
		fflush(fp2);	
	} // end for

	matchingValues = true;

	return matchingValues;
}

int sem_delete_from(token_list *t_list) {
	int rc = 0;
  token_list *cur;
  tpd_entry *tab_entry = NULL;
  cd_entry  *col_entry = NULL;

  FILE *fp = NULL;
  FILE *rp = NULL;

	int file_size = 0;
  int record_size = 0;
	int num_records = 0;
	int h_offset = 0;

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

			// Get record size
			if ((rp = fopen(filename, "rb")) == NULL) {
				printf("Error while opening %s file\n", filename);
				rc = FILE_OPEN_ERROR;
				cur->tok_value = INVALID;
			} else {
				// Read file size
				if ((fseek(rp, 0, SEEK_SET)) == 0) {
					fread(&file_size, sizeof(int), 1, rp);
				}
				// Read record size
				if ((fseek(rp, 4, SEEK_SET)) == 0) {
					fread(&record_size, sizeof(int), 1, rp);
				}
				// Read num records
				if ((fseek(rp, 8, SEEK_SET)) == 0) {
					fread(&num_records, sizeof(int), 1, rp);
				}
				// Read header offset
				if ((fseek(rp, 12, SEEK_SET)) == 0) {
					fread(&h_offset, sizeof(int), 1, rp);
				}
				fclose(rp);
			}
			
      if ((fp = fopen(filename, "rb+")) == NULL) {
        printf("Error while opening %s file\n", filename);
        rc = FILE_OPEN_ERROR;
        cur->tok_value = INVALID;
      } else {
        cur = cur->next;
				// delete from table\0

				if (cur->tok_value == EOC) {
					printf("EOC\n");
					// Delete all tuples from table
					// Set number of records to 0
					num_records = 0;
					if ((fseek(rp, 8, SEEK_SET)) == 0) {
						fwrite(&num_records, sizeof(int), 1, fp);
					}
					// Set file size to header offset
					file_size = h_offset;
					if ((fseek(rp, 0, SEEK_SET)) == 0) {
						fwrite(&file_size, sizeof(int), 1, fp);
					}
				}
				else 
				{
					// delete from table where colNam

					int comparisonOp = 0;
					int conditionCol_type = 0;
					int conditionCol_ID = 0;
				
					if (cur->tok_value != K_WHERE) {
						rc = INVALID_STATEMENT;
						cur->tok_value = INVALID;
					} else {
						cur = cur->next;
						col_entry = (cd_entry*)((char*)tab_entry + tab_entry->cd_offset);
						
						if ((cur->tok_class != keyword) &&
								(cur->tok_class != identifier) &&
								(cur->tok_class != type_name)) 
						{
							printf("Error: Invalid column name\n");
							rc = INVALID_COLUMN_NAME;
							cur->tok_value = INVALID;
						}
						else 
						{
							// Check if valid column name
							bool foundColumn = false;

							for (int ci = 0; ci < tab_entry->num_columns; ci++) {
								if (strcmp(col_entry->col_name, cur->tok_string) == 0) {
									foundColumn = true;
									conditionCol_type = col_entry->col_type;
									conditionCol_ID = col_entry->col_id;
									break;
								}
								col_entry++;
							}

							if (!foundColumn) {
								printf("Error: Invalid column name\n");
								rc = INVALID_COLUMN_NAME;
								cur->tok_value = INVALID;
							} else {
								// Check comparison operator
								cur = cur->next;
								if (cur->tok_value == S_GREATER && conditionCol_type == T_INT) {
									comparisonOp = S_GREATER;
								}
								else if (cur->tok_value == S_LESS && conditionCol_type == T_INT) {
									comparisonOp = S_LESS;
								}
								else if (cur->tok_value == S_EQUAL) {
									comparisonOp = S_EQUAL;
								} 
								else {
									printf("Error: Invalid comparison operator\n");
									rc = INVALID_TYPE;
									cur->tok_value = INVALID;
								}
							}
						}
					}
					
					if (!rc) {
						int compValue;
						char compString[MAX_TOK_LEN];
						cur = cur->next;
						if (cur->tok_value != INT_LITERAL && cur->tok_value != STRING_LITERAL && cur->tok_value != K_NULL) {
							rc = INVALID_TYPE;
							cur->tok_value = INVALID;
							printf("Error: Expected int, string, or null\n");
						}
						else if (conditionCol_type == T_INT && cur->tok_value != INT_LITERAL) {
							rc = INVALID_TYPE;
							cur->tok_value = INVALID;
							printf("Error: Expected int literal\n");
						}
						else if (conditionCol_type == T_CHAR && cur->tok_value != STRING_LITERAL) {
							rc = INVALID_TYPE;
							cur->tok_value = INVALID;
							printf("Error: Expected string literal\n");
						} 
						else {
							// save the comparison value
							if (cur->tok_value == INT_LITERAL) {
								compValue = atoi(cur->tok_string);
							} else {
								strcpy(compString, cur->tok_string);
							}

							bool atLeastOneRowFound = false;

							int records_read = 0;
							while (records_read < num_records) {
								fseek(fp, h_offset + (records_read * record_size), SEEK_SET); // move fp to next record
								col_entry = (cd_entry*)((char*)tab_entry + tab_entry->cd_offset);
								
								// Move fp to the conditional column
								for (int ci = 0; ci < conditionCol_ID; ci++) {
									fseek(fp, 1 + col_entry->col_len, SEEK_CUR);
									col_entry++;
								}

								// Check if condition is met
								bool conditionMet = false;
								char size_byte = fgetc(fp);
								if (size_byte == '\0') {
									records_read++;
								} else {
									if (conditionCol_type == T_INT) {
										int val = -1;
										fread(&val, sizeof(int), 1, fp);
										if (comparisonOp == S_EQUAL) {
											if (val == compValue) {
												conditionMet = true;
												atLeastOneRowFound = true;
											}
										} else if (comparisonOp == S_GREATER) {
											if (val > compValue) {
												conditionMet = true;
												atLeastOneRowFound = true;
											}
										} else if (comparisonOp == S_LESS) {
											if (val < compValue) {
												conditionMet = true;
												atLeastOneRowFound = true;
											}
										}
									} else { // char
										char *str = (char*)calloc(col_entry->col_len, 1);
										fread(str, col_entry->col_len, 1, fp);
										if (strcmp(str, compString) == 0) {
											conditionMet = true;
											atLeastOneRowFound = true;
										}
										free(str);
									}

									if (conditionMet) {
										// Move file pointer to the last tuple in table
										fseek(fp, h_offset + ((num_records - 1) * record_size), SEEK_SET);
										// Save the last tuple from table
										char *last_record = (char*)calloc(record_size, 1);
										fread(last_record, record_size, 1, fp);
										
										// Reset file pointer to beginning of the record to be deleted
										fseek(fp, h_offset + (records_read * record_size), SEEK_SET); 

										// Replace current record with the last record
										fwrite(last_record, record_size, 1, fp);
										free(last_record);

										// Decrement record count in tab file
										num_records--;
										if ((fseek(rp, 8, SEEK_SET)) == 0) {
											fwrite(&num_records, sizeof(int), 1, fp);
										}
										// Decrement file size in tab file
										file_size -= record_size;
										if ((fseek(rp, 0, SEEK_SET)) == 0) {
											fwrite(&file_size, sizeof(int), 1, fp);
										}

										fflush(fp);
										
										// Stay at this current record that was just placed
									} else {
										// Go to the next record
										records_read++;
									}
								}
							}

							if (!atLeastOneRowFound) {
								printf("No rows found.\n");
							}
						}
					}
				}
				fclose(fp);
			}
		}
	}

	return rc;

}

int sem_update(token_list *t_list) {
	int rc = 0;
  token_list *cur;
  tpd_entry *tab_entry = NULL;
  cd_entry  *col_entry = NULL;

  FILE *fp = NULL;
  FILE *rp = NULL;

  int record_size = 0;
	int num_records = 0;
	int h_offset = 0;
	int num_columns = 0;
	int newValue;
	char newString[MAX_TOK_LEN];
	int updateColumm_type = 0;
	int updateColumn_ID = 0;

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

			// Get number of columns
			num_columns = tab_entry->num_columns;

			// Get record size
			if ((rp = fopen(filename, "r+b")) == NULL) {
				printf("Error while opening %s file\n", filename);
				rc = FILE_OPEN_ERROR;
				cur->tok_value = INVALID;
			} else {
				// Read record size
				if ((fseek(rp, 4, SEEK_SET)) == 0) {
					fread(&record_size, sizeof(int), 1, rp);
				}
				// Read num records
				if ((fseek(rp, 8, SEEK_SET)) == 0) {
					fread(&num_records, sizeof(int), 1, rp);
				}
				// Read header offset
				if ((fseek(rp, 12, SEEK_SET)) == 0) {
					fread(&h_offset, sizeof(int), 1, rp);
				}
				fclose(rp);
			}

			if ((fp = fopen(filename, "rb+")) == NULL) {
        printf("Error while opening %s file\n", filename);
        rc = FILE_OPEN_ERROR;
        cur->tok_value = INVALID;
      } else {
				cur = cur->next;
				if (cur->tok_value != K_SET) {
					rc = INVALID_STATEMENT;
					cur->tok_value = INVALID;
					printf("Error: Invalid syntax. Expected SET\n");
				} else {
					
					cur = cur->next;
					col_entry = (cd_entry*)((char*)tab_entry + tab_entry->cd_offset);
					if ((cur->tok_class != keyword) &&
							(cur->tok_class != identifier) &&
							(cur->tok_class != type_name)) 
					{
						printf("Error: Invalid column name\n");
						rc = INVALID_COLUMN_NAME;
						cur->tok_value = INVALID;
					} else {
						// Check if valid column name
						bool foundColumn = false;

						for (int ci = 0; ci < tab_entry->num_columns; ci++) {
							if (strcmp(col_entry->col_name, cur->tok_string) == 0) {
								foundColumn = true;
								updateColumm_type = col_entry->col_type;
								updateColumn_ID = col_entry->col_id;
								break;
							}
							col_entry++;
						}

						if (!foundColumn) {
							printf("Error: Invalid column name\n");
							rc = INVALID_COLUMN_NAME;
							cur->tok_value = INVALID;
						} else {
							// Next operator should be =
							cur = cur->next;
							if (cur->tok_value != S_EQUAL) {
								printf("Error: Invalid operator\n");
								rc = INVALID_STATEMENT;
								cur->tok_value = INVALID;
							}
						}
					}
				}
			}
		}
	}

	if (!rc) {			
		cur = cur->next;
		if (cur->tok_value != INT_LITERAL && cur->tok_value != STRING_LITERAL && cur->tok_value != K_NULL) {
			rc = INVALID_TYPE;
			cur->tok_value = INVALID;
			printf("Error: Expected int, string, or null\n");
		} else if (updateColumm_type == T_INT && cur->tok_value != INT_LITERAL) {
			rc = INVALID_TYPE;
			cur->tok_value = INVALID;
			printf("Error: Expected int literal\n");
		} else if (updateColumm_type == T_CHAR && cur->tok_value != STRING_LITERAL) {
			rc = INVALID_TYPE;
			cur->tok_value = INVALID;
			printf("Error: Expected string literal\n");
		} else {
			// save the comparison value
			if (cur->tok_value == INT_LITERAL) {
				newValue = atoi(cur->tok_string);
			} else {
				strcpy(newString, cur->tok_string);
			}
		
			cur = cur->next;
			if (cur->tok_value == EOC) {

				// if there is no WHERE clause
				for (int ri = 0; ri < num_records; ri++) {
					fseek(fp, h_offset + (ri * record_size), SEEK_SET); // move fp to next record
					col_entry = (cd_entry*)((char*)tab_entry + tab_entry->cd_offset);
					
					// Move fp to the column to update
					for (int ci = 0; ci < updateColumn_ID; ci++) {
						fseek(fp, 1 + col_entry->col_len, SEEK_CUR);
						col_entry++;
					}
					
					// Update the value
					if (updateColumm_type == T_INT) {
						printf("here\n");
						int int_size = sizeof(int);
						fwrite(&int_size, 1, 1, fp);
						fwrite(&newValue, sizeof(int), 1, fp);
					} else {
						int str_length = strlen(newString);
						int max_length = col_entry->col_len;
						fwrite(&str_length, 1, 1, fp);
						fwrite(newString, str_length, 1, fp);
						int padding = max_length - str_length;
						int zero = 0;
						if (padding > 0) {
							fwrite(&zero, padding, 1, fp);
						}
					}		
					fflush(fp);
				}

			} else if (cur->tok_value != K_WHERE) {
				rc = INVALID_STATEMENT;
				cur->tok_value = INVALID;
				printf("Error: Invalid syntax after update\n");
			} else {
				cur = cur->next;

				int relationalOp = 0;
				int conditionCol_type = 0;
				int conditionCol_ID = 0;
				col_entry = (cd_entry*)((char*)tab_entry + tab_entry->cd_offset);
						
				if ((cur->tok_class != keyword) &&
						(cur->tok_class != identifier) &&
						(cur->tok_class != type_name)) 
				{
					printf("Error: Invalid column name\n");
					rc = INVALID_COLUMN_NAME;
					cur->tok_value = INVALID;
				}else 
				{
					// Check if valid column name
					bool foundColumn = false;

					for (int ci = 0; ci < tab_entry->num_columns; ci++) {
						if (strcmp(col_entry->col_name, cur->tok_string) == 0) {
							foundColumn = true;
							conditionCol_type = col_entry->col_type;
							conditionCol_ID = col_entry->col_id;
							break;
						}
						col_entry++;
					}

					if (!foundColumn) {
						printf("Error: Invalid column name\n");
						rc = INVALID_COLUMN_NAME;
						cur->tok_value = INVALID;
					} else {
						// Check comparison operator
						cur = cur->next;
						if (cur->tok_value == S_GREATER && conditionCol_type == T_INT) {
							relationalOp = S_GREATER;
						}
						else if (cur->tok_value == S_LESS  && conditionCol_type == T_INT) {
							relationalOp = S_LESS;
						}
						else if (cur->tok_value == S_EQUAL) {
							relationalOp = S_EQUAL;
						} 
						else {
							printf("Error: Invalid condition operator\n");
							rc = INVALID_TYPE;
							cur->tok_value = INVALID;
						}
					}
				}

				if (!rc) {
					int conditionalValue;
					char conditionalStr[MAX_TOK_LEN];
					cur = cur->next;
					if (cur->tok_value != INT_LITERAL && cur->tok_value != STRING_LITERAL && cur->tok_value != K_NULL) {
						rc = INVALID_TYPE;
						cur->tok_value = INVALID;
						printf("Error: Expected int, string, or null\n");
					}
					else if (conditionCol_type == T_INT && cur->tok_value != INT_LITERAL) {
						rc = INVALID_TYPE;
						cur->tok_value = INVALID;
						printf("Error: Expected int literal\n");
					}
					else if (conditionCol_type == T_CHAR && cur->tok_value != STRING_LITERAL) {
						rc = INVALID_TYPE;
						cur->tok_value = INVALID;
						printf("Error: Expected string literal\n");
					} else {
						// save the conditional value
						if (cur->tok_value == INT_LITERAL) {
							conditionalValue = atoi(cur->tok_string);
						} else {
							strcpy(conditionalStr, cur->tok_string);
						}

						bool atLeastOneRowFound = false;

						for (int ri = 0; ri < num_records; ri++) {
							fseek(fp, h_offset + (ri * record_size), SEEK_SET); // move fp to next record
							col_entry = (cd_entry*)((char*)tab_entry + tab_entry->cd_offset);
							
							// Move fp to the conditional column
							for (int ci = 0; ci < conditionCol_ID; ci++) {
								fseek(fp, 1 + col_entry->col_len, SEEK_CUR);
								col_entry++;
							}

							// Check if condition is met
							bool conditionMet = false;
							char size_byte = fgetc(fp);
							if (size_byte == '\0') {
								continue;
							} 

							if (conditionCol_type == T_INT) {
								int val = 0;
								fread(&val, sizeof(int), 1, fp);
								if (relationalOp == S_EQUAL) {
									if (val == conditionalValue) {
										conditionMet = true;
										atLeastOneRowFound = true;
									}
								} else if (relationalOp == S_GREATER) {
									if (val > conditionalValue) {
										conditionMet = true;
										atLeastOneRowFound = true;
									}
								} else if (relationalOp == S_LESS) {
									if (val < conditionalValue) {
										conditionMet = true;
										atLeastOneRowFound = true;
									}
								}
							} else { // char
								char *str = (char*)calloc(col_entry->col_len, 1);
								fread(str, col_entry->col_len, 1, fp);
								if (strcmp(str, conditionalStr) == 0) {
									conditionMet = true;
									atLeastOneRowFound = true;
								}
								free(str);
							}

							if (conditionMet) {
								// Reset file pointer to beginning of record
								fseek(fp, h_offset + (ri * record_size), SEEK_SET); // move fp to next record
								col_entry = (cd_entry*)((char*)tab_entry + tab_entry->cd_offset);
								
								// Move fp to the column to update
								for (int ci = 0; ci < updateColumn_ID; ci++) {
									fseek(fp, 1 + col_entry->col_len, SEEK_CUR);
									col_entry++;
								}
								// Update the value
								if (updateColumm_type == T_INT) {
									int int_size = sizeof(int);
									fwrite(&int_size, 1, 1, fp);
									fwrite(&newValue, sizeof(int), 1, fp);
								} else {
									int str_length = strlen(newString);
									int max_length = col_entry->col_len;
									fwrite(&str_length, 1, 1, fp);
									fwrite(newString, str_length, 1, fp);
									int padding = max_length - str_length;
									int zero = 0;
									if (padding > 0) {
										fwrite(&zero, padding, 1, fp);
									}
								}		
								fflush(fp);
							} 
						}

						if (!atLeastOneRowFound) {
							printf("No rows found.\n");
						}
					}
				}
			}
		}
	}

	return rc;
}


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
		// printf("dbfile.bin size = %d\n", file_stat.st_size);

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
						const char* table_name = (char *)malloc(strlen(cur->table_name) + strlen(".tab") + 1);
						if (g_tpd_list->num_tables == 0)
						{
							/* This is the last table, null out dummy header */
							memset((void*)g_tpd_list, '\0', sizeof(tpd_list));
							g_tpd_list->list_size = sizeof(tpd_list);
							fwrite(g_tpd_list, sizeof(tpd_list), 1, fhandle);
							remove(table_name);
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
							remove(table_name);
						}
					}
					else
					{
						const char* table_name = (char *)malloc(strlen(cur->table_name) + strlen(".tab") + 1);
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
							remove(table_name);
						}
						else
						{
							/* NOT the last entry, copy everything from the beginning of the
							   next entry which is (cur + cur->tpd_size) and the remaining size */
							fwrite((char*)cur + cur->tpd_size,
										 old_size - cur->tpd_size -
										 ((char*)cur - (char*)g_tpd_list),							     
								     1, fhandle);
							remove(table_name);
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
