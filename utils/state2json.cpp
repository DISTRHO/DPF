#include "extra/Base64.hpp"
#include "extra/String.hpp"

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        d_stdout("usage: %s [base64-dpf-state]", argv[0]);
        return 1;
    }

    const std::vector<uint8_t> data(d_getChunkFromBase64String(argv[1]));

    if (data.empty())
    {
        printf("{}\n");
        return 0;
    }

    String key, value;
    bool firstValue = true;
    bool hasValue = false;
    bool fillingKey = true; // if filling key or value
    char queryingType = 'i'; // can be 'n', 's' or 'p' (none, states, parameters)

    const char* const buffer = reinterpret_cast<const char*>(data.data());

    printf("{");

    for (size_t i = 0; i < data.size(); ++i)
    {
        // found terminator, stop here
        if (buffer[i] == '\xfe')
        {
            break;
        }

        // append to temporary vars
        if (fillingKey)
        {
            key += buffer + i;
        }
        else
        {
            value += buffer + i;
            hasValue = true;
        }

        // increase buffer offset by length of string
        i += std::strlen(buffer + i);

        // if buffer offset points to null, we found the end of a string, lets check
        if (buffer[i] == '\0')
        {
            // special keys
            if (key == "__dpf_state_begin__")
            {
                DISTRHO_SAFE_ASSERT_INT_RETURN(queryingType == 'i' || queryingType == 'n', queryingType, 1);
                if (queryingType == 'n')
                    printf(",");
                printf("\n  \"states\": {");
                queryingType = 's';
                key.clear();
                value.clear();
                firstValue = true;
                hasValue = false;
                continue;
            }
            if (key == "__dpf_state_end__")
            {
                DISTRHO_SAFE_ASSERT_INT_RETURN(queryingType == 's', queryingType, 1);
                printf("\n  }");
                queryingType = 'n';
                key.clear();
                value.clear();
                hasValue = false;
                continue;
            }
            if (key == "__dpf_parameters_begin__")
            {
                DISTRHO_SAFE_ASSERT_INT_RETURN(queryingType == 'i' || queryingType == 'n', queryingType, 1);
                if (queryingType == 'n')
                    printf(",");
                printf("\n  \"parameters\": {");
                queryingType = 'p';
                key.clear();
                value.clear();
                firstValue = true;
                hasValue = false;
                continue;
            }
            if (key == "__dpf_parameters_end__")
            {
                DISTRHO_SAFE_ASSERT_INT_RETURN(queryingType == 'p', queryingType, 1);
                printf("\n  }");
                queryingType = 'x';
                key.clear();
                value.clear();
                hasValue = false;
                continue;
            }

            // no special key, swap between reading real key and value
            fillingKey = !fillingKey;

            // if there is no value yet keep reading until we have one
            if (! hasValue)
                continue;

            if (key == "__dpf_program__")
            {
                DISTRHO_SAFE_ASSERT_INT_RETURN(queryingType == 'i', queryingType, 1);
                queryingType = 'n';

                printf("\n  \"program\": %s", value.buffer());
            }
            else if (queryingType == 's')
            {
                if (! firstValue)
                    printf(",");
                // TODO safely encode value as json compatible string
                printf("\n    \"%s\": %s", key.buffer(), value.buffer());
            }
            else if (queryingType == 'p')
            {
                if (! firstValue)
                    printf(",");
                printf("\n    \"%s\": %s", key.buffer(), value.buffer());
            }

            key.clear();
            value.clear();
            firstValue = hasValue = false;
        }
    }

    printf("\n}\n");
    return 0;
}
