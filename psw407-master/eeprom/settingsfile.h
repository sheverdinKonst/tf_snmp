
#ifndef SETTINGSFILE_H_
#define SETTINGSFILE_H_

#define BLOCK_SIZE	512

httpd_cgifunction bak_cgi(char *name);
u32 parse_bak_file(void);
u32 make_log(void);
u32 make_bak(void);
void set_from_search(u8 state);
#endif /* SETTINGSFILE_H_ */
