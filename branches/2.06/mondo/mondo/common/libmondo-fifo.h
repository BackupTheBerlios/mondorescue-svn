/* libmondo-fifo.h
 * $Id$
 */






FILE *open_device_via_buffer(char *dev, char direction,
							 long internal_block_size);
void sigpipe_occurred(int);
void kill_buffer();
