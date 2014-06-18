#include "iniparser.h"
#include <map>
#include <sstream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define INI_MAX_LINE 4086
#define MAX_SECTION  512

#define ISCOMMENT(c) ((c == ';' || c == '#')?1:0)
#define DQUATE '\"'
#define SQUATE '\''
#define VSEP ','
#define ISQUOTE(c) (c =='\"'?1:0)

/* Strip whitespace chars off end of given string, in place. Return s. */
static char* rstrip(char* s, char **end=NULL)
{
    char* p = (end && *end) ? (*end) : (s + strlen(s));
    while (p > s && isspace((unsigned char)(*--p)))
        *p = '\0';
    if( end ) *end = *p == '\0' ? p :(p+1);
    return s;
}

/* Return pointer to first non-whitespace char in given string. */
static char* lskip(const char* s)
{
    while (*s && isspace((unsigned char)(*s)))
        s++;
    return (char*)s;
}
static char *stripquote(char *s)
{
    s = lskip(s);
    char *p=NULL;
    rstrip(s, &p);
    p--;
    if( ISQUOTE(*s) && ISQUOTE(*p) ) {
        s++;
        *p = '\0';
    }
    return s;
}
/* Return pointer to first char c or ';' comment in given string, or pointer to
   null at end of string if neither found. ';' must be prefixed by a whitespace
   character to register as a comment. The quoted char will be ignored. */
static char* find_char_or_comment(const char* s, char c)
{
    int dquoted = 0, squoted = 0; // double quoated, single quoted
    char prev = 0;
    while (*s && (*s != c && !ISCOMMENT(*s) || dquoted || squoted) ) {
        //        was_whitespace = isspace((unsigned char)(*s));
        if( '\"' == *s &&  prev!= '\\') dquoted = ! dquoted;
        else if( '\'' == *s &&  prev!= '\\') squoted = ! squoted;
        prev = *s;
        s++;
    }
    return (char*)s;
}

/* Version of strncpy that ensures dest (size bytes) is null-terminated. */
static char* strncpy0(char* dest, const char* src, size_t size)
{
    strncpy(dest, src, size);
    dest[size - 1] = '\0';
    return dest;
}

struct IniFileImpl
        : public IniFile
{

    std::string    status;

    // read and parse
    int load(const char* filename) {
        FILE *file = fopen(filename, "r");
        if(!file) return false;

        char *line = new char[INI_MAX_LINE];
        std::string prev_value, prev_name;
        char *section = new char [MAX_SECTION];
        *section = '\0';

        int  bMultiline = 0;
        int lineno = 0, error=0;
        char *start, *end, *name, *value;
        while (fgets(line, INI_MAX_LINE, file) != NULL) {
            lineno++;
            start = line;
            start = lskip(start);
            // remove comment
            end = find_char_or_comment(start, '\0');
            if( *end != '\0' )  // it points to a comment char
                *end = '\0';
            rstrip(start, &end);

            if( start == end )
                continue;

            if ( bMultiline ) {
                if( *(end-1) == '\\' && (end-1 == start || *(end-2) == ' ') )  { // continue reading next line
                    // remove " \"
                    *(--end) = '\0';
                    if( end > start && *(end-1) == ' ')
                        *(--end) = '\0';

                    prev_value += start;
                }
                else{
                    prev_value += start;
                    if( !handle_record(section, prev_name.c_str(), prev_value.c_str()) )
                        error = lineno;
                    bMultiline = 0;
                    prev_value = "";
                    prev_name = "";
                }
            }
            else if( *start == '[' ) { // found section
                end = find_char_or_comment(start + 1, ']');
                if (*end == ']') {
                    *end = '\0';
                    strncpy0(section, start + 1, MAX_SECTION);
                }
                else{
                    /* No ']' found on section line */
                    error = lineno;
                }
                bMultiline = false;
            }
            else {
                /* Not a comment, must be a name[=:]value pair */
                bMultiline = false;
                char *nend = find_char_or_comment(start, '=');
                if (*nend != '=') {
                    nend = find_char_or_comment(start, ':');
                }
                if (*nend == '=' || *nend == ':') {
                    *nend = '\0';
                    value = lskip(nend + 1);
                    name = rstrip(start, &nend);

                    /* Valid name[=:]value pair found, call handler */
                    if( *(end-1) == '\\' && (end-1 == value || *(end-2) == ' ') )  { // continue reading next line
                        // remove " \"
                        *(--end) = '\0';
                        if( end > value && *(end-1) == ' ')
                            *(--end) = '\0';

                        prev_value = value;
                        prev_name = name;
                        bMultiline = true;
                    }
                    if( !bMultiline ) if (!handle_record(section, name, value))
                        error = lineno;
                }
                else if (!error) {
                    /* No '=' or ':' found on name[=:]value line */
                    error = lineno;
                }
            }
            if (error)
                break;
        } // while getline

        fclose(file);
        delete [] line;
        delete [] section;
        return error;
    } // read function
    bool handle_record(const char* section, const char *name, const char *avalue)
    {
        const int N = strlen(avalue)+1;
        char *buffer= (char*)malloc(N);
        char *end, *value = buffer;
        strcpy(value, avalue);
        ValueInfo& p = sections[std::string(section)][std::string(name)];
        p.first = value;
        p.second.clear();
        while( *value != '\0' ) {
            end = find_char_or_comment(value, VSEP);
            if( *end != '\0' ) {
                *end = '\0';
                p.second.push_back( std::string(stripquote(value)) );
                value = end+1;
            }else{
                p.second.push_back( std::string(stripquote(value)) );
                value = end;
            }
        }
        free(buffer);
        return true;
    }

    typedef std::list<std::string>                 ValueList;
    typedef ValueList::iterator                    ValueListIter;
    typedef std::pair<std::string, ValueList>      ValueInfo;
    typedef std::map<std::string, ValueInfo>       KeyValueMap;
    typedef std::map<std::string, KeyValueMap>     SectionMap;

    SectionMap sections;

    //-- interface IniGetter ---

    virtual int getSections(std::list<std::string> &sections) const {
        int n = 0;
        for(SectionMap::const_iterator it=this->sections.begin(); it!= this->sections.end(); ++it) {
            sections.push_back(it->first);
            n++;
        }
        return n;
    }

    virtual int getKeys(std::list<std::string> &keys, const char *section="") const {
        int n = 0;
        SectionMap::const_iterator it0 = this->sections.find(std::string(section));
        if( it0 == this->sections.end() )
            return 0;
        const KeyValueMap& kv = it0->second;

        for(KeyValueMap::const_iterator it=kv.begin(); it!= kv.end(); ++it) {
            keys.push_back(it->first);
            n++;
        }
        return n;
    }

    virtual int getValues(std::list<std::string> &values, const char *key, const char *section="") const {
        int n = 0;
        SectionMap::const_iterator it0 = this->sections.find(section);
        if( it0 == this->sections.end()) return 0;
        const KeyValueMap& kv = it0->second;

        KeyValueMap::const_iterator it1 = kv.find(key);
        if( it1 == kv.end() ) return 0;
        const ValueList& v = it1->second.second;

        for(ValueList::const_iterator it=v.begin(); it!= v.end(); ++it) {
            values.push_back(*it);
            n++;
        }
        return n;
    }
    virtual int getValue(std::string &value, const char *key, const char *section="") const {

        SectionMap::const_iterator it0 = this->sections.find(section);
        if( it0 == this->sections.end() ) return 0;
        const KeyValueMap& kv = it0->second;

        KeyValueMap::const_iterator it1 = kv.find(key);
        if( it1 == kv.end() ) return 0;
        const ValueInfo& v = it1->second;

        value = v.first;
        return 1;
    }
    //-- interface IniGetter ---

    virtual void setValues(std::list<std::string>& values, const char *key, const char *section="") {
        ValueInfo& v = this->sections[section][key];
        v.second = values;
        std::stringstream ss;
        for(ValueList::iterator it=values.begin(), prev=values.begin(); it!= values.end(); ++it) {
            if( it != prev) ss << VSEP;
            ss << *it;
        }
        v.first = ss.str();
    }

    virtual void setValue(const char* value, const char *key, const char *section="") {
        handle_record(section, key, value);
    }
    virtual bool save(const char* filename) {
        FILE *file = fopen(filename, "w");
        if(!file) return false;

        SectionMap::iterator it0 = this->sections.find(std::string(""));
        if( it0 != this->sections.end() ) {
            writeKeyValueMap(it0->second, file);
        }
        for(it0=this->sections.begin(); it0!= this->sections.end(); ++it0) {
            if( it0->first != std::string("") ) {
                fprintf(file, "[%s]\n", it0->first.c_str());
                writeKeyValueMap(it0->second, file);
            }
        }

        fclose(file);
        return true;
    }
    void writeKeyValueMap(KeyValueMap& kv, FILE* file) {
        for(KeyValueMap::iterator it= kv.begin(); it != kv.end(); ++it)
            fprintf(file, "%s = %s\n", it->first.c_str(), it->second.first.c_str());
    }

    void clear() {
        sections.clear();
    }

    virtual ~IniFileImpl(){}

};

IniFile *IniFile_New()
{
    printf("entering IniFile_New\n");
    return new IniFileImpl();
}

