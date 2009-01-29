/**
 * libusb helper functions
 *
 * Part of Gammu project
 *
 * Copyright (C) 2009 Michal Čihař
 *
 * Licensed under GNU GPL version 2 or later
 */

#include <libusb.h>
#include "usb.h"

#include "../../gsmstate.h"
#include "../../gsmcomon.h"

/**
 * Nokia USB vendor ID.
 */
#define NOKIA_VENDOR_ID 0x0421

/**
 * USB CDC Class ID.
 */
#define USB_CDC_CLASS           0x02
/**
 * USB CDC FBUS Subclass ID.
 */
#define USB_CDC_FBUS_SUBCLASS       0xfe

/**
 * Union of CDC descriptor.
 */
struct cdc_union_desc {
    u_int8_t      bLength;
    u_int8_t      bDescriptorType;
    u_int8_t      bDescriptorSubType;

    u_int8_t      bMasterInterface0;
    u_int8_t      bSlaveInterface0;
} __attribute__ ((packed));

struct cdc_extra_desc {
    u_int8_t      bLength;
    u_int8_t      bDescriptorType;
    u_int8_t      bDescriptorSubType;
} __attribute__ ((packed));

/* CDC header types (bDescriptorSubType) */
#define CDC_HEADER_TYPE         0x00
#define CDC_UNION_TYPE          0x06
#define CDC_FBUS_TYPE           0x15

/* Interface descriptor (bDescriptorType) */
#define USB_DT_CS_INTERFACE     0x24



GSM_Error GSM_USB_Probe(GSM_StateMachine *s, GSM_USB_Match_Function matcher)
{
	libusb_device **devs;
	libusb_device *dev;
	GSM_Device_USBData *d = &s->Device.Data.USB;
	ssize_t cnt;
	int rc;
	int i = 0;
	struct libusb_device_descriptor desc;
	GSM_Error error;

	cnt = libusb_get_device_list(d->context, &devs);
	if (cnt < 0) {
		smprintf(s, "Failed to list USB devices (%d)!\n", (int)cnt);
		return ERR_UNKNOWN;
	}

	while ((dev = devs[i++]) != NULL) {
		rc = libusb_get_device_descriptor(dev, &desc);
		if (rc < 0) {
			smprintf(s, "Failed to get device descriptor (%d)!\n", rc);
			continue;
		}

		smprintf(s, "Checking %04x:%04x (bus %d, device %d)\n",
			desc.idVendor, desc.idProduct,
			libusb_get_bus_number(dev), libusb_get_device_address(dev));

		/* TODO: add optional matching by ids based on device name from configuration */

		if (matcher(s, dev, &desc)) {
			break;
		}
	}

	if (dev == NULL) {
		error = ERR_DEVICENOTEXIST;
		goto done;
	}

	smprintf(s, "Trying to open device, config=%d, c_iface=%d, c_alt=%d, d_iface=%d, d_alt=%d\n",
		d->configuration, d->control_iface, d->control_altsetting, d->data_iface, d->data_altsetting);

	rc = libusb_open(dev, &d->handle);
	if (rc != 0) {
		d->handle = NULL;
		error = ERR_DEVICEOPENERROR;
		goto done;
	}

	rc = libusb_set_configuration(d->handle, d->configuration);
	if (rc != 0) {
		smprintf(s, "Failed to set device configuration %d (%d)!\n", d->configuration, rc);
		libusb_close(d->handle);
		d->handle = NULL;
		error = ERR_DEVICEOPENERROR;
		goto done;
	}

	rc = libusb_claim_interface(d->handle, d->control_iface);
	if (rc != 0) {
		smprintf(s, "Failed to set claim control interface %d (%d)!\n", d->control_iface, rc);
		libusb_close(d->handle);
		d->handle = NULL;
		error = ERR_DEVICEOPENERROR;
		goto done;
	}

	rc = libusb_set_interface_alt_setting(d->handle, d->control_iface, d->control_altsetting);
	if (rc != 0) {
		smprintf(s, "Failed to set control alt setting %d (%d)!\n", d->control_altsetting, rc);
		libusb_close(d->handle);
		d->handle = NULL;
		error = ERR_DEVICEOPENERROR;
		goto done;
	}

	rc = libusb_claim_interface(d->handle, d->data_iface);
	if (rc != 0) {
		smprintf(s, "Failed to set claim data interface %d (%d)!\n", d->data_iface, rc);
		libusb_close(d->handle);
		d->handle = NULL;
		error = ERR_DEVICEOPENERROR;
		goto done;
	}

	rc = libusb_set_interface_alt_setting(d->handle, d->data_iface, d->data_altsetting);
	if (rc != 0) {
		smprintf(s, "Failed to set data alt setting %d (%d)!\n", d->data_altsetting, rc);
		libusb_close(d->handle);
		d->handle = NULL;
		error = ERR_DEVICEOPENERROR;
		goto done;
	}

	error = ERR_NONE;

done:

	return error;
}

GSM_Error GSM_USB_Init(GSM_StateMachine *s)
{
	GSM_Device_USBData *d = &s->Device.Data.USB;
	int rc;

	d->handle = NULL;

	rc = libusb_init(&d->context);
	if (rc != 0) {
		d->context = NULL;
		smprintf(s, "Failed to init libusb (%d)!\n", rc);
		return ERR_UNKNOWN;
	}

	return ERR_NONE;
}

GSM_Error GSM_USB_Terminate(GSM_StateMachine *s)
{
	GSM_Device_USBData *d = &s->Device.Data.USB;

	if (d->handle != NULL) {
		libusb_set_interface_alt_setting(d->handle, d->data_iface, d->data_idlesetting);
		libusb_release_interface(d->handle, d->control_iface);
		libusb_release_interface(d->handle, d->data_iface);
		libusb_close(d->handle);
	}

	libusb_exit(d->context);

	d->handle = NULL;
	d->context = NULL;

	return ERR_NONE;
}

int GSM_USB_Read(GSM_StateMachine *s, void *buf, size_t nbytes)
{
	GSM_Device_USBData *d = &s->Device.Data.USB;
	int rc, ret;

	return -1;
	rc = libusb_bulk_transfer(d->handle, d->ep_read, buf, nbytes, &ret, 10000);
	if (rc != 0) {
		smprintf(s, "Failed to read from usb (%d)!\n", rc);
		return -1;
	}
	return ret;
}

int GSM_USB_Write(GSM_StateMachine *s, const void *buf, size_t nbytes)
{
	GSM_Device_USBData *d = &s->Device.Data.USB;
	int rc, ret;

	return -1;
	rc = libusb_bulk_transfer(d->handle, d->ep_write, (void *)buf, nbytes, &ret, 10000);
	if (rc != 0) {
		smprintf(s, "Failed to write to usb (%d)!\n", rc);
		return -1;
	}
	return ret;
}

bool FBUSUSB_Match(GSM_StateMachine *s, libusb_device *dev, struct libusb_device_descriptor *desc)
{
	int c, i, a;
	int rc;
	struct libusb_config_descriptor *config;
	GSM_Device_USBData *d = &s->Device.Data.USB;
	const unsigned char *buffer;
	int buflen;
	struct cdc_extra_desc *extra_desc;
	struct cdc_union_desc *union_desc = NULL;

	/* We care only about Nokia */
	if (desc->idVendor != NOKIA_VENDOR_ID) return false;

	/* Find configuration we want */
	for (c = 0; c < desc->bNumConfigurations; c++) {
		rc = libusb_get_config_descriptor(dev, c, &config);
		if (rc != 0) return false;
		/* Find interface we want */
		for (i = 0; i < config->bNumInterfaces; i++) {
			for (a = 0; a < config->interface[i].num_altsetting; a++) {
				/* We want only CDC FBUS settings */
				if (config->interface[i].altsetting[a].bInterfaceClass == USB_CDC_CLASS
						&& config->interface[i].altsetting[a].bInterfaceSubClass == USB_CDC_FBUS_SUBCLASS
					) {
					/* We have it */
					goto found_control;
				}
			}
		}
		libusb_free_config_descriptor(config);
	}
	return false;
found_control:
	/* Remember configuration which is interesting */
	d->configuration = config->bConfigurationValue;

	/* Remember control interface */
	d->control_iface = config->interface[i].altsetting[a].bInterfaceNumber;
	d->control_altsetting = config->interface[i].altsetting[a].bAlternateSetting;

	/* Find out data interface */

	/* Process extra descriptors */
	buffer = config->interface[i].altsetting[a].extra;
	buflen = config->interface[i].altsetting[a].extra_length;

	/* Each element has length as first byte and type as second */
	while (buflen > 0) {
		extra_desc = (struct cdc_extra_desc *)buffer; /* Convenience */
		if (extra_desc->bDescriptorType != USB_DT_CS_INTERFACE) {
			smprintf(s, "Extra CDC header: %d\n", extra_desc->bDescriptorType);
			goto next_el;
		}

		switch (extra_desc->bDescriptorSubType) {
			case CDC_UNION_TYPE:
				union_desc = (struct cdc_union_desc *)buffer;
				break;
			case CDC_HEADER_TYPE:
			case CDC_FBUS_TYPE:
				/* We know these, but ignore them */
				break;
			default:
				smprintf(s, "Extra CDC subheader: %d\n", extra_desc->bDescriptorSubType);
				break;
		}
next_el:
		buflen -= extra_desc->bLength;
		buffer += extra_desc->bLength;
	}

	if (union_desc == NULL) {
		smprintf(s, "Failed to find data end points!\n");
		libusb_free_config_descriptor(config);
		return false;
	}
	d->data_iface = union_desc->bSlaveInterface0;

	/* FIXME: Find out end points and settings from data_iface */

	/* Free config descriptor */
	libusb_free_config_descriptor(config);
	return true;
}

GSM_Error FBUSUSB_Open(GSM_StateMachine *s)
{
	GSM_Error error;

	error = GSM_USB_Init(s);
	if (error != ERR_NONE) return error;

	error = GSM_USB_Probe(s, FBUSUSB_Match);
	if (error != ERR_NONE) return error;

	return ERR_NONE;
}


GSM_Device_Functions FBUSUSBDevice = {
    	FBUSUSB_Open,
    	GSM_USB_Terminate,
	NONEFUNCTION,
	NONEFUNCTION,
	NONEFUNCTION,
    	GSM_USB_Read,
    	GSM_USB_Write
};

/* How should editor hadle tabs in this file? Add editor commands here.
 * vim: noexpandtab sw=8 ts=8 sts=8:
 */
