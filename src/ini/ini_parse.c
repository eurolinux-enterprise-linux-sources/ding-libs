/*
    INI LIBRARY

    Low level parsing functions

    Copyright (C) Dmitri Pal <dpal@redhat.com> 2010

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
#include <errno.h>
#include <string.h>
#include <ctype.h>
/* For error text */
#include <libintl.h>
#define _(String) gettext (String)
#include "trace.h"
#include "ini_defines.h"
#include "ini_valueobj.h"
#include "ini_config_priv.h"
#include "ini_configobj.h"
#include "collection.h"
#include "collection_queue.h"

#define INI_WARNING 0xA0000000 /* Warning bit */

/* This constant belongs to ini_defines.h. Move from ini_config - TBD */
#define COL_CLASS_INI_BASE        20000
#define COL_CLASS_INI_SECTION     COL_CLASS_INI_BASE + 1
/**
 * @brief Name of the default section.
 *
 * This is the name of the implied section where orphan key-value
 * pairs will be put.
 */
#define INI_DEFAULT_SECTION "default"


struct parser_obj {
    /* Externally passed and saved data */
    FILE *file;
    struct collection_item *top;
    struct collection_item *el;
    const char *filename;
    struct ini_cfgobj *co;
    /* Level of error reporting */
    int error_level;
    /* Collistion flags */
    uint32_t collision_flags;
    /* Parseing flags */
    uint32_t parse_flags;
    /* Wrapping boundary */
    uint32_t boundary;
    /* Action queue */
    struct collection_item *queue;
    /* Last error */
    uint32_t last_error;
    /* Last line number */
    uint32_t linenum;
    /* Line number of the last found key */
    uint32_t keylinenum;
    /* Line number of the last found section */
    uint32_t seclinenum;
    /* Internal variables */
    struct collection_item *sec;
    struct collection_item *merge_sec;
    struct ini_comment *ic;
    char *last_read;
    uint32_t last_read_len;
    int inside_comment;
    char *key;
    uint32_t key_len;
    struct ref_array *raw_lines;
    struct ref_array *raw_lengths;
    char *merge_key;
    struct value_obj *merge_vo;
    /* Merge error */
    uint32_t merge_error;
    int ret;
};

typedef int (*action_fn)(struct parser_obj *);

#define PARSE_ACTION       "action"

/* Actions */
#define PARSE_READ      0 /* Read from the file */
#define PARSE_INSPECT   1 /* Process read string */
#define PARSE_POST      2 /* Reading is complete  */
#define PARSE_ERROR     3 /* Handle error */
#define PARSE_DONE      4 /* We are done */

/* Declarations of the reusble functions: */
static int complete_value_processing(struct parser_obj *po);
static int save_error(struct collection_item *el,
                      unsigned line,
                      int error,
                      const char *err_txt);


static int is_just_spaces(const char *str, uint32_t len)
{
    uint32_t i;

    TRACE_FLOW_ENTRY();

    for (i = 0; i < len; i++) {
        if (!isspace(str[i])) return 0;
    }

    TRACE_FLOW_EXIT();
    return 1;
}

/* Functions checks whether the line
 * starts with the sequence of allowed blank characters.
 * If spaces are allowed - function will say that line
 * is OK. If tabls are allowed the function also would
 * say that line is OK. If the mixture of both is allowed
 * the line is OK too.
 * Any other character will cause an error.
 */
static int is_allowed_spaces(const char *str,
                             uint32_t len,
                             uint32_t parse_flags,
                             int *error)
{
    uint32_t i;
    int line_ok = 1;

    TRACE_FLOW_ENTRY();

    for (i = 0; i < len; i++) {
        if ((str[i] == ' ') &&
            (parse_flags & INI_PARSE_NOSPACE)) {
            /* Leading spaces are not allowed */
            *error = ERR_SPACE;
            line_ok = 0;
            break;
        }
        else if ((str[i] == '\t') &&
            (parse_flags & INI_PARSE_NOTAB)) {
            /* Leading tabs are not allowed */
            *error = ERR_TAB;
            line_ok = 0;
            break;
        }
        else if ((str[i] == '\f') ||
                 (str[i] == '\n') ||
                 (str[i] == '\r') ||
                 (str[i] == '\v')) {
            *error = ERR_SPECIAL;
            line_ok = 0;
            break;
        }
        if (!isblank(str[i])) break;
    }

    TRACE_FLOW_EXIT();
    return line_ok;
}

/* Destroy parser object */
static void parser_destroy(struct parser_obj *po)
{
    TRACE_FLOW_ENTRY();

    if(po) {
        col_destroy_queue(po->queue);
        col_destroy_collection_with_cb(po->sec, ini_cleanup_cb, NULL);
        ini_comment_destroy(po->ic);
        value_destroy_arrays(po->raw_lines,
                             po->raw_lengths);
        if (po->last_read) free(po->last_read);
        if (po->key) free(po->key);
        col_destroy_collection_with_cb(po->top, ini_cleanup_cb, NULL);
        free(po);
    }

    TRACE_FLOW_EXIT();
}

/* Create parse object
 *
 * It assumes that the ini collection
 * has been precreated.
 */
static int parser_create(struct ini_cfgobj *co,
                         FILE *file,
                         const char *config_filename,
                         int error_level,
                         uint32_t collision_flags,
                         uint32_t parse_flags,
                         struct parser_obj **po)
{
    int error = EOK;
    struct parser_obj *new_po = NULL;
    unsigned count = 0;

    TRACE_FLOW_ENTRY();

    /* Make sure that all the parts are initialized */
    if ((!po) ||
        (!co) ||
        (!(co->cfg)) ||
        (!file) ||
        (!config_filename)) {
        TRACE_ERROR_NUMBER("Invalid argument", EINVAL);
        return EINVAL;
    }

    error = col_get_collection_count(co->cfg, &count);
    if (error) {
        TRACE_ERROR_NUMBER("Failed to check object size", error);
        return error;
    }

    if (count != 1) {
        TRACE_ERROR_NUMBER("Configuration is not empty", EINVAL);
        return EINVAL;
    }

    new_po = malloc(sizeof(struct parser_obj));
    if (!new_po) {
        TRACE_ERROR_NUMBER("No memory", ENOMEM);
        return ENOMEM;
    }

    /* Save external data */
    new_po->file = file;
    new_po->el = co->error_list;
    new_po->filename = config_filename;
    new_po->error_level = error_level;
    new_po->collision_flags = collision_flags;
    new_po->parse_flags = parse_flags;
    new_po->boundary = co->boundary;
    new_po->co = co;

    /* Initialize internal varibles */
    new_po->sec = NULL;
    new_po->merge_sec = NULL;
    new_po->ic = NULL;
    new_po->last_error = 0;
    new_po->linenum = 0;
    new_po->keylinenum = 0;
    new_po->seclinenum = 0;
    new_po->last_read = NULL;
    new_po->last_read_len = 0;
    new_po->inside_comment = 0;
    new_po->key = NULL;
    new_po->key_len = 0;
    new_po->raw_lines = NULL;
    new_po->raw_lengths = NULL;
    new_po->ret = EOK;
    new_po->merge_key = NULL;
    new_po->merge_vo = NULL;
    new_po->merge_error = 0;
    new_po->top = NULL;
    new_po->queue = NULL;

    /* Create top collection */
    error = col_create_collection(&(new_po->top),
                                  INI_CONFIG_NAME,
                                  COL_CLASS_INI_CONFIG);
    if (error) {
        TRACE_ERROR_NUMBER("Failed to create top collection", error);
        parser_destroy(new_po);
        return error;
    }

    /* Create a queue */
    error = col_create_queue(&(new_po->queue));
    if (error) {
        TRACE_ERROR_NUMBER("Failed to create queue", error);
        parser_destroy(new_po);
        return error;
    }

    error = col_enqueue_unsigned_property(new_po->queue,
                                          PARSE_ACTION,
                                          PARSE_READ);
    if (error) {
        TRACE_ERROR_NUMBER("Failed to create queue", error);
        parser_destroy(new_po);
        return error;
    }

    *po = new_po;

    TRACE_FLOW_EXIT();
    return error;
}

/* Function to read next line from the file */
static int parser_read(struct parser_obj *po)
{
    int error = EOK;
    char *buffer = NULL;
    ssize_t res = 0;
    size_t len = 0;
    int32_t i = 0;
    uint32_t action;

    TRACE_FLOW_ENTRY();

    /* Adjust line number */
    (po->linenum)++;

    /* Get line from the file */
    res = getline(&buffer, &len, po->file);
    if (res == -1) {
        if (feof(po->file)) {
            TRACE_FLOW_STRING("Read nothing", "");
            if (po->inside_comment) {
                action = PARSE_ERROR;
                po->last_error = ERR_BADCOMMENT;
            }
            else action = PARSE_POST;
        }
        else {
            TRACE_ERROR_STRING("Error reading", "");
            action = PARSE_ERROR;
            po->last_error = ERR_READ;
        }
        if(buffer) free(buffer);
    }
    else {
        /* Read Ok */
        len = res;
        TRACE_INFO_STRING("Read line ok:", buffer);
        TRACE_INFO_NUMBER("Length:", len);
        TRACE_INFO_NUMBER("Strlen:", strlen(buffer));

        if (buffer[0] == '\0') {
            /* Empty line - read again (should not ever happen) */
            action = PARSE_READ;
            free(buffer);
        }
        else {
            /* Check length */
            if (len >= BUFFER_SIZE) {
                TRACE_ERROR_STRING("Too long", "");
                action = PARSE_ERROR;
                po->last_error = ERR_LONGDATA;
                free(buffer);
            }
            else {
                /* Trim end line */
                i = len - 1;
                while ((i >= 0) &&
                       ((buffer[i] == '\r') ||
                        (buffer[i] == '\n'))) {
                    TRACE_INFO_NUMBER("Offset:", i);
                    TRACE_INFO_NUMBER("Code:", buffer[i]);
                    buffer[i] = '\0';
                    i--;
                }

                po->last_read = buffer;
                po->last_read_len = i + 1;
                action = PARSE_INSPECT;
                TRACE_INFO_STRING("Line:", po->last_read);
                TRACE_INFO_NUMBER("Linelen:", po->last_read_len);
            }
        }
    }

    /* Move to the next action */
    error = col_enqueue_unsigned_property(po->queue,
                                          PARSE_ACTION,
                                          action);
    if (error) {
        TRACE_ERROR_NUMBER("Failed to schedule an action", error);
        return error;
    }

    TRACE_FLOW_EXIT();
    return EOK;
}

/* Find if there is a collistion */
static int check_section_collision(struct parser_obj *po)
{
    int error = EOK;
    struct collection_item *item = NULL;

    TRACE_FLOW_ENTRY();

    TRACE_INFO_STRING("Searching for:", col_get_item_property(po->sec, NULL));

    error = col_get_item(po->top,
                         col_get_item_property(po->sec, NULL),
                         COL_TYPE_ANY,
                         COL_TRAVERSE_DEFAULT,
                         &item);

    if (error) {
        TRACE_ERROR_NUMBER("Failed searching for dup", error);
        return error;
    }

    /* Check if there is a dup */
    if (item) {
        TRACE_INFO_STRING("Collision found:",
                          col_get_item_property(item, NULL));
        /* Get the actual section collection instead of reference */
        po->merge_sec = *((struct collection_item **)
                          (col_get_item_data(item)));
    }
    else {
        TRACE_INFO_STRING("Collision not found.", "");
        po->merge_sec = NULL;
    }

    TRACE_FLOW_EXIT();
    return EOK;
}

/* Clean all items in the section */
int empty_section(struct collection_item *sec)
{
    int error = EOK;
    struct collection_item *item = NULL;
    struct collection_item *save_item = NULL;
    struct value_obj *vo = NULL;
    int work_to_do = 1;

    TRACE_FLOW_ENTRY();

    do {
        item = NULL;
        error = col_extract_item_from_current(sec,
                                              COL_DSP_FRONT,
                                              NULL,
                                              0,
                                              COL_TYPE_ANY,
                                              &item);
        if ((error) && (error != ENOENT)) {
            TRACE_ERROR_NUMBER("Failed to extract item.", error);
            return error;
        }

        if (item) {
            TRACE_INFO_STRING("Item found:",
                              col_get_item_property(item, NULL));

            if (strncmp(col_get_item_property(item, NULL),
                        INI_SECTION_KEY, 1) == 0) {
                /* Just ignore the first item */
                save_item = item;
                continue;
            }

            vo = *((struct value_obj **)(col_get_item_data(item)));
            value_destroy(vo);
            col_delete_item(item);
        }
        else {
            TRACE_INFO_STRING("No more items:", "");
            /* Restore saved item */
            error = col_insert_item(sec,
                                    NULL,
                                    save_item,
                                    COL_DSP_END,
                                    NULL,
                                    0,
                                    COL_INSERT_NOCHECK);
            if (error) {
                TRACE_ERROR_NUMBER("Failed to restore item.", error);
                return error;
            }

            work_to_do = 0;
        }
    }
    while (work_to_do);

    TRACE_FLOW_EXIT();
    return EOK;
}

/* Merge contents of the section */
static int merge_section(struct parser_obj *po)
{
    int error = EOK;
    struct collection_item *item = NULL;
    struct value_obj *vo = NULL;
    int work_to_do = 1;
    const char *key;

    TRACE_FLOW_ENTRY();

    do {
        TRACE_INFO_STRING("Top of the merge loop", "");

        item = NULL;
        error = col_extract_item_from_current(po->sec,
                                              COL_DSP_FRONT,
                                              NULL,
                                              0,
                                              COL_TYPE_ANY,
                                              &item);
        if ((error) && (error != ENOENT)) {
            TRACE_ERROR_NUMBER("Failed to extract item.", error);
            return error;
        }

        if (item) {

            TRACE_INFO_STRING("Item found:", col_get_item_property(item, NULL));

            if (strncmp(col_get_item_property(item, NULL),
                        INI_SECTION_KEY, 1) == 0) {
                /* Just ignore the first item */
                vo = *((struct value_obj **)(col_get_item_data(item)));
                value_destroy(vo);
                col_delete_item(item);
                continue;
            }

            po->merge_vo = *((struct value_obj **)(col_get_item_data(item)));
            key = col_get_item_property(item, NULL);
            /* To be able to use po->merge_key in the loop
             * we have to overcome constraints imposed by
             * the "const" declaration.
             */
            memcpy(&(po->merge_key), &key, sizeof(char *));

            /* Use the value processing function to inser the value */
            error = complete_value_processing(po);

            /* In case of error value is already cleaned */
            po->merge_vo = NULL;
            po->merge_key = NULL;
            col_delete_item(item);
            /* Now we can check the error */
            if (error) {
                TRACE_ERROR_NUMBER("Failed to merge item.", error);
                return error;
            }
        }
        else {
            TRACE_INFO_STRING("No more items:", "");
            work_to_do = 0;
        }
    }
    while (work_to_do);

    /* If we reached this place the incoming section is empty.
     * but just to be safe clean with callback. */
    col_destroy_collection_with_cb(po->sec, ini_cleanup_cb, NULL);
    po->sec = NULL;

    TRACE_FLOW_EXIT();
    return EOK;
}

/* Function to read next line from the file */
static int parser_save_section(struct parser_obj *po)
{
    int error = EOK;
    uint32_t mergemode;
    int merge = 0;

    TRACE_FLOW_ENTRY();

    if (po->sec) {

        TRACE_INFO_STRING("Section exists.", "");

        /* First detect if we have collision */
        error = check_section_collision(po);
        if (error) {
            TRACE_ERROR_NUMBER("Failed to check for collision", error);
            return error;
        }

        if (po->merge_sec) {

            TRACE_INFO_STRING("Merge collision detected", "");

            mergemode = po->collision_flags & INI_MS_MASK;

            switch (mergemode) {
            case INI_MS_ERROR:
                /* Report error and return */
                TRACE_INFO_STRING("Reporting error", "duplicate section");
                error = save_error(po->el,
                                   po->seclinenum,
                                   ERR_DUPSECTION,
                                   ERROR_TXT);
                if (error) {
                    TRACE_ERROR_NUMBER("Failed to "
                                       "save error",
                                        error);
                    return error;
                }
                /* Return error */
                TRACE_FLOW_RETURN(EEXIST);
                return EEXIST;

            case INI_MS_PRESERVE:
                /* Delete new section */
                TRACE_INFO_STRING("Preserve mode", "");
                col_destroy_collection_with_cb(
                                        po->sec,
                                        ini_cleanup_cb,
                                        NULL);
                po->sec = NULL;
                break;

            case INI_MS_OVERWRITE:
                /* Empty existing section */
                TRACE_INFO_STRING("Ovewrite mode", "");
                error = empty_section(po->merge_sec);
                if (error) {
                    TRACE_ERROR_NUMBER("Failed to "
                                       "empty section",
                                        error);
                    return error;
                }
                merge = 1;
                break;

            case INI_MS_DETECT:
                /* Detect mode */
                TRACE_INFO_STRING("Detect mode", "");
                po->merge_error = EEXIST;
                error = save_error(po->el,
                                   po->seclinenum,
                                   ERR_DUPSECTION,
                                   ERROR_TXT);
                if (error) {
                    TRACE_ERROR_NUMBER("Failed to "
                                       "save error",
                                        error);
                    return error;
                }
                merge = 1;
                break;

            case INI_MS_MERGE:
                /* Merge */
            default:
                TRACE_INFO_STRING("Merge mode", "");
                merge = 1;
                break;
            }

            if (merge) {
                error = merge_section(po);
                if (error) {
                    TRACE_ERROR_NUMBER("Failed to merge section", error);
                    return error;
                }
            }

            po->merge_sec = NULL;
        }
        else {
            /* Add section to configuration */
            TRACE_INFO_STRING("Now adding collection", "");
            error = col_add_collection_to_collection(po->top,
                                                     NULL, NULL,
                                                     po->sec,
                                                     COL_ADD_MODE_EMBED);

            if (error) {
                TRACE_ERROR_NUMBER("Failed to embed section", error);
                return error;
            }

            po->sec = NULL;
        }
    }

    TRACE_FLOW_EXIT();
    return EOK;

}

/* Complete value processing */
static int complete_value_processing(struct parser_obj *po)
{
    int error = EOK;
    int error2 = EOK;
    struct value_obj *vo = NULL;
    struct value_obj *vo_old = NULL;
    unsigned insertmode;
    uint32_t mergemode;
    int suppress = 0;
    int doinsert = 0;
    struct collection_item *item = NULL;
    struct collection_item *section = NULL;
    int merging = 0;

    TRACE_FLOW_ENTRY();

    if (po->merge_sec) {
        TRACE_INFO_STRING("Processing value in merge mode", "");
        section = po->merge_sec;
        merging = 1;
    }
    else if(!(po->sec)) {
        TRACE_INFO_STRING("Creating default section", "");
        /* If there is not open section create a default one */
        error = col_create_collection(&po->sec,
                                      INI_DEFAULT_SECTION,
                                      COL_CLASS_INI_SECTION);
        if (error) {
            TRACE_ERROR_NUMBER("Failed to create default section", error);
            return error;
        }
        section = po->sec;
    }
    else {
        TRACE_INFO_STRING("Processing value in normal mode", "");
        section = po->sec;
    }

    if (merging) {
        TRACE_INFO_STRING("Using merge key:", po->merge_key);
        vo = po->merge_vo;
        /* We are adding to the merge section so use MV2S flags.
         * But flags are done in such a way that deviding MV2S by MV1S mask
         * will translate MV2S flags into MV1S so we can use
         * MV1S constants. */
        TRACE_INFO_NUMBER("Collisions flags:", po->collision_flags);
        mergemode = (po->collision_flags & INI_MV2S_MASK) / INI_MV1S_MASK;
    }
    else {
        /* Construct value object from what we have */
        error = value_create_from_refarray(po->raw_lines,
                                           po->raw_lengths,
                                           po->keylinenum,
                                           INI_VALUE_READ,
                                           po->key_len,
                                           po->boundary,
                                           po->ic,
                                           &vo);

        if (error) {
            TRACE_ERROR_NUMBER("Failed to create value object", error);
            return error;
        }
        /* Forget about the arrays. They are now owned by the value object */
        po->ic = NULL;
        po->raw_lines = NULL;
        po->raw_lengths = NULL;
        mergemode = po->collision_flags & INI_MV1S_MASK;
    }

    switch (mergemode) {
    case INI_MV1S_ERROR:

        insertmode = COL_INSERT_DUPERROR;
        doinsert = 1;
        break;

    case INI_MV1S_PRESERVE:

        insertmode = COL_INSERT_DUPERROR;
        doinsert = 1;
        suppress = 1;
        break;

    case INI_MV1S_ALLOW:

        insertmode = COL_INSERT_NOCHECK;
        doinsert = 1;
        break;

    case INI_MV1S_OVERWRITE: /* Special handling */
    case INI_MV1S_DETECT:
    default:
        break;
    }

    /* Do not insert but search for dups first */
    if (!doinsert) {
        TRACE_INFO_STRING("Overwrite mode. Looking for:",
                          (char *)(merging ? po->merge_key : po->key));

        error = col_get_item(section,
                             merging ? po->merge_key : po->key,
                             COL_TYPE_BINARY,
                             COL_TRAVERSE_DEFAULT,
                             &item);

        if (error) {
            TRACE_ERROR_NUMBER("Failed searching for dup", error);
            value_destroy(vo);
            return error;
        }

        /* Check if there is a dup */
        if (item) {
            /* Check if we are in the detect mode */
            if (mergemode == INI_MV1S_DETECT) {
                po->merge_error = EEXIST;
                /* There is a dup - inform user about it and continue */
                error = save_error(po->el,
                                   merging ? po->seclinenum : po->keylinenum,
                                   merging ? ERR_DUPKEYSEC : ERR_DUPKEY,
                                   ERROR_TXT);
                if (error) {
                    TRACE_ERROR_NUMBER("Failed to save error", error);
                    value_destroy(vo);
                    return error;
                }
                doinsert = 1;
                insertmode = COL_INSERT_NOCHECK;

            }
            else {

                /* Dup exists - update it */
                vo_old = *((struct value_obj **)(col_get_item_data(item)));
                error = col_modify_binary_item(item,
                                               NULL,
                                               &vo,
                                               sizeof(struct value_obj *));
                if (error) {
                    TRACE_ERROR_NUMBER("Failed updating the value", error);
                    value_destroy(vo);
                    return error;
                }

                /* If we failed to update it is better to leak then crash,
                 * so destroy original value only on the successful update.
                 */
                value_destroy(vo_old);
            }
        }
        else {
            /* No dup found so we can insert with no check */
            doinsert = 1;
            insertmode = COL_INSERT_NOCHECK;
        }
    }

    if (doinsert) {
        /* Add value to collection */
        error = col_insert_binary_property(section,
                                           NULL,
                                           COL_DSP_END,
                                           NULL,
                                           0,
                                           insertmode,
                                           merging ? po->merge_key : po->key,
                                           &vo,
                                           sizeof(struct value_obj *));
        if (error) {
            value_destroy(vo);

            if ((suppress) && (error == EEXIST)) {
                TRACE_INFO_STRING("Preseved exisitng value",
                                  (char *)(merging ? po->merge_key : po->key));
            }
            else {
                /* Check if this is a critical error or not */
                if ((mergemode == INI_MV1S_ERROR) && (error == EEXIST)) {
                    TRACE_ERROR_NUMBER("Failed to add value object "
                                       "to the section", error);
                    error2 = save_error(po->el,
                                       merging ? po->seclinenum : po->keylinenum,
                                       merging ? ERR_DUPKEYSEC : ERR_DUPKEY,
                                       ERROR_TXT);
                    if (error2) {
                        TRACE_ERROR_NUMBER("Failed to save error", error2);
                        return error2;
                    }
                    return error;
                }
                else {
                    TRACE_ERROR_NUMBER("Failed to add value object"
                                       " to the section", error);
                    return error;
                }
            }
        }
    }

    if (!merging) {
        free(po->key);
        po->key = NULL;
        po->key_len = 0;
    }

    TRACE_FLOW_EXIT();
    return EOK;
}


/* Process comment */
static int handle_comment(struct parser_obj *po, uint32_t *action)
{
    int error = EOK;

    TRACE_FLOW_ENTRY();

    /* We got a comment */
    if (po->key) {
        /* Previous value if any is complete */
        error = complete_value_processing(po);
        if (error) {
            TRACE_ERROR_NUMBER("Failed to finish saving value", error);
            return error;
        }
    }

    if (!(po->ic)) {
        /* Create a new comment */
        error = ini_comment_create(&(po->ic));
        if (error) {
            TRACE_ERROR_NUMBER("Failed to create comment", error);
            return error;
        }
    }

    /* Add line to comment */
    error = ini_comment_build_wl(po->ic,
                                 po->last_read,
                                 po->last_read_len);
    if (error) {
        TRACE_ERROR_NUMBER("Failed to add line to comment", error);
        return error;
    }
    /*
     * We are done with the comment line.
     * Free it since comment keeps a copy.
     */
    free(po->last_read);
    po->last_read = NULL;
    po->last_read_len = 0;
    *action = PARSE_READ;

    TRACE_FLOW_EXIT();
    return EOK;
}

/* Handle key-value pair */
static int handle_kvp(struct parser_obj *po, uint32_t *action)
{
    int error = EOK;
    char *eq = NULL;
    uint32_t len = 0;
    char *dupval = NULL;
    char *str;
    uint32_t full_len;

    TRACE_FLOW_ENTRY();

    str = po->last_read;
    full_len = po->last_read_len;

    TRACE_INFO_STRING("Last read:", str);

    /* Trim spaces at the beginning */
    while ((full_len > 0) && (isspace(*(str)))) {
        str++;
        full_len--;
    }

    /* Check if we have the key */
    if (*(str) == '=') {
        TRACE_ERROR_STRING("No key", str);
        po->last_error = ERR_NOKEY;
        *action = PARSE_ERROR;
        return EOK;
    }

    /* Find "=" */
    eq = strchr(str, '=');
    if (eq == NULL) {
        TRACE_ERROR_STRING("No equal sign", str);
        po->last_error = ERR_NOEQUAL;
        *action = PARSE_ERROR;
        return EOK;
    }

    /* Strip spaces around "=" */
    /* Since eq > str we can substract 1 */
    len = eq - str - 1;
    while ((len > 0) && (isspace(*(str + len)))) len--;
    /* Adjust length properly */
    len++;

    /* Check the key length */
    if(len >= MAX_KEY) {
        TRACE_ERROR_STRING("Key name is too long", str);
        po->last_error = ERR_LONGKEY;
        *action = PARSE_ERROR;
        return EOK;
    }

    if (po->key) {
        /* Complete processing of the previous value */
        error = complete_value_processing(po);
        if (error) {
            TRACE_ERROR_NUMBER("Failed to complete value processing", error);
            return error;
        }
    }

    /* Dup the key name */
    po->key = malloc(len + 1);
    if (!(po->key)) {
        TRACE_ERROR_NUMBER("Failed to dup key", ENOMEM);
        return ENOMEM;
    }

    memcpy(po->key, str, len);
    *(po->key + len) = '\0';
    po->key_len = len;

    TRACE_INFO_STRING("Key:", po->key);
    TRACE_INFO_NUMBER("Keylen:", po->key_len);

    len = full_len - (eq - str) - 1;

    /* Trim spaces after equal sign */
    eq++;
    while (isspace(*eq)) {
        eq++;
        len--;
    }

    TRACE_INFO_STRING("VALUE:", eq);
    TRACE_INFO_NUMBER("LENGTH:", len);

    /* Dup the part of the value */
    dupval = malloc(len + 1);
    if (!dupval) {
        TRACE_ERROR_NUMBER("Failed to dup value", ENOMEM);
        return ENOMEM;
    }

    memcpy(dupval, eq, len);
    *(dupval + len) = '\0';

    /* Create new arrays */
    error = value_create_arrays(&(po->raw_lines),
                                &(po->raw_lengths));
    if (error) {
        TRACE_ERROR_NUMBER("Failed to create arrays", error);
        free(dupval);
        return error;
    }

    /* Save a duplicated part in the value */
    error = value_add_to_arrays(dupval,
                                len,
                                po->raw_lines,
                                po->raw_lengths);

    if (error) {
        TRACE_ERROR_NUMBER("Failed to add value to arrays", error);
        free(dupval);
        return error;
    }

    /* Save the line number of the last found key */
    po->keylinenum = po->linenum;

    /* Prepare for reading */
    free(po->last_read);
    po->last_read = NULL;
    po->last_read_len = 0;

    *action = PARSE_READ;

    TRACE_FLOW_EXIT();
    return EOK;
}

/* Process line starts with space  */
static int handle_space(struct parser_obj *po, uint32_t *action)
{
    int error = EOK;
    int space_err = 0;

    TRACE_FLOW_ENTRY();

    if (po->parse_flags & INI_PARSE_NOWRAP) {
        /* In this case an empty line is a comment. */
        if (is_just_spaces(po->last_read, po->last_read_len)) {
            error = handle_comment(po, action);
            TRACE_FLOW_EXIT();
            return error;
        }

        /* Wrapping is not allowed */
        if (!is_allowed_spaces(po->last_read,
                               po->last_read_len,
                               po->parse_flags,
                               &space_err)) {
            *action = PARSE_ERROR;
            po->last_error = space_err;
            error = EOK;
        }
        else {
            /* Allowed spaces will be trimmed
             * inside KVP processing.
             */
            error = handle_kvp(po, action);
        }
        TRACE_FLOW_EXIT();
        return error;
    }

    /* Do we have current value object? */
    if (po->key) {
        /* This is a new line in a folded value */
        error = value_add_to_arrays(po->last_read,
                                    po->last_read_len,
                                    po->raw_lines,
                                    po->raw_lengths);
        if (error) {
            TRACE_ERROR_NUMBER("Failed to add line to value", error);
            return error;
        }
        /* Do not free the line, it is now an element of the array */
        po->last_read = NULL;
        po->last_read_len = 0;
        *action = PARSE_READ;
    }
    else {
        /* Check if this is a completely empty line */
        if (is_just_spaces(po->last_read, po->last_read_len)) {
            error = handle_comment(po, action);
            if (error) {
                TRACE_ERROR_NUMBER("Failed to process comment", error);
                return error;
            }
        }
        else {
            /* We do not have an active value
             * but have a line is starting with a space.
             * For now it is error.
             * We can change it in future if
             * people find it being too restrictive
             */
            *action = PARSE_ERROR;
            po->last_error = ERR_SPACE;
        }
    }

    TRACE_FLOW_EXIT();
    return EOK;
}

/* Parse and process section */
static int handle_section(struct parser_obj *po, uint32_t *action)
{
    int error = EOK;
    char *start;
    char *end;
    char *dupval;
    uint32_t len;

    TRACE_FLOW_ENTRY();

    /* We are safe to substract 1
     * since we know that there is at
     * least one character on the line
     * based on the check above.
     */
    end = po->last_read + po->last_read_len - 1;
    while (isspace(*end)) end--;
    if (*end != ']') {
        *action = PARSE_ERROR;
        po->last_error = ERR_NOCLOSESEC;
        return EOK;
    }

    /* Skip spaces at the beginning of the section name */
    start = po->last_read + 1;
    while (isspace(*start)) start++;

    /* Check if there is a section name */
    if (start == end) {
        *action = PARSE_ERROR;
        po->last_error = ERR_NOSECTION;
        return EOK;
    }

    /* Skip spaces at the end of the section name */
    end--;
    while (isspace(*end)) end--;

    /* We got section name */
    len = end - start + 1;

    if (len > MAX_KEY) {
        *action = PARSE_ERROR;
        po->last_error = ERR_SECTIONLONG;
        return EOK;
    }

    if (po->key) {
        /* Complete processing of the previous value */
        error = complete_value_processing(po);
        if (error) {
            TRACE_ERROR_NUMBER("Failed to complete value processing", error);
            return error;
        }
    }

    /* Save section if we have one*/
    error = parser_save_section(po);
    if (error) {
        TRACE_ERROR_NUMBER("Failed to save section", error);
        return error;
    }

    /* Dup the name */
    dupval = malloc(len + 1);
    if (!dupval) {
        TRACE_ERROR_NUMBER("Failed to dup section name", ENOMEM);
        return ENOMEM;
    }

    memcpy(dupval, start, len);
    dupval[len] = '\0';

    /* Create a new section */
    error = col_create_collection(&po->sec,
                                  dupval,
                                  COL_CLASS_INI_SECTION);
    if (error) {
        TRACE_ERROR_NUMBER("Failed to create a section", error);
        free(dupval);
        return error;
    }

    /* But if there is just a comment then create a special key */
    po->key_len = sizeof(INI_SECTION_KEY) - 1;
    po->key = strndup(INI_SECTION_KEY, sizeof(INI_SECTION_KEY));
    /* Create new arrays */
    error = value_create_arrays(&(po->raw_lines),
                                &(po->raw_lengths));
    if (error) {
        TRACE_ERROR_NUMBER("Failed to create arrays", error);
        free(dupval);
        return error;
    }

    /* Save a duplicated part in the value */
    error = value_add_to_arrays(dupval,
                                len,
                                po->raw_lines,
                                po->raw_lengths);
    if (error) {
        TRACE_ERROR_NUMBER("Failed to add value to the arrays", error);
        free(dupval);
        return error;
    }

    /* Save the line number of the last found key */
    po->seclinenum = po->linenum;

    /* Complete processing of this value.
     * A new section will be created inside and a special
     * value will be added.
     */
    error = complete_value_processing(po);
    if (error) {
        TRACE_ERROR_NUMBER("Failed to complete value processing", error);
        return error;
    }

    /* We are done dealing with section */
    free(po->last_read);
    po->last_read = NULL;
    po->last_read_len = 0;
    *action = PARSE_READ;

    TRACE_FLOW_EXIT();
    return EOK;

}

static int check_for_comment(char *buffer,
                             uint32_t buffer_len,
                             int allow_c_comments,
                             int *inside_comment)
{
    int pos;
    int is_comment = 0;

    TRACE_FLOW_ENTRY();

    if (*inside_comment) {
        /* We are already inside the comment
         * and we are looking for the end of the comment
         */
        if (buffer_len) {
            pos = buffer_len - 1;
            while(isspace(buffer[pos]) && pos > 0) pos--;

            /* Check for comment at the end of the line */
            if ((pos > 1) &&
                (buffer[pos] == '/') &&
                (buffer[pos - 1] == '*')) {
                *inside_comment = 0;
            }
        }
        is_comment = 1;
    }
    else {
        /* We do not allow spaces in front of comments
         * so we expect the comment to start right away.
         */
        if ((buffer[0] == '\0') ||
            (buffer[0] == ';') ||
            (buffer[0] == '#')) {
            is_comment = 1;
        }
        else if ((allow_c_comments) && (buffer_len > 1)) {
            if (buffer[0] == '/') {
                if (buffer[1] == '/') is_comment = 1;
                else if (buffer[1] == '*') {

                    is_comment = 1;
                    *inside_comment = 1;

                    /* Here we need to check whether this comment ends
                     * on this line or not
                     */
                    pos = buffer_len - 1;
                    while(isspace(buffer[pos]) && pos > 0) pos--;

                    /* Check for comment at the end of the line
                     * but make sure we have at least two asterisks
                     */
                    if ((pos > 2) &&
                        (buffer[pos] == '/') &&
                        (buffer[pos - 1] == '*')) {
                        *inside_comment = 0;
                    }
                }
            }
        }
    }

    TRACE_FLOW_EXIT();
    return is_comment;
}

/* Inspect the line */
static int parser_inspect(struct parser_obj *po)
{
    int error = EOK;
    uint32_t action = PARSE_DONE;

    TRACE_FLOW_ENTRY();

    TRACE_INFO_STRING("Buffer:", po->last_read);
    TRACE_INFO_NUMBER("In comment:", po->inside_comment);

    if (check_for_comment(po->last_read,
                          po->last_read_len,
                          !(po->parse_flags & INI_PARSE_NO_C_COMMENTS),
                          &(po->inside_comment))) {

        error = handle_comment(po, &action);
        if (error) {
            TRACE_ERROR_NUMBER("Failed to process comment", error);
            return error;
        }
    }
    else if (isspace(*(po->last_read))) {

        error = handle_space(po, &action);
        if (error) {
            TRACE_ERROR_NUMBER("Failed to process line wrapping", error);
            return error;
        }
    }
    else if (*(po->last_read) == '[') {

        error = handle_section(po, &action);
        if (error) {
            TRACE_ERROR_NUMBER("Failed to save section", error);
            return error;
        }
    }
    else {

        error = handle_kvp(po, &action);
        if (error) {
            TRACE_ERROR_NUMBER("Failed to save kvp", error);
            return error;
        }
    }

    /* Move to the next action */
    error = col_enqueue_unsigned_property(po->queue,
                                          PARSE_ACTION,
                                          action);
    if (error) {
        TRACE_ERROR_NUMBER("Failed to schedule an action", error);
        return error;
    }

    TRACE_FLOW_EXIT();
    return error;
}


/* Complete file processing */
static int parser_post(struct parser_obj *po)
{
    int error = EOK;

    TRACE_FLOW_ENTRY();

    /* If there was just a comment at the bottom
     * put it directly into the config object
     */
    if((po->ic) && (!(po->key))) {
        if (po->co->last_comment) {
            error = ini_comment_add(po->ic, po->co->last_comment);
            if (error) {
                TRACE_ERROR_NUMBER("Failed to merge comment", error);
                return error;
            }
        }
        else {
            error = ini_comment_copy(po->ic, &(po->co->last_comment));
            if (error) {
                TRACE_ERROR_NUMBER("Failed to copy comment", error);
                return error;
            }
        }

        ini_comment_destroy(po->ic);
        po->ic = NULL;
    }

    /* If there is a key being processed add it */
    if (po->key) {
        error = complete_value_processing(po);
        if (error) {
            TRACE_ERROR_NUMBER("Failed to complete value processing", error);
            return error;
        }
    }

    /* If we are done save the section */
    error = parser_save_section(po);
    if (error) {
        TRACE_ERROR_NUMBER("Failed to save section", error);
        return error;
    }

    /* Move to the next action */
    error = col_enqueue_unsigned_property(po->queue,
                                          PARSE_ACTION,
                                          PARSE_DONE);
    if (error) {
        TRACE_ERROR_NUMBER("Failed to schedule an action", error);
        return error;
    }

    TRACE_FLOW_EXIT();
    return EOK;
}


static int save_error(struct collection_item *el,
                      unsigned line,
                      int inerr,
                      const char *err_txt)
{
    int error = EOK;
    struct ini_parse_error pe;

    TRACE_FLOW_ENTRY();

    /* Clear the warning bit */
    pe.error = inerr;
    pe.line = line;
    error = col_add_binary_property(el, NULL,
                                    err_txt, &pe, sizeof(pe));
    TRACE_FLOW_RETURN(error);
    return error;
}


/* Error and warning processing */
static int parser_error(struct parser_obj *po)
{
    int error = EOK;
    uint32_t action;
    const char *err_str;

    TRACE_FLOW_ENTRY();

    if (po->last_error & INI_WARNING) err_str = WARNING_TXT;
    else err_str = ERROR_TXT;

    error = save_error(po->el,
                       po->linenum,
                       po->last_error & ~INI_WARNING,
                       err_str);
    if (error) {
        TRACE_ERROR_NUMBER("Failed to add error to error list",
                            error);
        return error;
    }

    if (po->last_error == ERR_BADCOMMENT) {
        /* Avoid endless loop */
        action = PARSE_DONE;
        po->ret = EIO;
    }
    else if (po->error_level == INI_STOP_ON_ANY) {
        action = PARSE_DONE;
        if (po->last_error & INI_WARNING) po->ret = EILSEQ;
        else po->ret = EIO;
    }
    else if (po->error_level == INI_STOP_ON_NONE) {
        if (po->last_error != ERR_READ) {
            action = PARSE_READ;
            if (po->ret == 0) {
                if (po->last_error & INI_WARNING) po->ret = EILSEQ;
                else po->ret = EIO;
            }
            /* It it was warning but now if it is an error
             * bump to return code to indicate error. */
            else if((po->ret == EILSEQ) &&
                    (!(po->last_error & INI_WARNING))) po->ret = EIO;
        }
        else {
            /* Avoid endless loop */
            action = PARSE_DONE;
            po->ret = EIO;
        }
    }
    else { /* Stop on error */
        if (po->last_error & INI_WARNING) {
            action = PARSE_READ;
            po->ret = EILSEQ;
        }
        else {
            action = PARSE_DONE;
            po->ret = EIO;
        }
    }

    /* Prepare for reading */
    if (action == PARSE_READ) {
        if (po->last_read) {
            free(po->last_read);
            po->last_read = NULL;
            po->last_read_len = 0;
        }
    }
    else {
        /* If we are done save the section */
        error = parser_save_section(po);
        if (error) {
            TRACE_ERROR_NUMBER("Failed to save section", error);
            /* If merging sections should produce error and we got error
             * or if we merge sections but dup values produce error and
             * we got error then it is not a fatal error so we need to handle
             * it nicely and suppress it here. We already in the procees
             * of handling another error and merge error does not matter here.
             * We check for reverse condition and return error,
             * otherwise fall through.
             */
            if (!((((po->collision_flags & INI_MS_MASK) == INI_MS_ERROR) &&
                 (error == EEXIST)) ||
                (((po->collision_flags & INI_MS_MASK) == INI_MS_MERGE) &&
                 ((po->collision_flags & INI_MV2S_MASK) == INI_MV2S_ERROR) &&
                 (error == EEXIST)))) {
                return error;
            }
        }
    }

    /* Move to the next action */
    error = col_enqueue_unsigned_property(po->queue,
                                          PARSE_ACTION,
                                          action);
    if (error) {
        TRACE_ERROR_NUMBER("Failed to schedule an action", error);
        return error;
    }

    TRACE_FLOW_EXIT();
    return EOK;
}


/* Run parser */
static int parser_run(struct parser_obj *po)
{
    int error = EOK;
    struct collection_item *item = NULL;
    uint32_t action = 0;
    action_fn operations[] = { parser_read,
                               parser_inspect,
                               parser_post,
                               parser_error,
                               NULL };

    TRACE_FLOW_ENTRY();

    while(1) {
        /* Get next action */
        item = NULL;
        error = col_dequeue_item(po->queue, &item);
        if (error) {
            TRACE_ERROR_NUMBER("Failed to get action", error);
            return error;
        }

        /* Get action, run operation */
        action = *((uint32_t *)(col_get_item_data(item)));
        col_delete_item(item);

        if (action == PARSE_DONE) {

            TRACE_INFO_NUMBER("We are done", error);

            /* Report merge error in detect mode
             * if no other error was detected. */
            if ((po->ret == 0) &&
                (po->merge_error != 0) &&
                ((po->collision_flags & INI_MV1S_DETECT) ||
                 (po->collision_flags & INI_MV2S_DETECT) ||
                 (po->collision_flags & INI_MS_DETECT)))
                po->ret = po->merge_error;

            error = po->ret;
            break;
        }

        error = operations[action](po);
        if (error) {
            TRACE_ERROR_NUMBER("Failed to perform an action", error);
            return error;
        }

    }

    TRACE_FLOW_EXIT();
    return error;
}

/* Top level wrapper around the parser */
int ini_config_parse(struct ini_cfgfile *file_ctx,
                     int error_level,
                     uint32_t collision_flags,
                     uint32_t parse_flags,
                     struct ini_cfgobj *ini_config)
{
    int error = EOK;
    struct parser_obj *po = NULL;
    uint32_t fl1, fl2, fl3;

    TRACE_FLOW_ENTRY();

    if ((!ini_config) || (!(ini_config->cfg))) {
        TRACE_ERROR_NUMBER("Invalid argument", EINVAL);
        return EINVAL;
    }

    if (!file_ctx) {
        TRACE_ERROR_NUMBER("Invalid file context", EINVAL);
        return EINVAL;
    }

    if (!valid_collision_flags(collision_flags)) {
        TRACE_ERROR_NUMBER("Invalid flags.", EINVAL);
        return EINVAL;
    }

    if ((error_level != INI_STOP_ON_ANY) &&
        (error_level != INI_STOP_ON_NONE) &&
        (error_level != INI_STOP_ON_ERROR)) {
        TRACE_ERROR_NUMBER("Invalid argument", EINVAL);
        return EINVAL;
    }

    error = parser_create(ini_config,
                          file_ctx->file,
                          file_ctx->filename,
                          error_level,
                          collision_flags,
                          parse_flags,
                          &po);
    if (error) {
        TRACE_ERROR_NUMBER("Failed to perform an action", error);
        return error;
    }

    error = parser_run(po);
    if (error) {
        fl1 = collision_flags & INI_MS_MASK;
        fl2 = collision_flags & INI_MV1S_MASK;
        fl3 = collision_flags & INI_MV2S_MASK;
        if ((error == EEXIST) &&
            (((fl1 == INI_MS_DETECT) &&
              (fl2 != INI_MV1S_ERROR) &&
              (fl3 != INI_MV2S_ERROR)) ||
             ((fl2 == INI_MV1S_DETECT) &&
              (fl1 != INI_MS_ERROR) &&
              (fl3 != INI_MV2S_ERROR)) ||
             ((fl3 == INI_MV2S_DETECT) &&
              (fl1 != INI_MS_ERROR) &&
              (fl2 != INI_MV1S_ERROR)))) {
            TRACE_ERROR_NUMBER("No error in detect mode", error);
            /* Fall through */
        }
        else {
            TRACE_ERROR_NUMBER("Failed to parse file", error);
            TRACE_ERROR_NUMBER("Mode", collision_flags);
            col_get_collection_count(ini_config->error_list, &(ini_config->count));
            if(ini_config->count) (ini_config->count)--;
            parser_destroy(po);
            return error;
        }
    }

    /* If should be empty anyways */
    col_destroy_collection_with_cb(ini_config->cfg, ini_cleanup_cb, NULL);
    ini_config->cfg = po->top;
    po->top = NULL;

    parser_destroy(po);

    TRACE_FLOW_EXIT();
    return error;
}
