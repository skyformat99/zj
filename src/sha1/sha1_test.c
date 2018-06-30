#include "sha1.c"
#include "utils.c"

#define __str_orig "abcdefg"
#define __res_lowercase "2fb5e13419fc89246865e7a324f476ec624e8740" // printf "abcdefg" | sha1sum
#define __res_uppercase "2FB5E13419FC89246865E7A324F476EC624E8740" // printf "abcdefg" | sha1sum

_i
main(void){
    char res[41];

    sha1_lower_case(__str_orig, sizeof(__str_orig) - 1, res);
    So(0, strcmp(res, __res_lowercase));

    sha1_upper_case(__str_orig, sizeof(__str_orig) - 1, res);
    So(0, strcmp(res, __res_uppercase));
}
