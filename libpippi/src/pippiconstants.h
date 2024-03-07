/* CONSTANTS */
#ifndef PI
#define PI 3.1415926535897932384626433832795028841971693993751058209749445923078164062
#endif

#ifndef HALFPI
#define HALFPI (PI/2.0)
#endif

#ifndef PI2
#define PI2 (PI*2.0)
#endif

#ifndef EULER
#define EULER 2.718281828459045235360287471352662497757247093
#endif

#define LPVSPEED_MIN 0.001

#define GRID_EMPTY 0x2800
#define GRID_FULL  0x28ff

#define LOGISTIC_SEED_DEFAULT 3.999
#define LOGISTIC_X_DEFAULT 0.555

#define LORENZ_TIMESTEP_DEFAULT 0.011
#define LORENZ_A_DEFAULT 10.0
#define LORENZ_B_DEFAULT 28.0
#define LORENZ_C_DEFAULT (8.0 / 3.0)
#define LORENZ_X_DEFAULT 0.1
#define LORENZ_Y_DEFAULT 0.0
#define LORENZ_Z_DEFAULT 0.0

/* plot width/height is measured in display chars */
#define PLOT_WIDTH 20
#define PLOT_HEIGHT 10

/* braille width/height correspond to the number of dots in 
 * each unicode braille char: 2 columns of 4 dots */
#define BRAILLE_WIDTH 2
#define BRAILLE_HEIGHT 4

/* This is the virtual pixel grid, drawn in braille dot 
 * configurations across the field of braille chars */
#define PIXEL_WIDTH (PLOT_WIDTH * BRAILLE_WIDTH)
#define PIXEL_HEIGHT (PLOT_HEIGHT * BRAILLE_HEIGHT)

#define DEFAULT_CHANNELS 2
#define DEFAULT_SAMPLERATE 48000
#define DEFAULT_TABLESIZE 4096

#ifdef LP_FLOAT
#define HANN_WINDOW_SIZE 256
#else
#define HANN_WINDOW_SIZE 4096
#endif

#ifndef PIPE_BUF
#define PIPE_BUF 4096
#endif

#define LPMAXNAME 24
#define LPMAXMSG (PIPE_BUF - (sizeof(double) * 4) - (sizeof(size_t) * 3) - sizeof(uint16_t) - LPMAXNAME)



