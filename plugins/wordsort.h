/*** -*- C++ -*- *********************************************************/

#ifndef _HEXTREME_WORDCOUNT_H
#define _HEXTREME_WORDCOUNT_H

#include <mapredo/mapreducer.h>

/**
 * Class used to sort words on popularity.
 */
class wordsort : public mapredo::mapreducer<int64_t>
{
public:
    wordsort() {}
    virtual ~wordsort() {}
    void map (char* line, const int length, mapredo::collector& output);
    void reduce (int64_t key, const vlist& values, mapredo::collector& output);

private:    
};

#endif
