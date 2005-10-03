/* libfifo-EXT.h */

extern FILE*open_device_via_buffer(char*dev, char direction, long internal_block_size);
extern void sigpipe_occurred(int);
extern void kill_buffer();

