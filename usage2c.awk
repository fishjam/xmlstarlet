#!/usr/bin/awk -f

BEGIN {
    print("#include <stdio.h>");
    print("#include <libxml/xmlversion.h>");
}

# text in src/foo-bar.txt results in
#   static const char foo_text[] = {
#     't', 'h', 'e', ' ', 't', 'e', 'x', 't', ...
#   }
length(command_name) == 0 {
    command_name = FILENAME;
    sub(/\.txt$/, "", command_name);
    sub(/^([^\/]+\/)*/, "", command_name);
    gsub(/-/, "_", command_name);
    printf("static const char %s[] = {\n", command_name);
    progs = 0;
}

# escape percent signs: we are going to use printf(). I don't think there are
# any %s currently, but it's bound to happen eventually.
{ gsub(/%/, "%%"); }

# white space separted instances of PROG will be replaced with the final name of
# the xmlstarlet executable
/PROG/ {
    for (i = 1; i <= NF; i++) {
        if ($i == "PROG") {
            progs++;
            $i = "%s";
        }
    }
}

# C-preprocessor directives are unchanged
/^#/

# convert plain text lines into equivalent char array literal
!/^#/ {
    for (i = 1; i <= length($0); i++) {
        c = substr($0, i, 1);
        if (c == "\\")          # \ --> '\\'
            printf("'\\\\'");
        else if (c == "'")      # ' --> '\''
            printf("'\\''");
        else                    # X --> 'X'
            printf("'%c'", c);
        printf(",");
    }
    print("'\\n',");
}

END {
    print("0 };");
    printf("void fprint_%s(FILE* out, const char* argv0) {\n", command_name);
    printf("  fprintf(out, %s", command_name);
    for (i = 1; i <= progs; i++)
        printf(", argv0");
    print(");\n}");
}

