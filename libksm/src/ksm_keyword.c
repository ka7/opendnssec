/*+
 * ksm_keyword - Keyword/Value Conversions
 *
 * Description:
 *      Some values in the database are numeric but need to be translated to
 *      and from strings.  This module does that.
 *
 *      Although the translations are held in tables, this nmodule hard-codes
 *      the strings in the code.
 *
 *
 * Copyright:
 *      Copyright 2008 Nominet
 *      
 * Licence:
 *      Licensed under the Apache Licence, Version 2.0 (the "Licence");
 *      you may not use this file except in compliance with the Licence.
 *      You may obtain a copy of the Licence at
 *      
 *          http://www.apache.org/licenses/LICENSE-2.0
 *      
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the Licence is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the Licence for the specific language governing permissions and
 *      limitations under the Licence.
-*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ksm.h"
#include "string_util.h"
#include "string_util2.h"

/* Mapping of keywords to values */

static STR_KEYWORD_ELEMENT m_algorithm_keywords[] = {
    {KSM_ALGORITHM_RSAMD5_STRING,   KSM_ALGORITHM_RSAMD5},
    {KSM_ALGORITHM_DH_STRING,       KSM_ALGORITHM_DH},
    {KSM_ALGORITHM_DSASHA1_STRING,  KSM_ALGORITHM_DSASHA1},
    {KSM_ALGORITHM_RSASHA1_STRING,  KSM_ALGORITHM_RSASHA1},
    {KSM_ALGORITHM_INDIRECT_STRING, KSM_ALGORITHM_INDIRECT},
    {KSM_ALGORITHM_PRIVDOM_STRING,  KSM_ALGORITHM_PRIVDOM},
    {KSM_ALGORITHM_PRIVOID_STRING,  KSM_ALGORITHM_PRIVOID},
    {NULL,                          -1}
};

static STR_KEYWORD_ELEMENT m_format_keywords[] = {
    {KSM_FORMAT_FILE_STRING,        KSM_FORMAT_FILE},
    {KSM_FORMAT_HSM_STRING,         KSM_FORMAT_HSM},
    {KSM_FORMAT_URI_STRING,         KSM_FORMAT_URI},
    {NULL,                          -1}
};

static STR_KEYWORD_ELEMENT m_state_keywords[] = {
    {KSM_STATE_GENERATE_STRING,     KSM_STATE_GENERATE},
    {KSM_STATE_PUBLISH_STRING,      KSM_STATE_PUBLISH},
    {KSM_STATE_READY_STRING,        KSM_STATE_READY},
    {KSM_STATE_ACTIVE_STRING,       KSM_STATE_ACTIVE},
    {KSM_STATE_RETIRE_STRING,       KSM_STATE_RETIRE},
    {KSM_STATE_DEAD_STRING,         KSM_STATE_DEAD},
    {NULL,                          -1}
};

static STR_KEYWORD_ELEMENT m_type_keywords[] = {
    {KSM_TYPE_KSK_STRING,           KSM_TYPE_KSK},
    {KSM_TYPE_ZSK_STRING,           KSM_TYPE_ZSK},
    {NULL,                          -1}
};

/*
 * Parameters do not have an associated number; instead, the numeric field
 * is the default value used if the parameter is not set.
 */

static STR_KEYWORD_ELEMENT m_parameter_keywords[] = {
    {KSM_PAR_CLOCKSKEW_STRING,  KSM_PAR_CLOCKSKEW},
    {KSM_PAR_NEMKEYS_STRING,    KSM_PAR_NEMKEYS},
    {KSM_PAR_KSKLIFE_STRING,    KSM_PAR_KSKLIFE},
    {KSM_PAR_PROPDELAY_STRING,  KSM_PAR_PROPDELAY},
    {KSM_PAR_SIGNINT_STRING,    KSM_PAR_SIGNINT},
    {KSM_PAR_SOAMIN_STRING,     KSM_PAR_SOAMIN},
    {KSM_PAR_SOATTL_STRING,     KSM_PAR_SOATTL},
    {KSM_PAR_ZSKSIGLIFE_STRING, KSM_PAR_ZSKSIGLIFE},
    {KSM_PAR_ZSKLIFE_STRING,    KSM_PAR_ZSKLIFE},
    {KSM_PAR_ZSKTTL_STRING,     KSM_PAR_ZSKTTL},
    {NULL,                      -1}
};

/*+
 * KsmKeywordNameToValue - Convert Name to Value
 * KsmKeywordValueToName - Convert Value to Name
 *
 * Description:
 *      Converts between keywords and associated values for the specific
 *      element.
 *
 *      When searching for a keyword, the given string need only be an
 *      unambiguous abbreviation of one of the keywords in the list.  For
 *      example, given the keywords
 *
 *              taiwan, tanzania, uganda
 *
 *      ... then "t" or "ta" are ambiguous but "tai" matches taiwan.  "u" (a
 *      single letter) will match uganda.
 *
 * Arguments:
 *      STR_KEYWORD_ELEMENT* elements
 *          Element list to search.
 *
 *      const char* name -or- int value
 *          Name or value to convert.
 *
 * Returns:
 *      int -or- const char*
 *          Converted value.  The return value is NULL or 0 if no conversion is
 *          found. (This implies that no keyword should have a value of 0.)
 *
 *          Note that the returned string pointer is a pointer to a static
 *          string in this module.  It should not be freed by the caller.
-*/

static int KsmKeywordNameToValue(STR_KEYWORD_ELEMENT* elements, const char* name)
{
    int     status = 1;     /* Status return - assume error */
    int     value;          /* Return value */

    if (name) {
        status = StrKeywordSearch(name, elements, &value);
    }
    return (status == 0) ? value : 0;
}

static const char* KsmKeywordValueToName(STR_KEYWORD_ELEMENT* elements, int value)
{
    int     i;                  /* Loop counter */
    const char* string = NULL;  /* Return value */

    for (i = 0; elements[i].string; ++i) {
        if (value == elements[i].value) {
            string = elements[i].string;
            break;
        }
    }

    return string;
}

/*+
 * KsmKeyword<type>NameToValue - Convert Name to Value
 * KsmKeyword<type>ValueToName - Convert Value to Name
 *
 * Description:
 *      Converts between keywords and associated values for the specific
 *      element.
 *
 * Arguments:
 *      const char* name -or- int value
 *          Name of ID to convert.
 *
 * Returns:
 *      int -or- const char*
 *          Converted value.  The return value is NULL or 0 if no conversion is
 *          found.
-*/

int KsmKeywordAlgorithmNameToValue(const char* name)
{
    return KsmKeywordNameToValue(m_algorithm_keywords, name);
}

int KsmKeywordFormatNameToValue(const char* name)
{
    return KsmKeywordNameToValue(m_format_keywords, name);
}

int KsmKeywordParameterNameToValue(const char* name)
{
    return KsmKeywordNameToValue(m_parameter_keywords, name);
}

int KsmKeywordStateNameToValue(const char* name)
{
    return KsmKeywordNameToValue(m_state_keywords, name);
}

int KsmKeywordTypeNameToValue(const char* name)
{
    return KsmKeywordNameToValue(m_type_keywords, name);
}

const char* KsmKeywordAlgorithmValueToName(int value)
{
    return KsmKeywordValueToName(m_algorithm_keywords, value);
}

const char* KsmKeywordFormatValueToName(int value)
{
    return KsmKeywordValueToName(m_format_keywords, value);
}

const char* KsmKeywordStateValueToName(int value)
{
    return KsmKeywordValueToName(m_state_keywords, value);
}

const char* KsmKeywordTypeValueToName(int value)
{
    return KsmKeywordValueToName(m_type_keywords, value);
}



/*+
 * KsmKeywordParameterExists - Check if Keyword Exists
 *
 * Description:
 *      Checks if the keyword is the name of a parameter, returning true (1) if
 *      it is and false (0) if it isn't.
 *
 *      Unlike the other keyword checks, the match must be exact.
 *
 * Arguments:
 *      const char* name
 *          Name of the keyword to check.
 *
 * Returns:
 *      int
 *          1   Keyword exists
 *          0   Keyword does not exist
-*/

int KsmKeywordParameterExists(const char* name)
{
    int     exists = 0;
    int     i;

    if (name) {
        for (i = 0; m_parameter_keywords[i].string; ++i) {
            if (strcmp(name, m_parameter_keywords[i].string) == 0) {
                exists = 1;
                break;
            }
        }
    }

    return exists;
}
