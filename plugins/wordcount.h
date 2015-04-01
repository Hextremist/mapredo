/*** -*- C++ -*- *********************************************************/

#ifndef _HEXTREME_WORDCOUNT_H
#define _HEXTREME_WORDCOUNT_H

#include <mapredo/mapreducer.h>

/**
 * Class used to count word frequency
 */
class wordcount final : public mapredo::mapreducer<char*>
{
public:
    void map (char* line, const int length, mapredo::mcollector& output);
    void reduce (char* key, vlist& values, mapredo::rcollector& output);
    bool reducer_can_combine() const {return true;}
};

#endif
