/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static int irq_enabled;
static int nmi_enabled;
int berzerk_irq_end_of_screen;

int berzerkplayvoice;

static int int_count;



MACHINE_INIT( berzerk )
{
	int i;

	/* Berzerk expects locations 3800-3fff to be ff */
	for (i = 0x3800; i < 0x4000; i++)
		memory_region(REGION_CPU1)[i] = 0xff;

	irq_enabled = 0;
	nmi_enabled = 0;
	berzerk_irq_end_of_screen = 0;
	int_count = 0;
}


WRITE_HANDLER( berzerk_irq_enable_w )
{
	irq_enabled = data;
}

WRITE_HANDLER( berzerk_nmi_enable_w )
{
	nmi_enabled = 1;
}

WRITE_HANDLER( berzerk_nmi_disable_w )
{
	nmi_enabled = 0;
}

READ_HANDLER( berzerk_nmi_enable_r )
{
	nmi_enabled = 1;
	return 0;
}

READ_HANDLER( berzerk_nmi_disable_r )
{
	nmi_enabled = 0;
	return 0;
}

READ_HANDLER( berzerk_led_on_r )
{
	set_led_status(0,1);

	return 0;
}

READ_HANDLER( berzerk_led_off_r )
{
	set_led_status(0,0);

	return 0;
}

READ_HANDLER( berzerk_voiceboard_r )
{
   if (!berzerkplayvoice)
      return 0;
   else
      return 0x40;
}

INTERRUPT_GEN( berzerk_interrupt )
{
	int_count++;

	switch (int_count)
	{
	default:
	case 4:
	case 8:
		if (int_count == 8)
		{
			berzerk_irq_end_of_screen = 0;
			int_count = 0;
		}
		else
		{
			berzerk_irq_end_of_screen = 1;
		}
		if (irq_enabled) cpu_set_irq_line_and_vector(0, 0, HOLD_LINE, 0xfc);
		break;

	case 1:
	case 2:
	case 3:
	case 5:
	case 6:
	case 7:
		if (int_count == 7)
		{
			berzerk_irq_end_of_screen = 1;
		}
		else
		{
			berzerk_irq_end_of_screen = 0;
		}
		if (nmi_enabled) cpu_set_irq_line(0, IRQ_LINE_NMI, PULSE_LINE);
		break;
	}
}

