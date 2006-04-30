#include <time.h>
#include <string.h>

#define _XSTR(x) #x
#define __XSTR(x) _XSTR(x)
#define SUBFUNCTION() subfunct(__FUNCTION__, __FILE__, __XSTR(__LINE__))
#define _SUBFUNCTION(a,b,c) subfunct(a, b, #c)




void subfunct(char *func, char *file, char *line)
{
    char *descr;
    /*              "file.c"   ":"  "122"      ", "  "main"      "():\n\0"  */
    descr = alloca(strlen(file)+1 +strlen(line)+ 2  +strlen(func)+5);
    sprintf(descr, "%s:%s, %s():\n", file, line, func);
    printf("'%s'", descr);
}

int main(int argc, char *argv [])
{
    time_t clock;
    struct tm tm;

    char buf[100];
    int x;

    printf("\\t=%u \\n=%u \\r=%u \\c=%u  \n", (unsigned) '\t',(unsigned) '\n',(unsigned) '\r',(unsigned) '\c' );

    time(&clock);
/*
    ctime_r(&clock, buf);
    printf("'%s' strlen()=%d\n", buf, strlen(buf));

    buf[strlen(buf)-6] = '*';
    buf[10] = '>';

    printf("'%s' strlen()=%d\n", buf, strlen(buf));
*/
    localtime_r(&clock, &tm);

    printf("%d.%d.%d %02d:%02d:%02d\n", tm.tm_mday, tm.tm_mon, tm.tm_year+1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
    usleep(500000);

    clock = time(NULL);
    localtime_r(&clock, &tm);
//    localtime_r(NULL, &tm);
    printf("%d.%d.%d %02d:%02d:%02d\n", tm.tm_mday, tm.tm_mon, tm.tm_year+1900, tm.tm_hour, tm.tm_min, tm.tm_sec);

    printf("func: %s, file: %s, line: %d\n", __FUNCTION__, __FILE__, __LINE__);

    SUBFUNCTION();

    printf("basename: %s\n", strrchr(argv[0], '/'));
#define BASENAME(a) printf("basename of \""a"\" is \"%s\"\n", strrchr(a, '/'))

    BASENAME("name");
    BASENAME("/name");
    BASENAME("path/name");
    BASENAME("/path/name");
    BASENAME("path/");
    BASENAME("/path/");
    return 0;
}