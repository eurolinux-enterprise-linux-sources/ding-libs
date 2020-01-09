/*
    INI LIBRARY

    Parsing functions of the INI interface

    Copyright (C) Dmitri Pal <dpal@redhat.com> 2009

    INI Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    INI Library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with INI Library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "config.h"
#include <stdio.h>
/* For error text */
#include <libintl.h>
#define _(String) gettext (String)
/* INI file is used as a collection */
#include "trace.h"
#include "collection.h"
#include "collection_tools.h"
#include "ini_defines.h"
#include "ini_configobj.h"
#include "ini_config_priv.h"

/* Following declarations are from header file ini_config.h. This file was not
 * included, because we don't want on include header file
 * with old interface(ini_config.h) and new interface(ini_configobj.h)
 * into the one file.
 */
void print_config_parsing_errors(FILE *file,
                                 struct collection_item *error_set);


void print_file_parsing_errors(FILE *file,
                               struct collection_item *error_list);


/*============================================================*/
/* The following classes moved here from the public header
 * They are reserved for future use.
 *
 * NOTE: before exposing these constants again in the common header
 * check that the class IDs did not get reused over time by
 * other classes.
 */
/**
 * @brief Collection of error collections.
 *
 * When multiple files are read during one call
 * each file has its own set of parsing errors
 * and warnings. This is the collection
 * of such sets.
 */
#define COL_CLASS_INI_PESET       COL_CLASS_INI_BASE + 3

/** @brief Collection of grammar errors.
 *
 * Reserved for future use.
 */
#define COL_CLASS_INI_GERROR      COL_CLASS_INI_BASE + 5
/** @brief Collection of validation errors.
 *
 * Reserved for future use.
 */
#define COL_CLASS_INI_VERROR      COL_CLASS_INI_BASE + 6

/**
 * @}
 */

/**
 * @defgroup gramerr Grammar errors and warnings
 *
 * Placeholder for now. Reserved for future use.
 *
 * @{
 */
#define ERR_MAXGRAMMAR      0
/**
 * @}
 */

/**
 * @defgroup valerr Validation errors and warnings
 *
 * Placeholder for now. Reserved for future use.
 *
 * @{
 */
#define ERR_MAXVALID        0


/**
 * @}
 */


#ifdef HAVE_VALIDATION

/** @brief Collection of lines from the INI file.
 *
 * Reserved for future use
 */
#define COL_CLASS_INI_LINES       COL_CLASS_INI_BASE + 7

#endif /* HAVE_VALIDATION */
/*============================================================*/


/* Function to return parsing error */
static const char *parsing_error_str(int parsing_error)
{
    const char *placeholder= _("Unknown pasing error.");
    const char *str_error[] = { _("Data is too long."),
                                _("No closing bracket."),
                                _("Section name is missing."),
                                _("Section name is too long."),
                                _("Equal sign is missing."),
                                _("Property name is missing."),
                                _("Property name is too long."),
                                _("Failed to read line."),
                                _("Invalid space character at the "
                                  "beginning of the line."),
                                _("Duplicate key is not allowed."),
                                _("Duplicate key is detected while "
                                  "merging sections."),
                                _("Duplicate section is not allowed."),
                                _("Invalid character at the "
                                  "beginning of the line."),
                                _("Invalid tab character at the "
                                  "beginning of the line."),
                                _("Incomplete comment at the "
                                  "end of the file.")
    };

    /* Check the range */
    if ((parsing_error < 1) || (parsing_error > ERR_MAXPARSE))
            return placeholder;
    else
            return str_error[parsing_error-1];
}

/* Function to return grammar error.
 * This function is currently not used.
 * It is planned to be used by the INI
 * file grammar parser.
 *
 * The following doxygen description is moved here.
 * When the function gets exposed move it into
 * the header file.
 */
/** @brief Function to return a grammar error in template.
 *
 * EXPERIMENTAL. Reserved for future use.
 *
 * This error is returned when the template
 * is translated into the grammar object.
 *
 * @param[in] grammar_error    Error code for the grammar error.
 *
 * @return Error string.
 */

static const char *grammar_error_str(int grammar_error)
{
    const char *placeholder= _("Unknown grammar error.");
    /* THIS IS A TEMPORARY PLACEHOLDER !!!! */
    const char *str_error[] = { _(""),
                                _(""),
                                _(""),
                                _(""),
                                _(""),
                                _(""),
                                _("")
    };

    /* Check the range */
    if ((grammar_error < 1) || (grammar_error > ERR_MAXGRAMMAR))
            return placeholder;
    else
            return str_error[grammar_error-1];
}

/* Function to return validation error.
 * This function is currently not used.
 * It is planned to be used by the INI
 * file grammar validator.
 *
 * The following doxygen description is moved here.
 * When the function gets exposed move it into
 * the header file.
 */
/** @brief Function to return a validation error.
 *
 * EXPERIMENTAL. Reserved for future use.
 *
 * This is the error that it is returned when
 * the INI file is validated against the
 * grammar object.
 *
 * @param[in] validation_error    Error code for the validation error.
 *
 * @return Error string.
 */
static const char *validation_error_str(int validation_error)
{
    const char *placeholder= _("Unknown validation error.");
    /* THIS IS A TEMPORARY PLACEHOLDER !!!! */
    const char *str_error[] = { _(""),
                                _(""),
                                _(""),
                                _(""),
                                _(""),
                                _(""),
                                _("")
    };

    /* Check the range */
    if ((validation_error < 1) || (validation_error > ERR_MAXVALID))
            return placeholder;
    else
            return str_error[validation_error-1];
}

/* Wrapper to print errors */
const char *ini_get_error_str(int error, int family)
{
    const char *val;
    TRACE_FLOW_ENTRY();

    switch(family) {
    case INI_FAMILY_PARSING:
        val = parsing_error_str(error);
        break;
    case INI_FAMILY_VALIDATION:
        val = validation_error_str(error);
        break;
    case INI_FAMILY_GRAMMAR:
        val = grammar_error_str(error);
        break;
    default:
        val = _("Unknown error category.");
        break;
    }

    TRACE_FLOW_EXIT();
    return val;
}

/* Internal function that prints errors */
static void print_error_list(FILE *file,
                             struct collection_item *error_list,
                             int cclass,
                             char *wrong_col_error,
                             char *failed_to_process,
                             char *error_header,
                             char *line_format,
                             int family)
{
    struct collection_iterator *iterator;
    int error;
    struct collection_item *item = NULL;
    struct ini_parse_error *pe;
    unsigned int count;

    TRACE_FLOW_STRING("print_error_list", "Entry");

    /* If we have something to print print it */
    if (error_list == NULL) {
        TRACE_ERROR_STRING("No error list","");
        return;
    }

    /* Make sure we go the right collection */
    if (!col_is_of_class(error_list, cclass)) {
        TRACE_ERROR_STRING("Wrong collection class:", wrong_col_error);
        fprintf(file,"%s\n", wrong_col_error);
        return;
    }

    /* Bind iterator */
    error =  col_bind_iterator(&iterator, error_list, COL_TRAVERSE_DEFAULT);
    if (error) {
        TRACE_ERROR_STRING("Error (bind):", failed_to_process);
        fprintf(file, "%s\n", failed_to_process);
        return;
    }

    while(1) {
        /* Loop through a collection */
        error = col_iterate_collection(iterator, &item);
        if (error) {
            TRACE_ERROR_STRING("Error (iterate):", failed_to_process);
            fprintf(file, "%s\n", failed_to_process);
            col_unbind_iterator(iterator);
            return;
        }

        /* Are we done ? */
        if (item == NULL) break;

        /* Process collection header */
        if (col_get_item_type(item) == COL_TYPE_COLLECTION) {
            col_get_collection_count(item, &count);
            if (count <= 2) break;
        } else if (col_get_item_type(item) == COL_TYPE_STRING) {
            fprintf(file, error_header, (char *)col_get_item_data(item));
        }
        else {
            /* Put error into provided format */
            pe = (struct ini_parse_error *)(col_get_item_data(item));
            fprintf(file, line_format,
                    col_get_item_property(item, NULL), /* Error or warning */
                    pe->error,                         /* Error */
                    pe->line,                          /* Line */
                    ini_get_error_str(pe->error,
                                      family));        /* Error str */
        }

    }

    /* Do not forget to unbind iterator - otherwise there will be a leak */
    col_unbind_iterator(iterator);

    TRACE_FLOW_STRING("print_error_list", "Exit");
}

/* Print errors and warnings that were detected while parsing one file */
void print_file_parsing_errors(FILE *file,
                               struct collection_item *error_list)
{
    print_error_list(file,
                     error_list,
                     COL_CLASS_INI_PERROR,
                     WRONG_COLLECTION,
                     FAILED_TO_PROCCESS,
                     ERROR_HEADER,
                     LINE_FORMAT,
                     INI_FAMILY_PARSING);
}


void print_grammar_errors(FILE *file,
                          struct collection_item *error_list);
/* Print errors and warnings that were detected while processing grammar.
 *
 * The following doxygen description is moved here.
 * When the function gets exposed move it into
 * the header file and remove prototype from this file.
 */
/**
 * @brief Print errors and warnings that were detected while
 * checking grammar of the template.
 *
 * EXPERIMENTAL. Reserved for future use.
 *
 * @param[in] file           File descriptor.
 * @param[in] error_list     List of the parsing errors.
 *
 */
void print_grammar_errors(FILE *file,
                          struct collection_item *error_list)
{
    print_error_list(file,
                     error_list,
                     COL_CLASS_INI_GERROR,
                     WRONG_GRAMMAR,
                     FAILED_TO_PROC_G,
                     ERROR_HEADER_G,
                     LINE_FORMAT,
                     INI_FAMILY_GRAMMAR);
}

void print_validation_errors(FILE *file,
                             struct collection_item *error_list);
/* Print errors and warnings that were detected while validating INI file.
 *
 * The following doxygen description is moved here.
 * When the function gets exposed move it into
 * the header file and remove prototype from this file.
 */
/**
 * @brief Print errors and warnings that were detected while
 * checking INI file against the grammar object.
 *
 * EXPERIMENTAL. Reserved for future use.
 *
 * @param[in] file           File descriptor.
 * @param[in] error_list     List of the parsing errors.
 *
 */
void print_validation_errors(FILE *file,
                             struct collection_item *error_list)
{
    print_error_list(file,
                     error_list,
                     COL_CLASS_INI_VERROR,
                     WRONG_VALIDATION,
                     FAILED_TO_PROC_V,
                     ERROR_HEADER_V,
                     LINE_FORMAT,
                     INI_FAMILY_VALIDATION);
}

/* Print errors and warnings that were detected while parsing
 * the whole configuration */
void print_config_parsing_errors(FILE *file,
                                 struct collection_item *error_list)
{
    struct collection_iterator *iterator;
    int error;
    struct collection_item *item = NULL;
    struct collection_item *file_errors = NULL;

    TRACE_FLOW_STRING("print_config_parsing_errors", "Entry");

    /* If we have something to print print it */
    if (error_list == NULL) {
        TRACE_ERROR_STRING("No error list", "");
        return;
    }

    /* Make sure we go the right collection */
    if (!col_is_of_class(error_list, COL_CLASS_INI_PESET)) {
        TRACE_ERROR_STRING("Wrong collection class:", WRONG_COLLECTION);
        fprintf(file, "%s\n", WRONG_COLLECTION);
        return;
    }

    /* Bind iterator */
    error =  col_bind_iterator(&iterator, error_list, COL_TRAVERSE_DEFAULT);
    if (error) {
        TRACE_ERROR_STRING("Error (bind):", FAILED_TO_PROCCESS);
        fprintf(file,"%s\n", FAILED_TO_PROCCESS);
        return;
    }

    while(1) {
        /* Loop through a collection */
        error = col_iterate_collection(iterator, &item);
        if (error) {
            TRACE_ERROR_STRING("Error (iterate):", FAILED_TO_PROCCESS);
            fprintf(file, "%s\n", FAILED_TO_PROCCESS);
            col_unbind_iterator(iterator);
            return;
        }

        /* Are we done ? */
        if (item == NULL) break;

        /* Print per file sets of errors */
        if (col_get_item_type(item) == COL_TYPE_COLLECTIONREF) {
            /* Extract a sub collection */
            error = col_get_reference_from_item(item, &file_errors);
            if (error) {
                TRACE_ERROR_STRING("Error (extract):", FAILED_TO_PROCCESS);
                fprintf(file, "%s\n", FAILED_TO_PROCCESS);
                col_unbind_iterator(iterator);
                return;
            }
            print_file_parsing_errors(file, file_errors);
            col_destroy_collection(file_errors);
        }
    }

    /* Do not forget to unbind iterator - otherwise there will be a leak */
    col_unbind_iterator(iterator);

    TRACE_FLOW_STRING("print_config_parsing_errors", "Exit");
}

/* Function to print errors from the list */
void ini_config_print_errors(FILE *file, char **error_list)
{
    unsigned count = 0;

    TRACE_FLOW_ENTRY();

    if (!error_list) {
        TRACE_FLOW_STRING("List is empty.", "");
        return;
    }

    while (error_list[count]) {
        fprintf(file, "%s\n", error_list[count]);
        count++;
    }

    TRACE_FLOW_EXIT();
    return;
}
