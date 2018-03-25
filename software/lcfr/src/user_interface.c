#include <stdio.h>
#include <unistd.h>
#include "system.h"
#include "altera_up_avalon_ps2.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"
#include "altera_up_avalon_video_character_buffer_with_dma.h"
#include "altera_up_avalon_video_pixel_buffer_dma.h"

#include "user_interface.h"

static unsigned char keyInput;


void ps2_isr(void* ps2_device, alt_u32 id){
	alt_up_ps2_read_data_byte_timeout(ps2_device, &keyInput);
	printf("Scan code: %x\n", keyInput);
}

void UserInterface_start(){
	VGA_start();
	KB_start();
}

void VGA_start(){
	//reset the display
	alt_up_pixel_buffer_dma_dev *pixel_buf;
	pixel_buf = alt_up_pixel_buffer_dma_open_dev(VIDEO_PIXEL_BUFFER_DMA_NAME);
	alt_up_pixel_buffer_dma_clear_screen(pixel_buf, 0);

	//initialize character buffer
	alt_up_char_buffer_dev *char_buf;
	char_buf = alt_up_char_buffer_open_dev("/dev/video_character_buffer_with_dma");
}

void KB_start(){
	//enable interrupt for keyboard
	alt_up_ps2_dev * ps2_kb = alt_up_ps2_open_dev(PS2_NAME);
	alt_up_ps2_enable_read_interrupt(ps2_kb);
	alt_irq_register(PS2_IRQ, ps2_kb, ps2_isr);
}

