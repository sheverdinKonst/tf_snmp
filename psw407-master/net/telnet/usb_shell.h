#ifndef USB_SHELL_H_
#define USB_SHELL_H_

void usb_shell_init(void);
void usb_shell_task_start(void);
void usb_shell_task_stop(void);
void usb_shell_task(void);
void set_char(char ch);
u8 usb_shell_state(void);














#endif /* USB_SHELL_H_ */
