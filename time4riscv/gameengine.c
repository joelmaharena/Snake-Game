#include <stdio.h>
#include <stdlib.h> // För abs-funktionen

extern void enable_interrupt();
#define VGA_WIDTH 320
#define VGA_HEIGHT 240
#define BOARD_WIDTH 160	// New game board width
#define BOARD_HEIGHT 120 // New game board height
#define BLUE 0x001F		// Assuming 5-bit blue in a 16-bit color format
#define GREEN 0x00FF
#define RED 0x07E0
int timeoutcount = 0; // Delay for Timer interrupts
int x_offset = (VGA_WIDTH - BOARD_WIDTH) / 2; // X Offset needed to center the board
int y_offset = (VGA_HEIGHT - BOARD_HEIGHT) / 2; // Y Offset needed to center the board
int moveX = BOARD_WIDTH / 2;	 // Updated start position X-axis
int moveY = BOARD_HEIGHT / 2;	 // Updated start position Y-axis
int switchDifference = 1;		 // The snake starts going right
int formerValue = 0;			 // Previous switchValue
int switchValue = 0;
int deltaX = 1; // Direction in x
int deltaY = 0; // Direction in Y
int snakeLength = 0;
int foodX = 0;
int foodY = 0;
volatile int snakeX[100];
volatile int snakeY[100];
char direction = 'E';

int get_sw(void) // Get the values of the switches
{
	volatile int *switches = (volatile int *)0x04000010;

	return *switches & 0x3; // Mask four LSB.
}

void set_displays(int display_number, int value) // Set a 7-digit display to the single digit value.
{
	volatile int *display = (volatile int *)(0x4000050 + display_number * 0x10);
	switch (value)
	{
	case 0:
		*display = 0b01000000;
		break;
	case 1:
		*display = 0b01111001;
		break;
	case 2:
		*display = 0b00100100;
		break;
	case 3:
		*display = 0b00110000;
		break;
	case 4:
		*display = 0b00011001;
		break;
	case 5:
		*display = 0b00010010;
		break;
	case 6:
		*display = 0b00000010;
		break;
	case 7:
		*display = 0b01111000;
		break;
	case 8:
		*display = 0b00000000;
		break;
	case 9:
		*display = 0b00011000;
		break;
	default:
		*display = 0b00000000;
		break;
	}
}

void set_displaysToZero() // Set all displays to Zero
{

	for (int i = 0; i <= 5; i++)
	{ // Assuming there are 4 displays
		set_displays(i, 0);
	}
}

void increase_score() // Increase the score on the 7-digit display after apple has been eaten
{
	if (snakeLength % 10)
	{
		set_displays(0, snakeLength / 2);
	}
	if (snakeLength % 100)
	{
		set_displays(1, (snakeLength / 20));
	}
}

void game_over() // Reset the game when snake collides with itself or wall.
{
	snakeLength = 0;
	set_displaysToZero();
	moveX = BOARD_WIDTH / 2;
	moveY = BOARD_HEIGHT / 2;
	deltaX = 1;
	deltaY = 0;
	direction = 'E';
	formerValue = get_sw();
	// Clear snake body positions
	for (int i = 0; i < 100; i++)
	{
		snakeX[i] = 0;
		snakeY[i] = 0;
	}
}

void detect_collision() // Need Review
{
	// Check collision with board edges (walls)
	if (moveX <= 0 || moveX >= BOARD_WIDTH - 1 || moveY <= 0 || moveY >= BOARD_HEIGHT - 1)
	{
		game_over();
	}
	// Check collision with the snake's own body
	for (int i = 0; i < snakeLength; i++)
	{
		if (moveX == snakeX[i] && moveY == snakeY[i])
		{
			game_over();
			break; // This breaks out of the for loop, not the if statement
		}
	}
}

void move_snake(int delta_X, int delta_Y) // Update the coordinates of the snakes head.
{
	// Move body segments manually
	for (int i = snakeLength; i > 0; i--)
	{
		snakeX[i] = snakeX[i - 1];
		snakeY[i] = snakeY[i - 1];
	}

	// Move head
	snakeX[0] = moveX;
	snakeY[0] = moveY;
	moveX += delta_X;
	moveY += delta_Y;
}

void calculateMove(int switchDifference) // Calculate which way to turn depending on controller input and direction.
{
	if ((direction == 'N' && switchDifference == 1) || (direction == 'S' && switchDifference == 2))
	{
		deltaX = 1;
		deltaY = 0;
		direction = 'E';
	}
	else if ((direction == 'E' && switchDifference == 1) || (direction == 'W' && switchDifference == 2))
	{
		deltaX = 0;
		deltaY = 1;
		direction = 'S';
	}
	else if ((direction == 'N' && switchDifference == 2) || (direction == 'S' && switchDifference == 1))
	{
		deltaX = -1;
		deltaY = 0;
		direction = 'W';
	}
	else if ((direction == 'E' && switchDifference == 2) || (direction == 'W' && switchDifference == 1))
	{
		deltaX = 0;
		deltaY = -1;
		direction = 'N';
	}
}

void clear_screen(volatile char *VGA, unsigned short color) // Fill the screen with black pixels
{
	for (int i = 0; i < VGA_HEIGHT; i++)
	{
		for (int j = 0; j < VGA_WIDTH; j++)
		{
			VGA[i * VGA_WIDTH + j] = color;
		}
	}
}

void clear_board(volatile char *VGA, unsigned short color) // Clear the board with black pixels
{
	for (int i = y_offset + 1; i < y_offset + BOARD_HEIGHT - 1; i++)
	{
		for (int j = x_offset + 1; j < x_offset + BOARD_WIDTH - 1; j++)
		{
			VGA[i * VGA_WIDTH + j] = color;
		}
	}
}

void draw_board(volatile char *VGA, unsigned short color)
{
	// Draw top and bottom edges
	for (int j = x_offset; j < x_offset + BOARD_WIDTH; j++)
	{
		VGA[y_offset * VGA_WIDTH + j] = color;						// Top edge
		VGA[(y_offset + BOARD_HEIGHT - 1) * VGA_WIDTH + j] = color; // Bottom edge
	}

	// Draw left and right edges
	for (int i = y_offset; i < y_offset + BOARD_HEIGHT; i++)
	{
		VGA[i * VGA_WIDTH + x_offset] = color;					   // Left edge
		VGA[i * VGA_WIDTH + (x_offset + BOARD_WIDTH - 1)] = color; // Right edge
	}
}

void draw_snake(volatile char *VGA, unsigned short color)
{
	// Draw head
	VGA[(moveY + y_offset) * VGA_WIDTH + (moveX + x_offset)] = color;

	// Draw body segments
	for (int i = 0; i < snakeLength; i++)
	{
		VGA[(snakeY[i] + y_offset) * VGA_WIDTH + (snakeX[i] + x_offset)] = color;
	}
}

void spawn_food() // Randomize the positions for the spawn of the food within game area
{
	foodX = (moveX * 3939) % (BOARD_WIDTH - 2) + 1;
	foodY = (moveY * 3913) % (BOARD_HEIGHT - 2) + 1;
}

void draw_food(volatile char *VGA, unsigned short color) // Draw food at random locations within the board
{
	if (moveX == foodX && moveY == foodY)
	{
		snakeLength += 2;
		spawn_food();
		increase_score();
	}
	else if (foodX == 0)
	{
		spawn_food();
	}
	VGA[(foodY + y_offset) * VGA_WIDTH + (foodX + x_offset)] = color;
}

/*
 * This functions uses Timer interrupts to update the game graphics
 * and also handles changing the direction of the snake
 */
void handle_interrupt(unsigned cause)
{
	volatile char *VGA = (volatile char *)0x08000000;	 // pointer to VGA pixel buffer
	volatile int *VGA_CTRL = (volatile int *)0x04000100; // Pointer to the VGA DMA
	volatile int *timeout_flag = (volatile int *)0x04000020;
	int volatile *const edge_mask = (int *)0x0400001C;
	if (timeoutcount == 0)
	{
		clear_screen(VGA, 0);
		set_displaysToZero();
	}

	if (cause == 16)
	{
		if (*timeout_flag == 0x3)
		{
			*timeout_flag = 0x0;
			timeoutcount++;
		}
	}

	if (cause == 17)
	{
		*edge_mask = 1;
		switchValue = get_sw();
		switchDifference = abs(switchValue - formerValue);
		calculateMove(switchDifference);
		formerValue = switchValue;
	}

	if (timeoutcount % 2 == 0)
	{
		move_snake(deltaX, deltaY);
		detect_collision();		  // Add collision detection after moving the snake
		clear_board(VGA, 0x0000); // Clear entire screen to remove old graphics
		draw_board(VGA, BLUE);
		draw_snake(VGA, GREEN);
		draw_food(VGA, RED);
		*(VGA_CTRL + 1) = (unsigned int)(VGA); // Update backbuffer to point to vga pixelBuffer
		*(VGA_CTRL + 0) = 0;				   // Write to buffer register to perform swap
	}
}

void labinit(void) // Initialize the Timer, Timer interrupts aswell as switches interrupt
{
	int volatile *const periodl = (int *)0x04000028;
	int volatile *const periodh = (int *)0x0400002C;
	int volatile *const ctrl = (int *)0x04000024;
	int volatile *const interrupt_mask = (int *)0x04000018;
	int volatile *const edge_mask = (int *)0x0400001C;

	*periodl = 0x1E8480 & 0xFFFF;
	*periodh = (0x1E8480 >> 16);
	*ctrl = 0x7;
	*interrupt_mask = 0xF;
	*edge_mask = 0x1;
	enable_interrupt();
}

int main()
{
	labinit();
}
