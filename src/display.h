#ifndef __DISPLAY__H__
#define __DISPLAY__H__

void display_init();
void display_clean();
void display_time(int h);
void display_msg(const char* msg);
void render_time(int h, int m, int s);
void spin();

#endif
