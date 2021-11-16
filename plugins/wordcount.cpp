#include <iostream>
#include <stdexcept>

#include "wordcount.h"

MAPREDO_FACTORIES (wordcount)

void
wordcount::map (char* line, const int length, mapredo::mcollector& output)
{
    bool seen_word = false;
    int start = 0;

    for (int i = 0; i < length; ++i)
    {
	if (isspace(line[i]) || ispunct(line[i]))
	{
	    if (seen_word) output.collect (line+start, i - start);
	    seen_word = false;
	}
	else
	{
	    if (!seen_word)
	    {
		seen_word = true;
		start = i;
	    }
	    line[i] = tolower (line[i]);
	}
    }

    if (seen_word) output.collect (line+start, length - start);
}

void
wordcount::reduce (char* key, vlist& values, mapredo::rcollector& output)
{
    int count = 0;

    for (char* value : values)
    {
	if (*value) count += atoi(value);
	else count++;
    }
    
    output.collect_keyval (key, count);
}
