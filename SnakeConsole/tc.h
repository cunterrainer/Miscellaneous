#ifndef TC_H
#define TC_H

// Linux
#ifdef __linux__

#include <stdio.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

// color
#define TC_DEFAULT      "\x1B[0m"
#define TC_BLACK        "\x1B[0;30m"
#define TC_RED          "\x1B[0;31m"
#define TC_GREEN        "\x1B[0;32m"
#define TC_YELLOW       "\x1B[0;33m"
#define TC_BLUE         "\x1B[0;34m"
#define TC_MAGENTA      "\x1B[0;35m"
#define TC_CYAN         "\x1B[0;36m"
#define TC_WHITE        "\x1B[0;37m"

// bold
#define TC_B_DEFAULT    "\x1B[1;0m"
#define TC_B_BLACK      "\x1B[1;30m"
#define TC_B_RED        "\x1B[1;31m"
#define TC_B_GREEN      "\x1B[1;32m"
#define TC_B_YELLOW     "\x1B[1;33m"
#define TC_B_BLUE       "\x1B[1;34m"
#define TC_B_MAGENTA    "\x1B[1;35m"
#define TC_B_CYAN       "\x1B[1;36m"
#define TC_B_WHITE      "\x1B[1;37m"

// background
#define TC_BG_DEFAULT   "\x1B[0m"
#define TC_BG_BLACK     "\x1B[40m"
#define TC_BG_RED       "\x1B[41m"
#define TC_BG_GREEN     "\x1B[42m"
#define TC_BG_YELLOW    "\x1B[43m"
#define TC_BG_BLUE      "\x1B[44m"
#define TC_BG_MAGENTA   "\x1B[45m"
#define TC_BG_CYNAN     "\x1B[46m"
#define TC_BG_WHTITE    "\x1B[47m"

// underline
#define TC_U_DEFAULT    "\x1B[0m"
#define TC_U_BLACK      "\x1B[0;30;4m"
#define TC_U_RED        "\x1B[0;31;4m"
#define TC_U_GREEN      "\x1B[0;32;4m"
#define TC_U_YELLOW     "\x1B[0;33;4m"
#define TC_U_BLUE       "\x1B[0;34;4m"
#define TC_U_MAGENTA    "\x1B[0;35;4m"
#define TC_U_CYAN       "\x1B[0;36;4m"
#define TC_U_WHITE      "\x1B[0;37;4m"

// underline bold
#define TC_U_B_DEFAULT  "\x1B[1;0;4m"
#define TC_U_B_BLACK    "\x1B[1;30;4m"
#define TC_U_B_RED      "\x1B[1;31;4m"
#define TC_U_B_GREEN    "\x1B[1;32;4m"
#define TC_U_B_YELLOW   "\x1B[1;33;4m"
#define TC_U_B_BLUE     "\x1B[1;34;4m"
#define TC_U_B_MAGENTA  "\x1B[1;35;4m"
#define TC_U_B_CYAN     "\x1B[1;36;4m"
#define TC_U_B_WHITE    "\x1B[1;37;4m"

// underline background
#define TC_U_BG_DEFAULT "\x1B[0;4m"
#define TC_U_BG_BLACK   "\x1B[40;4m"
#define TC_U_BG_RED     "\x1B[41;4m"
#define TC_U_BG_GREEN   "\x1B[42;4m"
#define TC_U_BG_YELLOW  "\x1B[43;4m"
#define TC_U_BG_BLUE    "\x1B[44;4m"
#define TC_U_BG_MAGENTA "\x1B[45;4m"
#define TC_U_BG_CYNAN   "\x1B[46;4m"
#define TC_U_BG_WHTITE  "\x1B[47;4m"


#define tcClearScreen() puts("\x1B[2J")
#define tcMoveCursor(X, Y) printf("\033[%d;%dH", Y, X)
#define tcEnterAltScreen() puts("\033[?1049h\033[H")
#define tcExitAltScreen() puts("\033[?1049l")

static void tcGetColsRows(int *cols, int *rows){

	struct winsize size;
	ioctl(1, TIOCGWINSZ, &size);
	*cols = size.ws_col;
	*rows = size.ws_row;

}//tec_get_cols_rows*/

static void tcEchoOff(){

	struct termios term;
	tcgetattr(1, &term);
	term.c_lflag &= ~ECHO;
	tcsetattr(1, TCSANOW, &term);

}//tc_echo_off*/

static void tcEchoOn(){

	struct termios term;
	tcgetattr(1, &term);
	term.c_lflag |= ECHO;
	tcsetattr(1, TCSANOW, &term);

}//tc_echo_on*/
#endif // Linux

#endif
