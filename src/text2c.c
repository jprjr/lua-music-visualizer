#include "str.h"
#include "text.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    jpr_text *text;
    FILE *output;
    const char *line;
    int i = 0;
    int comma = 0;
    int line_comma = 0;
    int escape = 0;

    if(argc < 5) {
        fprintf(stderr,"Usage: %s text_file output_c_file output_h_file array_name\n",argv[0]);
        return 1;
    }

    text = jpr_text_open(argv[1]);
    if(text == NULL) {
        fprintf(stderr,"Unable to open input %s\n",argv[1]);
        return 1;
    }

    output = fopen(argv[2],"wb");
    if(output == NULL) {
        fprintf(stderr,"Unable to open output  %s\n",argv[2]);
        return 1;
    }

    fprintf(output,"#include <stddef.h>\n\n");
    fprintf(output,"const char *const %s[] = {",argv[4]);
    while( (line = jpr_text_line(text)) != NULL) {
        if(line_comma) fprintf(output, ",");
        else line_comma = 1;

        comma = 0;
        fprintf(output, "\n    \"");
        for(i=0;i<str_len(line);i++) {
            escape = 0;
            if(line[i] == '"') {
                escape = 1;
            }
            if(escape) {
                fprintf(output,"\\");
            }
            fprintf(output,"%c",line[i]);
        }
        fprintf(output,"\"");
    }
    fprintf(output,",\n    NULL");
    fprintf(output,"\n};\n");

    fclose(output);

    output = fopen(argv[3],"wb");
    if(output == NULL) {
        fprintf(stderr,"Unable to open header output %s\n",argv[3]);
        return 1;
    }
    fprintf(output,"extern const char *const %s[];\n",argv[4]);
    fclose(output);

    return 0;
}
