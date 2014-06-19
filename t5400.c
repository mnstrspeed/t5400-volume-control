#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <mach/platform.h>

MODULE_LICENSE("GPL");

#define HIGH 0x1
#define LOW 0x0

#define CS_PIN GPIO_P1_15
#define SCK_PIN GPIO_P1_13
#define DIN_PIN GPIO_P1_11

struct GPIO_REGISTERS
{
	uint32_t GPFSEL[6];
	uint32_t RESERVED1;
	uint32_t GPSET[2];
	uint32_t RESERVED2;
	uint32_t GPCLR[2];	
};

typedef enum
{
	GPIO_FSEL_INPT = 0b000,
	GPIO_FSEL_OUTP = 0b001
} 
GPIO_FUNCTIONS;

typedef enum 
{
	GPIO_P1_03 = 0,  
	GPIO_P1_05 = 1,  
	GPIO_P1_07 = 4,  
	GPIO_P1_08 = 14,  
	GPIO_P1_10 = 15,  
	GPIO_P1_11 = 17,  
	GPIO_P1_12 = 18,  
	GPIO_P1_13 = 21,  
	GPIO_P1_15 = 22,  
	GPIO_P1_16 = 23,  
	GPIO_P1_18 = 24,  
	GPIO_P1_19 = 10,  
	GPIO_P1_21 = 9,  
	GPIO_P1_22 = 25,  
	GPIO_P1_23 = 11,  
	GPIO_P1_24 = 8,  
	GPIO_P1_26 = 7
} 
GPIO_PINS;

struct GPIO_REGISTERS *gpio_registers;

void gpio_init(void)
{	
	gpio_registers = (struct GPIO_REGISTERS*)__io_address(GPIO_BASE);
}
 
void gpio_fsel(uint8_t pin, uint8_t function)
{
	unsigned int pin_reg = pin / 10; // 10 pins per register
	unsigned int pin_reg_offset = (pin % 10) * 3; // 3 bits per pin

	unsigned mask = 0b111 << pin_reg_offset;
	unsigned value = gpio_registers->GPFSEL[pin_reg];
	value &= ~mask; // clear bits
	value |= (function << pin_reg_offset) & mask; // set bits
	gpio_registers->GPFSEL[pin_reg] = value;
}

void gpio_set(int pin)
{
	gpio_registers->GPSET[pin / 32] = (1 << (pin % 32)); // 32 pins per register
}

void gpio_clr(int pin)
{
	gpio_registers->GPCLR[pin / 32] = (1 << (pin % 32)); // 32 pins per register
}

void gpio_write(int pin, int value)
{
	if (value == HIGH)
		gpio_set(pin);
	else
		gpio_clr(pin);
}

typedef enum
{
	LOAD_A = 9, 
	LOAD_B = 10,
	LOAD_AB = 15
}
DAC_FUNCTIONS;

void sync(void)
{
	ndelay(50);
}

void send_bit(int val)
{
	gpio_write(DIN_PIN, val);
	sync();
	
	gpio_write(SCK_PIN, HIGH);
	sync();
	gpio_write(SCK_PIN, LOW);
	sync();
}

void dac_write(int command, unsigned int value)
{
	int i;
	gpio_write(CS_PIN, LOW);
	sync();

	for (i = 3; i >= 0; i--)
		send_bit(command & (1 << i) ? HIGH : LOW);
	for (i = 9; i >= 0; i--)
		send_bit(value & (1 << i) ? HIGH : LOW);
	for (i = 0; i < 2; i++)
		send_bit(LOW);

	gpio_write(CS_PIN, HIGH);
}

static ssize_t set_volume_callback(struct device* dev, struct device_attribute* attr,
	const char* buffer, size_t count)
{
	long value = 0;
	if (kstrtol(buffer, 10, &value) < 0 || value < 0 || value > 1023)
		return -EINVAL;
	printk("Setting volume to %ld", value);
	dac_write(LOAD_A, value);
	return count;
}

static ssize_t set_bass_callback(struct device* dev, struct device_attribute* attr,
	const char* buffer, size_t count)
{
	long value = 0;
	if (kstrtol(buffer, 10, &value) < 0 || value < 0 || value > 1023)
		return -EINVAL;
	printk("Setting bass to %ld", value);
	dac_write(LOAD_B, value);
	return count;
}

static DEVICE_ATTR(volume, S_IWUSR | S_IWGRP | S_IWOTH, NULL, set_volume_callback); 
static DEVICE_ATTR(bass, S_IWUSR | S_IWGRP | S_IWOTH, NULL, set_bass_callback);

static struct class *device_class;
static struct device *device_object;

static int __init t5400_init(void)
{
	int result;

	printk("Loading t5400\n");
	device_class = class_create(THIS_MODULE, "volume_control");
	device_object = device_create(device_class, NULL, 0, NULL, "t5400");

	result = device_create_file(device_object, &dev_attr_volume);
	result = device_create_file(device_object, &dev_attr_bass);

	gpio_init();
	gpio_fsel(CS_PIN, GPIO_FSEL_OUTP);
	gpio_fsel(SCK_PIN, GPIO_FSEL_OUTP);
	gpio_fsel(DIN_PIN, GPIO_FSEL_OUTP);

	gpio_write(CS_PIN, HIGH);
	gpio_write(SCK_PIN, LOW);
	sync();

	return result;
}

static void __exit t5400_exit(void)
{
	printk("Unloading t5400\n");

	device_remove_file(device_object, &dev_attr_volume);
	device_remove_file(device_object, &dev_attr_bass);
	device_destroy(device_class, 0);
	class_destroy(device_class);
}

module_init(t5400_init);
module_exit(t5400_exit);
