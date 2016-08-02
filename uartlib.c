#include "uartlib.h"

unsigned int BAUD_ = B115200 ;
unsigned int NUM_BITS_ = CS8 ;
char *UART_PATH_ = "/dev/ttyAMA0" ;
unsigned int MAX_SIZE_ = 128 ;
unsigned int OPEN_FLAG_ = O_RDWR ;
time_t TIMEOUT_SEC_ = 5 ;
suseconds_t TIMEOUT_USEC_ = 0 ;


int open_conf_UART_()
{
	int indicator;
	int uart_filestream ;
	struct termios options ;
	WAIT_CONSTANT_ = time_for_one_byte_();

	// Opening the port in a read/write mode
	// O_NOCTTY - Nice explanation given here: http://stackoverflow.com/questions/12079059/why-prevent-a-file-from-opening-as-controlling-terminal-with-o-noctty
	uart_filestream = open(UART_PATH_, OPEN_FLAG_ | O_NOCTTY | O_NONBLOCK);
	if (uart_filestream < 0)
	{
		// Unable to open the serial port, so produce an error and halt
		// All these calls will set the errno accordingly so there's no need to do that here.
		return -1;
	}

	// Configuring the options for UART
	// The flags (defined in /usr/include/termios.h - see http://pubs.opengroup.org/onlinepubs/007908799/xsh/termios.h.html):
	// Baud rate:- B1200, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800, B500000, B576000, B921600, B1000000, B1152000, B1500000, B2000000, B2500000, B3000000, B3500000, B4000000
	// CSIZE:- CS5, CS6, CS7, CS8
	// CLOCAL - Ignore modem status lines
	// CREAD - Enable receiver
	// IGNPAR = Ignore characters with parity errors
	// ICRNL - Map CR to NL on input (Use for ASCII comms where you want to auto correct end of line characters - don't use for bianry comms!)
	// PARENB - Parity enable
	// PARODD - Odd parity (else even)
	
	// Retrieve the options and modify them. 
	indicator = tcgetattr(uart_filestream, &options);
	if(indicator < 0)
	{	
		// Unable to get the attributes (so unable to establish the connection)
		close(uart_filestream);
		return -1;
	}

	options.c_cflag = CRTSCTS | BAUD_ | NUM_BITS_ | CLOCAL | CREAD ;		//See flags above
	options.c_iflag = 0;
	options.c_oflag = 0;
	options.c_lflag = 0;

	// Flushing the file stream (the input area)
	indicator = tcflush(uart_filestream, TCIFLUSH);
	if(indicator < 0)
	{	
		// Unable to flush
		close(uart_filestream);
		return -1;
	}

	// Setting the options for the file stream. The TCSANOW constant means the change occurs immediately
	indicator = tcsetattr(uart_filestream, TCSANOW, &options);
	if(indicator < 0)
	{	
		// Unable to set the attributes
		close(uart_filestream);
		return -1;
	}

	return uart_filestream;
}

int read_UART_(int uart_filestream, char** dest, int max_len)
{
	//Variable section
	int indicator;
	int buffer_length;
	unsigned int counter;
	char one_byte;
	fd_set set;
	struct timeval timeout;
	////

	// Initializing set (for timeout)
	FD_ZERO(&set);
	FD_SET(uart_filestream, &set);

	timeout.tv_sec = TIMEOUT_SEC_;
	timeout.tv_usec = TIMEOUT_USEC_;

	indicator = tcflush(uart_filestream, TCIFLUSH);
	if(indicator < 0)
	{	
		// Unable to flush
		return -1;
	}

	// select waits for the uart_filestream to be ready for reading
	indicator = select(uart_filestream + 1, &set, NULL, NULL, &timeout);
	if(indicator == -1)
	{
		// select error has occurred
		return -1;
	}
	else if(indicator == 0)
	{
		// A timeout occurred, returning -2 as an indicator.
		return -2;
	}


	// Read up to max_len characters from the port if they are there
	counter = 0;
	while(1)
	{
		indicator = read(uart_filestream, (void*)&one_byte, 1);
		(*dest)[counter] = one_byte;
		counter++;

		if(indicator < 0)
		{
			break;
		}

		timeout.tv_usec = WAIT_CONSTANT_.tv_usec;
		timeout.tv_sec = WAIT_CONSTANT_.tv_sec;
		indicator = select(uart_filestream + 1, &set, NULL, NULL, &timeout);
		if(indicator < 0)
		{
			break;
		}
	}

	buffer_length = counter;
	
	if (buffer_length == 0 && indicator < 0)
	{
		// An error occured while reading
		return -1;
	}
	else
	{
		// Returning number of read bytes
		return buffer_length;
	}	
	// Both branches of the if statement above have return, so this will not be reached
}

int write_UART_(int uart_filestream, char *src, unsigned int len)
{
	//Variable section
	int indicator, sel_ind;
	fd_set set;
	struct timeval timeout;
	////

	// Initializing set (for timeout)
	FD_ZERO(&set);
	FD_SET(uart_filestream, &set);

	timeout.tv_sec = TIMEOUT_SEC_;
	timeout.tv_usec = TIMEOUT_USEC_;

	indicator = tcflush(uart_filestream, TCOFLUSH);
	if(indicator < 0)
	{	
		// Unable to flush
		return -1;
	}

	// select waits for the uart_filestream to be ready for reading
	sel_ind = select(uart_filestream + 1, NULL, &set, NULL, &timeout);
	if(sel_ind == -1)
	{
		// select error has occurred
		return -1;
	}
	else if(sel_ind == 0)
	{
		// A timeout occurred, returning -2 as an indicator.
		return -2;
	}

	//Trying to write to the filestream
	indicator = write(uart_filestream, src, len) ;

	if (indicator < 0)
	{
		// An error occured while writing
		return -1;
	}
	else
	{
		return indicator;
	}	
	// Both branches of the if statement above have return, so this will not be reached
}


int available_bytes_UART_(int uart_filestream)
{
	int availableBytes, indicator;
	indicator = ioctl(uart_filestream, FIONREAD, &availableBytes);
	if(indicator < 0)
	{
		return -1;
	}

	return availableBytes;
}

struct timeval time_for_one_byte_()
{
	int baud;
	long double time_for_one_byte;
	struct timeval ret_val;
	switch (BAUD_) 
	{
		case B115200 : baud = 115200; break; 
		case B57600 : baud = 57600; break;
		case B38400 : baud = 38400; break;
		case B19200 : baud = 19200; break;
		case B9600 : baud = 9600; break;
		case B4800 : baud = 4800; break;
		case B2400 : baud = 2400; break;
		case B1200 : baud = 1200; break;
		default : -1;
	}

	time_for_one_byte = (((long double)BITS_PER_PACKAGE_)/(long double)baud) * 1000000  ;

	//number of microseconds in a second
	ret_val.tv_usec = (long unsigned int)(((long unsigned int)time_for_one_byte % 1000000)*WAIT_PROLONGATION_CONSTANT_);
	ret_val.tv_sec = (long unsigned int)((time_for_one_byte - ret_val.tv_usec) / 1000000);

	//printf("SEC: %lu | USEC: %lu\n", ret_val.tv_sec, ret_val.tv_usec);

	return ret_val;
}


