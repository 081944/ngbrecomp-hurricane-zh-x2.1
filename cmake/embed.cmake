# cmake -DIN=<file> -DOUT=<file.c> -DNAME=<symbol> -P embed.cmake
# A file's bytes as `const unsigned char NAME[]` and `NAME_len`.
file(READ "${IN}" hex HEX)
string(LENGTH "${hex}" n)
math(EXPR len "${n} / 2")
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," bytes "${hex}")
string(REGEX REPLACE "(0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,)" "\\1\n" bytes "${bytes}")
file(WRITE "${OUT}" "/* Generated from ${IN} by cmake/embed.cmake. */\n"
     "const unsigned char ${NAME}[] = {\n${bytes}\n};\n"
     "const unsigned int ${NAME}_len = ${len};\n")
