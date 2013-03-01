#include <util/delay.h>
#include "iodefs.h"
#include "Poppy.h"


/** Buffer to hold the previously generated HID report, for comparison purposes inside the HID class driver. */
static volatile uint8_t PrevHIDReportBuffer[GENERIC_REPORT_SIZE];

/** LUFA HID Class driver interface configuration and state information. This structure is
 *  passed to all HID Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_HID_Device_t Generic_HID_Interface =
	{
		.Config = {
			.InterfaceNumber              = 0,
			.ReportINEndpoint             = {
				.Address              = GENERIC_IN_EPADDR,
				.Size                 = GENERIC_EPSIZE,
				.Banks                = 1,
			},
			.PrevReportINBuffer           = PrevHIDReportBuffer,
			.PrevReportINBufferSize       = sizeof(PrevHIDReportBuffer),
		},
	};


/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	SetupHardware();
	sei();

	for (;;) {
		HID_Device_USBTask(&Generic_HID_Interface);		
	}
}

void Bootloader()
{
	// disable timer
	TIMSK0 &= ~(1 << OCIE0A); // mask timer interrupts
	TCCR0B &= ~(1 << CS00 | 1 << CS01 | 1 < CS02); // prescale = 0 is timer off

	// If USB is used, detach from the bus and reset it
	USB_Disable();

	// Disable all interrupts
	cli();
	((void (*)(void))0x3000)();
}

#define LED (C,5)

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	// Disable clock division 
	clock_prescale_set(clock_div_1);

	// output port C
	OUTPUT(LED);
	CLR(LED);

	/* Hardware Initialization */
	USB_Init();

	// Timer init
	OCR0A  = 0x20; 
	TCNT0 = 0;
	TCCR0A = (1 << WGM01); // CTC mode (clear on count)
	TCCR0B = (1 << CS00); // prescale = 1
	TIMSK0 = (1 << OCIE0A); // run!
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;
	ConfigSuccess &= HID_Device_ConfigureEndpoints(&Generic_HID_Interface);

	USB_Device_EnableSOFEvents();
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	HID_Device_ProcessControlRequest(&Generic_HID_Interface);
}

/** HID class driver callback function for the creation of HID reports to the host.
 *
 *  \param[in]     HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in,out] ReportID    Report ID requested by the host if non-zero, otherwise callback should set to the generated report ID
 *  \param[in]     ReportType  Type of the report to create, either HID_REPORT_ITEM_In or HID_REPORT_ITEM_Feature
 *  \param[out]    ReportData  Pointer to a buffer where the created report should be stored
 *  \param[out]    ReportSize  Number of bytes written in the report (or zero if no report is to be sent)
 *
 *  \return Boolean true to force the sending of the report, false to let the library determine if it needs to be sent
 */
bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                         uint8_t* const ReportID,
                                         const uint8_t ReportType,
                                         void* ReportData,
                                         uint16_t* const ReportSize)
{
	*ReportSize = GENERIC_REPORT_SIZE;
	uint8_t * data = (uint8_t*)ReportData;
	data[0] = 0x0;
	data[1] = 0x1;
	data[2] = 0x2;
	data[3] = 0x3;
	return false;
}

#define MAX_STEPS 0x10

// Interrupt handler for managing the software PWM channels
static volatile uint8_t s_level = 0;
static volatile uint8_t s_counter = 0;

/** HID class driver callback function for the processing of HID reports from the host.
 *
 *  \param[in] HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in] ReportID    Report ID of the received report from the host
 *  \param[in] ReportType  The type of report that the host has sent, either HID_REPORT_ITEM_Out or HID_REPORT_ITEM_Feature
 *  \param[in] ReportData  Pointer to a buffer where the received report has been stored
 *  \param[in] ReportSize  Size in bytes of the received HID report
 */
void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                          const uint8_t ReportID,
                                          const uint8_t ReportType,
                                          const void* ReportData,
                                          const uint16_t ReportSize)
{	
	uint8_t * data = (uint8_t*)ReportData;
	if(data[0] == 0xaa && data[1] == 0xbb) {
		Bootloader();
	} else {
		uint8_t val = data[2] < MAX_STEPS-1 ? data[2] : MAX_STEPS-1;
		s_level = val; 
	}
}

/** Timer interrupt handler. Manages led duty cycle.
*/
ISR(TIMER0_COMPA_vect)
{
	TCNT0 = 0;
	if (++s_counter == MAX_STEPS) {
		s_counter = 0;
		if(s_level > 0) {
		 	SET(LED);
		} else {
		 	CLR(LED);
		}
		SET(LED)
	} else {
		if(s_counter > s_level)	{
			CLR(LED);
		}
	}
}


