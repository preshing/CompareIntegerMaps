#include "../hashtable.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


static const char* whitespace = " \t\r\n";

int main(int argc, const char* argv[])
{
    HashTable ht;
    for (;;)
    {
        char buf[256];
        if (fgets(buf, 256, stdin) == NULL)
            break;
        char *command = strtok(buf, whitespace);
        unsigned int key;
        unsigned int value;
        if (strcmp(command, "insert") == 0)
        {
            sscanf(strtok(NULL, whitespace), "%u", &key);
            sscanf(strtok(NULL, whitespace), "%u", &value);
            ht.Insert(key)->value = value;
        }
        else if (strcmp(command, "lookup") == 0)
        {
            sscanf(strtok(NULL, whitespace), "%u", &key);
            HashTable::Cell* result = ht.Lookup(key);
            if (result)
                printf("%u\n", (unsigned int) result->value);
            else
                printf("None\n");
        }
        else if (strcmp(command, "increment") == 0)
        {
            sscanf(strtok(NULL, whitespace), "%u", &key);
            ht.Insert(key)->value++;
        }
        else if (strcmp(command, "delete") == 0)
        {
            sscanf(strtok(NULL, whitespace), "%u", &key);
            ht.Delete(key);
        }
        else if (strcmp(command, "clear") == 0)
        {
            ht.Clear();
        }
        else if (strcmp(command, "compact") == 0)
        {
            ht.Compact();
        }
        fflush(stdout);
    }

    // Dump entire table
    printf("{\n");
    for (HashTable::Iterator iter(ht); *iter; iter.Next())
    {
        printf("    %u: %u,\n", iter->key, iter->value);
    }
    printf("}\n");
    return 0;
}
