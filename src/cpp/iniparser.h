#pragma once

#include <list>
#include <string>

/*  config_sample.ini

[section1]
key1 = name1
key2 = e1, e2, e3 # value list
; comment
[section 2]
# quote will be removed by parser
key1 = "asd asdfsdaf", "23", "thos"  
# multiline value: " \" must be the last two chars or "\" is the only char in line
# the leading spaces in the continued line will be removed.
key2 = 12, 23, \
       43,23, \
\
56
# if quoted string continues to next line, comment should not be appended to unfinished lines
key3 = "as df", "123  \  
           45" , \
       "2343"  # prevous line should not have comment

*/

struct IniGetter
{
    virtual int getSections(std::list<std::string> &sections) const = 0;

    virtual int getKeys(std::list<std::string> &keys, const char *section="") const = 0;

    virtual int getValues(std::list<std::string> &values, const char *key, const char *section="") const = 0;
    virtual int getValue(std::string &values, const char *key, const char *section="") const = 0;

};
struct IniSetter
{
    virtual void setValues(std::list<std::string>& values, const char *key, const char *section="") = 0;
    virtual void setValue(const char* values, const char *key, const char *section="") = 0;

    virtual void clear() = 0;
};

struct IniFile
        : public IniSetter
        , public IniGetter
{
    virtual int load(const char* filename) = 0;
    virtual bool save(const char* filename) = 0;

};

#ifdef __cplusplus
extern "C"
#endif
IniFile *IniFile_New();
