/* libmondo-fifo.h
 * $Id: libmondo-fifo.h,v 1.2 2004/06/10 15:29:12 hugo Exp $
 */






FILE*open_device_via_buffer(char*dev, char direction, long internal_block_size);
void sigpipe_occurred(int);
void kill_buffer();
