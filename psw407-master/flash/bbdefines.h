#ifndef BB_DEFINES_H
#define BB_DEFINES_H

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif


/*структура файловой системы на spi flash*/
/*0-2Mb - временная область для обновления прошивки
 * 2-3Mb -  чёрный ящик
 * 3-4Мб - ФС для файлов справки, css, js и прочих статичных объектов, загружается отдельной операцией
 */

//временная область // загрузка прошивки, конфигурации
#define FL_TMP_START	0
#define FL_TMP_END		2097152

//область чёрного ящика
#define FL_BB_START		2097152// смещение ЧЯ относительно начала флеш
#define FL_BB_END		3145728//размер черного ящика

//область файлов справки
#define FL_FS_START		3145728
#define FL_FS_END		4194304

#define BB_MSG_LEN		64//размер записи во флеш

#if 0
typedef unsigned char  u8;
typedef signed char    i8;
typedef unsigned short u16;
typedef signed short   i16;
typedef unsigned long  u32;
typedef signed long    i32;
#endif

#endif
